#/*
# * UVCCamera
# * library and sample to access to UVC web camera on non-rooted Android device
# * 
# * Copyright (c) 2015-2017 saki t_saki@serenegiant.com
# * 
# * File name: Android.mk
# * 
# * Licensed under the Apache License, Version 2.0 (the "License");
# * you may not use this file except in compliance with the License.
# *  You may obtain a copy of the License at
# * 
# *     http://www.apache.org/licenses/LICENSE-2.0
# * 
# *  Unless required by applicable law or agreed to in writing, software
# *  distributed under the License is distributed on an "AS IS" BASIS,
# *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# *  See the License for the specific language governing permissions and
# *  limitations under the License.
# * 
# * All files in the folder are under this Apache License, Version 2.0.
# * Files in the jni/libjpeg-turbo1500, jni/libusb, jin/libuvc, jni/rapidjson folder may have a different license, see the respective files.
#*/
######################################################################
# libjpeg-turbo1500_static.a
######################################################################
LOCAL_PATH		:= $(call my-dir)
include $(CLEAR_VARS)

# CFLAGS := -Werror

#生成するモジュール名
LOCAL_MODULE    := jpeg-turbo1500_static

#インクルードファイルのパスを指定
LOCAL_C_INCLUDES := \
        $(LOCAL_PATH)/ \
        $(LOCAL_PATH)/include \
        $(LOCAL_PATH)/simd \

LOCAL_EXPORT_C_INCLUDES := \
		$(LOCAL_PATH)/ \
        $(LOCAL_PATH)/include \
        $(LOCAL_PATH)/simd \

#コンパイラのオプションフラグを指定
LOCAL_CFLAGS := $(LOCAL_C_INCLUDES:%=-I%)
LOCAL_CFLAGS += -DANDROID_NDK

#リンクするライブラリを指定(静的モジュールにする時は不要)
#LOCAL_LDLIBS := -L$(SYSROOT)/usr/lib -ldl	# to avoid NDK issue(no need for static library)

#このモジュールを外部モジュールとしてリンクする時のライブラリを指定

LOCAL_ARM_MODE := arm

LOCAL_ASMFLAGS += -DELF

# コンパイル・リンクするソースファイル

LOCAL_SRC_FILES += \
	jcapimin.c \
	jcapistd.c \
	jccoefct.c \
	jccolor.c \
	jcdctmgr.c \
	jchuff.c \
	jcinit.c \
	jcmainct.c \
	jcmarker.c \
	jcmaster.c \
	jcomapi.c \
	jcparam.c \
	jcphuff.c \
	jcprepct.c \
	jcsample.c \
	jctrans.c \
	jdapimin.c \
	jdapistd.c \
	jdatadst.c \
	jdatasrc.c \
	jdcoefct.c \
	jdcolor.c \
	jddctmgr.c \
	jdhuff.c \
	jdinput.c \
	jdmainct.c \
	jdmarker.c \
	jdmaster.c \
	jdmerge.c \
	jdphuff.c \
	jdpostct.c \
	jdsample.c \
	jdtrans.c \
	jerror.c \
	jfdctflt.c \
	jfdctfst.c \
	jfdctint.c \
	jidctflt.c \
	jidctfst.c \
	jidctint.c \
	jidctred.c \
	jquant1.c \
	jquant2.c \
	jutils.c \
	jmemmgr.c \
	jmemnobs.c \

LOCAL_SRC_FILES += \
	jaricom.c \
	jcarith.c \
	jdarith.c \

LOCAL_SRC_FILES += \
	turbojpeg.c \
	transupp.c \
	jdatadst-tj.c \
	jdatasrc-tj.c \

ifeq ($(TARGET_ARCH_ABI),armeabi)
#NEONを有効にする時
#LOCAL_ARM_NEON := true
LOCAL_SRC_FILES += simd/jsimd_arm.c simd/jsimd_arm_neon.S

LOCAL_CFLAGS += \
	-DSIZEOF_SIZE_T=4 \

else ifeq ($(TARGET_ARCH_ABI),armeabi-v7a)
#NEONを有効にする時
#LOCAL_ARM_NEON := true
LOCAL_SRC_FILES += simd/jsimd_arm.c simd/jsimd_arm_neon.S

LOCAL_CFLAGS += \
	-DSIZEOF_SIZE_T=4 \

else ifeq ($(TARGET_ARCH_ABI),arm64-v8a)
#NEONを有効にする時
#LOCAL_ARM_NEON := true
LOCAL_SRC_FILES += simd/jsimd_arm64.c simd/jsimd_arm64_neon.S

LOCAL_CFLAGS += \
	-DSIZEOF_SIZE_T=8 \

else ifeq ($(TARGET_ARCH_ABI),x86_64)
LOCAL_SRC_FILES += \
	simd/jsimd_x86_64.c \
	simd/jfdctflt-sse-64.asm \
	simd/jccolor-sse2-64.asm \
	simd/jcgray-sse2-64.asm \
	simd/jcsample-sse2-64.asm \
	simd/jdcolor-sse2-64.asm \
	simd/jdmerge-sse2-64.asm \
	simd/jdsample-sse2-64.asm \
	simd/jfdctfst-sse2-64.asm \
	simd/jfdctint-sse2-64.asm \
	simd/jidctflt-sse2-64.asm \
	simd/jidctfst-sse2-64.asm \
	simd/jidctint-sse2-64.asm \
	simd/jidctred-sse2-64.asm \
	simd/jquantf-sse2-64.asm \
	simd/jquanti-sse2-64.asm \
	simd/jchuff-sse2-64.asm \

LOCAL_CFLAGS += \
	-DSIZEOF_SIZE_T=8 \

LOCAL_ASMFLAGS += -D__x86_64__

else ifeq ($(TARGET_ARCH_ABI),x86)

LOCAL_SRC_FILES += \
	simd/jsimd_i386.c \
	simd/jsimdcpu.asm \
	simd/jfdctflt-3dn.asm \
	simd/jidctflt-3dn.asm \
	simd/jquant-3dn.asm \
	simd/jccolor-mmx.asm \
	simd/jcgray-mmx.asm \
	simd/jcsample-mmx.asm \
	simd/jdcolor-mmx.asm \
	simd/jdmerge-mmx.asm \
	simd/jdsample-mmx.asm \
	simd/jfdctfst-mmx.asm \
	simd/jfdctint-mmx.asm \
	simd/jidctfst-mmx.asm \
	simd/jidctint-mmx.asm \
	simd/jidctred-mmx.asm \
	simd/jquant-mmx.asm \
	simd/jfdctflt-sse.asm \
	simd/jidctflt-sse.asm \
	simd/jquant-sse.asm \
	simd/jccolor-sse2.asm \
	simd/jcgray-sse2.asm \
	simd/jcsample-sse2.asm \
	simd/jdcolor-sse2.asm \
	simd/jdmerge-sse2.asm \
	simd/jdsample-sse2.asm \
	simd/jfdctfst-sse2.asm \
	simd/jfdctint-sse2.asm \
	simd/jidctflt-sse2.asm \
	simd/jidctfst-sse2.asm \
	simd/jidctint-sse2.asm \
	simd/jidctred-sse2.asm \
	simd/jquantf-sse2.asm \
	simd/jquanti-sse2.asm \
	simd/jchuff-sse2.asm \

LOCAL_CFLAGS += \
	-DSIZEOF_SIZE_T=4 \

else ifeq ($(TARGET_ARCH_ABI),mips)

# Unfortunately it seems Clang in NDK(at least until r14)
# does not support SIMD for mips(MSA) correctly now.
# If you really needs MSA, try `NDK_TOOLCHAIN_VERSION := 4.9` in Application.mk
# with r13b/r14 (so that you can build libraries with GCC),
# but I don't recommend because supporting GCC on NDK is already deprecated
# and GCC will bre removed from NDK soon.

ifeq ($(NDK_TOOLCHAIN_VERSION),clang)

#disable MSA

LOCAL_SRC_FILES += \
	jsimd_none.c

else

LOCAL_SRC_FILES += \
	simd/jsimd_mips.c \
	simd/jsimd_mips_dspr2.S \

endif

LOCAL_CFLAGS += \
	-DSIZEOF_SIZE_T=4 \

else
LOCAL_SRC_FILES += jsimd_none.c

endif

# simd/jsimd.h simd/jcolsamp.inc simd/jsimdcfg.inc.h simd/jsimdext.inc simd/jdct.inc
#	jsimdext.inc jcolsamp.inc jdct.inc \

LOCAL_CPPFLAGS += -Wno-incompatible-pointer-types

LOCAL_DISABLE_FATAL_LINKER_WARNINGS := true

# 静的ライブラリとしてビルド
include $(BUILD_STATIC_LIBRARY)

######################################################################
# jpeg-turbo1500.so
######################################################################
include $(CLEAR_VARS)
LOCAL_EXPORT_C_INCLUDES := \
		$(LOCAL_PATH)/

LOCAL_DISABLE_FATAL_LINKER_WARNINGS := true

LOCAL_LDLIBS := -L$(SYSROOT)/usr/lib -ldl	# to avoid NDK issue(no need for static library)

LOCAL_WHOLE_STATIC_LIBRARIES = jpeg-turbo1500_static

LOCAL_MODULE := jpeg-turbo1500
include $(BUILD_SHARED_LIBRARY)

