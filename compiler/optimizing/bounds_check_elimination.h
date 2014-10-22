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

#ifndef ART_COMPILER_OPTIMIZING_BOUNDS_CHECK_ELIMINATION_H_
#define ART_COMPILER_OPTIMIZING_BOUNDS_CHECK_ELIMINATION_H_

#include "nodes.h"

namespace art {

class MonotonicValueRange;

/**
 * A value bound is represented as a pair of value and constant,
 * e.g. array.length - 1.
 */
class ValueBound : public ValueObject {
 public:
  static ValueBound MakeValueBound(HInstruction* instruction, int constant) {
    if (instruction == nullptr) {
      return ValueBound(nullptr, constant);
    }
    if (instruction->IsIntConstant()) {
      return ValueBound(nullptr, instruction->AsIntConstant()->GetValue() + constant);
    }
    return ValueBound(instruction, constant);
  }

  HInstruction* GetInstruction() const { return instruction_; }
  int GetConstant() const { return constant_; }

  bool IsRelativeToArrayLength() const {
    return instruction_ != nullptr && instruction_->IsArrayLength();
  }

  bool IsConstant() const {
    return instruction_ == nullptr;
  }

  // This bound has more information than that it's the bound of
  // 32-bit integer.
  bool IsUseful() const { return constant_ != INT_MAX && constant_ != INT_MIN; }

  static ValueBound Min() { return ValueBound(nullptr, INT_MIN); }
  static ValueBound Max() { return ValueBound(nullptr, INT_MAX); }

  bool Equals(ValueBound bound2) const {
    return instruction_ == bound2.instruction_ &&
           constant_ == bound2.constant_;
  }

  // Returns if it's certain bound1 >= bound2.
  bool GreaterThanOrEqual(ValueBound bound) const {
    if (instruction_ == bound.instruction_) {
      if (instruction_ == nullptr) {
        // Pure constant.
        return constant_ >= bound.constant_;
      }
      // There might be overflow/underflow. Be conservative for now.
      return false;
    }
    // Not comparable. Just return false.
    return false;
  }

  // Returns if it's certain bound1 <= bound2.
  bool LessThanOrEqual(ValueBound bound) const {
    if (instruction_ == bound.instruction_) {
      if (instruction_ == nullptr) {
        // Pure constant.
        return constant_ <= bound.constant_;
      }
      if (IsRelativeToArrayLength()) {
        // Array length is guaranteed to be no less than 0.
        // No overflow/underflow can happen if both constants are negative.
        if (constant_ <= 0 && bound.constant_ <= 0) {
          return constant_ <= bound.constant_;
        }
        // There might be overflow/underflow. Be conservative for now.
        return false;
      }
    }

    // In case the array length is some constant, we can
    // still compare.
    if (IsConstant() && bound.IsRelativeToArrayLength()) {
      HInstruction* array = bound.GetInstruction()->AsArrayLength()->InputAt(0);
      if (array->IsNullCheck()) {
        array = array->AsNullCheck()->InputAt(0);
      }
      if (array->IsNewArray()) {
        HInstruction* len = array->InputAt(0);
        if (len->IsIntConstant()) {
          int len_const = len->AsIntConstant()->GetValue();
          return constant_ <= len_const + bound.GetConstant();
        }
      }
    }

    // Not comparable. Just return false.
    return false;
  }

  // Try to narrow lower bound. Returns the bigger of the two.
  ValueBound NarrowLowerBound(ValueBound bound) const {
    if (instruction_ == bound.instruction_) {
      // Same instruction, compare the constant part.
      return ValueBound(bound.instruction_,
                        std::max(constant_, bound.constant_));
    }

    // Not comparable. Just pick one. We may lose some info, but that's ok.
    // Favor constant as lower bound.
    return bound.IsConstant() ? bound : *this;
  }

  // Try to narrow upper bound. Returns the smaller of the two.
  ValueBound NarrowUpperBound(ValueBound bound) const {
    if (instruction_ == bound.instruction_) {
      // Same instruction, compare the constant part.
      return ValueBound(instruction_,
                        std::min(constant_, bound.constant_));
    }

    // Not comparable. Just pick one. We may lose some info, but that's ok.
    // Favor array length as upper bound.
    return bound.IsRelativeToArrayLength() ? bound : *this;
  }

  ValueBound Add(int c) const {
    if (c == 0) {
      return *this;
    }
    int new_constant;
    if (c > 0) {
      if (constant_ >= INT_MAX - c) {
        // Constant part overflows.
        // Set it to INT_MAX, which isn't treated as useful anyway.
        new_constant = INT_MAX;
      } else {
        new_constant = constant_ + c;
      }
    } else {
      if (constant_ <= INT_MIN - c) {
        // Constant part underflows.
        // Set it to INT_MIN, which isn't treated as useful anyway.
        new_constant = INT_MIN;
      } else {
        new_constant = constant_ + c;
      }
    }
    return ValueBound(instruction_, new_constant);
  }

 private:
  ValueBound(HInstruction* instruction, int constant)
      : instruction_(instruction), constant_(constant) {}

  HInstruction* const instruction_;
  const int constant_;
};

/**
 * Represent a range of lower bound and upper bound, both being inclusive.
 */
class ValueRange : public ArenaObject<kArenaAllocMisc> {
 public:
  ValueRange(ArenaAllocator* allocator, ValueBound lower, ValueBound upper)
      : allocator_(allocator), lower_(lower), upper_(upper) {}

  virtual ~ValueRange() {}

  virtual const MonotonicValueRange* AsMonotonicValueRange() const { return nullptr; }
  bool IsMonotonicValueRange() const {
    return AsMonotonicValueRange() != nullptr;
  }

  ArenaAllocator* GetAllocator() const { return allocator_; }
  ValueBound GetLower() const { return lower_; }
  ValueBound GetUpper() const { return upper_; }

  // If it's certain that this value range fits in other_range.
  virtual bool FitsIn(ValueRange* other_range) const {
    if (other_range == nullptr) {
      return true;
    }

    if (other_range->IsMonotonicValueRange()) {
      // Be conservative here due to overflow/underflow stuffs.
      return false;
    }

    return lower_.GreaterThanOrEqual(other_range->lower_) &&
           upper_.LessThanOrEqual(other_range->upper_);
  }

  virtual ValueRange* Narrow(ValueRange* range) {
    if (range == nullptr) {
      return this;
    }

    if (range->IsMonotonicValueRange()) {
      return this;
    }

    return new (allocator_) ValueRange(
        allocator_,
        lower_.NarrowLowerBound(range->lower_),
        upper_.NarrowUpperBound(range->upper_));
  }

  // If this range has more information than that it isn't the full 32-bit
  // integer range. Either lower_ or upper_ must be useful.
  virtual bool IsUseful() const {
    return lower_.GetConstant() != INT_MIN || upper_.GetConstant() != INT_MAX;
  }

  virtual ValueRange* Add(int constant) const {
    return new (allocator_) ValueRange(allocator_, lower_.Add(constant), upper_.Add(constant));
  }

 private:
  ArenaAllocator* const allocator_;
  const ValueBound lower_;  // inclusive
  const ValueBound upper_;  // inclusive

  DISALLOW_COPY_AND_ASSIGN(ValueRange);
};

/**
 * A monotonically incrementing/decrementing value range, e.g.
 * the variable i in "for (int i=0; i<array.length; i++)".
 * Special care needs to be taken to account for overflow/underflow
 * of such value ranges.
 */
class MonotonicValueRange : public ValueRange {
 public:
  MonotonicValueRange(ArenaAllocator* allocator, ValueBound lower,
                      ValueBound upper, int increment)
      : ValueRange(allocator, lower, upper),
        increment_(increment) {
    if (increment > 0) {
      DCHECK(upper.IsConstant() && upper.GetConstant() == INT_MAX);
    } else if (increment < 0) {
      DCHECK(lower.IsConstant() && lower.GetConstant() == INT_MIN);
    } else {
      LOG(FATAL) << "Unreachable";
    }
  }

  virtual ~MonotonicValueRange() {}

  const MonotonicValueRange* AsMonotonicValueRange() const OVERRIDE { return this; }

  // If it's certain that this value range fits in other_range.
  bool FitsIn(ValueRange* other_range) const OVERRIDE {
    if (other_range == nullptr) {
      return true;
    }

    // Due to overflow/underflow issues, both ranges need to be the same.
    return other_range->IsMonotonicValueRange() &&
           GetLower().Equals(other_range->GetLower()) &&
           increment_ == other_range->AsMonotonicValueRange()->increment_;
  }

  ValueRange* Narrow(ValueRange* range) OVERRIDE {
    DCHECK(!range->IsMonotonicValueRange());

    if (increment_ > 0) {
      // Monotonically increasing.
      if (!range->GetUpper().IsUseful()) {
        return this;
      }

      ValueBound lower = GetLower().NarrowLowerBound(range->GetLower());
      // Need to take care of overflow. Basically need to be able to prove
      // for any value that's <= range->GetUpper(), it won't be negative with value+increment.

      if (range->GetUpper().IsConstant()) {
        int constant = range->GetUpper().GetConstant();
        if (constant <= INT_MAX - increment_) {
          return new (GetAllocator()) ValueRange(GetAllocator(), lower, range->GetUpper());
        }
      }

      if (range->GetUpper().IsRelativeToArrayLength()) {
        ValueBound next_bound = range->GetUpper().Add(increment_);
        DCHECK(next_bound.IsRelativeToArrayLength());
        int constant = next_bound.GetConstant();
        if (constant <= 0) {
          // A value of (array.length + constant) won't overflow. Luckily this covers
          // the very common case of incrementing by 1, e.g.
          // 'for (int i=0; i<array.length; i++)', even if we don't make any assumptions
          // about the max array length. If we can make assumptions about the max array length,
          // e.g. if we can establish an upper bound of the max array length, due to the max
          // heap size, divided by the element size (such as 4 bytes for each integer array),
          // we can be more aggressive about the constant part that we know
          // (array.length + constant) won't overflow.
          return new (GetAllocator()) ValueRange(GetAllocator(), lower, range->GetUpper());
        }
      }

      // There might be overflow. Just pick range.
      return range;
    } else if (increment_ < 0) {
      // Monotonically decreasing.
      if (!range->GetLower().IsUseful()) {
        return this;
      }

      ValueBound upper = GetUpper().NarrowUpperBound(range->GetUpper());
      // Need to take care of underflow. Basically need to be able to prove
      // for any value that's >= range->GetLower(), it won't be positive with value+increment.

      if (range->GetLower().IsConstant()) {
        int constant = range->GetLower().GetConstant();
        if (constant >= INT_MIN - increment_) {
          return new (GetAllocator()) ValueRange(GetAllocator(), range->GetLower(), upper);
        }
      }

      // There might be underflow. Just pick range.
      return range;
    } else {
      LOG(FATAL) << "Unreachable";
      return this;
    }
  }

  bool IsUseful() const OVERRIDE { return true; }

  ValueRange* Add(int constant) const OVERRIDE {
    if (increment_ > 0) {
      return new (GetAllocator()) MonotonicValueRange(
          GetAllocator(), GetLower().Add(constant), GetUpper(), increment_);
    } else if (increment_ < 0) {
      return new (GetAllocator()) MonotonicValueRange(
          GetAllocator(), GetLower(), GetLower().Add(constant), increment_);
    } else {
      LOG(FATAL) << "Unreachable";
      return nullptr;
    }
  }

 private:
  const int increment_;

  DISALLOW_COPY_AND_ASSIGN(MonotonicValueRange);
};

class ValueRangeMapEntry : public ArenaObject<kArenaAllocMisc> {
 public:
  ValueRangeMapEntry(HInstruction* instruction, ValueRange* range)
      : instruction_(instruction), range_(range) {}

  HInstruction* GetInstruction() const { return instruction_; }
  ValueRange* GetValueRange() const { return range_; }

 private:
  HInstruction* const instruction_;
  ValueRange* const range_;

  DISALLOW_COPY_AND_ASSIGN(ValueRangeMapEntry);
};

/**
 * A node in the collision list of a ValueRangeMap.
 */
class ValueRangeCollisionNode : public ArenaObject<kArenaAllocMisc> {
 public:
  ValueRangeCollisionNode(ValueRangeMapEntry* entry, ValueRangeCollisionNode* next)
      : value_range_map_entry_(entry), next_(next) {}

  ValueRangeMapEntry* GetValueRangeMapEntry() const { return value_range_map_entry_; }
  ValueRangeCollisionNode* GetNext() const { return next_; }

 private:
  ValueRangeMapEntry* const value_range_map_entry_;
  ValueRangeCollisionNode* next_;

  DISALLOW_COPY_AND_ASSIGN(ValueRangeCollisionNode);
};

class ValueRangeMap : public ArenaObject<kArenaAllocMisc> {
 public:
  explicit ValueRangeMap(ArenaAllocator* allocator)
      : allocator_(allocator), number_of_entries_(0), collisions_(nullptr) {
    for (size_t i = 0; i < kDefaultNumberOfEntries; ++i) {
      table_[i] = nullptr;
    }
  }

  ValueRange* Lookup(HInstruction* instruction) const {
    if (number_of_entries_ == 0) {
      return nullptr;
    }

    size_t hash_code = instruction->ComputeHashCode();
    size_t index = hash_code % kDefaultNumberOfEntries;
    ValueRangeMapEntry* entry = table_[index];
    if (entry != nullptr && entry->GetInstruction() == instruction) {
      return entry->GetValueRange();
    }

    for (ValueRangeCollisionNode* node = collisions_; node != nullptr; node = node->GetNext()) {
      entry = node->GetValueRangeMapEntry();
      if (entry->GetInstruction() == instruction) {
        return entry->GetValueRange();
      }
    }

    return nullptr;
  }

  // Adds an instruction to the map.
  void Add(HInstruction* instruction, ValueRange* value_range) {
    size_t hash_code = instruction->ComputeHashCode();
    size_t index = hash_code % kDefaultNumberOfEntries;
    ValueRangeMapEntry* entry = new (allocator_)
        ValueRangeMapEntry(instruction, value_range);
    if (table_[index] == nullptr) {
      table_[index] = entry;
      ++number_of_entries_;
    } else {
      if (table_[index]->GetInstruction() == instruction) {
        // Update the entry.
        table_[index] = entry;
      } else {
        collisions_ = new (allocator_) ValueRangeCollisionNode(entry, collisions_);
        ++number_of_entries_;
      }
    }
  }

  bool IsEmpty() const { return number_of_entries_ == 0; }
  size_t GetNumberOfEntries() const { return number_of_entries_; }

 private:
  static constexpr size_t kDefaultNumberOfEntries = 16;

  ArenaAllocator* const allocator_;

  // The number of entries in the set.
  size_t number_of_entries_;

  // The internal implementation of the map. It uses a combination of a hash code based
  // fixed-size list, and a linked list to handle hash code collisions.
  // TODO: Tune the fixed size list original size, and support growing it.
  ValueRangeCollisionNode* collisions_;
  ValueRangeMapEntry* table_[kDefaultNumberOfEntries];

  DISALLOW_COPY_AND_ASSIGN(ValueRangeMap);
};

class BoundsCheckElimination : public HGraphVisitor {
 public:
  BoundsCheckElimination(HGraph* graph)
      : HGraphVisitor(graph),
        value_range_maps_(graph->GetArena(), graph->GetBlocks().Size()) {
    value_range_maps_.SetSize(graph->GetBlocks().Size());
  }

  void Run();

 private:
  ValueRangeMap* GetValueRangeMap(HBasicBlock* basic_block) {
    int block_id = basic_block->GetBlockId();
    if (value_range_maps_.Get(block_id) == nullptr) {
      value_range_maps_.Put(
          block_id,
          new (GetGraph()->GetArena()) ValueRangeMap(GetGraph()->GetArena()));
    }
    return value_range_maps_.Get(block_id);
  }

  // Traverse up the dominator tree to look for value range info.
  ValueRange* LookupValueRange(HInstruction* instruction, HBasicBlock* basic_block) {
    while (basic_block != nullptr) {
      ValueRange* range = GetValueRangeMap(basic_block)->Lookup(instruction);
      if (range != nullptr) {
        return range;
      }
      basic_block = basic_block->GetDominator();
    }
    // Didn't find any.
    return nullptr;
  }

  ValueBound GetValueBoundFromValue(HInstruction* instruction);
  void ApplyLowerBound(HInstruction* instruction, HBasicBlock* basic_block,
                       HBasicBlock* successor, ValueBound bound);
  void ApplyUpperBound(HInstruction* instruction, HBasicBlock* basic_block,
                       HBasicBlock* successor, ValueBound bound);

  void VisitBoundsCheck(HBoundsCheck* bounds_check);
  void ReplaceBoundsCheck(HInstruction* bounds_check, HInstruction* index);
  void VisitPhi(HPhi* phi);
  void VisitIf(HIf* instruction);
  void HandleIf(HIf* instruction, HInstruction* left, HInstruction* right, IfCondition cond);
  void VisitAdd(HAdd* add);
  void VisitArrayGet(HArrayGet* array_get);
  void VisitArraySet(HArraySet* array_set);
  void HandleArrayAccess(HBoundsCheck* bounds_check);

  GrowableArray<ValueRangeMap*> value_range_maps_;

  DISALLOW_COPY_AND_ASSIGN(BoundsCheckElimination);
};

}  // namespace art

#endif  // ART_COMPILER_OPTIMIZING_BOUNDS_CHECK_ELIMINATION_H_
