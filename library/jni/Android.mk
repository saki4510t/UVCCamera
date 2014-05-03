#/*
# * UVCCamera
# * library and sample to access to UVC web camera on non-rooted Android device
# * 
# * Copyright (c) 2014 saki t_saki@serenegiant.com
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
# * Files in the jni/libjpeg, jni/libusb and jin/libuvc folder may have a different license, see the respective files.
#*/

UVCCAMERA_ROOT	:= $(call my-dir)

######################################################################
# Include all libs (built and prebuilt)
######################################################################
include $(CLEAR_VARS)
include $(UVCCAMERA_ROOT)/libuvc/android/jni/Android.mk

######################################################################
# Make shared library libUVCCamera.so
######################################################################
include $(CLEAR_VARS)

CFLAGS := -Werror

LOCAL_C_INCLUDES := \
        $(UVCCAMERA_ROOT)/ \
        $(UVCCAMERA_ROOT)/libjpeg \
        $(UVCCAMERA_ROOT)/libusb \
        $(UVCCAMERA_ROOT)/libuvc \
        $(UVCCAMERA_ROOT)/libuvc/include \
        $(UVCCAMERA_ROOT)/rapidjson/include

LOCAL_CFLAGS := $(LOCAL_C_INCLUDES:%=-I%)
LOCAL_CFLAGS += -DANDROID_NDK
LOCAL_CFLAGS += -O3 -fstrict-aliasing -fprefetch-loop-arrays

LOCAL_LDLIBS := -L$(SYSROOT)/usr/lib -ldl
LOCAL_LDLIBS += -llog
LOCAL_LDLIBS += -landroid

#LOCAL_SHARED_LIBRARIES += busb1.0
#LOCAL_SHARED_LIBRARIES += jpeg
LOCAL_SHARED_LIBRARIES += uvc
#LOCAL_STATIC_LIBRARIES += busb1.0_static
#LOCAL_STATIC_LIBRARIES += jpeg_static
#OCAL_STATIC_LIBRARIES += uvc_static

LOCAL_ARM_MODE := arm

LOCAL_SRC_FILES := \
		$(UVCCAMERA_ROOT)/_onload.cpp \
		$(UVCCAMERA_ROOT)/UVCCamera.cpp \
		$(UVCCAMERA_ROOT)/UVCPreview.cpp \
		$(UVCCAMERA_ROOT)/serenegiant_usb_UVCCamera.cpp

LOCAL_MODULE    := UVCCamera
include $(BUILD_SHARED_LIBRARY)
