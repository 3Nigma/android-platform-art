/*
 * Copyright (C) 2011 The Android Open Source Project
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

#include "string-inl.h"

#include "array.h"
#include "class-inl.h"
#include "gc/accounting/card_table-inl.h"
#include "intern_table.h"
#include "object-inl.h"
#include "runtime.h"
#include "sirt_ref.h"
#include "string-inl.h"
#include "thread.h"
#include "utf-inl.h"
#include "well_known_classes.h"

namespace art {
namespace mirror {

// TODO: get global references for these
Class* String::java_lang_String_ = NULL;

int32_t String::FastIndexOf(int32_t ch, int32_t start) {
  int32_t count = GetCount();
  if (start < 0) {
    start = 0;
  } else if (start > count) {
    start = count;
  }
  const uint16_t* chars = GetValue();
  const uint16_t* p = chars + start;
  const uint16_t* end = chars + count;
  while (p < end) {
    if (*p++ == ch) {
      return (p - 1) - chars;
    }
  }
  return -1;
}

void String::SetClass(Class* java_lang_String) {
  CHECK(java_lang_String_ == NULL);
  CHECK(java_lang_String != NULL);
  java_lang_String_ = java_lang_String;
}

void String::ResetClass() {
  CHECK(java_lang_String_ != NULL);
  java_lang_String_ = NULL;
}

int32_t String::GetHashCode() {
  int32_t result = GetField32(OFFSET_OF_OBJECT_MEMBER(String, hash_code_));
  if (UNLIKELY(result == 0)) {
    ComputeHashCode();
  }
  result = GetField32(OFFSET_OF_OBJECT_MEMBER(String, hash_code_));
  DCHECK(result != 0 || ComputeUtf16Hash(GetValue(), GetCount()) == 0)
          << ToModifiedUtf8() << " " << result;
  return result;
}

void String::ComputeHashCode() {
  SetHashCode(ComputeUtf16Hash(GetValue(), GetCount()));
}

int32_t String::GetUtfLength() {
  return CountUtf8Bytes(GetValue(), GetCount());
}

void String::SetCharAt(int32_t index, char c) {
  DCHECK((index >= 0) && (index < count_));
  GetValue()[index] = c;
}

String* String::AllocFromString(Thread* self, int32_t string_length, SirtRef<String>& string,
                                int32_t offset) {
  String* new_string = Alloc<true>(self, string_length);
  if (UNLIKELY(new_string == nullptr)) {
    return nullptr;
  }
  uint16_t* data = string->GetValue() + offset;
  uint16_t* new_value = new_string->GetValue();
  memcpy(new_value, data, string_length * sizeof(uint16_t));
  return new_string;
}

String* String::AllocFromStrings(Thread* self, SirtRef<String>& string, SirtRef<String>& string2) {
  int32_t length = string->GetCount();
  int32_t length2 = string2->GetCount();
  String* new_string = Alloc<true>(self, length + length2);
  if (UNLIKELY(new_string == nullptr)) {
    return nullptr;
  }
  uint16_t* new_value = new_string->GetValue();
  memcpy(new_value, string->GetValue(), length * sizeof(uint16_t));
  memcpy(new_value + length, string2->GetValue(), length2 * sizeof(uint16_t));
  return new_string;
}

String* String::AllocFromCharArray(Thread* self, int32_t array_length, SirtRef<CharArray>& array,
                                   int32_t offset) {
  String* new_string = Alloc<true>(self, array_length);
  if (UNLIKELY(new_string == nullptr)) {
    return nullptr;
  }
  uint16_t* data = array->GetData() + offset;
  uint16_t* new_value = new_string->GetValue();
  memcpy(new_value, data, array_length * sizeof(uint16_t));
  return new_string;
}

String* String::AllocFromUtf16(Thread* self, int32_t utf16_length, const uint16_t* utf16_data_in) {
  CHECK(utf16_data_in != nullptr || utf16_length == 0);
  String* string = Alloc<true>(self, utf16_length);
  if (UNLIKELY(string == nullptr)) {
    return nullptr;
  }
  uint16_t* array = string->GetValue();
  memcpy(array, utf16_data_in, utf16_length * sizeof(uint16_t));
  return string;
}

String* String::AllocFromByteArray(Thread* self, int32_t byte_length, SirtRef<ByteArray>& array,
                                   int32_t offset, int32_t high_byte) {
  String* string = Alloc<true>(self, byte_length);
  if (UNLIKELY(string == nullptr)) {
    return nullptr;
  }
  uint8_t* data = reinterpret_cast<uint8_t*>(array->GetData()) + offset;
  uint16_t* new_value = string->GetValue();
  high_byte <<= 8;
  for (int i = 0; i < byte_length; i++) {
    new_value[i] = high_byte + (data[i] & 0xFF);
  }
  return string;
}

String* String::AllocFromModifiedUtf8(Thread* self, const char* utf) {
  DCHECK(utf != nullptr);
  size_t char_count = CountModifiedUtf8Chars(utf);
  return AllocFromModifiedUtf8(self, char_count, utf);
}

String* String::AllocFromModifiedUtf8(Thread* self, int32_t utf16_length,
                                      const char* utf8_data_in) {
  String* string = Alloc<true>(self, utf16_length);
  if (UNLIKELY(string == nullptr)) {
    return nullptr;
  }
  uint16_t* utf16_data_out = string->GetValue();
  ConvertModifiedUtf8ToUtf16(utf16_data_out, utf8_data_in);
  return string;
}

bool String::Equals(String* that) {
  if (this == that) {
    // Quick reference equality test
    return true;
  } else if (that == NULL) {
    // Null isn't an instanceof anything
    return false;
  } else if (this->GetCount() != that->GetCount()) {
    // Quick length inequality test
    return false;
  } else {
    // Note: don't short circuit on hash code as we're presumably here as the
    // hash code was already equal
    for (int32_t i = 0; i < that->GetCount(); ++i) {
      if (this->CharAt(i) != that->CharAt(i)) {
        return false;
      }
    }
    return true;
  }
}

bool String::Equals(const uint16_t* that_chars, int32_t that_offset, int32_t that_length) {
  if (this->GetCount() != that_length) {
    return false;
  } else {
    for (int32_t i = 0; i < that_length; ++i) {
      if (this->CharAt(i) != that_chars[that_offset + i]) {
        return false;
      }
    }
    return true;
  }
}

bool String::Equals(const char* modified_utf8) {
  for (int32_t i = 0; i < GetCount(); ++i) {
    uint16_t ch = GetUtf16FromUtf8(&modified_utf8);
    if (ch == '\0' || ch != CharAt(i)) {
      return false;
    }
  }
  return *modified_utf8 == '\0';
}

bool String::Equals(const StringPiece& modified_utf8) {
  const char* p = modified_utf8.data();
  for (int32_t i = 0; i < GetCount(); ++i) {
    uint16_t ch = GetUtf16FromUtf8(&p);
    if (ch != CharAt(i)) {
      return false;
    }
  }
  return true;
}

// Create a modified UTF-8 encoded std::string from a java/lang/String object.
std::string String::ToModifiedUtf8() {
  const uint16_t* chars = GetValue();
  size_t byte_count = GetUtfLength();
  std::string result(byte_count, static_cast<char>(0));
  ConvertUtf16ToModifiedUtf8(&result[0], chars, GetCount());
  return result;
}

#ifdef HAVE__MEMCMP16
// "count" is in 16-bit units.
extern "C" uint32_t __memcmp16(const uint16_t* s0, const uint16_t* s1, size_t count);
#define MemCmp16 __memcmp16
#else
static uint32_t MemCmp16(const uint16_t* s0, const uint16_t* s1, size_t count) {
  for (size_t i = 0; i < count; i++) {
    if (s0[i] != s1[i]) {
      return static_cast<int32_t>(s0[i]) - static_cast<int32_t>(s1[i]);
    }
  }
  return 0;
}
#endif

int32_t String::CompareTo(String* rhs) {
  // Quick test for comparison of a string with itself.
  String* lhs = this;
  if (lhs == rhs) {
    return 0;
  }
  // TODO: is this still true?
  // The annoying part here is that 0x00e9 - 0xffff != 0x00ea,
  // because the interpreter converts the characters to 32-bit integers
  // *without* sign extension before it subtracts them (which makes some
  // sense since "char" is unsigned).  So what we get is the result of
  // 0x000000e9 - 0x0000ffff, which is 0xffff00ea.
  int lhsCount = lhs->GetCount();
  int rhsCount = rhs->GetCount();
  int countDiff = lhsCount - rhsCount;
  int minCount = (countDiff < 0) ? lhsCount : rhsCount;
  const uint16_t* lhsChars = lhs->GetValue();
  const uint16_t* rhsChars = rhs->GetValue();
  int otherRes = MemCmp16(lhsChars, rhsChars, minCount);
  if (otherRes != 0) {
    return otherRes;
  }
  return countDiff;
}

void String::VisitRoots(RootCallback* callback, void* arg) {
  if (java_lang_String_ != nullptr) {
    callback(reinterpret_cast<mirror::Object**>(&java_lang_String_), arg, 0, kRootStickyClass);
  }
}

CharArray* String::ToCharArray(Thread* self) {
  SirtRef<String> sirt_this(self, this);
  CharArray* result = CharArray::Alloc(self, GetCount());
  memcpy(result->GetData(), sirt_this->GetValue(), sirt_this->GetCount() * sizeof(uint16_t));
  return result;
}

void String::GetChars(int32_t start, int32_t end, SirtRef<CharArray>& array, int32_t index) {
  uint16_t* data = array->GetData() + index;
  uint16_t* value = GetValue() + start;
  memcpy(data, value, (end - start) * sizeof(uint16_t));
}

}  // namespace mirror
}  // namespace art
