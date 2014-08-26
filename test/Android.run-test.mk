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

LOCAL_PATH := $(call my-dir)

include art/build/Android.common_test.mk

# List of all tests of the form 003-omnibus-opcodes.
TEST_ART_RUN_TESTS := $(wildcard $(LOCAL_PATH)/[0-9]*)
TEST_ART_RUN_TESTS := $(subst $(LOCAL_PATH)/,, $(TEST_ART_RUN_TESTS))

########################################################################
# The art-run-tests module, used to build all run-tests into an image.

# The path where build only targets will be output, e.g.
# out/target/product/generic_x86_64/obj/PACKAGING/art-run-tests_intermediates/DATA
art_run_tests_dir := $(call intermediates-dir-for,PACKAGING,art-run-tests)/DATA

# A generated list of prerequisites that call 'run-test --build-only', the actual prerequisite is
# an empty file touched in the intermediate directory.
TEST_ART_RUN_TEST_BUILD_RULES :=

# Helper to create individual build targets for tests. Must be called with $(eval).
# $(1): the test number
define define-build-art-run-test
  dmart_target := $(art_run_tests_dir)/art-run-tests/$(1)/touch
$$(dmart_target): $(DX) $(HOST_OUT_EXECUTABLES)/jasmin
	$(hide) rm -rf $$(dir $$@) && mkdir -p $$(dir $$@)
	$(hide) DX=$(abspath $(DX)) JASMIN=$(abspath $(HOST_OUT_EXECUTABLES)/jasmin) \
	  $(LOCAL_PATH)/run-test --build-only --output-path $$(abspath $$(dir $$@)) $(1)
	$(hide) touch $$@

  TEST_ART_RUN_TEST_BUILD_RULES += $$(dmart_target)
  dmart_target :=
endef
$(foreach test, $(TEST_ART_RUN_TESTS), $(eval $(call define-build-art-run-test,$(test))))

include $(CLEAR_VARS)
LOCAL_MODULE_TAGS := tests
LOCAL_MODULE := art-run-tests
LOCAL_ADDITIONAL_DEPENDENCIES := $(TEST_ART_RUN_TEST_BUILD_RULES)
# The build system use this flag to pick up files generated by declare-make-art-run-test.
LOCAL_PICKUP_FILES := $(art_run_tests_dir)

include $(BUILD_PHONY_PACKAGE)

# Clear temp vars.
art_run_tests_dir :=
define-build-art-run-test :=
TEST_ART_RUN_TEST_BUILD_RULES :=

########################################################################
# General rules to build and run a run-test.

# Test rule names or of the form:
# test-art-{1: host or target}-run-test-{2: prebuild no-prebuild no-dex2oat}-
#    {3: interpreter default optimizing}-{4: relocate no-relocate relocate-no-patchoat}-
#    {5: trace or no-trace}-{6: gcstress gcverify cms}-{7: forcecopy checkjni jni}-
#    {8: no-image or image}-{9: test name}{10: 32 or 64}
TARGET_TYPES := host target
PREBUILD_TYPES := prebuild no-prebuild no-dex2oat
COMPILER_TYPES := default interpreter optimizing
RELOCATE_TYPES := relocate no-relocate relocate-no-patchoat
TRACE_TYPES := trace no-trace
GC_TYPES := gcstress gcverify cms
JNI_TYPES := jni checkjni forcecopy
IMAGE_TYPES := image no-image
ALL_ADDRESS_SIZES := 64 32
# List all run test names with number arguments agreeing with the comment above.
define all-run-test-names
  $(foreach target, $(1), \
    $(foreach prebuild, $(2), \
      $(foreach compiler, $(3), \
        $(foreach relocate, $(4), \
          $(foreach trace, $(5), \
            $(foreach gc, $(6), \
              $(foreach jni, $(7), \
                $(foreach image, $(8), \
                  $(foreach test, $(9), \
                    $(foreach address_size, $(10), \
                      test-art-$(target)-run-test-$(prebuild)-$(compiler)-$(relocate)-$(trace)-$(gc)-$(jni)-$(image)-$(test)$(address_size) \
                  ))))))))))
endef  # all-run-test-names

# To generate a full list or tests:
# $(call all-run-test-names,$(TARGET_TYPES),$(PREBUILD_TYPES),$(COMPILER_TYPES), \
#        $(RELOCATE_TYPES),$(TRACE_TYPES),$(GC_TYPES),$(JNI_TYPES),$(TEST_ART_RUN_TESTS), \
#        $(ALL_ADDRESS_SIZES))

# Convert's a rule name to the form used in variables, e.g. no-relocate to NO_RELOCATE
define name-to-var
$(shell echo $(1) | tr '[:lower:]' '[:upper:]' | tr '-' '_')
endef  # name-to-var

# Tests that are timing sensitive and flaky on heavily loaded systems.
TEST_ART_TIMING_SENSITIVE_RUN_TESTS := \
  053-wait-some \
  055-enum-performance

 # disable timing sensitive tests on "dist" builds.
ifdef dist_goal
  ART_TEST_KNOWN_BROKEN += $(call all-run-test-names,$(TARGET_TYPES),$(PREBUILD_TYPES), \
        $(COMPILER_TYPES), $(RELOCATE_TYPES),$(TRACE_TYPES),$(GC_TYPES),$(JNI_TYPES), \
        $(TEST_ART_TIMING_SENSITIVE_RUN_TESTS), $(ALL_ADDRESS_SIZES))
        $(IMAGE_TYPES), $(TEST_ART_TIMING_SENSITIVE_RUN_TESTS), $(ALL_ADDRESS_SIZES))
endif

# NB 116-nodex2oat is not broken per-se it just doesn't (and isn't meant to) work with --prebuild.
TEST_ART_BROKEN_PREBUILD_RUN_TESTS := \
  116-nodex2oat

ART_TEST_KNOWN_BROKEN += $(call all-run-test-names,$(TARGET_TYPES),prebuild, \
    $(COMPILER_TYPES), $(RELOCATE_TYPES),$(TRACE_TYPES),$(GC_TYPES),$(JNI_TYPES), \
    $(TEST_ART_BROKEN_PREBUILD_RUN_TESTS), $(ALL_ADDRESS_SIZES))

# NB 117-nopatchoat is not broken per-se it just doesn't work (and isn't meant to) without --prebuild --relocate
TEST_ART_BROKEN_NO_RELOCATE_TESTS := \
  117-nopatchoat

ART_TEST_KNOWN_BROKEN += $(call all-run-test-names,$(TARGET_TYPES),$(PREBUILD_TYPES), \
    $(COMPILER_TYPES), no-relocate,$(TRACE_TYPES),$(GC_TYPES),$(JNI_TYPES), \
    $(TEST_ART_BROKEN_NO_RELOCATE_TESTS), $(ALL_ADDRESS_SIZES))

TEST_ART_BROKEN_NO_PREBUILD_TESTS := \
  117-nopatchoat

ART_TEST_KNOWN_BROKEN += $(call all-run-test-names,$(TARGET_TYPES),no-prebuild, \
    $(COMPILER_TYPES), $(RELOCATE_TYPES),$(TRACE_TYPES),$(GC_TYPES),$(JNI_TYPES), \
    $(TEST_ART_BROKEN_NO_PREBUILD_TESTS), $(ALL_ADDRESS_SIZES))

# Tests that are broken with tracing.
TEST_ART_BROKEN_TRACE_RUN_TESTS := \
  004-SignalTest \
  018-stack-overflow \
  097-duplicate-method \
  107-int-math2

ART_TEST_KNOWN_BROKEN += $(call all-run-test-names,$(TARGET_TYPES),$(PREBUILD_TYPES), \
    $(COMPILER_TYPES), $(RELOCATE_TYPES),trace,$(GC_TYPES),$(JNI_TYPES), \
    $(IMAGE_TYPES), $(TEST_ART_BROKEN_TRACE_RUN_TESTS), $(ALL_ADDRESS_SIZES))

TEST_ART_BROKEN_TRACE_RUN_TESTS :=

# 115-native-bridge setup is complicated. Need to implement it correctly for the target.
ART_TEST_KNOWN_BROKEN += $(call all-run-test-names,target,$(PREBUILD_TYPES),$(COMPILER_TYPES), \
    $(RELOCATE_TYPES),$(TRACE_TYPES),$(GC_TYPES),$(JNI_TYPES),$(IMAGE_TYPES),115-native-bridge, \
    $(ALL_ADDRESS_SIZES))

# All these tests check that we have sane behavior if we don't have a patchoat or dex2oat.
# Therefore we shouldn't run them in situations where we actually don't have these since they
# explicitly test for them. These all also assume we have an image.
TEST_ART_BROKEN_FALLBACK_RUN_TESTS := \
  116-nodex2oat \
  117-nopatchoat \
  118-noimage-dex2oat \
  119-noimage-patchoat

ART_TEST_KNOWN_BROKEN += $(call all-run-test-names,$(TARGET_TYPES),no-dex2oat,$(COMPILER_TYPES), \
    $(RELOCATE_TYPES),$(TRACE_TYPES),$(GC_TYPES),$(JNI_TYPES),$(IMAGE_TYPES), \
    $(TEST_ART_BROKEN_FALLBACK_RUN_TESTS),$(ALL_ADDRESS_SIZES))

ART_TEST_KNOWN_BROKEN += $(call all-run-test-names,$(TARGET_TYPES),$(PREBUILD_TYPES),$(COMPILER_TYPES), \
    $(RELOCATE_TYPES),$(TRACE_TYPES),$(GC_TYPES),$(JNI_TYPES),no-image, \
    $(TEST_ART_BROKEN_FALLBACK_RUN_TESTS),$(ALL_ADDRESS_SIZES))

ART_TEST_KNOWN_BROKEN += $(call all-run-test-names,$(TARGET_TYPES),$(PREBUILD_TYPES),$(COMPILER_TYPES), \
    relocate-no-patchoat,$(TRACE_TYPES),$(GC_TYPES),$(JNI_TYPES),$(IMAGE_TYPES), \
    $(TEST_ART_BROKEN_FALLBACK_RUN_TESTS),$(ALL_ADDRESS_SIZES))

# Clear variables ahead of appending to them when defining tests.
$(foreach target, $(TARGET_TYPES), $(eval ART_RUN_TEST_$(call name-to-var,$(target))_RULES :=))
$(foreach target, $(TARGET_TYPES), \
  $(foreach prebuild, $(PREBUILD_TYPES), \
    $(eval ART_RUN_TEST_$(call name-to-var,$(target))_$(call name-to-var,$(prebuild))_RULES :=)))
$(foreach target, $(TARGET_TYPES), \
  $(foreach compiler, $(COMPILER_TYPES), \
    $(eval ART_RUN_TEST_$(call name-to-var,$(target))_$(call name-to-var,$(compiler))_RULES :=)))
$(foreach target, $(TARGET_TYPES), \
  $(foreach relocate, $(RELOCATE_TYPES), \
    $(eval ART_RUN_TEST_$(call name-to-var,$(target))_$(call name-to-var,$(relocate))_RULES :=)))
$(foreach target, $(TARGET_TYPES), \
  $(foreach trace, $(TRACE_TYPES), \
    $(eval ART_RUN_TEST_$(call name-to-var,$(target))_$(call name-to-var,$(trace))_RULES :=)))
$(foreach target, $(TARGET_TYPES), \
  $(foreach gc, $(GC_TYPES), \
    $(eval ART_RUN_TEST_$(call name-to-var,$(target))_$(call name-to-var,$(gc))_RULES :=)))
$(foreach target, $(TARGET_TYPES), \
  $(foreach jni, $(JNI_TYPES), \
    $(eval ART_RUN_TEST_$(call name-to-var,$(target))_$(call name-to-var,$(jni))_RULES :=)))
$(foreach target, $(TARGET_TYPES), \
  $(foreach image, $(IMAGE_TYPES), \
    $(eval ART_RUN_TEST_$(call name-to-var,$(target))_$(call name-to-var,$(image))_RULES :=)))
$(foreach target, $(TARGET_TYPES), \
  $(foreach test, $(TEST_ART_RUN_TESTS), \
    $(eval ART_RUN_TEST_$(call name-to-var,$(target))_$(call name-to-var,$(test))_RULES :=)))
$(foreach target, $(TARGET_TYPES), \
  $(foreach address_size, $(ALL_ADDRESS_SIZES), \
    $(eval ART_RUN_TEST_$(call name-to-var,$(target))_$(call name-to-var,$(address_size))_RULES :=)))

# We need dex2oat and dalvikvm on the target as well as the core image.
TEST_ART_TARGET_SYNC_DEPS += $(ART_TARGET_EXECUTABLES) $(TARGET_CORE_IMG_OUT) $(2ND_TARGET_CORE_IMG_OUT)

# Also need libarttest.
TEST_ART_TARGET_SYNC_DEPS += $(ART_TARGET_TEST_OUT)/$(TARGET_ARCH)/libarttest.so
ifdef TARGET_2ND_ARCH
TEST_ART_TARGET_SYNC_DEPS += $(ART_TARGET_TEST_OUT)/$(TARGET_2ND_ARCH)/libarttest.so
endif

# Also need libnativebridgetest.
TEST_ART_TARGET_SYNC_DEPS += $(ART_TARGET_TEST_OUT)/$(TARGET_ARCH)/libnativebridgetest.so
ifdef TARGET_2ND_ARCH
TEST_ART_TARGET_SYNC_DEPS += $(ART_TARGET_TEST_OUT)/$(TARGET_2ND_ARCH)/libnativebridgetest.so
endif

# All tests require the host executables and the core images.
ART_TEST_HOST_RUN_TEST_DEPENDENCIES := \
  $(ART_HOST_EXECUTABLES) \
  $(ART_HOST_OUT_SHARED_LIBRARIES)/libarttest$(ART_HOST_SHLIB_EXTENSION) \
  $(ART_HOST_OUT_SHARED_LIBRARIES)/libnativebridgetest$(ART_HOST_SHLIB_EXTENSION) \
  $(ART_HOST_OUT_SHARED_LIBRARIES)/libjavacore$(ART_HOST_SHLIB_EXTENSION) \
  $(HOST_CORE_IMG_OUT)

ifneq ($(HOST_PREFER_32_BIT),true)
ART_TEST_HOST_RUN_TEST_DEPENDENCIES += \
  $(2ND_ART_HOST_OUT_SHARED_LIBRARIES)/libarttest$(ART_HOST_SHLIB_EXTENSION) \
  $(2ND_ART_HOST_OUT_SHARED_LIBRARIES)/libnativebridgetest$(ART_HOST_SHLIB_EXTENSION) \
  $(2ND_ART_HOST_OUT_SHARED_LIBRARIES)/libjavacore$(ART_HOST_SHLIB_EXTENSION) \
  $(2ND_HOST_CORE_IMG_OUT)
endif

# Create a rule to build and run a tests following the form:
# test-art-{1: host or target}-run-test-{2: prebuild no-prebuild no-dex2oat}-
#    {3: interpreter default optimizing}-{4: relocate no-relocate relocate-no-patchoat}-
#    {5: trace or no-trace}-{6: gcstress gcverify cms}-{7: forcecopy checkjni jni}-
#    {8: no-image image}-{9: test name}{10: 32 or 64}
define define-test-art-run-test
  run_test_options := $(addprefix --runtime-option ,$(DALVIKVM_FLAGS))
  prereq_rule :=
  test_groups :=
  uc_host_or_target :=
  ifeq ($(ART_TEST_RUN_TEST_ALWAYS_CLEAN),true)
    run_test_options += --always-clean
  endif
  ifeq ($(1),host)
    uc_host_or_target := HOST
    test_groups := ART_RUN_TEST_HOST_RULES
    run_test_options += --host
    prereq_rule := $(ART_TEST_HOST_RUN_TEST_DEPENDENCIES)
  else
    ifeq ($(1),target)
      uc_host_or_target := TARGET
      test_groups := ART_RUN_TEST_TARGET_RULES
      prereq_rule := test-art-target-sync
    else
      $$(error found $(1) expected $(TARGET_TYPES))
    endif
  endif
  ifeq ($(2),prebuild)
    test_groups += ART_RUN_TEST_$$(uc_host_or_target)_PREBUILD_RULES
    run_test_options += --prebuild
  else
    ifeq ($(2),no-prebuild)
      test_groups += ART_RUN_TEST_$$(uc_host_or_target)_NO_PREBUILD_RULES
      run_test_options += --no-prebuild
    else
      ifeq ($(2),no-dex2oat)
        test_groups += ART_RUN_TEST_$$(uc_host_or_target)_NO_DEX2OAT_RULES
        run_test_options += --no-prebuild --no-dex2oat
      else
        $$(error found $(2) expected $(PREBUILD_TYPES))
      endif
    endif
  endif
  ifeq ($(3),optimizing)
    test_groups += ART_RUN_TEST_$$(uc_host_or_target)_OPTIMIZING_RULES
    run_test_options += -Xcompiler-option --compiler-backend=Optimizing
  else
    ifeq ($(3),interpreter)
      test_groups += ART_RUN_TEST_$$(uc_host_or_target)_INTERPRETER_RULES
      run_test_options += --interpreter
    else
      ifeq ($(3),default)
        test_groups += ART_RUN_TEST_$$(uc_host_or_target)_DEFAULT_RULES
      else
        $$(error found $(3) expected $(COMPILER_TYPES))
      endif
    endif
  endif
  ifeq ($(4),relocate)
    test_groups += ART_RUN_TEST_$$(uc_host_or_target)_RELOCATE_RULES
    run_test_options += --relocate
  else
    ifeq ($(4),no-relocate)
      test_groups += ART_RUN_TEST_$$(uc_host_or_target)_NO_RELOCATE_RULES
      run_test_options += --no-relocate
    else
      ifeq ($(4),relocate-no-patchoat)
        test_groups += ART_RUN_TEST_$$(uc_host_or_target)_RELOCATE_NO_PATCHOAT_RULES
        run_test_options += --relocate --no-patchoat
      else
        $$(error found $(4) expected $(RELOCATE_TYPES))
      endif
    endif
  endif
  ifeq ($(5),trace)
    test_groups += ART_RUN_TEST_$$(uc_host_or_target)_TRACE_RULES
    run_test_options += --trace
  else
    ifeq ($(5),no-trace)
      test_groups += ART_RUN_TEST_$$(uc_host_or_target)_NO_TRACE_RULES
    else
      $$(error found $(5) expected $(TRACE_TYPES))
    endif
  endif
  ifeq ($(6),gcverify)
    test_groups += ART_RUN_TEST_$$(uc_host_or_target)_GC_VERIFY_RULES
    run_test_options += --gcverify
  else
    ifeq ($(6),gcstress)
      test_groups += ART_RUN_TEST_$$(uc_host_or_target)_GC_STRESS_RULES
      run_test_options += --gcstress
    else
      ifeq ($(6),cms)
        test_groups += ART_RUN_TEST_$$(uc_host_or_target)_CMS_RULES
      else
        $$(error found $(6) expected $(GC_TYPES))
      endif
    endif
  endif
  ifeq ($(7),forcecopy)
    test_groups += ART_RUN_TEST_$$(uc_host_or_target)_FORCE_COPY_RULES
    run_test_options += --runtime-option -Xjniopts:forcecopy
    ifneq ($$(ART_TEST_JNI_FORCECOPY),true)
      skip_test := true
    endif
  else
    ifeq ($(7),checkjni)
      test_groups += ART_RUN_TEST_$$(uc_host_or_target)_CHECK_JNI_RULES
      run_test_options += --runtime-option -Xcheck:jni
    else
      ifeq ($(7),jni)
        test_groups += ART_RUN_TEST_$$(uc_host_or_target)_JNI_RULES
      else
        $$(error found $(7) expected $(JNI_TYPES))
      endif
    endif
  endif
  ifeq ($(8),no-image)
    test_groups += ART_RUN_TEST_$$(uc_host_or_target)_NO_IMAGE_RULES
    run_test_options += --no-image
  else
    ifeq ($(8),image)
      test_groups += ART_RUN_TEST_$$(uc_host_or_target)_IMAGE_RULES
    else
      $$(error found $(8) expected $(IMAGE_TYPES))
    endif
  endif
  # $(9) is the test name
  test_groups += ART_RUN_TEST_$$(uc_host_or_target)_$(call name-to-var,$(9))_RULES
  ifeq ($(10),64)
    test_groups += ART_RUN_TEST_$$(uc_host_or_target)_64_RULES
    run_test_options += --64
  else
    ifeq ($(10),32)
      test_groups += ART_RUN_TEST_$$(uc_host_or_target)_32_RULES
    else
      $$(error found $(10) expected $(ALL_ADDRESS_SIZES))
    endif
  endif
  run_test_rule_name := test-art-$(1)-run-test-$(2)-$(3)-$(4)-$(5)-$(6)-$(7)-$(8)-$(9)$(10)
  run_test_options := --output-path $(ART_HOST_TEST_DIR)/run-test-output/$$(run_test_rule_name) \
      $$(run_test_options)
$$(run_test_rule_name): PRIVATE_RUN_TEST_OPTIONS := $$(run_test_options)
.PHONY: $$(run_test_rule_name)
$$(run_test_rule_name): $(DX) $(HOST_OUT_EXECUTABLES)/jasmin $$(prereq_rule)
	$(hide) $$(call ART_TEST_SKIP,$$@) && \
	  DX=$(abspath $(DX)) JASMIN=$(abspath $(HOST_OUT_EXECUTABLES)/jasmin) \
	    art/test/run-test $$(PRIVATE_RUN_TEST_OPTIONS) $(9) \
	      && $$(call ART_TEST_PASSED,$$@) || $$(call ART_TEST_FAILED,$$@)
	$$(hide) (echo $(MAKECMDGOALS) | grep -q $$@ && \
	  echo "run-test run as top-level target, removing test directory $(ART_HOST_TEST_DIR)" && \
	  rm -r $(ART_HOST_TEST_DIR)) || true

  $$(foreach test_group,$$(test_groups), $$(eval $$(value test_group) += $$(run_test_rule_name)))

  # Clear locally defined variables.
  uc_host_or_target :=
  test_groups :=
  run_test_options :=
  run_test_rule_name :=
  prereq_rule :=
endef  # define-test-art-run-test

test_prebuild_types := prebuild
ifeq ($(ART_TEST_RUN_TEST_NO_PREBUILD),true)
  test_prebuild_types += no-prebuild
endif
ifeq ($(ART_TEST_RUN_TEST_NO_DEX2OAT),true)
  test_prebuild_types += no-dex2oat
endif
test_compiler_types :=
ifeq ($(ART_TEST_DEFAULT_COMPILER),true)
  test_compiler_types += default
endif
ifeq ($(ART_TEST_INTERPRETER),true)
  test_compiler_types += interpreter
endif
ifeq ($(ART_TEST_OPTIMIZING),true)
  test_compiler_types += optimizing
endif
test_relocate_types := relocate
ifeq ($(ART_TEST_RUN_TEST_NO_RELOCATE),true)
  test_relocate_types += no-relocate
endif
ifeq ($(ART_TEST_RUN_TEST_RELOCATE_NO_PATCHOAT),true)
  test_relocate_types := relocate-no-patchoat
endif
test_trace_types := no-trace
ifeq ($(ART_TEST_TRACE),true)
  test_trace_types += trace
endif
test_gc_types := cms
ifeq ($(ART_TEST_GCSTRESS),true)
  test_gc_types += gcstress
endif
ifeq ($(ART_TEST_GCVERIFY),true)
  test_gc_types += gcverify
endif
test_jni_types := checkjni
ifeq ($(ART_TEST_JNI_FORCECOPY),true)
  test_jni_types += forcecopy
endif
test_image_types := image
ifeq ($(ART_TEST_RUN_TEST_NO_IMAGE),true)
  test_image_types += no-image
endif

ADDRESS_SIZES_TARGET := $(ART_PHONY_TEST_TARGET_SUFFIX) $(2ND_ART_PHONY_TEST_TARGET_SUFFIX)
ADDRESS_SIZES_HOST := $(ART_PHONY_TEST_HOST_SUFFIX) $(2ND_ART_PHONY_TEST_HOST_SUFFIX)

$(foreach target, $(TARGET_TYPES), \
  $(foreach test, $(TEST_ART_RUN_TESTS), \
    $(foreach address_size, $(ADDRESS_SIZES_$(call name-to-var,$(target))), \
      $(foreach prebuild, $(test_prebuild_types), \
        $(foreach compiler, $(test_compiler_types), \
          $(foreach relocate, $(test_relocate_types), \
            $(foreach trace, $(test_trace_types), \
              $(foreach gc, $(test_gc_types), \
                $(foreach jni, $(test_jni_types), \
                  $(foreach image, $(test_image_types), \
                    $(eval $(call define-test-art-run-test,$(target),$(prebuild),$(compiler),$(relocate),$(trace),$(gc),$(jni),$(image),$(test),$(address_size))) \
                ))))))))))
define-test-art-run-test :=
test_prebuild_types :=
test_compiler_types :=
test_relocate_types :=
test_trace_types :=
test_gc_types :=
test_jni_types :=
test_image_types :=
ART_TEST_HOST_RUN_TEST_DEPENDENCIES :=
ADDRESS_SIZES_TARGET :=
ADDRESS_SIZES_HOST :=

# Define a phony rule whose purpose is to test its prerequisites.
# $(1): host or target
# $(2): list of prerequisites
define define-test-art-run-test-group
.PHONY: $(1)
$(1): $(2)
	$(hide) $$(call ART_TEST_PREREQ_FINISHED,$$@)

endef  # define-test-art-run-test-group


$(foreach target, $(TARGET_TYPES), $(eval \
  $(call define-test-art-run-test-group,test-art-$(target)-run-test,$(ART_RUN_TEST_$(call name-to-var,$(target))_RULES))))
$(foreach target, $(TARGET_TYPES), \
  $(foreach prebuild, $(PREBUILD_TYPES), $(eval \
    $(call define-test-art-run-test-group,test-art-$(target)-run-test-$(prebuild),$(ART_RUN_TEST_$(call name-to-var,$(target))_$(call name-to-var,$(prebuild))_RULES)))))
$(foreach target, $(TARGET_TYPES), \
  $(foreach compiler, $(COMPILER_TYPES), $(eval \
    $(call define-test-art-run-test-group,test-art-$(target)-run-test-$(compiler),$(ART_RUN_TEST_$(call name-to-var,$(target))_$(call name-to-var,$(compiler))_RULES)))))
$(foreach target, $(TARGET_TYPES), \
  $(foreach relocate, $(RELOCATE_TYPES), $(eval \
    $(call define-test-art-run-test-group,test-art-$(target)-run-test-$(relocate),$(ART_RUN_TEST_$(call name-to-var,$(target))_$(call name-to-var,$(relocate))_RULES)))))
$(foreach target, $(TARGET_TYPES), \
  $(foreach trace, $(TRACE_TYPES), $(eval \
    $(call define-test-art-run-test-group,test-art-$(target)-run-test-$(trace),$(ART_RUN_TEST_$(call name-to-var,$(target))_$(call name-to-var,$(trace))_RULES)))))
$(foreach target, $(TARGET_TYPES), \
  $(foreach gc, $(GC_TYPES), $(eval \
    $(call define-test-art-run-test-group,test-art-$(target)-run-test-$(gc),$(ART_RUN_TEST_$(call name-to-var,$(target))_$(call name-to-var,$(gc))_RULES)))))
$(foreach target, $(TARGET_TYPES), \
  $(foreach jni, $(JNI_TYPES), $(eval \
    $(call define-test-art-run-test-group,test-art-$(target)-run-test-$(jni),$(ART_RUN_TEST_$(call name-to-var,$(target))_$(call name-to-var,$(jni))_RULES)))))
$(foreach target, $(TARGET_TYPES), \
  $(foreach image, $(IMAGE_TYPES), $(eval \
    $(call define-test-art-run-test-group,test-art-$(target)-run-test-$(image),$(ART_RUN_TEST_$(call name-to-var,$(target))_$(call name-to-var,$(image))_RULES)))))
$(foreach target, $(TARGET_TYPES), \
  $(foreach test, $(TEST_ART_RUN_TESTS), $(eval \
    $(call define-test-art-run-test-group,test-art-$(target)-run-test-$(test),$(ART_RUN_TEST_$(call name-to-var,$(target))_$(call name-to-var,$(test))_RULES)))))
$(foreach target, $(TARGET_TYPES), \
  $(foreach address_size, $(ALL_ADDRESS_SIZES), $(eval \
    $(call define-test-art-run-test-group,test-art-$(target)-run-test-$(address_size),$(ART_RUN_TEST_$(address_size)_RULES)))))

# Clear variables now we're finished with them.
$(foreach target, $(TARGET_TYPES), $(eval ART_RUN_TEST_$(call name-to-var,$(target))_RULES :=))
$(foreach target, $(TARGET_TYPES), \
  $(foreach prebuild, $(PREBUILD_TYPES), \
    $(eval ART_RUN_TEST_$(call name-to-var,$(target))_$(call name-to-var,$(prebuild))_RULES :=)))
$(foreach target, $(TARGET_TYPES), \
  $(foreach compiler, $(COMPILER_TYPES), \
    $(eval ART_RUN_TEST_$(call name-to-var,$(target))_$(call name-to-var,$(compiler))_RULES :=)))
$(foreach target, $(TARGET_TYPES), \
  $(foreach relocate, $(RELOCATE_TYPES), \
    $(eval ART_RUN_TEST_$(call name-to-var,$(target))_$(call name-to-var,$(relocate))_RULES :=)))
$(foreach target, $(TARGET_TYPES), \
  $(foreach trace, $(TRACE_TYPES), \
    $(eval ART_RUN_TEST_$(call name-to-var,$(target))_$(call name-to-var,$(trace))_RULES :=)))
$(foreach target, $(TARGET_TYPES), \
  $(foreach gc, $(GC_TYPES), \
    $(eval ART_RUN_TEST_$(call name-to-var,$(target))_$(call name-to-var,$(gc))_RULES :=)))
$(foreach target, $(TARGET_TYPES), \
  $(foreach jni, $(JNI_TYPES), \
    $(eval ART_RUN_TEST_$(call name-to-var,$(target))_$(call name-to-var,$(jni))_RULES :=)))
$(foreach target, $(TARGET_TYPES), \
  $(foreach image, $(IMAGE_TYPES), \
    $(eval ART_RUN_TEST_$(call name-to-var,$(target))_$(call name-to-var,$(image))_RULES :=)))
$(foreach target, $(TARGET_TYPES), \
  $(foreach test, $(TEST_ART_RUN_TESTS), \
    $(eval ART_RUN_TEST_$(call name-to-var,$(target))_$(call name-to-var,$(test))_RULES :=)))
$(foreach target, $(TARGET_TYPES), \
  $(foreach address_size, $(ALL_ADDRESS_SIZES), \
    $(eval ART_RUN_TEST_$(call name-to-var,$(target))_$(call name-to-var,$(address_size))_RULES :=)))
define-test-art-run-test-group :=
TARGET_TYPES :=
PREBUILD_TYPES :=
COMPILER_TYPES :=
RELOCATE_TYPES :=
TRACE_TYPES :=
GC_TYPES :=
JNI_TYPES :=
IMAGE_TYPES :=
ALL_ADDRESS_SIZES :=