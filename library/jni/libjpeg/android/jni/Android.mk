LOCAL_PATH	:= $(call my-dir)
#JPEG_ROOT_REL:= ../..
JPEG_ROOT:= $(LOCAL_PATH)/../..

######################################################################
# libjpeg.a
######################################################################
include $(CLEAR_VARS)

CFLAGS := -Werror

LOCAL_C_INCLUDES := \
        $(JPEG_ROOT)/

LOCAL_EXPORT_C_INCLUDES := \
		$(JPEG_ROOT)/

LOCAL_CFLAGS := $(LOCAL_C_INCLUDES:%=-I%)
LOCAL_CFLAGS += -DANDROID_NDK
LOCAL_CFLAGS += -DAVOID_TABLES
LOCAL_CFLAGS += -O3 -fstrict-aliasing -fprefetch-loop-arrays
LOCAL_EXPORT_LDLIBS += -llog
LOCAL_ARM_MODE := arm

LOCAL_SRC_FILES := \
		$(JPEG_ROOT)/ckconfig.c \
		$(JPEG_ROOT)/jaricom.c \
		$(JPEG_ROOT)/jcapimin.c \
		$(JPEG_ROOT)/jcapistd.c \
		$(JPEG_ROOT)/jcarith.c \
		$(JPEG_ROOT)/jccoefct.c \
		$(JPEG_ROOT)/jccolor.c \
		$(JPEG_ROOT)/jcdctmgr.c \
		$(JPEG_ROOT)/jchuff.c \
		$(JPEG_ROOT)/jcinit.c \
		$(JPEG_ROOT)/jcmainct.c \
		$(JPEG_ROOT)/jcmarker.c \
		$(JPEG_ROOT)/jcmaster.c \
		$(JPEG_ROOT)/jcomapi.c \
		$(JPEG_ROOT)/jcparam.c \
		$(JPEG_ROOT)/jcprepct.c \
		$(JPEG_ROOT)/jcsample.c \
		$(JPEG_ROOT)/jctrans.c \
		$(JPEG_ROOT)/jdapimin.c \
		$(JPEG_ROOT)/jdapistd.c \
		$(JPEG_ROOT)/jdarith.c \
		$(JPEG_ROOT)/jdatadst.c \
		$(JPEG_ROOT)/jdatasrc.c \
		$(JPEG_ROOT)/jdcoefct.c \
		$(JPEG_ROOT)/jdcolor.c \
		$(JPEG_ROOT)/jddctmgr.c \
		$(JPEG_ROOT)/jdhuff.c \
		$(JPEG_ROOT)/jdinput.c \
		$(JPEG_ROOT)/jdmainct.c \
		$(JPEG_ROOT)/jdmarker.c \
		$(JPEG_ROOT)/jdmaster.c \
		$(JPEG_ROOT)/jdmerge.c \
		$(JPEG_ROOT)/jdpostct.c \
		$(JPEG_ROOT)/jdsample.c \
		$(JPEG_ROOT)/jdtrans.c \
		$(JPEG_ROOT)/jerror.c \
		$(JPEG_ROOT)/jfdctflt.c \
		$(JPEG_ROOT)/jfdctfst.c \
		$(JPEG_ROOT)/jfdctint.c \
		$(JPEG_ROOT)/jidctflt.c \
		$(JPEG_ROOT)/jidctfst.c \
		$(JPEG_ROOT)/jidctint.c \
		$(JPEG_ROOT)/jmemmgr.c \
		$(JPEG_ROOT)/jmemansi.c \
		$(JPEG_ROOT)/jquant1.c \
		$(JPEG_ROOT)/jquant2.c \
		$(JPEG_ROOT)/jutils.c \
		$(JPEG_ROOT)/wrbmp.c

LOCAL_MODULE    := jpeg_static
include $(BUILD_STATIC_LIBRARY)

######################################################################
# libjpeg.so
######################################################################
include $(CLEAR_VARS)
LOCAL_MODULE_TAGS := optional
LOCAL_EXPORT_LDLIBS += -llog
LOCAL_EXPORT_C_INCLUDES := \
	$(JPEG_ROOT)/

LOCAL_WHOLE_STATIC_LIBRARIES = libjpeg_static

LOCAL_MODULE := jpeg
include $(BUILD_SHARED_LIBRARY)
