######################################################################
# libuvc_static.a
######################################################################
LOCAL_PATH		:= $(call my-dir)
include $(CLEAR_VARS)

CFLAGS := -Werror

LOCAL_MODULE := libuvc_static

LOCAL_C_INCLUDES += \
	$(LOCAL_PATH)/.. \
	$(LOCAL_PATH)/include \
	$(LOCAL_PATH)/include/libuvc \

LOCAL_EXPORT_C_INCLUDES := \
	$(LOCAL_PATH)/include \
	$(LOCAL_PATH)/include/libuvc

LOCAL_CFLAGS := $(LOCAL_C_INCLUDES:%=-I%)
LOCAL_CFLAGS += -DANDROID_NDK
#LOCAL_CFLAGS += -DNDEBUG			# デバッグメッセージ・assertを無効にする場合
#LOCAL_CFLAGS += -DLOG_NDEBUG		# LOGV/LOGD/MARKデバッグメッセージを出さないようにする時
#LOCAL_CFLAGS += -DUVC_DEBUGGING	# UVCのデバッグメッセージを有効にする場合
#LOCAL_CFLAGS += -DPRINT_DIAG		# カメラ接続時にカメラのディスクリプタをログに出力する
#最適化設定
#LOCAL_CFLAGS += -DAVOID_TABLES
LOCAL_CFLAGS += -O3 -fstrict-aliasing
#LOCAL_CFLAGS += -fprefetch-loop-arrays

LOCAL_EXPORT_LDLIBS += -L$(SYSROOT)/usr/lib -ldl	# to avoid NDK issue(no need for static library)
LOCAL_EXPORT_LDLIBS += -llog

LOCAL_ARM_MODE := arm

#静的リンクする外部モジュール
#LOCAL_STATIC_LIBRARIES += common_static
#LOCAL_STATIC_LIBRARIES += usb_static
#LOCAL_STATIC_LIBRARIES += jpeg9a_static
#LOCAL_STATIC_LIBRARIES += jpeg-turbo131_static

#動的リンクする外部モジュール
LOCAL_SHARED_LIBRARIES += common
LOCAL_STATIC_LIBRARIES += usb
#LOCAL_SHARED_LIBRARIES += jpeg9a
#LOCAL_SHARED_LIBRARIES += jpeg-turbo131
LOCAL_SHARED_LIBRARIES += jpeg-turbo1390

LOCAL_SRC_FILES := \
	ctrl.c \
	device.c \
	diag.c \
	frame.c \
	frame-mjpeg.c \
	init.c \
	stream.c

#スタティックライブラリとしてリンク
include $(BUILD_STATIC_LIBRARY)

######################################################################
# libuvc.so (shared library with shared link to libcommon, libusb, libjpeg)
######################################################################
include $(CLEAR_VARS)
LOCAL_EXPORT_LDLIBS += -llog
LOCAL_EXPORT_C_INCLUDES := \
	$(LOCAL_PATH)/include \
	$(LOCAL_PATH)/include/libuvc

LOCAL_WHOLE_STATIC_LIBRARIES = libuvc_static

LOCAL_MODULE := libuvc
include $(BUILD_SHARED_LIBRARY)

#$(call import-module,serenegiant/libjpeg-9a)
#$(call import-module,serenegiant/libjpeg-turbo-1.3.1)
$(call import-module,serenegiant/libjpeg-turbo-1.3.90)
