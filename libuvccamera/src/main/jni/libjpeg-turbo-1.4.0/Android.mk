#/*
# * UVCCamera
# * library and sample to access to UVC web camera on non-rooted Android device
# * 
# * Copyright (c) 2015 saki t_saki@serenegiant.com
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
# * Files in the jni/libjpeg-turbo1400, jni/libusb, jin/libuvc, jni/rapidjson folder may have a different license, see the respective files.
#*/
######################################################################
# libjpeg-turbo1400_static.a
######################################################################
LOCAL_PATH		:= $(call my-dir)
include $(CLEAR_VARS)

CFLAGS := -Werror

#生成するモジュール名
LOCAL_MODULE    := jpeg-turbo1400_static

#インクルードファイルのパスを指定
LOCAL_C_INCLUDES := \
        $(LOCAL_PATH)/

LOCAL_EXPORT_C_INCLUDES := \
		$(LOCAL_PATH)/

#コンパイラのオプションフラグを指定
LOCAL_CFLAGS := $(LOCAL_C_INCLUDES:%=-I%)
LOCAL_CFLAGS += -DANDROID_NDK

LOCAL_CFLAGS += -DAVOID_TABLES
LOCAL_CFLAGS += -O3 -fstrict-aliasing
LOCAL_CFLAGS += -fprefetch-loop-arrays

#リンクするライブラリを指定(静的モジュールにする時は不要)
#LOCAL_LDLIBS := -L$(SYSROOT)/usr/lib -ldl	# to avoid NDK issue(no need for static library)

#このモジュールを外部モジュールとしてリンクする時のライブラリを指定

LOCAL_ARM_MODE := arm

# コンパイル・リンクするソースファイル
LOCAL_SRC_FILES := \
	jcapimin.c jcapistd.c jccoefct.c \
	jccolor.c jcdctmgr.c jchuff.c jcinit.c jcmainct.c jcmarker.c \
	jcmaster.c jcomapi.c jcparam.c jcphuff.c jcprepct.c jcsample.c \
	jctrans.c jdapimin.c jdapistd.c jdatadst.c jdatasrc.c \
	jdcoefct.c jdcolor.c jddctmgr.c jdhuff.c jdinput.c jdmainct.c \
	jdmarker.c jdmaster.c jdmerge.c jdphuff.c jdpostct.c \
	jdsample.c jdtrans.c jerror.c jfdctflt.c jfdctfst.c jfdctint.c \
	jidctflt.c jidctfst.c jidctint.c jidctred.c jquant1.c \
	jquant2.c jutils.c jmemmgr.c jmemnobs.c \
	jaricom.c jdarith.c jcarith.c \
	turbojpeg.c transupp.c jdatadst-tj.c jdatasrc-tj.c \

ifeq ($(TARGET_ARCH_ABI),armeabi)
#NEONを有効にする時
#LOCAL_ARM_NEON := true
LOCAL_SRC_FILES += simd/jsimd_arm.c simd/jsimd_arm_neon.S

else ifeq ($(TARGET_ARCH_ABI),armeabi-v7a)
#NEONを有効にする時
#LOCAL_ARM_NEON := true
LOCAL_SRC_FILES += simd/jsimd_arm.c simd/jsimd_arm_neon.S

else ifeq ($(TARGET_ARCH_ABI),arm64-v8a)
#NEONを有効にする時
#LOCAL_ARM_NEON := true
LOCAL_SRC_FILES += simd/jsimd_arm64.c simd/jsimd_arm64_neon.S

# x86はyasmを使って.asmファイルをアセンブル出来るように設定しないとだめみたい
# とりあえずSIMD(MMX/SSE)無しでビルドする
#else ifeq ($(TARGET_ARCH_ABI),x86)
#LOCAL_C_INCLUDES += $(LOCAL_PATH)/win/
#LOCAL_SRC_FILES += simd/jsimd_i386.c \
#	simd/jccolext-mmx.asm	simd/jccolext-sse2.asm	simd/jccolor-mmx.asm \
#	simd/jccolor-sse2.asm	simd/jcgray-mmx.asm		simd/jcgray-sse2.asm \
#	simd/jcgryext-mmx.asm	simd/jcgryext-sse2.asm	simd/jcsample-mmx.asm \
#	simd/jcsample-sse2.asm	simd/jdcolext-mmx.asm	simd/jdcolext-sse2.asm \
#	simd/jdcolor-mmx.asm	simd/jdcolor-sse2.asm	simd/jdmerge-mmx.asm \
#	simd/jdmerge-sse2.asm	simd/jdmrgext-mmx.asm	simd/jdmrgext-sse2.asm \
#	simd/jdsample-mmx.asm	simd/jdsample-sse2.asm	simd/jfdctflt-3dn.asm \
#	simd/jfdctflt-sse.asm	simd/jfdctfst-mmx.asm	simd/jfdctfst-sse2.asm \
#	simd/jfdctint-mmx.asm	simd/jfdctint-sse2.asm	simd/jidctflt-3dn.asm \
#	simd/jidctflt-sse.asm	simd/jidctflt-sse2.asm	simd/jidctfst-mmx.asm \
#	simd/jidctfst-sse2.asm	simd/jidctint-mmx.asm	simd/jidctint-sse2.asm \
#	simd/jidctred-mmx.asm	simd/jidctred-sse2.asm	simd/jquant-3dn.asm \
#	simd/jquant-mmx.asm		simd/jquant-sse.asm		simd/jquantf-sse2.asm \
#	simd/jquanti-sse2.asm	simd/jsimdcpu.asm \

else
LOCAL_SRC_FILES += jsimd_none.c
endif

#	jsimdext.inc jcolsamp.inc jdct.inc \


# 静的ライブラリとしてビルド
include $(BUILD_STATIC_LIBRARY)

######################################################################
# jpeg-turbo1400.so
######################################################################
include $(CLEAR_VARS)
LOCAL_EXPORT_C_INCLUDES := \
		$(LOCAL_PATH)/

LOCAL_LDLIBS := -L$(SYSROOT)/usr/lib -ldl	# to avoid NDK issue(no need for static library)

LOCAL_WHOLE_STATIC_LIBRARIES = jpeg-turbo1400_static

LOCAL_MODULE := jpeg-turbo1400
include $(BUILD_SHARED_LIBRARY)

