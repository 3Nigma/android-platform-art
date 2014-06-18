#
# Copyright (C) 2011 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

LOCAL_PATH := art

RUNTIME_GTEST_COMMON_SRC_FILES := \
	runtime/arch/arch_test.cc \
	runtime/arch/stub_test.cc \
	runtime/barrier_test.cc \
	runtime/base/bit_field_test.cc \
	runtime/base/bit_vector_test.cc \
	runtime/base/hex_dump_test.cc \
	runtime/base/histogram_test.cc \
	runtime/base/mutex_test.cc \
	runtime/base/scoped_flock_test.cc \
	runtime/base/timing_logger_test.cc \
	runtime/base/unix_file/fd_file_test.cc \
	runtime/base/unix_file/mapped_file_test.cc \
	runtime/base/unix_file/null_file_test.cc \
	runtime/base/unix_file/random_access_file_utils_test.cc \
	runtime/base/unix_file/string_file_test.cc \
	runtime/class_linker_test.cc \
	runtime/dex_file_test.cc \
	runtime/dex_file_verifier_test.cc \
	runtime/dex_instruction_visitor_test.cc \
	runtime/dex_method_iterator_test.cc \
	runtime/entrypoints/math_entrypoints_test.cc \
	runtime/entrypoints/quick/quick_trampoline_entrypoints_test.cc \
	runtime/entrypoints_order_test.cc \
	runtime/exception_test.cc \
	runtime/gc/accounting/space_bitmap_test.cc \
	runtime/gc/heap_test.cc \
	runtime/gc/space/dlmalloc_space_base_test.cc \
	runtime/gc/space/dlmalloc_space_static_test.cc \
	runtime/gc/space/dlmalloc_space_random_test.cc \
	runtime/gc/space/rosalloc_space_base_test.cc \
	runtime/gc/space/rosalloc_space_static_test.cc \
	runtime/gc/space/rosalloc_space_random_test.cc \
	runtime/gc/space/large_object_space_test.cc \
	runtime/gtest_test.cc \
	runtime/handle_scope_test.cc \
	runtime/indenter_test.cc \
	runtime/indirect_reference_table_test.cc \
	runtime/instruction_set_test.cc \
	runtime/intern_table_test.cc \
	runtime/leb128_test.cc \
	runtime/mem_map_test.cc \
	runtime/mirror/dex_cache_test.cc \
	runtime/mirror/object_test.cc \
	runtime/parsed_options_test.cc \
	runtime/reference_table_test.cc \
	runtime/thread_pool_test.cc \
	runtime/transaction_test.cc \
	runtime/utils_test.cc \
	runtime/verifier/method_verifier_test.cc \
	runtime/verifier/reg_type_test.cc \
	runtime/zip_archive_test.cc

COMPILER_GTEST_COMMON_SRC_FILES := \
	runtime/jni_internal_test.cc \
	runtime/proxy_test.cc \
	runtime/reflection_test.cc \
	compiler/dex/local_value_numbering_test.cc \
	compiler/dex/mir_optimization_test.cc \
	compiler/driver/compiler_driver_test.cc \
	compiler/elf_writer_test.cc \
	compiler/image_test.cc \
	compiler/jni/jni_compiler_test.cc \
	compiler/oat_test.cc \
	compiler/optimizing/codegen_test.cc \
	compiler/optimizing/dominator_test.cc \
	compiler/optimizing/find_loops_test.cc \
	compiler/optimizing/graph_test.cc \
	compiler/optimizing/linearize_test.cc \
	compiler/optimizing/liveness_test.cc \
	compiler/optimizing/live_interval_test.cc \
	compiler/optimizing/live_ranges_test.cc \
	compiler/optimizing/parallel_move_test.cc \
	compiler/optimizing/pretty_printer_test.cc \
	compiler/optimizing/register_allocator_test.cc \
	compiler/optimizing/ssa_test.cc \
	compiler/output_stream_test.cc \
	compiler/utils/arena_allocator_test.cc \
	compiler/utils/dedupe_set_test.cc \
	compiler/utils/arm/managed_register_arm_test.cc \
	compiler/utils/arm64/managed_register_arm64_test.cc \
	compiler/utils/x86/managed_register_x86_test.cc \

ifeq ($(ART_SEA_IR_MODE),true)
COMPILER_GTEST_COMMON_SRC_FILES += \
	compiler/utils/scoped_hashtable_test.cc \
	compiler/sea_ir/types/type_data_test.cc \
	compiler/sea_ir/types/type_inference_visitor_test.cc \
	compiler/sea_ir/ir/regions_test.cc
endif

RUNTIME_GTEST_TARGET_SRC_FILES := \
	$(RUNTIME_GTEST_COMMON_SRC_FILES)

RUNTIME_GTEST_HOST_SRC_FILES := \
	$(RUNTIME_GTEST_COMMON_SRC_FILES)

COMPILER_GTEST_TARGET_SRC_FILES := \
	$(COMPILER_GTEST_COMMON_SRC_FILES)

COMPILER_GTEST_HOST_SRC_FILES := \
	$(COMPILER_GTEST_COMMON_SRC_FILES) \
	compiler/utils/x86/assembler_x86_test.cc \
	compiler/utils/x86_64/assembler_x86_64_test.cc

ART_TEST_CFLAGS :=
ifeq ($(ART_USE_PORTABLE_COMPILER),true)
  ART_TEST_CFLAGS += -DART_USE_PORTABLE_COMPILER=1
endif

ART_TEST_HOST_GTEST$(ART_PHONY_TEST_HOST_SUFFIX)_RULES :=
ART_TEST_HOST_GTEST$(2ND_ART_PHONY_TEST_HOST_SUFFIX)_RULES :=
ART_TEST_HOST_GTEST_RULES :=
ART_TEST_HOST_VALGRIND_GTEST$(ART_PHONY_TEST_HOST_SUFFIX)_RULES :=
ART_TEST_HOST_VALGRIND_GTEST$(2ND_ART_PHONY_TEST_HOST_SUFFIX)_RULES :=
ART_TEST_HOST_VALGRIND_GTEST_RULES :=
ART_TEST_TARGET_GTEST$(ART_PHONY_TEST_TARGET_SUFFIX)_RULES :=
ART_TEST_TARGET_GTEST$(2ND_ART_PHONY_TEST_TARGET_SUFFIX)_RULES :=
ART_TEST_TARGET_GTEST_RULES :=

# Define a make rule for a target device gtest.
# $(1): gtest name - the name of the test we're building such as leb128_test.
# $(2): 2ND_ or undefined - used to differentiate between the primary and secondary architecture.
define define-art-gtest-rule-target
	target_gtest_rule := test-art-target-gtest-$(1)$$($(2)ART_PHONY_TEST_TARGET_SUFFIX)

.PHONY: $$(target_gtest_rule)
$$(target_gtest_rule): $(ART_NATIVETEST_OUT)/$(TARGET_$(2)ARCH)/$(1) test-art-target-sync
	adb shell touch $(ART_TEST_DIR)/$(TARGET_$(2)ARCH)/$$@-$$$$PPID
	adb shell rm $(ART_TEST_DIR)/$(TARGET_$(2)ARCH)/$$@-$$$$PPID
	adb shell chmod 755 $(ART_NATIVETEST_DIR)/$(TARGET_$(2)ARCH)/$$(notdir $$<)
	adb shell sh -c "$(ART_NATIVETEST_DIR)/$(TARGET_$(2)ARCH)/$$(notdir $$<) \
		&& touch $(ART_TEST_DIR)/$(TARGET_$(2)ARCH)/$$@-$$$$PPID"
	$(hide) (adb pull $(ART_TEST_DIR)/$(TARGET_$(2)ARCH)/$$@-$$$$PPID /tmp/ && echo $$@ PASSED) \
		|| (echo $$@ FAILED && exit 1)
	$(hide) rm /tmp/$$@-$$$$PPID

  ART_TEST_TARGET_GTEST$($(2)ART_PHONY_TEST_TARGET_SUFFIX)_RULES += $$(target_gtest_rule)
	ART_TEST_TARGET_GTEST_RULES += $$(target_gtest_rule)
  ART_TEST_TARGET_GTEST_$(1)_RULES += $$(target_gtest_rule)
  
  # Clear locally used variables.
  target_gtest_rule :=
endef  # define-art-gtest-rule-target

# Define make rules for a host gtests.
# $(1): gtest name - the name of the test we're building such as leb128_test.
# $(2): 2ND_ or undefined - used to differentiate between the primary and secondary architecture.
define define-art-gtest-rule-host
	host_gtest_rule := test-art-host-gtest-$(1)$$($(2)ART_PHONY_TEST_HOST_SUFFIX)
  host_gtest_exe := $(HOST_OUT_EXECUTABLES)/$(1)$$($(2)ART_PHONY_TEST_HOST_SUFFIX)
	
.PHONY: $$(host_gtest_rule)
$$(host_gtest_rule): $$(host_gtest_exe) test-art-host-dependencies
	$$<
	@echo $$@ PASSED

  ART_TEST_HOST_GTEST$($(2)ART_PHONY_TEST_HOST_SUFFIX)_RULES += $$(host_gtest_rule)
	ART_TEST_HOST_GTEST_RULES += $$(host_gtest_rule)
  ART_TEST_HOST_GTEST_$(1)_RULES += $$(host_gtest_rule)

.PHONY: valgrind-$$(host_gtest_rule)
valgrind-$$(host_gtest_rule): $$(host_gtest_exe) test-art-host-dependencies
	valgrind --leak-check=full --error-exitcode=1 $$<
	@echo $$@ PASSED

  ART_TEST_HOST_VALGRIND_GTEST$($(2)ART_PHONY_TEST_HOST_SUFFIX)_RULES += valgrind-$$(host_gtest_rule)
	ART_TEST_HOST_VALGRIND_GTEST_RULES += valgrind-$$(host_gtest_rule)
  ART_TEST_HOST_VALGRIND_GTEST_$(1)_RULES += valgrind-$$(host_gtest_rule)

  # Clear locally used variables.
  host_gtest_rule :=
  host_gtest_exe :=
endef  # define-art-gtest-rule-host

# Define the rules to build and run host and target gtests.
# $(1): target or host
# $(2): file name
# $(3): extra C includes
# $(4): extra shared libraries
define define-art-gtest
  ifneq ($(1),target)
    ifneq ($(1),host)
      $$(error expected target or host for argument 1, received $(1))
    endif
  endif

  art_target_or_host := $(1)
  art_gtest_filename := $(2)
  art_gtest_extra_c_includes := $(3)
  art_gtest_extra_shared_libraries := $(4)

  include $(CLEAR_VARS)
  art_gtest_name := $$(notdir $$(basename $$(art_gtest_filename)))
  LOCAL_MODULE := $$(art_gtest_name)
  ifeq ($$(art_target_or_host),target)
    LOCAL_MODULE_TAGS := tests
  endif
  LOCAL_CPP_EXTENSION := $(ART_CPP_EXTENSION)
  LOCAL_SRC_FILES := $$(art_gtest_filename) runtime/common_runtime_test.cc
  LOCAL_C_INCLUDES += $(ART_C_INCLUDES) art/runtime $$(art_gtest_extra_c_includes)
  LOCAL_SHARED_LIBRARIES += libartd $$(art_gtest_extra_shared_libraries)
  # dex2oatd is needed to go with libartd
  LOCAL_REQUIRED_MODULES := dex2oatd

  LOCAL_ADDITIONAL_DEPENDENCIES := art/build/Android.common.mk
  # LOCAL_ADDITIONAL_DEPENDENCIES += art/build/Android.gtest.mk

  # Mac OS linker doesn't understand --export-dynamic.
  ifneq ($(HOST_OS)-$$(art_target_or_host),darwin-host)
    # Allow jni_compiler_test to find Java_MyClassNatives_bar within itself using dlopen(NULL, ...).
    LOCAL_LDFLAGS := -Wl,--export-dynamic -Wl,-u,Java_MyClassNatives_bar -Wl,-u,Java_MyClassNatives_sbar
  endif

  LOCAL_CFLAGS := $(ART_TEST_CFLAGS)
  include external/libcxx/libcxx.mk
  ifeq ($$(art_target_or_host),target)
  	$(call set-target-local-clang-vars)
  	$(call set-target-local-cflags-vars,debug)
    LOCAL_SHARED_LIBRARIES += libdl libicuuc libicui18n libnativehelper libz libcutils libvixl
    LOCAL_STATIC_LIBRARIES += libgtest_libc++
    LOCAL_MODULE_PATH_32 := $(ART_NATIVETEST_OUT)/$(ART_TARGET_ARCH_32)
    LOCAL_MODULE_PATH_64 := $(ART_NATIVETEST_OUT)/$(ART_TARGET_ARCH_64)
    LOCAL_MULTILIB := both
    include $(BUILD_EXECUTABLE)

    ART_TEST_TARGET_GTEST_$$(art_gtest_name)_RULES :=
    ifdef TARGET_2ND_ARCH
      $(call define-art-gtest-rule-target,$$(art_gtest_name),2ND_)
    endif
    $(call define-art-gtest-rule-target,$$(art_gtest_name),)

		# A rule to run the different architecture versions of the gtest.
.PHONY: test-art-target-gtest-$$(art_gtest_name)
test-art-target-gtest-$$(art_gtest_name): $$(ART_TEST_TARGET_GTEST_$$(art_gtest_name)_RULES)
	@echo $$@ PASSED

    # Clear locally used variables.
    ART_TEST_TARGET_GTEST_$(art_gtest_name)_RULES :=
  else # host
    LOCAL_CLANG := $(ART_HOST_CLANG)
    LOCAL_CFLAGS += $(ART_HOST_CFLAGS) $(ART_HOST_DEBUG_CFLAGS)
    LOCAL_SHARED_LIBRARIES += libicuuc-host libicui18n-host libnativehelper libz-host
    LOCAL_STATIC_LIBRARIES += libcutils libvixl
    ifneq ($(WITHOUT_HOST_CLANG),true)
      # GCC host compiled tests fail with this linked, presumably due to destructors that run.
      LOCAL_STATIC_LIBRARIES += libgtest_libc++_host
    endif
    LOCAL_LDLIBS += -lpthread -ldl
    LOCAL_IS_HOST_MODULE := true
    LOCAL_MULTILIB := both
		LOCAL_MODULE_STEM_32 := $$(art_gtest_name)32
		LOCAL_MODULE_STEM_64 := $$(art_gtest_name)64
    include $(BUILD_HOST_EXECUTABLE)

    ART_TEST_HOST_GTEST_$$(art_gtest_name)_RULES :=
    ifneq ($(HOST_PREFER_32_BIT),true)
      $(call define-art-gtest-rule-host,$$(art_gtest_name),2ND_)
    endif
    $(call define-art-gtest-rule-host,$$(art_gtest_name),)

		# A rule to run the different architecture versions of the gtest.
.PHONY: test-art-host-gtest-$$(art_gtest_name)
test-art-host-gtest-$$(art_gtest_name): $$(ART_TEST_HOST_GTEST_$$(art_gtest_name)_RULES)
	@echo $$@ PASSED

    # Clear locally used variables.
    ART_TEST_HOST_GTEST_$$(art_gtest_name)_RULES :=
  endif  # host_or_target
  
  # Clear locally used variables.
  art_target_or_host :=
  art_gtest_filename :=
  art_gtest_extra_c_includes :=
  art_gtest_extra_shared_libraries :=
  art_gtest_name :=
endef  # define-art-gtest

ifeq ($(ART_BUILD_TARGET),true)
TEST=$(foreach file,$(RUNTIME_GTEST_TARGET_SRC_FILES), $(call define-art-gtest,target,$(file),,))
$(info $(TEST))

  $(foreach file,$(RUNTIME_GTEST_TARGET_SRC_FILES), $(eval $(call define-art-gtest,target,$(file),,)))
  $(foreach file,$(COMPILER_GTEST_TARGET_SRC_FILES), $(eval $(call define-art-gtest,target,$(file),art/compiler,libartd-compiler)))
endif
ifeq ($(ART_BUILD_HOST),true)
  $(foreach file,$(RUNTIME_GTEST_HOST_SRC_FILES), $(eval $(call define-art-gtest,host,$(file),,)))
  $(foreach file,$(COMPILER_GTEST_HOST_SRC_FILES), $(eval $(call define-art-gtest,host,$(file),art/compiler,libartd-compiler)))
endif

# Used outside the art project to get a list of the current tests
RUNTIME_TARGET_GTEST_MAKE_TARGETS :=
$(foreach file, $(RUNTIME_GTEST_TARGET_SRC_FILES), $(eval RUNTIME_TARGET_GTEST_MAKE_TARGETS += $$(notdir $$(basename $$(file)))))
COMPILER_TARGET_GTEST_MAKE_TARGETS :=
$(foreach file, $(COMPILER_GTEST_TARGET_SRC_FILES), $(eval COMPILER_TARGET_GTEST_MAKE_TARGETS += $$(notdir $$(basename $$(file)))))

# "mm test-art-host-gtest" to build and run all host gtests.
.PHONY: test-art-host-gtest
test-art-host-gtest: $(ART_TEST_HOST_GTEST_RULES)
	@echo $@ PASSED

# Primary architecture variants.
.PHONY: test-art-host-gtest$(ART_PHONY_TEST_HOST_SUFFIX)
test-art-host-gtest$(ART_PHONY_TEST_HOST_SUFFIX): $(ART_TEST_HOST_GTEST$(ART_PHONY_TEST_HOST_SUFFIX)_RULES)
	@echo $@ PASSED

# Secondary architecture variants.
ifneq ($(HOST_PREFER_32_BIT),true)
.PHONY: test-art-host-gtest$(2ND_ART_PHONY_TEST_HOST_SUFFIX)
test-art-host-gtest$(2ND_ART_PHONY_TEST_HOST_SUFFIX): $(ART_TEST_HOST_GTEST$(2ND_ART_PHONY_TEST_HOST_SUFFIX)_RULES)
	@echo $@ PASSED $(ART_TEST_HOST_GTEST32_RULES)
endif

# "mm valgrind-test-art-host-gtest" to build and run the host gtests under valgrind.
.PHONY: valgrind-test-art-host-gtest
valgrind-test-art-host-gtest: $(ART_TEST_HOST_VALGRIND_GTEST_RULES)
	@echo $@ PASSED

	# Primary architecture variants.
.PHONY: valgrind-test-art-host-gtest$(ART_PHONY_TEST_HOST_SUFFIX)
valgrind-test-art-host-gtest$(ART_PHONY_TEST_HOST_SUFFIX): $(ART_TEST_HOST_VALGRIND_GTEST$(ART_PHONY_TEST_HOST_SUFFIX)_RULES)
	@echo $@ PASSED

# Secondary architecture variants.
ifneq ($(HOST_PREFER_32_BIT),true)
.PHONY: valgrind-test-art-host-gtest$(2ND_ART_PHONY_TEST_HOST_SUFFIX)
valgrind-test-art-host-gtest$(2ND_ART_PHONY_TEST_HOST_SUFFIX): $(ART_TEST_HOST_VALGRIND_GTEST$(2ND_ART_PHONY_TEST_HOST_SUFFIX)_RULES)
	@echo $@ PASSED
endif

# "mm test-art-target-gtest" to build and run all target gtests.
.PHONY: test-art-target-gtest
test-art-target-gtest: $(ART_TEST_TARGET_GTEST_RULES)
	@echo $@ PASSED

# Primary architecture variants.
.PHONY: test-art-target-gtest$(ART_PHONY_TEST_TARGET_SUFFIX)
test-art-target-gtest$(ART_PHONY_TEST_TARGET_SUFFIX): $(ART_TEST_TARGET_GTEST$(ART_PHONY_TEST_TARGET_SUFFIX)_RULES)
	@echo $@ PASSED

# Secondary architecture variants.
ifdef TARGET_2ND_ARCH
.PHONY: test-art-target-gtest$(2ND_ART_PHONY_TEST_TARGET_SUFFIX)
test-art-target-gtest$(2ND_ART_PHONY_TEST_TARGET_SUFFIX): $(ART_TEST_TARGET_GTEST$(2ND_ART_PHONY_TEST_TARGET_SUFFIX)_RULES)
	@echo $@ PASSED
endif

# Clear locally used variables.
RUNTIME_GTEST_COMMON_SRC_FILES :=
COMPILER_GTEST_COMMON_SRC_FILES :=
RUNTIME_GTEST_TARGET_SRC_FILES :=
RUNTIME_GTEST_HOST_SRC_FILES :=
COMPILER_GTEST_TARGET_SRC_FILES :=
COMPILER_GTEST_HOST_SRC_FILES :=
ART_TEST_CFLAGS :=
ART_TEST_HOST_GTEST$(ART_PHONY_TEST_HOST_SUFFIX)_RULES :=
ART_TEST_HOST_GTEST$(2ND_ART_PHONY_TEST_HOST_SUFFIX)_RULES :=
ART_TEST_HOST_GTEST_RULES :=
ART_TEST_HOST_VALGRIND_GTEST$(ART_PHONY_TEST_HOST_SUFFIX)_RULES :=
ART_TEST_HOST_VALGRIND_GTEST$(2ND_ART_PHONY_TEST_HOST_SUFFIX)_RULES :=
ART_TEST_HOST_VALGRIND_GTEST_RULES :=
ART_TEST_TARGET_GTEST$(ART_PHONY_TEST_TARGET_SUFFIX)_RULES :=
ART_TEST_TARGET_GTEST$(2ND_ART_PHONY_TEST_TARGET_SUFFIX)_RULES :=
ART_TEST_TARGET_GTEST_RULES :=
