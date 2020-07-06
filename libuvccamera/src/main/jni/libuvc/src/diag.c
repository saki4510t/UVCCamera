/*********************************************************************
 *********************************************************************/
/*********************************************************************
 * changed to using LogCat than stderr when compiling for Android
 * Copyright (C) 2014 saki@serenegiant All rights reserved.
 *********************************************************************/
/*********************************************************************
 * Software License Agreement (BSD License)
 *
 *  Copyright (C) 2010-2012 Ken Tossell
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above
 *     copyright notice, this list of conditions and the following
 *     disclaimer in the documentation and/or other materials provided
 *     with the distribution.
 *   * Neither the name of the author nor other contributors may be
 *     used to endorse or promote products derived from this software
 *     without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 *  FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 *  COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 *  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 *  BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 *  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 *  CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 *  LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 *  ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *  POSSIBILITY OF SUCH DAMAGE.
 *********************************************************************/
/**
 * @defgroup diag Diagnostics
 * @brief Interpretation of devices, error codes and negotiated stream parameters
 */

#include "libuvc/libuvc.h"
#include "libuvc/libuvc_internal.h"

/** @internal */
typedef struct _uvc_error_msg {
	uvc_error_t err;
	const char *msg;
} _uvc_error_msg_t;

static const _uvc_error_msg_t uvc_error_msgs[] = {
	{ UVC_SUCCESS, "Success" },
	{ UVC_ERROR_IO, "I/O error" },
	{ UVC_ERROR_INVALID_PARAM, "Invalid parameter" },
	{ UVC_ERROR_ACCESS, "Access denied" },
	{ UVC_ERROR_NO_DEVICE, "No such device" },
	{ UVC_ERROR_NOT_FOUND, 	"Not found" },
	{ UVC_ERROR_BUSY, "Busy" },
	{ UVC_ERROR_TIMEOUT, "Timeout" },
	{ UVC_ERROR_OVERFLOW, "Overflow" },
	{ UVC_ERROR_PIPE, "Pipe" },
	{ UVC_ERROR_INTERRUPTED, "Interrupted" },
	{ UVC_ERROR_NO_MEM, "Out of memory" },
	{ UVC_ERROR_NOT_SUPPORTED, "Not supported" },
	{ UVC_ERROR_INVALID_DEVICE, "Invalid device" },
	{ UVC_ERROR_INVALID_MODE, "Invalid mode" },
	{ UVC_ERROR_CALLBACK_EXISTS, "Callback exists" }
};

#ifdef __ANDROID__
#define FPRINTF(stream, ...) MARK(__VA_ARGS__); usleep(1000);
#define FPRINTF_ERR(stream, ...) LOGW(__VA_ARGS__)
#else
#define FPRINTF(stream, ...) fprintf(stream, __VA_ARGS__)
#define FPRINTF_ERR(stream, ...) fprintf(stream, __VA_ARGS__)
#endif
/** @brief Print a message explaining an error in the UVC driver
 * @ingroup diag
 *
 * @param err UVC error code
 * @param msg Optional custom message, prepended to output
 *
 * 打印一条消息，解释UVC驱动程序中的错误
 */
void uvc_perror(uvc_error_t err, const char *msg) {
	if (msg && *msg) {
		FPRINTF_ERR(stderr, "%s:%s (%d)\n", msg, uvc_strerror(err), err);
	} else {
		FPRINTF_ERR(stderr, "%s (%d)\n", uvc_strerror(err), err);
	}
/*
	 if (msg && *msg) {
	 	 fputs(msg, stderr);
	 	 fputs(": ", stderr);
	 }

	 FPRINTF(stderr, "%s (%d)\n", uvc_strerror(err), err);
*/
}

/** @brief Return a string explaining an error in the UVC driver
 * @ingroup diag
 *
 * @param err UVC error code
 * @return error message
 *
 * 返回说明UVC驱动程序中错误的字符串
 */
const char* uvc_strerror(uvc_error_t err) {
	size_t idx;

	for (idx = 0; idx < sizeof(uvc_error_msgs) / sizeof(*uvc_error_msgs);
			++idx) {
		if (uvc_error_msgs[idx].err == err) {
			return uvc_error_msgs[idx].msg;
		}
	}

	return "Unknown error";
}

/** @brief Print the values in a stream control block
 * @ingroup diag
 *
 * @param devh UVC device
 * @param stream Output stream (stderr if NULL)
 *
 * Print the values in a stream control block
 *
 * 在流控制块中打印值
 */
void uvc_print_stream_ctrl(uvc_stream_ctrl_t *ctrl, FILE *stream) {
	if (stream == NULL)
		stream = stderr;

	FPRINTF(stream, "bmHint: %04x\n", ctrl->bmHint);
	FPRINTF(stream, "bFormatIndex: %d\n", ctrl->bFormatIndex);
	FPRINTF(stream, "bFrameIndex: %d\n", ctrl->bFrameIndex);
	FPRINTF(stream, "dwFrameInterval: %u\n", ctrl->dwFrameInterval);
	FPRINTF(stream, "wKeyFrameRate: %d\n", ctrl->wKeyFrameRate);
	FPRINTF(stream, "wPFrameRate: %d\n", ctrl->wPFrameRate);
	FPRINTF(stream, "wCompQuality: %d\n", ctrl->wCompQuality);
	FPRINTF(stream, "wCompWindowSize: %d\n", ctrl->wCompWindowSize);
	FPRINTF(stream, "wDelay: %d\n", ctrl->wDelay);
	FPRINTF(stream, "dwMaxVideoFrameSize: %u\n", ctrl->dwMaxVideoFrameSize);
	FPRINTF(stream, "dwMaxPayloadTransferSize: %u\n", ctrl->dwMaxPayloadTransferSize);
	FPRINTF(stream, "bInterfaceNumber: %d\n", ctrl->bInterfaceNumber);
//
	/** add UVC 1.1 parameters
	 * 添加UVC 1.1参数
	 */
	FPRINTF(stream, "dwClockFrequency: %d\n", ctrl->dwClockFrequency);
	FPRINTF(stream, "bmFramingInfo: %d\n", ctrl->bmFramingInfo);
	FPRINTF(stream, "bPreferedVersion: %d\n", ctrl->bPreferedVersion);
	FPRINTF(stream, "bMinVersion: %d\n", ctrl->bMinVersion);
	FPRINTF(stream, "bMaxVersion: %d\n", ctrl->bMaxVersion);
	/** add UVC 1.5 parameters
	 * 添加UVC 1.5参数
	 */
	FPRINTF(stream, "bUsage: %d\n", ctrl->bUsage);
	FPRINTF(stream, "bBitDepthLuma: %d\n", ctrl->bBitDepthLuma);
	FPRINTF(stream, "bmSettings: %d\n", ctrl->bmSettings);
	FPRINTF(stream, "bMaxNumberOfRefFramesPlus1: %d\n", ctrl->bMaxNumberOfRefFramesPlus1);
	FPRINTF(stream, "bmRateControlModes: %d\n", ctrl->bmRateControlModes);
#if !defined(__LP64__)
	FPRINTF(stream, "bmLayoutPerStream: %llx\n", ctrl->bmLayoutPerStream);
#else
	FPRINTF(stream, "bmLayoutPerStream: %lx\n", ctrl->bmLayoutPerStream);
#endif
}

static const char *_uvc_name_for_desc_type(const uint8_t type) {
	switch (type) {
	case LIBUSB_DT_DEVICE:						// 0x01,
		return "Device Descriptor(0x01)";
	case LIBUSB_DT_CONFIG:						// 0x02,
		return "Config Descriptor(0x02)";
	case LIBUSB_DT_STRING:						// 0x03,
		return "String Descriptor(0x03)";
	case LIBUSB_DT_INTERFACE:					// 0x04,
		return "Interface Descriptor(0x04)";
	case LIBUSB_DT_ENDPOINT:					// 0x05,
		return "Endpoint Descriptor(0x05)";
	case LIBUSB_DT_DEVICE_QUALIFIER:			// 0x06,	// deprecated on USB3.0  在USB3.0上已弃用
		return "Qualifier Descriptor(0x06)";
	case LIBUSB_DT_OTHER_SPEED_CONFIGURATION:	// 0x07,	// deprecated on USB3.0  在USB3.0上已弃用
		return "Other speed Config Descriptor(0x07)";
	case LIBUSB_DT_INTERFACE_POWER:				// 0x08,
		return "Power interface Descriptor(0x08)";
	case LIBUSB_DT_OTG:							// 0x09,
		return "OTG Interface Descriptor(0x09)";
	case LIBUSB_DT_DEBUG:						// 0x0a,
		return "Debug Descriptor(0x0a)";
	case LIBUSB_DT_ASSOCIATION:					// 0x0b,
		return "Association Descriptor(0x0b)";
	case LIBUSB_DT_BOS:							// 0x0f,
		return "BOS Descriptor(0x0f)";
	case LIBUSB_DT_DEVICE_CAPABILITY:			// 0x10,
		return "Device capability Descriptor(0x10)";
// Class specified descriptors
	case LIBUSB_DT_HID:							// 0x21,
		return "CS:HID Descriptor(0x21)";
	case LIBUSB_DT_HID_REPORT:					// 0x22,
		return "CS:HID Report Descriptor(0x22)";
	case LIBUSB_DT_HID_PHYSICAL: 				// 0x23,
		return "CS:HID Pysical Descriptor(0x23)";
	case LIBUSB_DT_CS_INTERFACE:				// 0x24,
		return "CS:Interface Descriptor(0x24)";
	case LIBUSB_DT_CS_ENDPOINT:					// 0x25,
		return "CS:Endpoint Descriptor(0x25)";
	case LIBUSB_DT_HUB: 						// 0x29,
		return "CS:Hub Descriptor(0x29)";
	case LIBUSB_DT_SUPERSPEED_HUB: 				// 0x2a,
		return "CS:Superspeed Hub Descriptor(0x2a)";
	case LIBUSB_DT_SS_ENDPOINT_COMPANION: 		// 0x30		// defined on USB 3.0  在USB 3.0上定义
		return "CS:Superspeed Endpoint Companion Descriptor(0x30)";
	default:
		return "Unknown descriptor";
	}
}

static const char *_uvc_name_for_class(const uint8_t clazz) {
	switch (clazz) {
	case LIBUSB_CLASS_AUDIO:				// 1,		/** Audio class  音频类 */
		return "Audio";
	case LIBUSB_CLASS_COMM:					// 2,		/** Communications class  通讯类 */
		return "Comm";
	case LIBUSB_CLASS_HID:					// 3,		/** Human Interface Device class  人机界面设备类 */
		return "HID";
	case LIBUSB_CLASS_PHYSICAL:				// 5,		/** Physical  物理 */
		return "Physical";
	case LIBUSB_CLASS_IMAGE:				// 6,		/** Image class  图片类 */
		return "Image";
	case LIBUSB_CLASS_PRINTER:				// 7,		/** Printer class  打印类 */
		return "Printer";
	case LIBUSB_CLASS_MASS_STORAGE:			// 8,		/** Mass storage class  大容量存储类 */
		return "Mass storage";
	case LIBUSB_CLASS_HUB:					// 9,		/** Hub class  集线器类 */
		return "Hub";
	case LIBUSB_CLASS_DATA:					// 0x0a,	/** Data class  资料类 */
		return "Data";
	case LIBUSB_CLASS_SMART_CARD:			// 0x0b,	/** Smart Card  智能卡 */
		return "Smart card";
	case LIBUSB_CLASS_CONTENT_SECURITY:		// 0x0d,	/** Content Security  内容安全 */
		return "Security";
	case LIBUSB_CLASS_VIDEO:				// 0x0e,	/** Video  视频 */
		return "Video";
	case LIBUSB_CLASS_PERSONAL_HEALTHCARE:	// 0x0f,	/** Personal Healthcare  个人保健 */
		return "Helthcare";
	case LIBUSB_CLASS_DIAGNOSTIC_DEVICE:	// 0xdc,	/** Diagnostic Device  诊断装置 */
		return "Diag";
	case LIBUSB_CLASS_WIRELESS:				// 0xe0,	/** Wireless class  无线类 */
		return "Wireless";
	case LIBUSB_CLASS_APPLICATION:			// 0xfe,	/** Application class  应用类 */
		return "App";
	case LIBUSB_CLASS_VENDOR_SPEC:			// 0xff		/** Class is vendor-specific  类是特定于供应商的 */
		return "Vender specific";
	default:
		return "Unknown";
	}
}

static const char *_uvc_name_for_subtype(const uint8_t subtype) {
	switch (subtype) {
	case UVC_VS_UNDEFINED:				// 0x00,
		return "Undefined(0x00)";
	case UVC_VS_INPUT_HEADER:			// 0x01,
		return "Header(Input,0x01)";
	case UVC_VS_OUTPUT_HEADER:			// 0x02,
		return "Header(Output,0x02)";
	case UVC_VS_STILL_IMAGE_FRAME:		// 0x03,
		return "Frame(StillImage,0x03)";
	case UVC_VS_FORMAT_UNCOMPRESSED:	// 0x04,
		return "Format(Uncompressed,0x04)";
	case UVC_VS_FRAME_UNCOMPRESSED:		// 0x05,
		return "Frame(Uncompressed,0x05)";
	case UVC_VS_FORMAT_MJPEG:			// 0x06,
		return "Format(MJPEG,0x06)";
	case UVC_VS_FRAME_MJPEG:			// 0x07,
		return "Frame(MJPEG,0x07)";
	case UVC_VS_FORMAT_MPEG2TS:			// 0x0a,
		return "Format(MPEG2TS,0x0a)";
	case UVC_VS_FORMAT_DV:				// 0x0c,
		return "Format(DV,0x0c)";
	case UVC_VS_COLORFORMAT:			// 0x0d,
		return "ColorFormat(0x0d)";
	case UVC_VS_FORMAT_FRAME_BASED:		// 0x10,
		return "Format(FRAME_BASED,0x10)";
	case UVC_VS_FRAME_FRAME_BASED:		// 0x11,
		return "Frame(FRAME_BASED,0x10)";
	case UVC_VS_FORMAT_STREAM_BASED:	// 0x12
		return "Format(STREAM_BASED,0x10)";
	default:
		return "Unknown";
	}
}

/** @brief Print camera capabilities and configuration.
 * @ingroup diag
 *
 * @param devh UVC device
 * @param stream Output stream (stderr if NULL)
 *
 * 打印相机功能和配置。
 */
void uvc_print_diag(uvc_device_handle_t *devh, FILE *stream) {
	UVC_ENTER();

	if (stream == NULL)
		stream = stderr;

	if (devh->info->ctrl_if.bcdUVC) {
		uvc_streaming_interface_t *stream_if;
		int stream_idx = 0;

		uvc_device_descriptor_t *desc;
		uvc_get_device_descriptor(devh->dev, &desc);

		FPRINTF(stream, "DEVICE CONFIGURATION (%04x:%04x/%s) ---\n",
				desc->idVendor, desc->idProduct,
				desc->serialNumber ? desc->serialNumber : "[none]");

		uvc_free_device_descriptor(desc);

		FPRINTF(stream, "Status: %s\n", devh->streams ? "streaming" : "idle");

		FPRINTF(stream, "VideoControl:\n"
				"\tbcdUVC: 0x%04x\n", devh->info->ctrl_if.bcdUVC);

		DL_FOREACH(devh->info->stream_ifs, stream_if)
		{
			uvc_format_desc_t *fmt_desc;

			++stream_idx;

			FPRINTF(stream,
				"VideoStreaming(%d):\n"
				"\tbEndpointAddress: %d\n\tFormats:\n",
				stream_idx, stream_if->bEndpointAddress);
			uvc_print_format_desc(stream_if->format_descs, stream);
		}

		FPRINTF(stream, "END DEVICE CONFIGURATION\n");
	} else {
		FPRINTF(stream, "uvc_print_diag: Device not configured!\n");
	}
	UVC_EXIT_VOID();
}

void uvc_print_format_desc_one(uvc_format_desc_t *format_desc, FILE *stream) {

	uvc_frame_desc_t *frame_desc;
	int i;

	switch (format_desc->bDescriptorSubtype) {
	case UVC_VS_FORMAT_UNCOMPRESSED:
	case UVC_VS_FORMAT_MJPEG:
		FPRINTF(stream, "\t\tFormatDescriptor(bFormatIndex=%d)", format_desc->bFormatIndex);
		FPRINTF(stream, "\t\t  bDescriptorSubtype: %s",
			_uvc_name_for_subtype(format_desc->bDescriptorSubtype));
		FPRINTF(stream, "\t\t  bits per pixel: %d", format_desc->bBitsPerPixel);
		FPRINTF(stream,
				"\t\t  GUID:%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x",
				format_desc->guidFormat[0], format_desc->guidFormat[1],
				format_desc->guidFormat[2], format_desc->guidFormat[3],
				format_desc->guidFormat[4], format_desc->guidFormat[5],
				format_desc->guidFormat[6], format_desc->guidFormat[7],
				format_desc->guidFormat[8], format_desc->guidFormat[9],
				format_desc->guidFormat[10], format_desc->guidFormat[11],
				format_desc->guidFormat[12], format_desc->guidFormat[13],
				format_desc->guidFormat[14], format_desc->guidFormat[15]);

		FPRINTF(stream, "\t\t  bDefaultFrameIndex: %d", format_desc->bDefaultFrameIndex);
		FPRINTF(stream, "\t\t  bAspectRatio(x,y): %dx%d", format_desc->bAspectRatioX, format_desc->bAspectRatioY);
		FPRINTF(stream, "\t\t  bmInterlaceFlags: 0x%02x", format_desc->bmInterlaceFlags);
		FPRINTF(stream, "\t\t  bCopyProtect: 0x%02x", format_desc->bCopyProtect);

		DL_FOREACH(format_desc->frame_descs, frame_desc)
		{
			uint32_t *interval_ptr;

			FPRINTF(stream, "\t\t\tFrameDescriptor(bFrameIndex=%d)", frame_desc->bFrameIndex);
			FPRINTF(stream, "\t\t\t  bDescriptorSubtype: %s", _uvc_name_for_subtype(frame_desc->bDescriptorSubtype));
			FPRINTF(stream, "\t\t\t  bmCapabilities: 0x%02x", frame_desc->bmCapabilities);
			FPRINTF(stream, "\t\t\t  size(w,h):(%d,%d)", frame_desc->wWidth, frame_desc->wHeight);
			FPRINTF(stream, "\t\t\t  bit rate(min,max): (%d,%d)", frame_desc->dwMinBitRate, frame_desc->dwMaxBitRate);
			FPRINTF(stream, "\t\t\t  dwMaxVideoFrameBufferSize: %d", frame_desc->dwMaxVideoFrameBufferSize);
			FPRINTF(stream, "\t\t\t  dwDefaultFrameInterval: 1/%d", 10000000 / frame_desc->dwDefaultFrameInterval);
			if (frame_desc->intervals) {
				for (interval_ptr = frame_desc->intervals;
						*interval_ptr; ++interval_ptr) {
					FPRINTF(stream, "\t\t\t  interval[%d]: 1/%d",
							(int ) (interval_ptr - frame_desc->intervals),
							10000000 / *interval_ptr);
				}
			} else {
				FPRINTF(stream, "\t\t\t  min interval[%d] = 1/%d",
					frame_desc->dwMinFrameInterval,
					10000000 / frame_desc->dwMinFrameInterval);
				FPRINTF(stream, "\t\t\t  max interval[%d] = 1/%d",
					frame_desc->dwMaxFrameInterval,
					10000000 / frame_desc->dwMaxFrameInterval);
				if (frame_desc->dwFrameIntervalStep)
					FPRINTF(stream, "\t\t\t  interval step[%d] = 1/%d",
						frame_desc->dwFrameIntervalStep,
						10000000 / frame_desc->dwFrameIntervalStep);
			}
		}
		break;
	default:
		FPRINTF(stream, "\t-UnknownFormat:0x%2d", format_desc->bDescriptorSubtype);
	}
}

void uvc_print_format_desc(uvc_format_desc_t *format_descriptors, FILE *stream) {
	ENTER();

	if (stream == NULL)
		stream = stderr;

	uvc_format_desc_t *fmt_desc;
	MARK("FORMAT DESCRIPTOR");
	DL_FOREACH(format_descriptors, fmt_desc)
	{
		uvc_print_format_desc_one(fmt_desc, stream);
	}
	MARK("END FORMAT DESCRIPTOR");

	EXIT();
}

void uvc_print_device_desc(uvc_device_handle_t *devh, FILE *stream) {
	ENTER();

	if (stream == NULL)
		stream = stderr;

	struct libusb_device_descriptor usb_desc;
	uvc_error_t ret;

	ret = libusb_get_device_descriptor(devh->dev->usb_dev, &usb_desc);

	if (UNLIKELY(ret != UVC_SUCCESS)) {
		LOGE("failed libusb_get_device_descriptor");
		EXIT();
	}

	FPRINTF(stream, "DEVICE DESCRIPTOR (%04x:%04x)", usb_desc.idVendor, usb_desc.idProduct);
	FPRINTF(stream, "\t bLength:%d", usb_desc.bLength);
	FPRINTF(stream, "\t bDescriptorType: %s", _uvc_name_for_desc_type(usb_desc.bDescriptorType));
	FPRINTF(stream, "\t bcdUSB:0x%04x", usb_desc.bcdUSB);
	FPRINTF(stream, "\t bDeviceClass: %s(0x%02x)", _uvc_name_for_class(usb_desc.bDeviceClass), usb_desc.bDeviceClass);
	FPRINTF(stream, "\t bDeviceSubClass:0x%02x", usb_desc.bDeviceSubClass);
	FPRINTF(stream, "\t bDeviceProtocol:0x%02x", usb_desc.bDeviceProtocol);
	FPRINTF(stream, "\t bMaxPacketSize0:%d", usb_desc.bMaxPacketSize0);
	FPRINTF(stream, "\t idVendor:0x%04x", usb_desc.idVendor);
	FPRINTF(stream, "\t idProduct:0x%04x", usb_desc.idProduct);
	FPRINTF(stream, "\t bcdDevice:0x%04x", usb_desc.bcdDevice);
	FPRINTF(stream, "\t iManufacturer:%d", usb_desc.iManufacturer);
	FPRINTF(stream, "\t iProduct:%d", usb_desc.iProduct);
	FPRINTF(stream, "\t iSerialNumber:%d", usb_desc.iSerialNumber);
	FPRINTF(stream, "\t bNumConfigurations:%d", usb_desc.bNumConfigurations);

	EXIT();
}

void uvc_print_endpoint_desc(
	const struct libusb_endpoint_descriptor *endpoint, const int num_endpoint,
	const char *prefix, FILE *stream) {

//	ENTER();

	if (stream == NULL)
		stream = stderr;

	int ep_ix;
	const struct libusb_endpoint_descriptor *ep;

	for (ep_ix = 0; ep_ix < num_endpoint; ep_ix++) {
		ep = endpoint + ep_ix;
		FPRINTF(stream, "%s endpoint(%d)", prefix, ep_ix);
		if LIKELY(ep) {
			FPRINTF(stream, "%s\t bLength:%d", prefix, ep->bLength);
			FPRINTF(stream, "%s\t bDescriptorType: %s", prefix, _uvc_name_for_desc_type(ep->bDescriptorType));
			FPRINTF(stream, "%s\t bEndpointAddress:0x%02x", prefix, ep->bEndpointAddress);
			FPRINTF(stream, "%s\t bmAttributes:0x%02x", prefix, ep->bmAttributes);
			FPRINTF(stream, "%s\t wMaxPacketSize:%d", prefix, ep->wMaxPacketSize);
			FPRINTF(stream, "%s\t bInterval:%d", prefix, ep->bInterval);
			FPRINTF(stream, "%s\t bRefresh(audio):%d", prefix, ep->bRefresh);
			FPRINTF(stream, "%s\t bSynchAddress(audio):%d", prefix, ep->bSynchAddress);
			FPRINTF(stream, "%s\t extra_length:%d", prefix, ep->extra_length);
		}
	}
//	EXIT();
}

void uvc_print_interface_desc(
	const struct libusb_interface *interface, const int num_interface,
	const char *prefix, FILE *stream) {

//	ENTER();

	if (stream == NULL)
		stream = stderr;

	const struct libusb_interface *usb_if;
	const struct libusb_interface_descriptor *altsetting;
	int if_ix, alt_ix;
	char pre[64];
	sprintf(pre, "%s\t\t", prefix);
	for (if_ix = 0; if_ix < num_interface; if_ix++) {
		usb_if = interface + if_ix;
		if LIKELY(usb_if) {
			FPRINTF(stream, "%s interface(%d)", prefix, if_ix);
			for (alt_ix = 0; alt_ix < usb_if->num_altsetting; alt_ix++) {
				altsetting = usb_if->altsetting + alt_ix;
				if LIKELY(altsetting) {
					FPRINTF(stream, "%s\t altsetting:%d", prefix, alt_ix);
					FPRINTF(stream, "%s\t\t bLength:%d", prefix, altsetting->bLength);
					FPRINTF(stream, "%s\t\t bDescriptorType: %s", prefix, _uvc_name_for_desc_type(altsetting->bDescriptorType));
					FPRINTF(stream, "%s\t\t bInterfaceNumber:%d", prefix, altsetting->bInterfaceNumber);
					FPRINTF(stream, "%s\t\t bAlternateSetting:%d", prefix, altsetting->bAlternateSetting);
					FPRINTF(stream, "%s\t\t bNumEndpoints:%d", prefix, altsetting->bNumEndpoints);
					FPRINTF(stream, "%s\t\t bInterfaceClass: %s(0x%02x)", prefix, _uvc_name_for_class(altsetting->bInterfaceClass), altsetting->bInterfaceClass);
					FPRINTF(stream, "%s\t\t bInterfaceSubClass:0x%02x", prefix, altsetting->bInterfaceSubClass);
					FPRINTF(stream, "%s\t\t bInterfaceProtocol:0x%02x", prefix, altsetting->bInterfaceProtocol);
					FPRINTF(stream, "%s\t\t iInterface:%d", prefix, altsetting->iInterface);
					FPRINTF(stream, "%s\t\t extra_length:%d", prefix, altsetting->extra_length);
					if (altsetting->bNumEndpoints)
						uvc_print_endpoint_desc(altsetting->endpoint, altsetting->bNumEndpoints, pre, stream);
				}
			}
		}
	}

//	EXIT();
}

void uvc_print_configuration_desc(uvc_device_handle_t *devh, FILE *stream) {
	ENTER();

	if (stream == NULL)
		stream = stderr;

	int ret;
	libusb_device_handle *usb_devh = uvc_get_libusb_handle(devh);
	libusb_device *usb_dev = devh->dev->usb_dev;
	struct libusb_config_descriptor *config_desc;
	int config;

	FPRINTF(stream, "CONFIGURATION DESCRIPTOR");
	ret = libusb_get_configuration(usb_devh, &config);
	if (!ret) {
		FPRINTF(stream, "\t current=%d", config);
		if LIKELY(config >= 0) {
			ret = libusb_get_active_config_descriptor(usb_dev, &config_desc);
			if LIKELY(!ret) {
				FPRINTF(stream, "\t\t bLength:%d", config_desc->bLength);
				FPRINTF(stream, "\t\t bDescriptorType: %s", _uvc_name_for_desc_type(config_desc->bDescriptorType));
				FPRINTF(stream, "\t\t wTotalLength:%d", config_desc->wTotalLength);
				FPRINTF(stream, "\t\t bNumInterfaces:%d", config_desc->bNumInterfaces);
				FPRINTF(stream, "\t\t bConfigurationValue:%d", config_desc->bConfigurationValue);
				FPRINTF(stream, "\t\t iConfiguration:%d", config_desc->iConfiguration);
				FPRINTF(stream, "\t\t bmAttributes:0x%02x", config_desc->bmAttributes);
				FPRINTF(stream, "\t\t MaxPower:%d x2[mA]", config_desc->MaxPower);
				FPRINTF(stream, "\t\t extra_length:%d", config_desc->extra_length);
				if (config_desc->wTotalLength && config_desc->bNumInterfaces)
					uvc_print_interface_desc(config_desc->interface, config_desc->bNumInterfaces, "\t\t", stream);
				libusb_free_config_descriptor(config_desc);
			}
		}
	}

	EXIT();
}
