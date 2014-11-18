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

public class Main {

  public static void expectEquals(int expected, int result) {
    if (expected != result) {
      throw new Error("Expected: " + expected + ", found: " + result);
    }
  }

  public static void expectEquals(long expected, long result) {
    if (expected != result) {
      throw new Error("Expected: " + expected + ", found: " + result);
    }
  }

  public static void main(String[] args) {
    shlInt();
    shlLong();
    shrInt();
    shrLong();
    ushrInt();
    ushrLong();
  }

  private static void shlInt() {
    expectEquals(-48, $opt$Shl(-12, 2));
    expectEquals(1024, $opt$Shl(32, 5));

    expectEquals(7, $opt$Shl(7, 0));
    expectEquals(14, $opt$Shl(7, 1));
    expectEquals(0, $opt$Shl(0, 30));


    expectEquals(1073741824L, $opt$Shl(1, 30));
    expectEquals(Integer.MIN_VALUE, $opt$Shl(1, 31));  // overflow
    expectEquals(Integer.MIN_VALUE, $opt$Shl(1073741824, 1));  // overflow
    expectEquals(1073741824, $opt$Shl(268435456, 2));

    // only 5 lower bits should be used for shifting (& 0x1f)
    expectEquals(7, $opt$Shl(7, 32));  // 32 & 0x1f = 0
    expectEquals(14, $opt$Shl(7, 33));  // 33 & 0x1f = 1
    expectEquals(32, $opt$Shl(1, 101));  // 101 & 0x1f = 5

    expectEquals(Integer.MIN_VALUE, $opt$Shl(1, -1));  // -1 & 0x1f = 31
    expectEquals(14, $opt$Shl(7, -31));  // -31 & 0x1f = 1
    expectEquals(7, $opt$Shl(7, -32));  // -32 & 0x1f = 0
    expectEquals(-536870912, $opt$Shl(7, -3));  // -3 & 0x1f = 29

    expectEquals(Integer.MIN_VALUE, $opt$Shl(7, Integer.MAX_VALUE));
    expectEquals(7, $opt$Shl(7, Integer.MIN_VALUE));
  }

  private static void shlLong() {
    expectEquals(-48L, $opt$Shl(-12L, 2L));
    expectEquals(1024L, $opt$Shl(32L, 5L));

    expectEquals(7L, $opt$Shl(7L, 0L));
    expectEquals(14L, $opt$Shl(7L, 1L));
    expectEquals(0L, $opt$Shl(0L, 30L));

    expectEquals(1073741824L, $opt$Shl(1L, 30L));
    expectEquals(2147483648L, $opt$Shl(1L, 31L));
    expectEquals(2147483648L, $opt$Shl(1073741824L, 1L));

    // with long we can use up to 6 lower bits
    expectEquals(4294967296L, $opt$Shl(1L, 32L));
    expectEquals(60129542144L, $opt$Shl(7L, 33L));
    expectEquals(Long.MIN_VALUE, $opt$Shl(1L, 63L));  // overflow

    // only 6 lower bits should be used for shifting (& 0x3f)
    expectEquals(7L, $opt$Shl(7L, 64L));  // 64 & 0x3f = 0
    expectEquals(14L, $opt$Shl(7L, 65L));  // 65 & 0x3f = 1
    expectEquals(137438953472L, $opt$Shl(1L, 101L));  // 101 & 0x3f = 37

    expectEquals(Long.MIN_VALUE, $opt$Shl(1L, -1L));  // -1 & 0x3f = 63
    expectEquals(14L, $opt$Shl(7L, -63L));  // -63 & 0x3f = 1
    expectEquals(7L, $opt$Shl(7L, -64L));  // -64 & 0x3f = 0
    expectEquals(2305843009213693952L, $opt$Shl(1L, -3L));  // -3 & 0x3f = 61

    expectEquals(Long.MIN_VALUE, $opt$Shl(7L, Long.MAX_VALUE));
    expectEquals(7L, $opt$Shl(7L, Long.MIN_VALUE));
  }

private static void shrInt() {
    expectEquals(-3, $opt$Shr(-12, 2));
    expectEquals(1, $opt$Shr(32, 5));

    expectEquals(7, $opt$Shr(7, 0));
    expectEquals(3, $opt$Shr(7, 1));
    expectEquals(0, $opt$Shr(0, 30));
    expectEquals(0, $opt$Shr(1, 30));
    expectEquals(-1, $opt$Shr(-1, 30));

    expectEquals(0, $opt$Shr(Integer.MAX_VALUE, 31));
    expectEquals(-1, $opt$Shr(Integer.MIN_VALUE, 31));

    // only 5 lower bits should be used for shifting (& 0x1f)
    expectEquals(7, $opt$Shr(7, 32));  // 32 & 0x1f = 0
    expectEquals(3, $opt$Shr(7, 33));  // 33 & 0x1f = 1

    expectEquals(0, $opt$Shr(1, -1));  // -1 & 0x1f = 31
    expectEquals(3, $opt$Shr(7, -31));  // -31 & 0x1f = 1
    expectEquals(7, $opt$Shr(7, -32));  // -32 & 0x1f = 0
    expectEquals(-4, $opt$Shr(Integer.MIN_VALUE, -3));  // -3 & 0x1f = 29

    expectEquals(0, $opt$Shr(7, Integer.MAX_VALUE));
    expectEquals(7, $opt$Shr(7, Integer.MIN_VALUE));
  }

  private static void shrLong() {
    expectEquals(-3L, $opt$Shr(-12L, 2L));
    expectEquals(1, $opt$Shr(32, 5));

    expectEquals(7L, $opt$Shr(7L, 0L));
    expectEquals(3L, $opt$Shr(7L, 1L));
    expectEquals(0L, $opt$Shr(0L, 30L));
    expectEquals(0L, $opt$Shr(1L, 30L));
    expectEquals(-1L, $opt$Shr(-1L, 30L));


    expectEquals(1L, $opt$Shr(1073741824L, 30L));
    expectEquals(1L, $opt$Shr(2147483648L, 31L));
    expectEquals(1073741824L, $opt$Shr(2147483648L, 1L));

    // with long we can use up to 6 lower bits
    expectEquals(1L, $opt$Shr(4294967296L, 32L));
    expectEquals(7L, $opt$Shr(60129542144L, 33L));
    expectEquals(0L, $opt$Shr(Long.MAX_VALUE, 63L));
    expectEquals(-1L, $opt$Shr(Long.MIN_VALUE, 63L));

    // only 6 lower bits should be used for shifting (& 0x3f)
    expectEquals(7L, $opt$Shr(7L, 64L));  // 64 & 0x3f = 0
    expectEquals(3L, $opt$Shr(7L, 65L));  // 65 & 0x3f = 1

    expectEquals(-1L, $opt$Shr(Long.MIN_VALUE, -1L));  // -1 & 0x3f = 63
    expectEquals(3L, $opt$Shr(7L, -63L));  // -63 & 0x3f = 1
    expectEquals(7L, $opt$Shr(7L, -64L));  // -64 & 0x3f = 0
    expectEquals(1L, $opt$Shr(2305843009213693952L, -3L));  // -3 & 0x3f = 61
    expectEquals(-4L, $opt$Shr(Integer.MIN_VALUE, -3));  // -3 & 0x1f = 29

    expectEquals(0L, $opt$Shr(7L, Long.MAX_VALUE));
    expectEquals(7L, $opt$Shr(7L, Long.MIN_VALUE));
  }

  private static void ushrInt() {
    expectEquals(1073741821, $opt$UShr(-12, 2));
    expectEquals(1, $opt$UShr(32, 5));

    expectEquals(7, $opt$UShr(7, 0));
    expectEquals(3, $opt$UShr(7, 1));
    expectEquals(0, $opt$UShr(0, 30));
    expectEquals(0, $opt$UShr(1, 30));
    expectEquals(3, $opt$UShr(-1, 30));

    expectEquals(0, $opt$UShr(Integer.MAX_VALUE, 31));
    expectEquals(1, $opt$UShr(Integer.MIN_VALUE, 31));

    // only 5 lower bits should be used for shifting (& 0x1f)
    expectEquals(7, $opt$UShr(7, 32));  // 32 & 0x1f = 0
    expectEquals(3, $opt$UShr(7, 33));  // 33 & 0x1f = 1

    expectEquals(0, $opt$UShr(1, -1));  // -1 & 0x1f = 31
    expectEquals(3, $opt$UShr(7, -31));  // -31 & 0x1f = 1
    expectEquals(7, $opt$UShr(7, -32));  // -32 & 0x1f = 0
    expectEquals(4, $opt$UShr(Integer.MIN_VALUE, -3));  // -3 & 0x1f = 29

    expectEquals(0, $opt$UShr(7, Integer.MAX_VALUE));
    expectEquals(7, $opt$UShr(7, Integer.MIN_VALUE));
  }

  private static void ushrLong() {
    expectEquals(4611686018427387901L, $opt$UShr(-12L, 2L));
    expectEquals(1, $opt$UShr(32, 5));

    expectEquals(7L, $opt$UShr(7L, 0L));
    expectEquals(3L, $opt$UShr(7L, 1L));
    expectEquals(0L, $opt$UShr(0L, 30L));
    expectEquals(0L, $opt$UShr(1L, 30L));
    expectEquals(17179869183L, $opt$UShr(-1L, 30L));


    expectEquals(1L, $opt$UShr(1073741824L, 30L));
    expectEquals(1L, $opt$UShr(2147483648L, 31L));
    expectEquals(1073741824L, $opt$UShr(2147483648L, 1L));

    // with long we can use up to 6 lower bits
    expectEquals(1L, $opt$UShr(4294967296L, 32L));
    expectEquals(7L, $opt$UShr(60129542144L, 33L));
    expectEquals(0L, $opt$UShr(Long.MAX_VALUE, 63L));
    expectEquals(1L, $opt$UShr(Long.MIN_VALUE, 63L));

    // only 6 lower bits should be used for shifting (& 0x3f)
    expectEquals(7L, $opt$UShr(7L, 64L));  // 64 & 0x3f = 0
    expectEquals(3L, $opt$UShr(7L, 65L));  // 65 & 0x3f = 1

    expectEquals(1L, $opt$UShr(Long.MIN_VALUE, -1L));  // -1 & 0x3f = 63
    expectEquals(3L, $opt$UShr(7L, -63L));  // -63 & 0x3f = 1
    expectEquals(7L, $opt$UShr(7L, -64L));  // -64 & 0x3f = 0
    expectEquals(1L, $opt$UShr(2305843009213693952L, -3L));  // -3 & 0x3f = 61
    expectEquals(4L, $opt$UShr(Long.MIN_VALUE, -3L));  // -3 & 0x3f = 61

    expectEquals(0L, $opt$UShr(7L, Long.MAX_VALUE));
    expectEquals(7L, $opt$UShr(7L, Long.MIN_VALUE));
  }

  static int $opt$Shl(int a, int b) {
    return a << b;
  }

  static long $opt$Shl(long a, long b) {
    return a << b;
  }

  static int $opt$Shr(int a, int b) {
    return a >> b;
  }

  static long $opt$Shr(long a, long b) {
    return a >> b;
  }

  static int $opt$UShr(int a, int b) {
    return a >>> b;
  }

  static long $opt$UShr(long a, long b) {
    return a >>> b;
  }

}
