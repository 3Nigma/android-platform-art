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

#ifndef ART_COMPILER_OPTIMIZING_GRAPH_VISUALIZER_H_
#define ART_COMPILER_OPTIMIZING_GRAPH_VISUALIZER_H_

#include <ostream>
#include <sstream>

#include "base/mutex.h"
#include "base/value_object.h"

namespace art {

class CodeGenerator;
class DexCompilationUnit;
class HGraph;

// TODO: Create an analysis/optimization abstraction.
static const char* kLivenessPassName = "liveness";
static const char* kRegisterAllocatorPassName = "register";

/**
 * If enabled, emits compilation information suitable for the c1visualizer tool
 * and IRHydra.
 * Currently only works if the compiler is single threaded.
 */
class HGraphVisualizer : public ValueObject {
 public:
  /**
   * If output is not null, and the method name of the dex compilation
   * unit contains `string_filter`, the compilation information will be
   * emitted.
   */
  HGraphVisualizer(std::ostream* output,
                   HGraph* graph,
                   const char* string_filter,
                   const CodeGenerator& codegen,
                   const DexCompilationUnit& cu);

  /**
   * Version of `HGraphVisualizer` for unit testing, that is when a
   * `DexCompilationUnit` is not available.
   */
  HGraphVisualizer(std::ostream* output,
                   HGraph* graph,
                   const CodeGenerator& codegen,
                   const char* name);

  ~HGraphVisualizer() {
    Finalize();
  }

  /**
   * If this visualizer is enabled, emit the compilation information
   * into `oss_`. Actual writing to `output_` will happen in Finalize().
   */
  void DumpGraph(const char* pass_name);

  /**
   * Write the temporary buffer from oss_ to output_.
   */
  void Finalize() const LOCKS_EXCLUDED(dump_mutex);

 private:
  // This is the final output stream.
  std::ostream* const output_;
  // This is for temporary internal output, so we can write everything in one go at the end.
  std::ostringstream oss_;
  HGraph* const graph_;
  const CodeGenerator& codegen_;

  // Is true when `output_` is not null, and the compiled method's name
  // contains the string_filter given in the constructor.
  bool is_enabled_;

  static Mutex dump_mutex DEFAULT_MUTEX_ACQUIRED_AFTER;

  DISALLOW_COPY_AND_ASSIGN(HGraphVisualizer);
};

}  // namespace art

#endif  // ART_COMPILER_OPTIMIZING_GRAPH_VISUALIZER_H_
