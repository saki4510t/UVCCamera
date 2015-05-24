# modified saki@serenegiant <t_saki@serenegiant.com>
# Copyright (C)2014-2015
#
# Android build config for libusb
# Copyright © 2012-2013 RealVNC Ltd. <toby.gray@realvnc.com>
#
# This library is free software; you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public
# License as published by the Free Software Foundation; either
# version 2.1 of the License, or (at your option) any later version.
#
# This library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public
# License along with this library; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
#
######################################################################
# libusb.a
######################################################################
LOCAL_PATH			:= $(call my-dir)/../..
include $(CLEAR_VARS)

# changed linux_usbfs.c => android_usbfs.c
# changed linux_netlink.c => android_netlink.c
# these sources are also modified.
LOCAL_SRC_FILES := \
	libusb/core.c \
	libusb/descriptor.c \
	libusb/hotplug.c \
	libusb/io.c \
	libusb/sync.c \
	libusb/strerror.c \
	libusb/os/android_usbfs.c \
	libusb/os/poll_posix.c \
	libusb/os/threads_posix.c \
	libusb/os/android_netlink.c

LOCAL_C_INCLUDES += \
	$(LOCAL_PATH)/ \
	$(LOCAL_PATH)/libusb \
	$(LOCAL_PATH)/libusb/os \
	$(LOCAL_PATH)/../ \

LOCAL_EXPORT_C_INCLUDES := \
	$(LOCAL_PATH)/ \
	$(LOCAL_PATH)/libusb

# add some flags
LOCAL_CFLAGS := $(LOCAL_C_INCLUDES:%=-I%)
LOCAL_CFLAGS += -DANDROID_NDK
LOCAL_CFLAGS += -DLOG_NDEBUG
LOCAL_CFLAGS += -DACCESS_RAW_DESCRIPTORS
LOCAL_CFLAGS += -O3 -fstrict-aliasing -fprefetch-loop-arrays
LOCAL_EXPORT_LDLIBS += -llog
LOCAL_ARM_MODE := arm

LOCAL_MODULE := libusb100_static
include $(BUILD_STATIC_LIBRARY)

######################################################################
# libusb.so
######################################################################
include $(CLEAR_VARS)
LOCAL_MODULE_TAGS := optional
LOCAL_EXPORT_LDLIBS += -llog

LOCAL_WHOLE_STATIC_LIBRARIES = libusb100_static

LOCAL_MODULE := libusb100
include $(BUILD_SHARED_LIBRARY)
