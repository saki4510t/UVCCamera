#/*********************************************************************
#* Software License Agreement (BSD License)
#*
#* Copyright (C)2014 saki@serenegiant <t_saki@serenegiant.com>
#*  All rights reserved.
#*
#*  Redistribution and use in source and binary forms, with or without
#*  modification, are permitted provided that the following conditions
#*  are met:
#*
#*   * Redistributions of source code must retain the above copyright
#*     notice, this list of conditions and the following disclaimer.
#*   * Redistributions in binary form must reproduce the above
#*     copyright notice, this list of conditions and the following
#*     disclaimer in the documentation and/or other materials provided
#*     with the distribution.
#*   * Neither the name of the author nor other contributors may be
#*     used to endorse or promote products derived from this software
#*     without specific prior written permission.
#*
#*  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
#*  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
#*  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
#*  FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
#*  COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
#*  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
#*  BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
#*  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
#*  CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
#*  LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
#*  ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
#*  POSSIBILITY OF SUCH DAMAGE.
#*********************************************************************/

LIBUVC_ROOT		:= $(call my-dir)/../..

include $(LIBUVC_ROOT)/../libjpeg/android/jni/Android.mk
include $(LIBUVC_ROOT)/../libusb/android/jni/Android.mk

######################################################################
# libuvc_static.a (static library with static link to libjpeg.so, libusb1.0.so)
######################################################################
include $(CLEAR_VARS)

LOCAL_C_INCLUDES += \
  $(LIBUVC_ROOT)/.. \
  $(LIBUVC_ROOT)/../libusb \
  $(LIBUVC_ROOT)/../libjpeg \
  $(LIBUVC_ROOT)/include \
  $(LIBUVC_ROOT)/include/libuvc

LOCAL_EXPORT_C_INCLUDES := \
  $(LIBUVC_ROOT)/include \
  $(LIBUVC_ROOT)/include/libuvc

LOCAL_CFLAGS := $(LOCAL_C_INCLUDES:%=-I%)
LOCAL_CFLAGS += -DANDROID_NDK
LOCAL_CFLAGS += -O3 -fstrict-aliasing -fprefetch-loop-arrays

LOCAL_EXPORT_LDLIBS := -llog

LOCAL_ARM_MODE := arm

LOCAL_STATIC_LIBRARIES += usb1.0_static
LOCAL_STATIC_LIBRARIES += jpeg_static
#if you want to use shared library of libusb & libjpeg
#LOCAL_SHARED_LIBRARIES += usb1.0
#LOCAL_SHARED_LIBRARIES += jpeg

LOCAL_SRC_FILES := \
  $(LIBUVC_ROOT)/src/ctrl.c \
  $(LIBUVC_ROOT)/src/device.c \
  $(LIBUVC_ROOT)/src/diag.c \
  $(LIBUVC_ROOT)/src/frame.c \
  $(LIBUVC_ROOT)/src/frame-mjpeg.c \
  $(LIBUVC_ROOT)/src/init.c \
  $(LIBUVC_ROOT)/src/stream.c

LOCAL_MODULE := libuvc_static
include $(BUILD_STATIC_LIBRARY)

######################################################################
# libuvc.so (shared library with static link to libusb, libjpeg)
######################################################################
include $(CLEAR_VARS)
LOCAL_MODULE_TAGS := optional
LOCAL_EXPORT_LDLIBS += -llog
LOCAL_EXPORT_C_INCLUDES := \
  $(LIBUVC_ROOT)/include \
  $(LIBUVC_ROOT)/include/libuvc

LOCAL_WHOLE_STATIC_LIBRARIES = libuvc_static

LOCAL_MODULE := uvc
include $(BUILD_SHARED_LIBRARY)
