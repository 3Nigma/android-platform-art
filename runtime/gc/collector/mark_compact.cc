/*
 * Copyright (C) 2014 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "mark_compact.h"

#include "base/logging.h"
#include "base/mutex-inl.h"
#include "base/timing_logger.h"
#include "gc/accounting/heap_bitmap-inl.h"
#include "gc/accounting/mod_union_table.h"
#include "gc/accounting/remembered_set.h"
#include "gc/accounting/space_bitmap-inl.h"
#include "gc/heap.h"
#include "gc/reference_processor.h"
#include "gc/space/bump_pointer_space.h"
#include "gc/space/bump_pointer_space-inl.h"
#include "gc/space/image_space.h"
#include "gc/space/large_object_space.h"
#include "gc/space/space-inl.h"
#include "indirect_reference_table.h"
#include "intern_table.h"
#include "jni_internal.h"
#include "mark_sweep-inl.h"
#include "monitor.h"
#include "mirror/art_field.h"
#include "mirror/art_field-inl.h"
#include "mirror/class-inl.h"
#include "mirror/class_loader.h"
#include "mirror/dex_cache.h"
#include "mirror/reference-inl.h"
#include "mirror/object-inl.h"
#include "mirror/object_array.h"
#include "mirror/object_array-inl.h"
#include "runtime.h"
#include "stack.h"
#include "thread-inl.h"
#include "thread_list.h"

using ::art::mirror::Class;
using ::art::mirror::Object;

namespace art {
namespace gc {
namespace collector {

void MarkCompact::BindBitmaps() {
  timings_.StartSplit("BindBitmaps");
  WriterMutexLock mu(Thread::Current(), *Locks::heap_bitmap_lock_);
  // Mark all of the spaces we never collect as immune.
  for (const auto& space : GetHeap()->GetContinuousSpaces()) {
    if (space->GetGcRetentionPolicy() == space::kGcRetentionPolicyNeverCollect ||
        space->GetGcRetentionPolicy() == space::kGcRetentionPolicyFullCollect) {
      CHECK(immune_region_.AddContinuousSpace(space)) << "Failed to add space " << *space;
    }
  }
  timings_.EndSplit();
}

MarkCompact::MarkCompact(Heap* heap, const std::string& name_prefix)
    : GarbageCollector(heap, name_prefix + (name_prefix.empty() ? "" : " ") + "mark compact"),
      space_(nullptr), collector_name_(name_) {
}

void MarkCompact::RunPhases() {
  Thread* self = Thread::Current();
  InitializePhase();
  // Semi-space collector is special since it is sometimes called with the mutators suspended
  // during the zygote creation and collector transitions. If we already exclusively hold the
  // mutator lock, then we can't lock it again since it will cause a deadlock.
  if (Locks::mutator_lock_->IsExclusiveHeld(self)) {
    GetHeap()->PreGcVerificationPaused(this);
    GetHeap()->PrePauseRosAllocVerification(this);
    MarkingPhase();
    ReclaimPhase();
    GetHeap()->PostGcVerificationPaused(this);
  } else {
    Locks::mutator_lock_->AssertNotHeld(self);
    {
      ScopedPause pause(this);
      GetHeap()->PreGcVerificationPaused(this);
      GetHeap()->PrePauseRosAllocVerification(this);
      MarkingPhase();
      ReclaimPhase();
    }
    GetHeap()->PostGcVerification(this);
  }
  FinishPhase();
}

void MarkCompact::FowardObject(mirror::Object* obj) {
  const size_t object_size = RoundUp(obj->SizeOf(), kObjectAlignment);
  LockWord lock_word = obj->GetLockWord(false);
  // If we have a non empty lock word, store it and restore it later.
  if (lock_word.GetValue() != LockWord().GetValue()) {
    // Set the bit in the bitmap so that we know to restore it later.
    objects_with_lockword_->Set(obj);
    lock_words_to_restore_.push_back(lock_word);
  }
  obj->SetLockWord(LockWord::FromForwardingAddress(reinterpret_cast<size_t>(bump_pointer_)),
                   false);
  bump_pointer_ += object_size;
  // Set the end bit of the object, the way we can differentiate between this and the start bit is
  // that the end bit is kObjectAlignment / 2 mod kObjectAlignment.
  objects_before_forwarding_->Set(reinterpret_cast<mirror::Object*>(
      reinterpret_cast<uintptr_t>(obj) + object_size - kObjectAlignment / 2));
  ++live_objects_in_space_;
}

class CalculateObjectForwardingAddressVisitor {
 public:
  explicit CalculateObjectForwardingAddressVisitor(MarkCompact* collector)
      : collector_(collector) {}
  void operator()(mirror::Object* obj) const EXCLUSIVE_LOCKS_REQUIRED(Locks::mutator_lock_,
                                                                      Locks::heap_bitmap_lock_) {
    // Objects not aligned by kObjectAlignment are the end of object markers.
    if (IsAligned<kObjectAlignment>(obj)) {
      DCHECK(collector_->IsMarked(obj));
      collector_->FowardObject(obj);
    }
  }

 private:
  MarkCompact* const collector_;
};

void MarkCompact::CalculateObjectForwardingAddresses() {
  timings_.NewSplit(__FUNCTION__);
  // The bump pointer in the space where the next forwarding address will be.
  bump_pointer_ = reinterpret_cast<byte*>(space_->Begin());
  // Visit all the marked objects in the bitmap.
  CalculateObjectForwardingAddressVisitor visitor(this);
  objects_before_forwarding_->VisitMarkedRange(reinterpret_cast<uintptr_t>(space_->Begin()),
                                               reinterpret_cast<uintptr_t>(space_->End()),
                                               visitor);
}

void MarkCompact::InitializePhase() {
  TimingLogger::ScopedSplit split("InitializePhase", &timings_);
  mark_stack_ = heap_->GetMarkStack();
  DCHECK(mark_stack_ != nullptr);
  immune_region_.Reset();
  CHECK(space_->CanMoveObjects()) << "Attempting compact non-movable space from " << *space_;
  {
    // TODO: I don't think we should need heap bitmap lock to Get the mark bitmap.
    ReaderMutexLock mu(Thread::Current(), *Locks::heap_bitmap_lock_);
    mark_bitmap_ = heap_->GetMarkBitmap();
  }
  live_objects_in_space_ = 0;
}

void MarkCompact::ProcessReferences(Thread* self) {
  TimingLogger::ScopedSplit split("ProcessReferences", &timings_);
  WriterMutexLock mu(self, *Locks::heap_bitmap_lock_);
  heap_->GetReferenceProcessor()->ProcessReferences(
      false, &timings_, clear_soft_references_, &IsMarkedCallback, &MarkObjectCallback,
      &ProcessMarkStackCallback, this);
}

class BitmapSetSlowPathVisitor {
 public:
  void operator()(const mirror::Object* obj) const {
    // Marking a large object, make sure its aligned as a sanity check.
    if (!IsAligned<kPageSize>(obj)) {
      Runtime::Current()->GetHeap()->DumpSpaces(LOG(ERROR));
      LOG(FATAL) << obj;
    }
  }
};

inline void MarkCompact::MarkObject(mirror::Object* obj) {
  if (obj == nullptr) {
    return;
  }
  if (kUseBakerOrBrooksReadBarrier) {
    // Verify all the objects have the correct forward pointer installed.
    obj->AssertReadBarrierPointer();
  }
  if (immune_region_.ContainsObject(obj)) {
    return;
  }
  if (objects_before_forwarding_->HasAddress(obj)) {
    if (!objects_before_forwarding_->Set(obj)) {
      MarkStackPush(obj);  // This object was not previously marked.
    }
  } else {
    DCHECK(!space_->HasAddress(obj));
    BitmapSetSlowPathVisitor visitor;
    if (!mark_bitmap_->Set(obj, visitor)) {
      // This object was not previously marked.
      MarkStackPush(obj);
    }
  }
}

void MarkCompact::MarkingPhase() {
  Thread* self = Thread::Current();
  // Bitmap which describes which objects we have to move.
  objects_before_forwarding_.reset(ObjectsBeforeForwardingBitmap::Create(
      "objects before forwarding", space_->Begin(), space_->Size()));
  // Bitmap which describes which lock words we need to restore.
  objects_with_lockword_.reset(accounting::ContinuousSpaceBitmap::Create(
      "objects with lock words", space_->Begin(), space_->Size()));
  CHECK(Locks::mutator_lock_->IsExclusiveHeld(self));
  TimingLogger::ScopedSplit split("MarkingPhase", &timings_);
  // Assume the cleared space is already empty.
  BindBitmaps();
  // Process dirty cards and add dirty cards to mod-union tables.
  heap_->ProcessCards(timings_, false);
  // Clear the whole card table since we can not Get any additional dirty cards during the
  // paused GC. This saves memory but only works for pause the world collectors.
  timings_.NewSplit("ClearCardTable");
  heap_->GetCardTable()->ClearCardTable();
  // Need to do this before the checkpoint since we don't want any threads to add references to
  // the live stack during the recursive mark.
  timings_.NewSplit("SwapStacks");
  if (kUseThreadLocalAllocationStack) {
    heap_->RevokeAllThreadLocalAllocationStacks(self);
  }
  heap_->SwapStacks(self);
  {
    WriterMutexLock mu(self, *Locks::heap_bitmap_lock_);
    MarkRoots();
    // Mark roots of immune spaces.
    UpdateAndMarkModUnion();
    // Recursively mark remaining objects.
    MarkReachableObjects();
  }
  ProcessReferences(self);
  {
    ReaderMutexLock mu(self, *Locks::heap_bitmap_lock_);
    SweepSystemWeaks();
  }
  timings_.NewSplit("RecordFree");
  // Revoke buffers before measuring how many objects were moved since the TLABs need to be revoked
  // before they are properly counted.
  RevokeAllThreadLocalBuffers();
  timings_.StartSplit("PreSweepingGcVerification");
  // Disabled due to an issue where we have objects in the bump pointer space which reference dead
  // objects.
  // heap_->PreSweepingGcVerification(this);
  timings_.EndSplit();
}

void MarkCompact::UpdateAndMarkModUnion() {
  for (auto& space : heap_->GetContinuousSpaces()) {
    // If the space is immune then we need to mark the references to other spaces.
    if (immune_region_.ContainsSpace(space)) {
      accounting::ModUnionTable* table = heap_->FindModUnionTableFromSpace(space);
      if (table != nullptr) {
        // TODO: Improve naming.
        TimingLogger::ScopedSplit split(
            space->IsZygoteSpace() ? "UpdateAndMarkZygoteModUnionTable" :
                                     "UpdateAndMarkImageModUnionTable",
                                     &timings_);
        table->UpdateAndMarkReferences(MarkHeapReferenceCallback, this);
      }
    }
  }
}

void MarkCompact::MarkReachableObjects() {
  timings_.StartSplit("MarkStackAsLive");
  accounting::ObjectStack* live_stack = heap_->GetLiveStack();
  heap_->MarkAllocStackAsLive(live_stack);
  live_stack->Reset();
  // Recursively process the mark stack.
  ProcessMarkStack();
}

void MarkCompact::ReclaimPhase() {
  TimingLogger::ScopedSplit split("ReclaimPhase", &timings_);
  WriterMutexLock mu(Thread::Current(), *Locks::heap_bitmap_lock_);
  // Reclaim unmarked objects.
  Sweep(false);
  // Swap the live and mark bitmaps for each space which we modified space. This is an
  // optimization that enables us to not clear live bits inside of the sweep. Only swaps unbound
  // bitmaps.
  timings_.StartSplit("SwapBitmapsAndUnBindBitmaps");
  SwapBitmaps();
  GetHeap()->UnBindBitmaps();  // Unbind the live and mark bitmaps.
  Compact();
  timings_.EndSplit();
}

void MarkCompact::ResizeMarkStack(size_t new_size) {
  std::vector<Object*> temp(mark_stack_->Begin(), mark_stack_->End());
  CHECK_LE(mark_stack_->Size(), new_size);
  mark_stack_->Resize(new_size);
  for (const auto& obj : temp) {
    mark_stack_->PushBack(obj);
  }
}

inline void MarkCompact::MarkStackPush(Object* obj) {
  if (UNLIKELY(mark_stack_->Size() >= mark_stack_->Capacity())) {
    ResizeMarkStack(mark_stack_->Capacity() * 2);
  }
  // The object must be pushed on to the mark stack.
  mark_stack_->PushBack(obj);
}

void MarkCompact::ProcessMarkStackCallback(void* arg) {
  reinterpret_cast<MarkCompact*>(arg)->ProcessMarkStack();
}

mirror::Object* MarkCompact::MarkObjectCallback(mirror::Object* root, void* arg) {
  reinterpret_cast<MarkCompact*>(arg)->MarkObject(root);
  return root;
}

void MarkCompact::MarkHeapReferenceCallback(mirror::HeapReference<mirror::Object>* obj_ptr,
                                          void* arg) {
  reinterpret_cast<MarkCompact*>(arg)->MarkObject(obj_ptr->AsMirrorPtr());
}

void MarkCompact::DelayReferenceReferentCallback(mirror::Class* klass, mirror::Reference* ref,
                                               void* arg) {
  reinterpret_cast<MarkCompact*>(arg)->DelayReferenceReferent(klass, ref);
}

void MarkCompact::MarkRootCallback(Object** root, void* arg, uint32_t /*thread_id*/,
                                   RootType /*root_type*/) {
  reinterpret_cast<MarkCompact*>(arg)->MarkObject(*root);
}

void MarkCompact::UpdateRootCallback(Object** root, void* arg, uint32_t /*thread_id*/,
                                     RootType /*root_type*/) {
  mirror::Object* obj = *root;
  if (!obj->IsClass()) {
    mirror::Object* new_obj = reinterpret_cast<MarkCompact*>(arg)->GetMarkedForwardAddress(obj);
    if (obj != new_obj) {
      *root = new_obj;
      DCHECK(new_obj != nullptr);
    }
  }
}

void MarkCompact::UpdateClassRootCallback(Object** root, void* arg, uint32_t /*thread_id*/,
                                          RootType /*root_type*/) {
  mirror::Object* obj = *root;
  DCHECK(obj->IsClass());
  mirror::Object* new_obj = reinterpret_cast<MarkCompact*>(arg)->GetMarkedForwardAddress(obj);
  if (obj != new_obj) {
    *root = new_obj;
    DCHECK(new_obj != nullptr);
  }
}

class UpdateObjectReferencesVisitor {
 public:
  explicit UpdateObjectReferencesVisitor(MarkCompact* collector) : collector_(collector) {
  }
  void operator()(mirror::Object* obj) const SHARED_LOCKS_REQUIRED(Locks::heap_bitmap_lock_)
          EXCLUSIVE_LOCKS_REQUIRED(Locks::mutator_lock_) ALWAYS_INLINE {
    if (IsAligned<kObjectAlignment>(obj)) {
      collector_->UpdateObjectReferences(obj);
    }
  }

 private:
  MarkCompact* const collector_;
};

void MarkCompact::UpdateReferences() {
  timings_.NewSplit(__FUNCTION__);
  Runtime* runtime = Runtime::Current();
  // Update roots.
  runtime->VisitRoots(UpdateRootCallback, this);
  // Update object references in mod union tables and spaces.
  for (const auto& space : heap_->GetContinuousSpaces()) {
    // If the space is immune then we need to mark the references to other spaces.
    accounting::ModUnionTable* table = heap_->FindModUnionTableFromSpace(space);
    if (table != nullptr) {
      // TODO: Improve naming.
      TimingLogger::ScopedSplit split(
          space->IsZygoteSpace() ? "UpdateZygoteModUnionTableReferences" :
                                   "UpdateImageModUnionTableReferences",
                                   &timings_);
      table->UpdateAndMarkReferences(&UpdateHeapReferenceCallback, this);
    } else {
      // No mod union table, so we need to scan the space using bitmap visit.
      // Scan the space using bitmap visit.
      accounting::ContinuousSpaceBitmap* bitmap = space->GetLiveBitmap();
      if (bitmap != nullptr) {
        UpdateObjectReferencesVisitor visitor(this);
        bitmap->VisitMarkedRange(reinterpret_cast<uintptr_t>(space->Begin()),
                                 reinterpret_cast<uintptr_t>(space->End()),
                                 visitor);
      }
    }
    // TODO: Update large objects classes, but these classes shouldn't move usually.
  }
  // Update the system weaks, these should already have been swept.
  runtime->SweepSystemWeaks(&MarkedForwardingAddressCallback, this);
  // Update the objects in the bump pointer space last, these objects don't have a bitmap.
  UpdateObjectReferencesVisitor visitor(this);
  objects_before_forwarding_->VisitMarkedRange(reinterpret_cast<uintptr_t>(space_->Begin()),
                                               reinterpret_cast<uintptr_t>(space_->End()),
                                               visitor);
  // Update the reference processor cleared list.
  heap_->GetReferenceProcessor()->UpdateRoots(&MarkedForwardingAddressCallback, this);
  // Update the references of classes last.
  runtime->GetClassLinker()->VisitClassRoots(&UpdateClassRootCallback, this,
                                             kVisitRootFlagAllRoots);
}

void MarkCompact::Compact() {
  timings_.NewSplit(__FUNCTION__);
  CalculateObjectForwardingAddresses();
  UpdateReferences();
  MoveObjects();
  // Space
  int64_t objects_freed = space_->GetObjectsAllocated() - live_objects_in_space_;
  int64_t bytes_freed = reinterpret_cast<int64_t>(space_->End()) -
      reinterpret_cast<int64_t>(bump_pointer_);
  LOG(INFO) << "Space end " << reinterpret_cast<void*>(space_->End()) << " -> "
      << reinterpret_cast<void*>(bump_pointer_);
  LOG(INFO) << "Freed bytes " << bytes_freed << " objects " << objects_freed;
  space_->RecordFree(objects_freed, bytes_freed);
  RecordFree(objects_freed, bytes_freed);
  space_->SetEnd(bump_pointer_);
  // Need to zero out the memory we freed.
  memset(bump_pointer_, 0, bytes_freed);
}

// Marks all objects in the root set.
void MarkCompact::MarkRoots() {
  timings_.NewSplit("MarkRoots");
  Runtime::Current()->VisitRoots(MarkRootCallback, this);
}

mirror::Object* MarkCompact::MarkedForwardingAddressCallback(mirror::Object* obj, void* arg) {
  return reinterpret_cast<MarkCompact*>(arg)->GetMarkedForwardAddress(obj);
}

inline void MarkCompact::UpdateHeapReference(mirror::HeapReference<mirror::Object>* reference) {
  mirror::Object* obj = reference->AsMirrorPtr();
  if (obj != nullptr && !obj->IsClass()) {
    mirror::Object* new_obj = GetMarkedForwardAddress(obj);
    if (obj != new_obj) {
      DCHECK(new_obj != nullptr);
      reference->Assign(new_obj);
    }
  }
}

void MarkCompact::UpdateHeapReferenceCallback(mirror::HeapReference<mirror::Object>* reference,
                                              void* arg) {
  reinterpret_cast<MarkCompact*>(arg)->UpdateHeapReference(reference);
}

class UpdateReferenceVisitor {
 public:
  explicit UpdateReferenceVisitor(MarkCompact* collector) : collector_(collector) {
  }

  void operator()(Object* obj, MemberOffset offset, bool /*is_static*/) const
      ALWAYS_INLINE EXCLUSIVE_LOCKS_REQUIRED(Locks::mutator_lock_, Locks::heap_bitmap_lock_) {
    collector_->UpdateHeapReference(obj->GetFieldObjectReferenceAddr<kVerifyNone>(offset));
  }

  void operator()(mirror::Class* /*klass*/, mirror::Reference* ref) const
      EXCLUSIVE_LOCKS_REQUIRED(Locks::mutator_lock_, Locks::heap_bitmap_lock_) {
    collector_->UpdateHeapReference(
        ref->GetFieldObjectReferenceAddr<kVerifyNone>(mirror::Reference::ReferentOffset()));
  }

 private:
  MarkCompact* const collector_;
};

void MarkCompact::UpdateObjectReferences(mirror::Object* obj) {
  UpdateReferenceVisitor visitor(this);
  obj->VisitReferences<kMovingClasses>(visitor, visitor);
}

inline mirror::Object* MarkCompact::GetMarkedForwardAddress(mirror::Object* obj) const {
  DCHECK(obj != nullptr);
  if (objects_before_forwarding_->HasAddress(obj)) {
    DCHECK(objects_before_forwarding_->Test(obj));
    mirror::Object* ret =
        reinterpret_cast<mirror::Object*>(obj->GetLockWord(false).ForwardingAddress());
    DCHECK(ret != nullptr);
    return ret;
  }
  DCHECK(!space_->HasAddress(obj));
  DCHECK(IsMarked(obj));
  return obj;
}

inline bool MarkCompact::IsMarked(const Object* object) const {
  if (immune_region_.ContainsObject(object)) {
    return true;
  }
  if (objects_before_forwarding_->HasAddress(object)) {
    return objects_before_forwarding_->Test(object);
  }
  return mark_bitmap_->Test(object);
}

mirror::Object* MarkCompact::IsMarkedCallback(mirror::Object* object, void* arg) {
  return reinterpret_cast<MarkCompact*>(arg)->IsMarked(object) ? object : nullptr;
}

void MarkCompact::SweepSystemWeaks() {
  timings_.StartSplit("SweepSystemWeaks");
  Runtime::Current()->SweepSystemWeaks(IsMarkedCallback, this);
  timings_.EndSplit();
}

bool MarkCompact::ShouldSweepSpace(space::ContinuousSpace* space) const {
  return space != space_ && !immune_region_.ContainsSpace(space);
}

class MoveObjectVisitor {
 public:
  explicit MoveObjectVisitor(MarkCompact* collector)
      : object_start_(nullptr), collector_(collector) {
  }
  void operator()(mirror::Object* obj) const SHARED_LOCKS_REQUIRED(Locks::heap_bitmap_lock_)
          EXCLUSIVE_LOCKS_REQUIRED(Locks::mutator_lock_) ALWAYS_INLINE {
    if (IsAligned<kObjectAlignment>(obj)) {
      const_cast<MoveObjectVisitor*>(this)->object_start_ = obj;
    } else {
      // End of the object is at obj + kObjectAlignment / 2.
      size_t len = reinterpret_cast<uintptr_t>(obj) -
          reinterpret_cast<uintptr_t>(object_start_) + kObjectAlignment / 2;
      collector_->MoveObject(object_start_, len);
    }
  }

 private:
  mirror::Object* object_start_;
  MarkCompact* const collector_;
};

void MarkCompact::MoveObject(mirror::Object* obj, size_t len) {
  // Look at the forwarding address stored in the lock word to know where to copy.
  uintptr_t dest_addr = obj->GetLockWord(false).ForwardingAddress();
  mirror::Object* dest_obj = reinterpret_cast<mirror::Object*>(dest_addr);
  // Use memmove since there may be overlap.
  memmove(reinterpret_cast<void*>(dest_addr), reinterpret_cast<const void*>(obj), len);
  // Restore the saved lock word if needed.
  LockWord lock_word;
  if (UNLIKELY(objects_with_lockword_->Test(obj))) {
    lock_word = lock_words_to_restore_.front();
    lock_words_to_restore_.pop_front();
  }
  dest_obj->SetLockWord(lock_word, false);
}

void MarkCompact::MoveObjects() {
  timings_.NewSplit(__FUNCTION__);
  // Move the objects in the before forwarding bitmap.
  MoveObjectVisitor visitor(this);
  objects_before_forwarding_->VisitMarkedRange(reinterpret_cast<uintptr_t>(space_->Begin()),
                                               reinterpret_cast<uintptr_t>(space_->End()),
                                               visitor);
  CHECK(lock_words_to_restore_.empty());
}

void MarkCompact::Sweep(bool swap_bitmaps) {
  DCHECK(mark_stack_->IsEmpty());
  TimingLogger::ScopedSplit split("Sweep", &timings_);
  for (const auto& space : GetHeap()->GetContinuousSpaces()) {
    if (space->IsContinuousMemMapAllocSpace()) {
      space::ContinuousMemMapAllocSpace* alloc_space = space->AsContinuousMemMapAllocSpace();
      if (!ShouldSweepSpace(alloc_space)) {
        continue;
      }
      TimingLogger::ScopedSplit split(
          alloc_space->IsZygoteSpace() ? "SweepZygoteSpace" : "SweepAllocSpace", &timings_);
      size_t freed_objects = 0;
      size_t freed_bytes = 0;
      alloc_space->Sweep(swap_bitmaps, &freed_objects, &freed_bytes);
      RecordFree(freed_objects, freed_bytes);
    }
  }
  SweepLargeObjects(swap_bitmaps);
}

void MarkCompact::SweepLargeObjects(bool swap_bitmaps) {
  TimingLogger::ScopedSplit split("SweepLargeObjects", &timings_);
  size_t freed_objects = 0;
  size_t freed_bytes = 0;
  heap_->GetLargeObjectsSpace()->Sweep(swap_bitmaps, &freed_objects, &freed_bytes);
  RecordFreeLargeObjects(freed_objects, freed_bytes);
}

// Process the "referent" field in a java.lang.ref.Reference.  If the referent has not yet been
// marked, put it on the appropriate list in the heap for later processing.
void MarkCompact::DelayReferenceReferent(mirror::Class* klass, mirror::Reference* reference) {
  heap_->GetReferenceProcessor()->DelayReferenceReferent(klass, reference, &IsMarkedCallback, this);
}

class MarkCompactMarkObjectVisitor {
 public:
  explicit MarkCompactMarkObjectVisitor(MarkCompact* collector) : collector_(collector) {
  }

  void operator()(Object* obj, MemberOffset offset, bool /*is_static*/) const ALWAYS_INLINE
      EXCLUSIVE_LOCKS_REQUIRED(Locks::mutator_lock_, Locks::heap_bitmap_lock_) {
    // Object was already verified when we scanned it.
    collector_->MarkObject(obj->GetFieldObject<mirror::Object, kVerifyNone>(offset));
  }

  void operator()(mirror::Class* klass, mirror::Reference* ref) const
      SHARED_LOCKS_REQUIRED(Locks::mutator_lock_)
      EXCLUSIVE_LOCKS_REQUIRED(Locks::heap_bitmap_lock_) {
    collector_->DelayReferenceReferent(klass, ref);
  }

 private:
  MarkCompact* const collector_;
};

// Visit all of the references of an object and update.
void MarkCompact::ScanObject(Object* obj) {
  MarkCompactMarkObjectVisitor visitor(this);
  obj->VisitReferences<kMovingClasses>(visitor, visitor);
}

// Scan anything that's on the mark stack.
void MarkCompact::ProcessMarkStack() {
  timings_.StartSplit("ProcessMarkStack");
  while (!mark_stack_->IsEmpty()) {
    Object* obj = mark_stack_->PopBack();
    DCHECK(obj != nullptr);
    ScanObject(obj);
  }
  timings_.EndSplit();
}

void MarkCompact::SetSpace(space::BumpPointerSpace* space) {
  DCHECK(space != nullptr);
  space_ = space;
}

void MarkCompact::FinishPhase() {
  TimingLogger::ScopedSplit split("FinishPhase", &timings_);
  space_ = nullptr;
  CHECK(mark_stack_->IsEmpty());
  mark_stack_->Reset();
  // Clear all of the spaces' mark bitmaps.
  WriterMutexLock mu(Thread::Current(), *Locks::heap_bitmap_lock_);
  heap_->ClearMarkedObjects();
  // Release our bitmaps.
  objects_before_forwarding_.reset(nullptr);
  objects_with_lockword_.reset(nullptr);
}

void MarkCompact::RevokeAllThreadLocalBuffers() {
  timings_.StartSplit("(Paused)RevokeAllThreadLocalBuffers");
  GetHeap()->RevokeAllThreadLocalBuffers();
  timings_.EndSplit();
}

}  // namespace collector
}  // namespace gc
}  // namespace art
