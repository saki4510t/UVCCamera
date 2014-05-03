# modified saki@serenegiant <t_saki@serenegiant.com>
# Copyright (C)2014
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

LOCAL_PATH:= $(call my-dir)
LIBUSB_ROOT:= $(LOCAL_PATH)/../..

######################################################################
# libusb.a
######################################################################
include $(CLEAR_VARS)

# changed linux_usbfs.c => android_usbfs.c
# changed linux_netlink.c => android_netlink.c
# these sources are also modified.
LOCAL_SRC_FILES := \
	$(LIBUSB_ROOT)/libusb/core.c \
	$(LIBUSB_ROOT)/libusb/descriptor.c \
	$(LIBUSB_ROOT)/libusb/hotplug.c \
	$(LIBUSB_ROOT)/libusb/io.c \
	$(LIBUSB_ROOT)/libusb/sync.c \
	$(LIBUSB_ROOT)/libusb/strerror.c \
	$(LIBUSB_ROOT)/libusb/os/android_usbfs.c \
	$(LIBUSB_ROOT)/libusb/os/poll_posix.c \
	$(LIBUSB_ROOT)/libusb/os/threads_posix.c \
	$(LIBUSB_ROOT)/libusb/os/android_netlink.c

LOCAL_C_INCLUDES += \
	$(LOCAL_PATH)/.. \
	$(LIBUSB_ROOT)/../ \
	$(LIBUSB_ROOT)/libusb \
	$(LIBUSB_ROOT)/libusb/os

LOCAL_EXPORT_C_INCLUDES := \
	$(LIBUSB_ROOT)/libusb

# add some flags
LOCAL_CFLAGS := $(LOCAL_C_INCLUDES:%=-I%)
LOCAL_CFLAGS += -DANDROID_NDK
LOCAL_CFLAGS += -O3 -fstrict-aliasing -fprefetch-loop-arrays
LOCAL_EXPORT_LDLIBS += -llog
LOCAL_ARM_MODE := arm

LOCAL_MODULE := libusb1.0_static
include $(BUILD_STATIC_LIBRARY)

######################################################################
# libusb.so
######################################################################
include $(CLEAR_VARS)
LOCAL_MODULE_TAGS := optional
LOCAL_EXPORT_LDLIBS += -llog
LOCAL_EXPORT_C_INCLUDES := \
	$(LIBUSB_ROOT)/libusb

LOCAL_WHOLE_STATIC_LIBRARIES = libusb1.0_static

LOCAL_MODULE := libusb1.0
include $(BUILD_SHARED_LIBRARY)
