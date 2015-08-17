######################################################################
# libjpeg.a
######################################################################
LOCAL_PATH	:= $(call my-dir)/../..
include $(CLEAR_VARS)

CFLAGS := -Werror

LOCAL_C_INCLUDES := \
        $(LOCAL_PATH)/

LOCAL_EXPORT_C_INCLUDES := \
		$(LOCAL_PATH)/

LOCAL_CFLAGS := $(LOCAL_C_INCLUDES:%=-I%)
LOCAL_CFLAGS += -DANDROID_NDK
LOCAL_CFLAGS += -DAVOID_TABLES
LOCAL_CFLAGS += -O3 -fstrict-aliasing -fprefetch-loop-arrays
LOCAL_EXPORT_LDLIBS += -llog
LOCAL_ARM_MODE := arm

LOCAL_SRC_FILES := \
		ckconfig.c \
		jaricom.c \
		jcapimin.c \
		jcapistd.c \
		jcarith.c \
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
		jcprepct.c \
		jcsample.c \
		jctrans.c \
		jdapimin.c \
		jdapistd.c \
		jdarith.c \
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
		jmemmgr.c \
		jmemansi.c \
		jquant1.c \
		jquant2.c \
		jutils.c \
		wrbmp.c

LOCAL_MODULE    := jpeg_static
include $(BUILD_STATIC_LIBRARY)

######################################################################
# libjpeg.so
######################################################################
#include $(CLEAR_VARS)
#LOCAL_MODULE_TAGS := optional
#LOCAL_EXPORT_LDLIBS += -llog
#LOCAL_EXPORT_C_INCLUDES := \
#	$(LOCAL_PATH)/
#
#LOCAL_WHOLE_STATIC_LIBRARIES = libjpeg_static
#
#LOCAL_MODULE := jpeg
#include $(BUILD_SHARED_LIBRARY)
