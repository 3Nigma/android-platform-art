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

ifndef ANDROID_COMMON_PATH_MK
ANDROID_COMMON_PATH_MK := true

include art/build/Android.common.mk

# Directory used for dalvik-cache on device.
ART_TARGET_DALVIK_CACHE_DIR := /data/dalvik-cache

# Directory used for gtests on device.
ART_TARGET_NATIVETEST_DIR := /data/nativetest/art
ART_TARGET_NATIVETEST_OUT := $(TARGET_OUT_DATA_NATIVE_TESTS)/art

# Directory used for oat tests on device.
ART_TARGET_TEST_DIR := /data/art-test
ART_TARGET_TEST_OUT := $(TARGET_OUT_DATA)/art-test

# Directory used for temporary test files on the host.
ART_HOST_TEST_DIR := /tmp/test-art-$(shell echo $$PPID)

# Core.oat location on the device.
TARGET_CORE_OAT := $(ART_TARGET_TEST_DIR)/$(DEX2OAT_TARGET_ARCH)/core.oat
ifdef TARGET_2ND_ARCH
2ND_TARGET_CORE_OAT := $(ART_TARGET_TEST_DIR)/$($(TARGET_2ND_ARCH_VAR_PREFIX)DEX2OAT_TARGET_ARCH)/core.oat
endif

# Core.oat locations under the out directory.
HOST_CORE_OAT_OUT := $(HOST_OUT_JAVA_LIBRARIES)/$(ART_HOST_ARCH)/core.oat
ifneq ($(HOST_PREFER_32_BIT),true)
2ND_HOST_CORE_OAT_OUT := $(HOST_OUT_JAVA_LIBRARIES)/$(2ND_ART_HOST_ARCH)/core.oat
endif
TARGET_CORE_OAT_OUT := $(ART_TARGET_TEST_OUT)/$(DEX2OAT_TARGET_ARCH)/core.oat
ifdef TARGET_2ND_ARCH
2ND_TARGET_CORE_OAT_OUT := $(ART_TARGET_TEST_OUT)/$($(TARGET_2ND_ARCH_VAR_PREFIX)DEX2OAT_TARGET_ARCH)/core.oat
endif

# Core.art locations under the out directory.
HOST_CORE_IMG_OUT := $(HOST_OUT_JAVA_LIBRARIES)/$(ART_HOST_ARCH)/core.art
ifneq ($(HOST_PREFER_32_BIT),true)
2ND_HOST_CORE_IMG_OUT := $(HOST_OUT_JAVA_LIBRARIES)/$(2ND_ART_HOST_ARCH)/core.art
endif
TARGET_CORE_IMG_OUT := $(ART_TARGET_TEST_OUT)/$(DEX2OAT_TARGET_ARCH)/core.art
ifdef TARGET_2ND_ARCH
2ND_TARGET_CORE_IMG_OUT := $(ART_TARGET_TEST_OUT)/$($(TARGET_2ND_ARCH_VAR_PREFIX)DEX2OAT_TARGET_ARCH)/core.art
endif

# Oat location of core.art.
HOST_CORE_IMG_LOCATION := $(HOST_OUT_JAVA_LIBRARIES)/core.art
TARGET_CORE_IMG_LOCATION := $(ART_TARGET_TEST_OUT)/core.art

endif # ANDROID_COMMON_PATH_MK
