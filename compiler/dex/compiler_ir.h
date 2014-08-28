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

#ifndef ART_COMPILER_DEX_COMPILER_IR_H_
#define ART_COMPILER_DEX_COMPILER_IR_H_

#include <string>
#include <vector>

#include "compiler_enums.h"
#include "driver/compiler_driver.h"
#include "utils/scoped_arena_allocator.h"
#include "base/timing_logger.h"
#include "utils/arena_allocator.h"

namespace art {

class Backend;
class ClassLinker;
class MIRGraph;

/*
 * TODO: refactoring pass to move these (and other) typedefs towards usage style of runtime to
 * add type safety (see runtime/offsets.h).
 */
typedef uint32_t DexOffset;          // Dex offset in code units.
typedef uint16_t NarrowDexOffset;    // For use in structs, Dex offsets range from 0 .. 0xffff.
typedef uint32_t CodeOffset;         // Native code offset in bytes.

struct CompilationUnit {
  explicit CompilationUnit(ArenaPool* pool);
  ~CompilationUnit();

  void StartTimingSplit(const char* label);
  void NewTimingSplit(const char* label);
  void EndTiming();

  /*
   * Fields needed/generated by common frontend and generally used throughout
   * the compiler.
  */
  CompilerDriver* compiler_driver;
  ClassLinker* class_linker;           // Linker to resolve fields and methods.
  const DexFile* dex_file;             // DexFile containing the method being compiled.
  jobject class_loader;                // compiling method's class loader.
  uint16_t class_def_idx;              // compiling method's defining class definition index.
  uint32_t method_idx;                 // compiling method's index into method_ids of DexFile.
  const DexFile::CodeItem* code_item;  // compiling method's DexFile code_item.
  uint32_t access_flags;               // compiling method's access flags.
  InvokeType invoke_type;              // compiling method's invocation type.
  const char* shorty;                  // compiling method's shorty.
  uint32_t disable_opt;                // opt_control_vector flags.
  uint32_t enable_debug;               // debugControlVector flags.
  bool verbose;
  const Compiler* compiler;
  InstructionSet instruction_set;
  bool target64;

  InstructionSetFeatures GetInstructionSetFeatures() {
    return compiler_driver->GetInstructionSetFeatures();
  }

  // If non-empty, apply optimizer/debug flags only to matching methods.
  std::string compiler_method_match;
  // Flips sense of compiler_method_match - apply flags if doesn't match.
  bool compiler_flip_match;

  // TODO: move memory management to mir_graph, or just switch to using standard containers.
  ArenaAllocator arena;
  ArenaStack arena_stack;  // Arenas for ScopedArenaAllocator.

  std::unique_ptr<MIRGraph> mir_graph;   // MIR container.
  std::unique_ptr<Backend> cg;           // Target-specific codegen.
  TimingLogger timings;
  bool print_pass;                 // Do we want to print a pass or not?

  /**
   * @brief Holds pass options for current pass being applied to compilation unit.
   * @details This is updated for every pass to contain the overridden pass options
   * that were specified by user. The pass itself will check this to see if the
   * default settings have been changed. The key is simply the option string without
   * the pass name.
   */
  SafeMap<const std::string, int> overridden_pass_options;
};

}  // namespace art

#endif  // ART_COMPILER_DEX_COMPILER_IR_H_