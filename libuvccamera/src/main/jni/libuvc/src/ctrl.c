/*********************************************************************
 * add some functions unsupported on original libuvc library
 * and fixed some issues
 * Copyright (C) 2014-2015 saki@serenegiant All rights reserved.
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
 * @defgroup ctrl Video capture and processing controls
 * @brief Functions for manipulating device settings and stream parameters
 *
 * The `uvc_get_*` and `uvc_set_*` functions are used to read and write the settings associated
 * with the device's input, processing and output units.
 */

#include "libuvc/libuvc.h"
#include "libuvc/libuvc_internal.h"

static const int REQ_TYPE_SET = 0x21;
static const int REQ_TYPE_GET = 0xa1;

#define CTRL_TIMEOUT_MILLIS 0

/***** GENERIC CONTROLS *****/
/**
 * @brief Get the length of a control on a terminal or unit.
 * 获取终端或单元上控件的长度。
 * 
 * @param devh UVC device handle
 * @param unit Unit or Terminal ID; obtain this from the uvc_extension_unit_t describing the extension unit
 * @param ctrl Vendor-specific control number to query
 * @return On success, the length of the control as reported by the device. Otherwise,
 *   a uvc_error_t error describing the error encountered.
 * @ingroup ctrl
 */
int uvc_get_ctrl_len(uvc_device_handle_t *devh, uint8_t unit, uint8_t ctrl) {
	unsigned char buf[2];
    // 获取设备的Probe数据结构的长度
	int ret = libusb_control_transfer(devh->usb_devh, REQ_TYPE_GET, UVC_GET_LEN,
			ctrl << 8,
			unit << 8,	// FIXME this will work wrong, invalid wIndex value  这将工作错误，wIndex值无效
			buf, 2, CTRL_TIMEOUT_MILLIS);

	if (UNLIKELY(ret < 0))
		return ret;
	else
		return (unsigned short) SW_TO_SHORT(buf);
}

/**
 * @brief Perform a GET_* request from an extension unit.
 * 从扩展单元执行GET_ *请求。
 * 
 * @param devh UVC device handle
 * @param unit Unit ID; obtain this from the uvc_extension_unit_t describing the extension unit
 * @param ctrl Control number to query
 * @param data Data buffer to be filled by the device
 * @param len Size of data buffer
 * @param req_code GET_* request to execute
 * @return On success, the number of bytes actually transferred. Otherwise,
 *   a uvc_error_t error describing the error encountered.
 * @ingroup ctrl
 */
int uvc_get_ctrl(uvc_device_handle_t *devh, uint8_t unit, uint8_t ctrl,
		void *data, int len, enum uvc_req_code req_code) {
	return libusb_control_transfer(devh->usb_devh, REQ_TYPE_GET, req_code,
			ctrl << 8,
			unit << 8,	// FIXME this will work wrong, invalid wIndex value  这将工作错误，wIndex值无效
			data, len, CTRL_TIMEOUT_MILLIS);
}

/**
 * @brief Perform a SET_CUR request to a terminal or unit.
 * 对终端或单元执行SET_CUR请求。
 * 
 * @param devh UVC device handle
 * @param unit Unit or Terminal ID
 * @param ctrl Control number to set
 * @param data Data buffer to be sent to the device
 * @param len Size of data buffer
 * @return On success, the number of bytes actually transferred. Otherwise,
 *   a uvc_error_t error describing the error encountered.
 * @ingroup ctrl
 */
int uvc_set_ctrl(uvc_device_handle_t *devh, uint8_t unit, uint8_t ctrl,
		void *data, int len) {
	// 获取设备的流接口探测状态参数
	return libusb_control_transfer(devh->usb_devh, REQ_TYPE_SET, UVC_SET_CUR,
			ctrl << 8,
			unit << 8,	// FIXME this will work wrong, invalid wIndex value  这将工作错误，wIndex值无效
			data, len, CTRL_TIMEOUT_MILLIS);
}

/***** INTERFACE CONTROLS *****/
/**
 * VC Request Error Code Control (UVC 4.2.1.2)
 * VC请求错误代码控制（UVC 4.2.1.2）
 */ // XXX added saki
uvc_error_t uvc_vc_get_error_code(uvc_device_handle_t *devh,
		uvc_vc_error_code_control_t *error_code, enum uvc_req_code req_code) {
	uint8_t error_char = 0;
	uvc_error_t ret = UVC_SUCCESS;

	ret = libusb_control_transfer(devh->usb_devh, REQ_TYPE_GET, req_code,
			UVC_VC_REQUEST_ERROR_CODE_CONTROL << 8,
			devh->info->ctrl_if.bInterfaceNumber,	// XXX saki
			&error_char, sizeof(error_char), CTRL_TIMEOUT_MILLIS);

	if (LIKELY(ret == 1)) {
		*error_code = error_char;
		return UVC_SUCCESS;
	} else {
		return ret;
	}
}

/**
 * VS Request Error Code Control
 * VS请求错误代码控制
 */ // XXX added saki
uvc_error_t uvc_vs_get_error_code(uvc_device_handle_t *devh,
		uvc_vs_error_code_control_t *error_code, enum uvc_req_code req_code) {
	uint8_t error_char = 0;
	uvc_error_t ret = UVC_SUCCESS;

#if 0 // This code may cause hang-up on some combinations of device and camera and temporary disabled.  此代码可能会导致设备和相机的某些组合挂断，并暂时禁用。
	ret = libusb_control_transfer(devh->usb_devh, REQ_TYPE_GET, req_code,
			UVC_VS_STREAM_ERROR_CODE_CONTROL << 8,
			devh->info->stream_ifs->bInterfaceNumber,	// XXX is this OK?
			&error_char, sizeof(error_char), CTRL_TIMEOUT_MILLIS);

	if (LIKELY(ret == 1)) {
		*error_code = error_char;
		return UVC_SUCCESS;
	} else {
		return ret;
	}
#else
	return ret;
#endif
}

uvc_error_t uvc_get_power_mode(uvc_device_handle_t *devh,
		enum uvc_device_power_mode *mode, enum uvc_req_code req_code) {
	uint8_t mode_char = 0;
	uvc_error_t ret;

	ret = libusb_control_transfer(devh->usb_devh, REQ_TYPE_GET, req_code,
			UVC_VC_VIDEO_POWER_MODE_CONTROL << 8,
			devh->info->ctrl_if.bInterfaceNumber,	// XXX saki
			&mode_char, sizeof(mode_char), CTRL_TIMEOUT_MILLIS);

	if (LIKELY(ret == 1)) {
		*mode = mode_char;
		return UVC_SUCCESS;
	} else {
		return ret;
	}
}

uvc_error_t uvc_set_power_mode(uvc_device_handle_t *devh,
		enum uvc_device_power_mode mode) {
	uint8_t mode_char = mode;
	uvc_error_t ret;

	ret = libusb_control_transfer(devh->usb_devh, REQ_TYPE_SET, UVC_SET_CUR,
			UVC_VC_VIDEO_POWER_MODE_CONTROL << 8,
			devh->info->ctrl_if.bInterfaceNumber,	// XXX saki
			&mode_char, sizeof(mode_char), CTRL_TIMEOUT_MILLIS);

	if (LIKELY(ret == 1))
		return UVC_SUCCESS;
	else
		return ret;
}

/***** CAMERA TERMINAL CONTROLS *****/
uvc_error_t uvc_get_ae_mode(uvc_device_handle_t *devh, uint8_t *mode,
		enum uvc_req_code req_code) {
	uint8_t data[1];
	uvc_error_t ret;

	ret = libusb_control_transfer(devh->usb_devh, REQ_TYPE_GET, req_code,
			UVC_CT_AE_MODE_CONTROL << 8,
//			1 << 8, /* = fixed ID(00) and wrong VideoControl interface descriptor subtype(UVC_VC_HEADER) on original libuvc */
			devh->info->ctrl_if.input_term_descs->request,
			data, sizeof(data), CTRL_TIMEOUT_MILLIS);

	if (LIKELY(ret == sizeof(data))) {
		*mode = data[0];
		return UVC_SUCCESS;
	} else {
		return ret;
	}
}

uvc_error_t uvc_set_ae_mode(uvc_device_handle_t *devh, uint8_t mode) {
	uint8_t data[1];
	uvc_error_t ret;

	data[0] = mode;

	ret = libusb_control_transfer(devh->usb_devh, REQ_TYPE_SET, UVC_SET_CUR,
			UVC_CT_AE_MODE_CONTROL << 8,
//			1 << 8, /* = fixed ID(00) and wrong VideoControl interface descriptor subtype(UVC_VC_HEADER) on original libuvc */
			devh->info->ctrl_if.input_term_descs->request,
			data, sizeof(data), CTRL_TIMEOUT_MILLIS);

	if (LIKELY(ret == sizeof(data)))
		return UVC_SUCCESS;
	else
		return ret;
}

//----------------------------------------------------------------------
uvc_error_t uvc_get_ae_priority(uvc_device_handle_t *devh, uint8_t *priority,
		enum uvc_req_code req_code) {
	uint8_t data[1];
	uvc_error_t ret;

	ret = libusb_control_transfer(devh->usb_devh, REQ_TYPE_GET, req_code,
			UVC_CT_AE_PRIORITY_CONTROL << 8,
//			1 << 8, /* = fixed ID(00) and wrong VideoControl interface descriptor subtype(UVC_VC_HEADER) on original libuvc */
			devh->info->ctrl_if.input_term_descs->request,
			data, sizeof(data), CTRL_TIMEOUT_MILLIS);

	if (LIKELY(ret == sizeof(data))) {
		*priority = data[0];
		return UVC_SUCCESS;
	} else {
		return ret;
	}
}

uvc_error_t uvc_set_ae_priority(uvc_device_handle_t *devh, uint8_t priority) {
	uint8_t data[1];
	uvc_error_t ret;

	data[0] = priority;

	ret = libusb_control_transfer(devh->usb_devh, REQ_TYPE_SET, UVC_SET_CUR,
			UVC_CT_AE_PRIORITY_CONTROL << 8,
//			1 << 8, /* = fixed ID(00) and wrong VideoControl interface descriptor subtype(UVC_VC_HEADER) on original libuvc */
			devh->info->ctrl_if.input_term_descs->request,
			data, sizeof(data), CTRL_TIMEOUT_MILLIS);

	if (LIKELY(ret == sizeof(data)))
		return UVC_SUCCESS;
	else
		return ret;
}

//----------------------------------------------------------------------
uvc_error_t uvc_get_exposure_abs(uvc_device_handle_t *devh, int *time,
		enum uvc_req_code req_code) {
	uint8_t data[4];
	uvc_error_t ret;

	ret = libusb_control_transfer(devh->usb_devh, REQ_TYPE_GET, req_code,
			UVC_CT_EXPOSURE_TIME_ABSOLUTE_CONTROL << 8,
//			1 << 8, /* = fixed ID(00) and wrong VideoControl interface descriptor subtype(UVC_VC_HEADER) on original libuvc */
			devh->info->ctrl_if.input_term_descs->request,
			data, sizeof(data), CTRL_TIMEOUT_MILLIS);

	if (LIKELY(ret == sizeof(data))) {
		*time = DW_TO_INT(data);
		return UVC_SUCCESS;
	} else {
		return ret;
	}
}

uvc_error_t uvc_set_exposure_abs(uvc_device_handle_t *devh, int time) {
	uint8_t data[4];
	uvc_error_t ret;

	INT_TO_DW(time, data);

	ret = libusb_control_transfer(devh->usb_devh, REQ_TYPE_SET, UVC_SET_CUR,
			UVC_CT_EXPOSURE_TIME_ABSOLUTE_CONTROL << 8,
//			1 << 8, /* = fixed ID(00) and wrong VideoControl interface descriptor subtype(UVC_VC_HEADER) on original libuvc */
			devh->info->ctrl_if.input_term_descs->request,
			data, sizeof(data), CTRL_TIMEOUT_MILLIS);

	if (LIKELY(ret == sizeof(data)))
		return UVC_SUCCESS;
	else
		return ret;
}

uvc_error_t uvc_get_exposure_rel(uvc_device_handle_t *devh, int *step,
		enum uvc_req_code req_code) {
	uint8_t data[1];
	uvc_error_t ret;

	ret = libusb_control_transfer(devh->usb_devh, REQ_TYPE_GET, req_code,
			UVC_CT_EXPOSURE_TIME_RELATIVE_CONTROL << 8,
//			1 << 8, /* = fixed ID(00) and wrong VideoControl interface descriptor subtype(UVC_VC_HEADER) on original libuvc */
			devh->info->ctrl_if.input_term_descs->request,
			data, sizeof(data), CTRL_TIMEOUT_MILLIS);

	if (LIKELY(ret == sizeof(data))) {
		*step = data[0];
		return UVC_SUCCESS;
	} else {
		return ret;
	}
}

uvc_error_t uvc_set_exposure_rel(uvc_device_handle_t *devh, int step) {
	uint8_t data[1];
	uvc_error_t ret;

	data[0] = step;

	ret = libusb_control_transfer(devh->usb_devh, REQ_TYPE_SET, UVC_SET_CUR,
			UVC_CT_EXPOSURE_TIME_RELATIVE_CONTROL << 8,
//			1 << 8, /* = fixed ID(00) and wrong VideoControl interface descriptor subtype(UVC_VC_HEADER) on original libuvc */
			devh->info->ctrl_if.input_term_descs->request,
			data, sizeof(data), CTRL_TIMEOUT_MILLIS);

	if (LIKELY(ret == sizeof(data)))
		return UVC_SUCCESS;
	else
		return ret;
}

//----------------------------------------------------------------------
uvc_error_t uvc_get_scanning_mode(uvc_device_handle_t *devh, uint8_t *mode,
		enum uvc_req_code req_code) {
	uint8_t data[1];
	uvc_error_t ret;

	ret = libusb_control_transfer(devh->usb_devh, REQ_TYPE_GET, req_code,
			UVC_CT_SCANNING_MODE_CONTROL << 8,
//			1 << 8, /* = fixed ID(00) and wrong VideoControl interface descriptor subtype(UVC_VC_HEADER) on original libuvc */
			devh->info->ctrl_if.input_term_descs->request,
			data, sizeof(data), CTRL_TIMEOUT_MILLIS);

	if (LIKELY(ret == sizeof(data))) {
		*mode = data[0];
		return UVC_SUCCESS;
	} else {
		return ret;
	}
}

uvc_error_t uvc_set_scanning_mode(uvc_device_handle_t *devh, uint8_t mode) {
	uint8_t data[1];
	uvc_error_t ret;

	data[0] = mode;

	ret = libusb_control_transfer(devh->usb_devh, REQ_TYPE_SET, UVC_SET_CUR,
			UVC_CT_SCANNING_MODE_CONTROL << 8,
//			1 << 8, /* = fixed ID(00) and wrong VideoControl interface descriptor subtype(UVC_VC_HEADER) on original libuvc */
			devh->info->ctrl_if.input_term_descs->request,
			data, sizeof(data), CTRL_TIMEOUT_MILLIS);

	if (LIKELY(ret == sizeof(data)))
		return UVC_SUCCESS;
	else
		return ret;
}

//----------------------------------------------------------------------
uvc_error_t uvc_get_focus_auto(uvc_device_handle_t *devh, uint8_t *autofocus,
		enum uvc_req_code req_code) {
	uint8_t data[1];
	uvc_error_t ret;

	ret = libusb_control_transfer(devh->usb_devh, REQ_TYPE_GET, req_code,
			UVC_CT_FOCUS_AUTO_CONTROL << 8,
//			1 << 8, /* = fixed ID(00) and wrong VideoControl interface descriptor subtype(UVC_VC_HEADER) on original libuvc */
			devh->info->ctrl_if.input_term_descs->request,
			data, sizeof(data), CTRL_TIMEOUT_MILLIS);

	if (LIKELY(ret == sizeof(data))) {
		*autofocus = data[0];
		return UVC_SUCCESS;
	} else {
		return ret;
	}
}

uvc_error_t uvc_set_focus_auto(uvc_device_handle_t *devh, uint8_t autofocus) {
	uint8_t data[1];
	uvc_error_t ret;

	data[0] = autofocus;

	ret = libusb_control_transfer(devh->usb_devh, REQ_TYPE_SET, UVC_SET_CUR,
			UVC_CT_FOCUS_AUTO_CONTROL << 8,
//			1 << 8, /* = fixed ID(00) and wrong VideoControl interface descriptor subtype(UVC_VC_HEADER) on original libuvc */
			devh->info->ctrl_if.input_term_descs->request,
			data, sizeof(data), CTRL_TIMEOUT_MILLIS);

	if (LIKELY(ret == sizeof(data)))
		return UVC_SUCCESS;
	else
		return ret;
}

//----------------------------------------------------------------------
uvc_error_t uvc_get_focus_abs(uvc_device_handle_t *devh, short *focus,
		enum uvc_req_code req_code) {
	uint8_t data[2];
	uvc_error_t ret;

	ret = libusb_control_transfer(devh->usb_devh, REQ_TYPE_GET, req_code,
			UVC_CT_FOCUS_ABSOLUTE_CONTROL << 8,
//			1 << 8, /* = fixed ID(00) and wrong VideoControl interface descriptor subtype(UVC_VC_HEADER) on original libuvc */
			devh->info->ctrl_if.input_term_descs->request,
			data, sizeof(data), CTRL_TIMEOUT_MILLIS);

	if (LIKELY(ret == sizeof(data))) {
		*focus = SW_TO_SHORT(data);
		return UVC_SUCCESS;
	} else {
		return ret;
	}
}

uvc_error_t uvc_set_focus_abs(uvc_device_handle_t *devh, short focus) {
	uint8_t data[2];
	uvc_error_t ret;

	SHORT_TO_SW(focus, data);

	ret = libusb_control_transfer(devh->usb_devh, REQ_TYPE_SET, UVC_SET_CUR,
			UVC_CT_FOCUS_ABSOLUTE_CONTROL << 8,
//			1 << 8, /* = fixed ID(00) and wrong VideoControl interface descriptor subtype(UVC_VC_HEADER) on original libuvc */
			devh->info->ctrl_if.input_term_descs->request,
			data, sizeof(data), CTRL_TIMEOUT_MILLIS);

	if (LIKELY(ret == sizeof(data)))
		return UVC_SUCCESS;
	else
		return ret;
}

uvc_error_t uvc_get_focus_rel(uvc_device_handle_t *devh, int8_t *focus, uint8_t *speed,
		enum uvc_req_code req_code) {
	uint8_t data[2];
	uvc_error_t ret;

	ret = libusb_control_transfer(devh->usb_devh, REQ_TYPE_GET, req_code,
			UVC_CT_FOCUS_RELATIVE_CONTROL << 8,
//			1 << 8, /* = fixed ID(00) and wrong VideoControl interface descriptor subtype(UVC_VC_HEADER) on original libuvc */
			devh->info->ctrl_if.input_term_descs->request,
			data, sizeof(data), CTRL_TIMEOUT_MILLIS);

	if (LIKELY(ret == sizeof(data))) {
		*focus = data[0];
		*speed = data[1];
		return UVC_SUCCESS;
	} else {
		return ret;
	}
}

uvc_error_t uvc_set_focus_rel(uvc_device_handle_t *devh, int8_t focus, uint8_t speed) {
	uint8_t data[2];
	uvc_error_t ret;

	data[0] = focus;
	data[1] = speed;

	ret = libusb_control_transfer(devh->usb_devh, REQ_TYPE_SET, UVC_SET_CUR,
			UVC_CT_FOCUS_RELATIVE_CONTROL << 8,
//			1 << 8, /* = fixed ID(00) and wrong VideoControl interface descriptor subtype(UVC_VC_HEADER) on original libuvc */
			devh->info->ctrl_if.input_term_descs->request,
			data, sizeof(data), CTRL_TIMEOUT_MILLIS);

	if (LIKELY(ret == sizeof(data)))
		return UVC_SUCCESS;
	else
		return ret;
}

//----------------------------------------------------------------------
uvc_error_t uvc_get_iris_abs(uvc_device_handle_t *devh, uint16_t *iris,
		enum uvc_req_code req_code) {
	uint8_t data[2];
	uvc_error_t ret;

	ret = libusb_control_transfer(devh->usb_devh, REQ_TYPE_GET, req_code,
			UVC_CT_FOCUS_ABSOLUTE_CONTROL << 8,
//			1 << 8, /* = fixed ID(00) and wrong VideoControl interface descriptor subtype(UVC_VC_HEADER) on original libuvc */
			devh->info->ctrl_if.input_term_descs->request,
			data, sizeof(data), CTRL_TIMEOUT_MILLIS);

	if (LIKELY(ret == sizeof(data))) {
		*iris = SW_TO_SHORT(data);
		return UVC_SUCCESS;
	} else {
		return ret;
	}
}

uvc_error_t uvc_set_iris_abs(uvc_device_handle_t *devh, uint16_t iris) {
	uint8_t data[2];
	uvc_error_t ret;

	SHORT_TO_SW(iris, data);

	ret = libusb_control_transfer(devh->usb_devh, REQ_TYPE_SET, UVC_SET_CUR,
			UVC_CT_FOCUS_ABSOLUTE_CONTROL << 8,
//			1 << 8, /* = fixed ID(00) and wrong VideoControl interface descriptor subtype(UVC_VC_HEADER) on original libuvc */
			devh->info->ctrl_if.input_term_descs->request,
			data, sizeof(data), CTRL_TIMEOUT_MILLIS);

	if (LIKELY(ret == sizeof(data)))
		return UVC_SUCCESS;
	else
		return ret;
}

uvc_error_t uvc_get_iris_rel(uvc_device_handle_t *devh, uint8_t *iris,
		enum uvc_req_code req_code) {
	uint8_t data[1];
	uvc_error_t ret;

	ret = libusb_control_transfer(devh->usb_devh, REQ_TYPE_GET, req_code,
			UVC_CT_FOCUS_RELATIVE_CONTROL << 8,
//			1 << 8, /* = fixed ID(00) and wrong VideoControl interface descriptor subtype(UVC_VC_HEADER) on original libuvc */
			devh->info->ctrl_if.input_term_descs->request,
			data, sizeof(data), CTRL_TIMEOUT_MILLIS);

	if (LIKELY(ret == sizeof(data))) {
		*iris = data[0];
		return UVC_SUCCESS;
	} else {
		return ret;
	}
}

uvc_error_t uvc_set_iris_rel(uvc_device_handle_t *devh, uint8_t iris) {
	uint8_t data[1];
	uvc_error_t ret;

	data[0] = iris;

	ret = libusb_control_transfer(devh->usb_devh, REQ_TYPE_SET, UVC_SET_CUR,
			UVC_CT_FOCUS_RELATIVE_CONTROL << 8,
//			1 << 8, /* = fixed ID(00) and wrong VideoControl interface descriptor subtype(UVC_VC_HEADER) on original libuvc */
			devh->info->ctrl_if.input_term_descs->request,
			data, sizeof(data), CTRL_TIMEOUT_MILLIS);

	if (LIKELY(ret == sizeof(data)))
		return UVC_SUCCESS;
	else
		return ret;
}

//----------------------------------------------------------------------
uvc_error_t uvc_get_zoom_abs(uvc_device_handle_t *devh, uint16_t *zoom,
		enum uvc_req_code req_code) {
	uint8_t data[2];
	uvc_error_t ret;

	ret = libusb_control_transfer(devh->usb_devh, REQ_TYPE_GET, req_code,
			UVC_CT_ZOOM_ABSOLUTE_CONTROL << 8,
			devh->info->ctrl_if.input_term_descs->request,
			data, sizeof(data), CTRL_TIMEOUT_MILLIS);

	if (LIKELY(ret == sizeof(data))) {
		*zoom = SW_TO_SHORT(data);
		return UVC_SUCCESS;
	} else {
		return ret;
	}
}

uvc_error_t uvc_set_zoom_abs(uvc_device_handle_t *devh, uint16_t zoom) {
	uint8_t data[2];
	uvc_error_t ret;

	SHORT_TO_SW(zoom, data);

	ret = libusb_control_transfer(devh->usb_devh, REQ_TYPE_SET, UVC_SET_CUR,
			UVC_CT_ZOOM_ABSOLUTE_CONTROL << 8,
			devh->info->ctrl_if.input_term_descs->request,
			data, sizeof(data), CTRL_TIMEOUT_MILLIS);

	if (LIKELY(ret == sizeof(data)))
		return UVC_SUCCESS;
	else
		return ret;
}

uvc_error_t uvc_get_zoom_rel(uvc_device_handle_t *devh, int8_t *zoom, uint8_t *isdigital, uint8_t *speed,
		enum uvc_req_code req_code) {
	uint8_t data[3];
	uvc_error_t ret;

	ret = libusb_control_transfer(devh->usb_devh, REQ_TYPE_GET, req_code,
			UVC_CT_ZOOM_RELATIVE_CONTROL << 8,
			devh->info->ctrl_if.input_term_descs->request,
			data, sizeof(data), CTRL_TIMEOUT_MILLIS);

	if (LIKELY(ret == sizeof(data))) {
		*zoom = data[0];
		*isdigital = data[1];
		*speed = data[2];
		return UVC_SUCCESS;
	} else {
		return ret;
	}
}

uvc_error_t uvc_set_zoom_rel(uvc_device_handle_t *devh, int8_t zoom, uint8_t isdigital, uint8_t speed) {
	uint8_t data[3];
	uvc_error_t ret;

	data[0] = zoom;
	data[1] = isdigital;
	data[2] = speed;

	ret = libusb_control_transfer(devh->usb_devh, REQ_TYPE_SET, UVC_SET_CUR,
			UVC_CT_ZOOM_RELATIVE_CONTROL << 8,
			devh->info->ctrl_if.input_term_descs->request,
			data, sizeof(data), CTRL_TIMEOUT_MILLIS);

	if (LIKELY(ret == sizeof(data)))
		return UVC_SUCCESS;
	else
		return ret;
}

//----------------------------------------------------------------------
uvc_error_t uvc_get_pantilt_abs(uvc_device_handle_t *devh, int32_t *pan, int32_t *tilt,
	enum uvc_req_code req_code) {

	uint8_t data[8];
	uvc_error_t ret;

	ret = libusb_control_transfer(devh->usb_devh, REQ_TYPE_GET, req_code,
			UVC_CT_PANTILT_ABSOLUTE_CONTROL << 8,
			devh->info->ctrl_if.input_term_descs->request,
			data, sizeof(data), CTRL_TIMEOUT_MILLIS);

	if (LIKELY(ret == sizeof(data))) {
		*pan = DW_TO_INT(data);
		*tilt = DW_TO_INT(data + 4);
		return UVC_SUCCESS;
	} else {
		return ret;
	}
}

uvc_error_t uvc_set_pantilt_abs(uvc_device_handle_t *devh, int32_t pan, int32_t tilt) {
	uint8_t data[8];
	uvc_error_t ret;

	INT_TO_DW(pan, data);
	INT_TO_DW(tilt, data + 4);

	ret = libusb_control_transfer(devh->usb_devh, REQ_TYPE_SET, UVC_SET_CUR,
			UVC_CT_PANTILT_ABSOLUTE_CONTROL << 8,
			devh->info->ctrl_if.input_term_descs->request,
			data, sizeof(data), CTRL_TIMEOUT_MILLIS);

	if (LIKELY(ret == sizeof(data)))
		return UVC_SUCCESS;
	else
		return ret;
}

uvc_error_t uvc_get_pantilt_rel(uvc_device_handle_t *devh,
	int8_t *pan_rel, uint8_t *pan_speed,
	int8_t* tilt_rel, uint8_t* tilt_speed,
	enum uvc_req_code req_code) {

	uint8_t data[4];
	uvc_error_t ret;

	ret = libusb_control_transfer(devh->usb_devh, REQ_TYPE_GET, req_code,
			UVC_CT_PANTILT_RELATIVE_CONTROL << 8,
			devh->info->ctrl_if.input_term_descs->request,
			data, sizeof(data), CTRL_TIMEOUT_MILLIS);

	if (LIKELY(ret == sizeof(data))) {
		*pan_rel = data[0];
		*pan_speed = data[1];
		*tilt_rel = data[2];
		*tilt_speed = data[3];
		return UVC_SUCCESS;
	} else {
		return ret;
	}
}

uvc_error_t uvc_set_pantilt_rel(uvc_device_handle_t *devh,
	int8_t pan_rel, uint8_t pan_speed,
	int8_t tilt_rel, uint8_t tilt_speed) {

	uint8_t data[4];
	uvc_error_t ret;

	data[0] = pan_rel;
	data[1] = pan_speed;
	data[2] = tilt_rel;
	data[3] = tilt_speed;

	ret = libusb_control_transfer(devh->usb_devh, REQ_TYPE_SET, UVC_SET_CUR,
			UVC_CT_PANTILT_RELATIVE_CONTROL << 8,
			devh->info->ctrl_if.input_term_descs->request,
			data, sizeof(data), CTRL_TIMEOUT_MILLIS);

	if (LIKELY(ret == sizeof(data)))
		return UVC_SUCCESS;
	else
		return ret;
}

uvc_error_t uvc_get_roll_abs(uvc_device_handle_t *devh, int16_t *roll,
	enum uvc_req_code req_code) {

	uint8_t data[2];
	uvc_error_t ret;

	ret = libusb_control_transfer(devh->usb_devh, REQ_TYPE_GET, req_code,
			UVC_CT_ROLL_ABSOLUTE_CONTROL << 8,
			devh->info->ctrl_if.input_term_descs->request,
			data, sizeof(data), CTRL_TIMEOUT_MILLIS);

	if (LIKELY(ret == sizeof(data))) {
		*roll = SW_TO_SHORT(data + 0);
		return UVC_SUCCESS;
	} else {
		return ret;
	}
}

uvc_error_t uvc_set_roll_abs(uvc_device_handle_t *devh, int16_t roll) {

	uint8_t data[2];
	uvc_error_t ret;

	SHORT_TO_SW(roll, data + 0);

	ret = libusb_control_transfer(devh->usb_devh, REQ_TYPE_SET, UVC_SET_CUR,
			UVC_CT_ROLL_ABSOLUTE_CONTROL << 8,
			devh->info->ctrl_if.input_term_descs->request,
			data, sizeof(data), CTRL_TIMEOUT_MILLIS);

	if (LIKELY(ret == sizeof(data)))
		return UVC_SUCCESS;
	else
		return ret;
}

/** @ingroup ctrl
 * @brief Reads the ROLL_RELATIVE control.
 * @param devh UVC device handle
 * @param[out] roll_rel TODO
 * @param[out] speed TODO
 * @param req_code UVC_GET_* request to execute
 */
uvc_error_t uvc_get_roll_rel(uvc_device_handle_t *devh, int8_t *roll_rel, uint8_t *speed,
	enum uvc_req_code req_code) {

	uint8_t data[2];
	uvc_error_t ret;

	ret = libusb_control_transfer(devh->usb_devh, REQ_TYPE_GET, req_code,
			UVC_CT_ROLL_RELATIVE_CONTROL << 8,
			devh->info->ctrl_if.input_term_descs->request,
			data, sizeof(data), CTRL_TIMEOUT_MILLIS);

	if (LIKELY(ret == sizeof(data))) {
		*roll_rel = data[0];
		*speed = data[1];
		return UVC_SUCCESS;
	} else {
		return ret;
	}
}


/** @ingroup ctrl
 * @brief Sets the ROLL_RELATIVE control.
 * @param devh UVC device handle
 * @param roll_rel TODO
 * @param speed TODO
 */
uvc_error_t uvc_set_roll_rel(uvc_device_handle_t *devh, int8_t roll_rel, uint8_t speed) {

	uint8_t data[2];
	uvc_error_t ret;

	data[0] = roll_rel;
	data[1] = speed;

	ret = libusb_control_transfer(devh->usb_devh, REQ_TYPE_SET, UVC_SET_CUR,
			UVC_CT_ROLL_RELATIVE_CONTROL << 8,
			devh->info->ctrl_if.input_term_descs->request,
			data, sizeof(data), CTRL_TIMEOUT_MILLIS);

	if (LIKELY(ret == sizeof(data)))
		return UVC_SUCCESS;
	else
		return ret;
}

/** @ingroup ctrl
 * @brief Reads the PRIVACY control.
 * @param devh UVC device handle
 * @param[out] privacy TODO
 * @param req_code UVC_GET_* request to execute
 */
uvc_error_t uvc_get_privacy(uvc_device_handle_t *devh, uint8_t *privacy,
	enum uvc_req_code req_code) {

	uint8_t data[1];
	uvc_error_t ret;

	ret = libusb_control_transfer(devh->usb_devh, REQ_TYPE_GET, req_code,
			UVC_CT_PRIVACY_CONTROL << 8,
			devh->info->ctrl_if.input_term_descs->request,
			data, sizeof(data), CTRL_TIMEOUT_MILLIS);

	if (LIKELY(ret == sizeof(data))) {
		*privacy = data[0];
		return UVC_SUCCESS;
	} else {
		return ret;
	}
}


/** @ingroup ctrl
 * @brief Sets the PRIVACY control.
 * @param devh UVC device handle
 * @param privacy TODO
 */
uvc_error_t uvc_set_privacy(uvc_device_handle_t *devh, uint8_t privacy) {

	uint8_t data[1];
	uvc_error_t ret;

	data[0] = privacy;

	ret = libusb_control_transfer(devh->usb_devh, REQ_TYPE_SET, UVC_SET_CUR,
			UVC_CT_PRIVACY_CONTROL << 8,
			devh->info->ctrl_if.input_term_descs->request,
			data, sizeof(data), CTRL_TIMEOUT_MILLIS);

	if (LIKELY(ret == sizeof(data)))
		return UVC_SUCCESS;
	else
		return ret;
}

/** @ingroup ctrl
 * @brief Reads the DIGITAL_WINDOW control.
 * @param devh UVC device handle
 * @param[out] window_top TODO
 * @param[out] window_left TODO
 * @param[out] window_bottom TODO
 * @param[out] window_right TODO
 * @param[out] num_steps TODO
 * @param[out] num_steps_units TODO
 * @param req_code UVC_GET_* request to execute
 */
uvc_error_t uvc_get_digital_window(uvc_device_handle_t *devh,
	uint16_t *window_top, uint16_t *window_left,
	uint16_t *window_bottom, uint16_t *window_right,
	uint16_t *num_steps, uint16_t *num_steps_units,
	enum uvc_req_code req_code) {

	uint8_t data[12];
	uvc_error_t ret;

	ret = libusb_control_transfer(devh->usb_devh, REQ_TYPE_GET, req_code,
			UVC_CT_DIGITAL_WINDOW_CONTROL << 8,
			devh->info->ctrl_if.input_term_descs->request,
			data, sizeof(data), CTRL_TIMEOUT_MILLIS);

	if (LIKELY(ret == sizeof(data))) {
		*window_top = SW_TO_SHORT(data + 0);
		*window_left = SW_TO_SHORT(data + 2);
		*window_bottom = SW_TO_SHORT(data + 4);
		*window_right = SW_TO_SHORT(data + 6);
		*num_steps = SW_TO_SHORT(data + 8);
		*num_steps_units = SW_TO_SHORT(data + 10);
		return UVC_SUCCESS;
	} else {
		return ret;
	}
}


/** @ingroup ctrl
 * @brief Sets the DIGITAL_WINDOW control.
 * @param devh UVC device handle
 * @param window_top TODO
 * @param window_left TODO
 * @param window_bottom TODO
 * @param window_right TODO
 * @param num_steps TODO
 * @param num_steps_units TODO
 */
uvc_error_t uvc_set_digital_window(uvc_device_handle_t *devh,
	uint16_t window_top, uint16_t window_left,
	uint16_t window_bottom, uint16_t window_right,
	uint16_t num_steps, uint16_t num_steps_units) {

	uint8_t data[12];
	uvc_error_t ret;

	SHORT_TO_SW(window_top, data + 0);
	SHORT_TO_SW(window_left, data + 2);
	SHORT_TO_SW(window_bottom, data + 4);
	SHORT_TO_SW(window_right, data + 6);
	SHORT_TO_SW(num_steps, data + 8);
	SHORT_TO_SW(num_steps_units, data + 10);

	ret = libusb_control_transfer(devh->usb_devh, REQ_TYPE_SET, UVC_SET_CUR,
			UVC_CT_DIGITAL_WINDOW_CONTROL << 8,
			devh->info->ctrl_if.input_term_descs->request,
			data, sizeof(data), CTRL_TIMEOUT_MILLIS);

	if (LIKELY(ret == sizeof(data)))
		return UVC_SUCCESS;
	else
		return ret;
}

/** @ingroup ctrl
 * @brief Reads the REGION_OF_INTEREST control.
 * @param devh UVC device handle
 * @param[out] roi_top TODO
 * @param[out] roi_left TODO
 * @param[out] roi_bottom TODO
 * @param[out] roi_right TODO
 * @param[out] auto_controls TODO
 * @param req_code UVC_GET_* request to execute
 */
uvc_error_t uvc_get_digital_roi(uvc_device_handle_t *devh,
	uint16_t *roi_top, uint16_t *roi_left,
	uint16_t* roi_bottom, uint16_t *roi_right, uint16_t *auto_controls,
	enum uvc_req_code req_code) {

	uint8_t data[10];
	uvc_error_t ret;

	ret = libusb_control_transfer(devh->usb_devh, REQ_TYPE_GET, req_code,
			UVC_CT_REGION_OF_INTEREST_CONTROL << 8,
			devh->info->ctrl_if.input_term_descs->request,
			data, sizeof(data), CTRL_TIMEOUT_MILLIS);

	if (LIKELY(ret == sizeof(data))) {
		*roi_top = SW_TO_SHORT(data + 0);
		*roi_left = SW_TO_SHORT(data + 2);
		*roi_bottom = SW_TO_SHORT(data + 4);
		*roi_right = SW_TO_SHORT(data + 6);
		*auto_controls = SW_TO_SHORT(data + 8);
		return UVC_SUCCESS;
	} else {
		return ret;
	}
}


/** @ingroup ctrl
 * @brief Sets the REGION_OF_INTEREST control.
 * @param devh UVC device handle
 * @param roi_top TODO
 * @param roi_left TODO
 * @param roi_bottom TODO
 * @param roi_right TODO
 * @param auto_controls TODO
 */
uvc_error_t uvc_set_digital_roi(uvc_device_handle_t *devh,
	uint16_t roi_top, uint16_t roi_left,
	uint16_t roi_bottom, uint16_t roi_right, uint16_t auto_controls) {

	uint8_t data[10];
	uvc_error_t ret;

	SHORT_TO_SW(roi_top, data + 0);
	SHORT_TO_SW(roi_left, data + 2);
	SHORT_TO_SW(roi_bottom, data + 4);
	SHORT_TO_SW(roi_right, data + 6);
	SHORT_TO_SW(auto_controls, data + 8);

	ret = libusb_control_transfer(devh->usb_devh, REQ_TYPE_SET, UVC_SET_CUR,
			UVC_CT_REGION_OF_INTEREST_CONTROL << 8,
			devh->info->ctrl_if.input_term_descs->request,
			data, sizeof(data), CTRL_TIMEOUT_MILLIS);

	if (LIKELY(ret == sizeof(data)))
		return UVC_SUCCESS;
	else
		return ret;
}

/***** SELECTOR UNIT CONTROLS *****/

/** @todo input_select */

/***** PROCESSING UNIT CONTROLS *****/
uvc_error_t uvc_get_backlight_compensation(uvc_device_handle_t *devh, short *comp,
		enum uvc_req_code req_code) {
	uint8_t data[2];
	uvc_error_t ret;

	ret = libusb_control_transfer(devh->usb_devh, REQ_TYPE_GET, req_code,
			UVC_PU_BACKLIGHT_COMPENSATION_CONTROL << 8,
			devh->info->ctrl_if.processing_unit_descs->request,
			data, sizeof(data), CTRL_TIMEOUT_MILLIS);

	if (LIKELY(ret == sizeof(data))) {
		*comp = SW_TO_SHORT(data);
		return UVC_SUCCESS;
	} else {
		return ret;
	}
	RETURN(-1, uvc_error_t);
}

uvc_error_t uvc_set_backlight_compensation(uvc_device_handle_t *devh, short comp) {
	uint8_t data[2];
	uvc_error_t ret;

	SHORT_TO_SW(comp, data);

	ret = libusb_control_transfer(devh->usb_devh, REQ_TYPE_SET, UVC_SET_CUR,
			UVC_PU_BACKLIGHT_COMPENSATION_CONTROL << 8,
			devh->info->ctrl_if.processing_unit_descs->request,
			data, sizeof(data), CTRL_TIMEOUT_MILLIS);

	if (LIKELY(ret == sizeof(data)))
		return UVC_SUCCESS;
	else
		return ret;
}

uvc_error_t uvc_get_brightness(uvc_device_handle_t *devh, short *brightness,
		enum uvc_req_code req_code) {
	uint8_t data[2];
	uvc_error_t ret;

	ret = libusb_control_transfer(devh->usb_devh, REQ_TYPE_GET, req_code,
			UVC_PU_BRIGHTNESS_CONTROL << 8,
			devh->info->ctrl_if.processing_unit_descs->request,
			data, sizeof(data), CTRL_TIMEOUT_MILLIS);

	if (LIKELY(ret == sizeof(data))) {
		*brightness = SW_TO_SHORT(data);
		return UVC_SUCCESS;
	} else {
		return ret;
	}
	RETURN(-1, uvc_error_t);
}

uvc_error_t uvc_set_brightness(uvc_device_handle_t *devh, short brightness) {
	uint8_t data[2];
	uvc_error_t ret;

	SHORT_TO_SW(brightness, data);

	ret = libusb_control_transfer(devh->usb_devh, REQ_TYPE_SET, UVC_SET_CUR,
			UVC_PU_BRIGHTNESS_CONTROL << 8,
			devh->info->ctrl_if.processing_unit_descs->request,
			data, sizeof(data), CTRL_TIMEOUT_MILLIS);

	if (LIKELY(ret == sizeof(data)))
		return UVC_SUCCESS;
	else
		return ret;
}

uvc_error_t uvc_get_contrast(uvc_device_handle_t *devh, uint16_t *contrast,
		enum uvc_req_code req_code) {
	uint8_t data[2];
	uvc_error_t ret;

	ret = libusb_control_transfer(devh->usb_devh, REQ_TYPE_GET, req_code,
			UVC_PU_CONTRAST_CONTROL << 8,
			devh->info->ctrl_if.processing_unit_descs->request,
			data, sizeof(data), CTRL_TIMEOUT_MILLIS);

	if (LIKELY(ret == sizeof(data))) {
		*contrast = SW_TO_SHORT(data);
		return UVC_SUCCESS;
	} else {
		return ret;
	}
	RETURN(-1, uvc_error_t);
}

uvc_error_t uvc_set_contrast(uvc_device_handle_t *devh, uint16_t contrast) {
	uint8_t data[2];
	uvc_error_t ret;

	SHORT_TO_SW(contrast, data);

	ret = libusb_control_transfer(devh->usb_devh, REQ_TYPE_SET, UVC_SET_CUR,
			UVC_PU_CONTRAST_CONTROL << 8,
			devh->info->ctrl_if.processing_unit_descs->request,
			data, sizeof(data), CTRL_TIMEOUT_MILLIS);

	if (LIKELY(ret == sizeof(data)))
		return UVC_SUCCESS;
	else
		return ret;
}

uvc_error_t uvc_get_contrast_auto(uvc_device_handle_t *devh, uint8_t *autoContrast,
		enum uvc_req_code req_code) {
	uint8_t data[1];
	uvc_error_t ret;

	ret = libusb_control_transfer(devh->usb_devh, REQ_TYPE_GET, req_code,
			UVC_PU_CONTRAST_AUTO_CONTROL << 8,
			devh->info->ctrl_if.processing_unit_descs->request,
			data, sizeof(data), CTRL_TIMEOUT_MILLIS);

	if (LIKELY(ret == sizeof(data))) {
		*autoContrast = data[0];
		return UVC_SUCCESS;
	} else {
		return ret;
	}
	RETURN(-1, uvc_error_t);
}

uvc_error_t uvc_set_contrast_auto(uvc_device_handle_t *devh, uint8_t autoContrast) {
	uint8_t data[1];
	uvc_error_t ret;

	data[0] = autoContrast ? 1 : 0;

	ret = libusb_control_transfer(devh->usb_devh, REQ_TYPE_SET, UVC_SET_CUR,
			UVC_PU_CONTRAST_AUTO_CONTROL << 8,
			devh->info->ctrl_if.processing_unit_descs->request,
			data, sizeof(data), CTRL_TIMEOUT_MILLIS);

	if (LIKELY(ret == sizeof(data)))
		return UVC_SUCCESS;
	else
		return ret;
}

uvc_error_t uvc_get_gain(uvc_device_handle_t *devh, uint16_t *gain,
		enum uvc_req_code req_code) {
	uint8_t data[2];
	uvc_error_t ret;

	ret = libusb_control_transfer(devh->usb_devh, REQ_TYPE_GET, req_code,
			UVC_PU_GAIN_CONTROL << 8,
			devh->info->ctrl_if.processing_unit_descs->request,
			data, sizeof(data), CTRL_TIMEOUT_MILLIS);

	if (LIKELY(ret == sizeof(data))) {
		*gain = SW_TO_SHORT(data);
		return UVC_SUCCESS;
	} else {
		return ret;
	}
	RETURN(-1, uvc_error_t);
}

uvc_error_t uvc_set_gain(uvc_device_handle_t *devh, uint16_t gain) {
	uint8_t data[2];
	uvc_error_t ret;

	SHORT_TO_SW(gain, data);

	ret = libusb_control_transfer(devh->usb_devh, REQ_TYPE_SET, UVC_SET_CUR,
			UVC_PU_GAIN_CONTROL << 8,
			devh->info->ctrl_if.processing_unit_descs->request,
			data, sizeof(data), CTRL_TIMEOUT_MILLIS);

	if (LIKELY(ret == sizeof(data)))
		return UVC_SUCCESS;
	else
		return ret;
}

uvc_error_t uvc_get_powerline_freqency(uvc_device_handle_t *devh, uint8_t *freq,
		enum uvc_req_code req_code) {
	uint8_t data[1];
	uvc_error_t ret;

	ret = libusb_control_transfer(devh->usb_devh, REQ_TYPE_GET, req_code,
			UVC_PU_POWER_LINE_FREQUENCY_CONTROL << 8,
			devh->info->ctrl_if.processing_unit_descs->request,
			data, sizeof(data), CTRL_TIMEOUT_MILLIS);

	if (LIKELY(ret == sizeof(data))) {
		*freq = data[0];
		return UVC_SUCCESS;
	} else {
		return ret;
	}
	RETURN(-1, uvc_error_t);
}

uvc_error_t uvc_set_powerline_freqency(uvc_device_handle_t *devh, uint8_t freq) {
	uint8_t data[1];
	uvc_error_t ret;

	// XXX AUTO(0x03) is only available for UVC1.5.
	// AUTO（0x03）仅适用于UVC1.5。
	if ( ((freq & 0xff) == 0xff)
		|| (((freq & 0x03) == 0x03) && (devh->info->ctrl_if.bcdUVC < 0x0150)) ) {

		ret = uvc_get_powerline_freqency(devh, &freq, UVC_GET_DEF);
		if (UNLIKELY(ret)) {
			LOGE("failed to uvc_get_powerline_freqency:err=%d", ret);
			return ret;
		}
	}

	data[0] = freq & 0x03;

	ret = libusb_control_transfer(devh->usb_devh, REQ_TYPE_SET, UVC_SET_CUR,
			UVC_PU_POWER_LINE_FREQUENCY_CONTROL << 8,
			devh->info->ctrl_if.processing_unit_descs->request,
			data, sizeof(data), CTRL_TIMEOUT_MILLIS);

	if (LIKELY(ret == sizeof(data)))
		return UVC_SUCCESS;
	else
		return ret;
}

uvc_error_t uvc_get_hue(uvc_device_handle_t *devh, short *hue,
		enum uvc_req_code req_code) {
	uint8_t data[2];
	uvc_error_t ret;

	ret = libusb_control_transfer(devh->usb_devh, REQ_TYPE_GET, req_code,
			UVC_PU_HUE_CONTROL << 8,
			devh->info->ctrl_if.processing_unit_descs->request,
			data, sizeof(data), CTRL_TIMEOUT_MILLIS);

	if (LIKELY(ret == sizeof(data))) {
		*hue = SW_TO_SHORT(data);
		return UVC_SUCCESS;
	} else {
		return ret;
	}
	RETURN(-1, uvc_error_t);
}

uvc_error_t uvc_set_hue(uvc_device_handle_t *devh, short hue) {
	uint8_t data[2];
	uvc_error_t ret;

	SHORT_TO_SW(hue, data);

	ret = libusb_control_transfer(devh->usb_devh, REQ_TYPE_SET, UVC_SET_CUR,
			UVC_PU_HUE_CONTROL << 8,
			devh->info->ctrl_if.processing_unit_descs->request,
			data, sizeof(data), CTRL_TIMEOUT_MILLIS);

	if (LIKELY(ret == sizeof(data)))
		return UVC_SUCCESS;
	else
		return ret;
}

uvc_error_t uvc_get_hue_auto(uvc_device_handle_t *devh, uint8_t *autoHue,
		enum uvc_req_code req_code) {
	uint8_t data[1];
	uvc_error_t ret;

	ret = libusb_control_transfer(devh->usb_devh, REQ_TYPE_GET, req_code,
			UVC_PU_HUE_AUTO_CONTROL << 8,
			devh->info->ctrl_if.processing_unit_descs->request,
			data, sizeof(data), CTRL_TIMEOUT_MILLIS);

	if (LIKELY(ret == sizeof(data))) {
		*autoHue = data[0];
		return UVC_SUCCESS;
	} else {
		return ret;
	}
	RETURN(-1, uvc_error_t);
}

uvc_error_t uvc_set_hue_auto(uvc_device_handle_t *devh, uint8_t autoHue) {
	uint8_t data[1];
	uvc_error_t ret;

	data[0] = autoHue ? 1 : 0;

	ret = libusb_control_transfer(devh->usb_devh, REQ_TYPE_SET, UVC_SET_CUR,
			UVC_PU_HUE_AUTO_CONTROL << 8,
			devh->info->ctrl_if.processing_unit_descs->request,
			data, sizeof(data), CTRL_TIMEOUT_MILLIS);

	if (LIKELY(ret == sizeof(data)))
		return UVC_SUCCESS;
	else
		return ret;
}

uvc_error_t uvc_get_saturation(uvc_device_handle_t *devh, uint16_t *saturation,
		enum uvc_req_code req_code) {
	uint8_t data[2];
	uvc_error_t ret;

	ret = libusb_control_transfer(devh->usb_devh, REQ_TYPE_GET, req_code,
			UVC_PU_SATURATION_CONTROL << 8,
			devh->info->ctrl_if.processing_unit_descs->request,
			data, sizeof(data), CTRL_TIMEOUT_MILLIS);

	if (LIKELY(ret == sizeof(data))) {
		*saturation = SW_TO_SHORT(data);
		return UVC_SUCCESS;
	} else {
		return ret;
	}
	RETURN(-1, uvc_error_t);
}

uvc_error_t uvc_set_saturation(uvc_device_handle_t *devh, uint16_t saturation) {
	uint8_t data[2];
	uvc_error_t ret;

	SHORT_TO_SW(saturation, data);

	ret = libusb_control_transfer(devh->usb_devh, REQ_TYPE_SET, UVC_SET_CUR,
			UVC_PU_SATURATION_CONTROL << 8,
			devh->info->ctrl_if.processing_unit_descs->request,
			data, sizeof(data), CTRL_TIMEOUT_MILLIS);

	if (LIKELY(ret == sizeof(data)))
		return UVC_SUCCESS;
	else
		return ret;
}

uvc_error_t uvc_get_sharpness(uvc_device_handle_t *devh, uint16_t *sharpness,
		enum uvc_req_code req_code) {
	uint8_t data[2];
	uvc_error_t ret;

	ret = libusb_control_transfer(devh->usb_devh, REQ_TYPE_GET, req_code,
			UVC_PU_SHARPNESS_CONTROL << 8,
			devh->info->ctrl_if.processing_unit_descs->request,
			data, sizeof(data), CTRL_TIMEOUT_MILLIS);

	if (LIKELY(ret == sizeof(data))) {
		*sharpness = SW_TO_SHORT(data);
		return UVC_SUCCESS;
	} else {
		return ret;
	}
	RETURN(-1, uvc_error_t);
}

uvc_error_t uvc_set_sharpness(uvc_device_handle_t *devh, uint16_t sharpness) {
	uint8_t data[2];
	uvc_error_t ret;

	SHORT_TO_SW(sharpness, data);

	ret = libusb_control_transfer(devh->usb_devh, REQ_TYPE_SET, UVC_SET_CUR,
			UVC_PU_SHARPNESS_CONTROL << 8,
			devh->info->ctrl_if.processing_unit_descs->request,
			data, sizeof(data), CTRL_TIMEOUT_MILLIS);

	if (LIKELY(ret == sizeof(data)))
		return UVC_SUCCESS;
	else
		return ret;
}

uvc_error_t uvc_get_gamma(uvc_device_handle_t *devh, uint16_t *gamma,
		enum uvc_req_code req_code) {
	uint8_t data[2];
	uvc_error_t ret;

	ret = libusb_control_transfer(devh->usb_devh, REQ_TYPE_GET, req_code,
			UVC_PU_GAMMA_CONTROL << 8,
			devh->info->ctrl_if.processing_unit_descs->request,
			data, sizeof(data), CTRL_TIMEOUT_MILLIS);

	if (LIKELY(ret == sizeof(data))) {
		*gamma = SW_TO_SHORT(data);
		return UVC_SUCCESS;
	} else {
		return ret;
	}
	RETURN(-1, uvc_error_t);
}

uvc_error_t uvc_set_gamma(uvc_device_handle_t *devh, uint16_t gamma) {
	uint8_t data[2];
	uvc_error_t ret;

	SHORT_TO_SW(gamma, data);

	ret = libusb_control_transfer(devh->usb_devh, REQ_TYPE_SET, UVC_SET_CUR,
			UVC_PU_GAMMA_CONTROL << 8,
			devh->info->ctrl_if.processing_unit_descs->request,
			data, sizeof(data), CTRL_TIMEOUT_MILLIS);

	if (LIKELY(ret == sizeof(data)))
		return UVC_SUCCESS;
	else
		return ret;
}

uvc_error_t uvc_get_white_balance_temperature(uvc_device_handle_t *devh, uint16_t *wb_temperature,
		enum uvc_req_code req_code) {
	uint8_t data[2];
	uvc_error_t ret;

	ret = libusb_control_transfer(devh->usb_devh, REQ_TYPE_GET, req_code,
			UVC_PU_WHITE_BALANCE_TEMPERATURE_CONTROL << 8,
			devh->info->ctrl_if.processing_unit_descs->request,
			data, sizeof(data), CTRL_TIMEOUT_MILLIS);

	if (LIKELY(ret == sizeof(data))) {
		*wb_temperature = SW_TO_SHORT(data);
		return UVC_SUCCESS;
	} else {
		return ret;
	}
	RETURN(-1, uvc_error_t);
}

uvc_error_t uvc_set_white_balance_temperature(uvc_device_handle_t *devh, uint16_t wb_temperature) {
	uint8_t data[2];
	uvc_error_t ret;

	SHORT_TO_SW(wb_temperature, data);

	ret = libusb_control_transfer(devh->usb_devh, REQ_TYPE_SET, UVC_SET_CUR,
			UVC_PU_WHITE_BALANCE_TEMPERATURE_CONTROL << 8,
			devh->info->ctrl_if.processing_unit_descs->request,
			data, sizeof(data), CTRL_TIMEOUT_MILLIS);

	if (LIKELY(ret == sizeof(data)))
		return UVC_SUCCESS;
	else
		return ret;
}

uvc_error_t uvc_get_white_balance_temperature_auto(uvc_device_handle_t *devh, uint8_t *autoWbTemp,
		enum uvc_req_code req_code) {
	uint8_t data[1];
	uvc_error_t ret;

	ret = libusb_control_transfer(devh->usb_devh, REQ_TYPE_GET, req_code,
			UVC_PU_WHITE_BALANCE_TEMPERATURE_AUTO_CONTROL << 8,
			devh->info->ctrl_if.processing_unit_descs->request,
			data, sizeof(data), CTRL_TIMEOUT_MILLIS);

	if (LIKELY(ret == sizeof(data))) {
		*autoWbTemp = data[0];
		return UVC_SUCCESS;
	} else {
		return ret;
	}
	RETURN(-1, uvc_error_t);
}

uvc_error_t uvc_set_white_balance_temperature_auto(uvc_device_handle_t *devh, uint8_t autoWbTemp) {
	uint8_t data[1];
	uvc_error_t ret;

	data[0] = autoWbTemp ? 1 : 0;

	ret = libusb_control_transfer(devh->usb_devh, REQ_TYPE_SET, UVC_SET_CUR,
			UVC_PU_WHITE_BALANCE_TEMPERATURE_AUTO_CONTROL << 8,
			devh->info->ctrl_if.processing_unit_descs->request,
			data, sizeof(data), CTRL_TIMEOUT_MILLIS);

	if (LIKELY(ret == sizeof(data)))
		return UVC_SUCCESS;
	else
		return ret;
}

uvc_error_t uvc_get_white_balance_component(uvc_device_handle_t *devh, uint32_t *wb_compo,
		enum uvc_req_code req_code) {
	uint8_t data[4];
	uvc_error_t ret;

	ret = libusb_control_transfer(devh->usb_devh, REQ_TYPE_GET, req_code,
			UVC_PU_WHITE_BALANCE_COMPONENT_CONTROL << 8,
			devh->info->ctrl_if.processing_unit_descs->request,
			data, sizeof(data), CTRL_TIMEOUT_MILLIS);

	if (LIKELY(ret == sizeof(data))) {
		*wb_compo = DW_TO_INT(data);
		return UVC_SUCCESS;
	} else {
		return ret;
	}
	RETURN(-1, uvc_error_t);
}

uvc_error_t uvc_set_white_balance_component(uvc_device_handle_t *devh, uint32_t wb_compo) {
	uint8_t data[4];
	uvc_error_t ret;

	INT_TO_DW(wb_compo, data);

	ret = libusb_control_transfer(devh->usb_devh, REQ_TYPE_SET, UVC_SET_CUR,
			UVC_PU_WHITE_BALANCE_COMPONENT_CONTROL << 8,
			devh->info->ctrl_if.processing_unit_descs->request,
			data, sizeof(data), CTRL_TIMEOUT_MILLIS);

	if (LIKELY(ret == sizeof(data)))
		return UVC_SUCCESS;
	else
		return ret;
}

uvc_error_t uvc_get_white_balance_component_auto(uvc_device_handle_t *devh, uint8_t *autoWbCompo,
		enum uvc_req_code req_code) {
	uint8_t data[1];
	uvc_error_t ret;

	ret = libusb_control_transfer(devh->usb_devh, REQ_TYPE_GET, req_code,
			UVC_PU_WHITE_BALANCE_COMPONENT_AUTO_CONTROL << 8,
			devh->info->ctrl_if.processing_unit_descs->request,
			data, sizeof(data), CTRL_TIMEOUT_MILLIS);

	if (LIKELY(ret == sizeof(data))) {
		*autoWbCompo = data[0];
		return UVC_SUCCESS;
	} else {
		return ret;
	}
	RETURN(-1, uvc_error_t);
}

uvc_error_t uvc_set_white_balance_component_auto(uvc_device_handle_t *devh, uint8_t autoWbCompo) {
	uint8_t data[1];
	uvc_error_t ret;

	data[0] = autoWbCompo ? 1 : 0;

	ret = libusb_control_transfer(devh->usb_devh, REQ_TYPE_SET, UVC_SET_CUR,
			UVC_PU_WHITE_BALANCE_COMPONENT_AUTO_CONTROL << 8,
			devh->info->ctrl_if.processing_unit_descs->request,
			data, sizeof(data), CTRL_TIMEOUT_MILLIS);

	if (LIKELY(ret == sizeof(data)))
		return UVC_SUCCESS;
	else
		return ret;
}

uvc_error_t uvc_get_digital_multiplier(uvc_device_handle_t *devh, uint16_t *multiplier,
		enum uvc_req_code req_code) {
	uint8_t data[2];
	uvc_error_t ret;

	ret = libusb_control_transfer(devh->usb_devh, REQ_TYPE_GET, req_code,
			UVC_PU_DIGITAL_MULTIPLIER_CONTROL << 8,
			devh->info->ctrl_if.processing_unit_descs->request,
			data, sizeof(data), CTRL_TIMEOUT_MILLIS);

	if (LIKELY(ret == sizeof(data))) {
		*multiplier = SW_TO_SHORT(data);
		return UVC_SUCCESS;
	} else {
		return ret;
	}
	RETURN(-1, uvc_error_t);
}

uvc_error_t uvc_set_digital_multiplier(uvc_device_handle_t *devh, uint16_t multiplier) {
	uint8_t data[2];
	uvc_error_t ret;

	SHORT_TO_SW(multiplier, data);

	ret = libusb_control_transfer(devh->usb_devh, REQ_TYPE_SET, UVC_SET_CUR,
			UVC_PU_DIGITAL_MULTIPLIER_CONTROL << 8,
			devh->info->ctrl_if.processing_unit_descs->request,
			data, sizeof(data), CTRL_TIMEOUT_MILLIS);

	if (LIKELY(ret == sizeof(data)))
		return UVC_SUCCESS;
	else
		return ret;
}

uvc_error_t uvc_get_digital_multiplier_limit(uvc_device_handle_t *devh, uint16_t *limit,
		enum uvc_req_code req_code) {
	uint8_t data[2];
	uvc_error_t ret;

	ret = libusb_control_transfer(devh->usb_devh, REQ_TYPE_GET, req_code,
			UVC_PU_DIGITAL_MULTIPLIER_LIMIT_CONTROL << 8,
			devh->info->ctrl_if.processing_unit_descs->request,
			data, sizeof(data), CTRL_TIMEOUT_MILLIS);

	if (LIKELY(ret == sizeof(data))) {
		*limit = SW_TO_SHORT(data);
		return UVC_SUCCESS;
	} else {
		return ret;
	}
	RETURN(-1, uvc_error_t);
}

uvc_error_t uvc_set_digital_multiplier_limit(uvc_device_handle_t *devh, uint16_t limit) {
	uint8_t data[2];
	uvc_error_t ret;

	SHORT_TO_SW(limit, data);

	ret = libusb_control_transfer(devh->usb_devh, REQ_TYPE_SET, UVC_SET_CUR,
			UVC_PU_DIGITAL_MULTIPLIER_LIMIT_CONTROL << 8,
			devh->info->ctrl_if.processing_unit_descs->request,
			data, sizeof(data), CTRL_TIMEOUT_MILLIS);

	if (LIKELY(ret == sizeof(data)))
		return UVC_SUCCESS;
	else
		return ret;
}

uvc_error_t uvc_get_analog_video_standard(uvc_device_handle_t *devh, uint8_t *standard,
		enum uvc_req_code req_code) {
	uint8_t data[1];
	uvc_error_t ret;

	ret = libusb_control_transfer(devh->usb_devh, REQ_TYPE_GET, req_code,
			UVC_PU_ANALOG_VIDEO_STANDARD_CONTROL << 8,
			devh->info->ctrl_if.processing_unit_descs->request,
			data, sizeof(data), CTRL_TIMEOUT_MILLIS);

	if (LIKELY(ret == sizeof(data))) {
		*standard = data[0];
		return UVC_SUCCESS;
	} else {
		return ret;
	}
	RETURN(-1, uvc_error_t);
}

uvc_error_t uvc_set_analog_video_standard(uvc_device_handle_t *devh, uint8_t standard) {
	uint8_t data[1];
	uvc_error_t ret;

	data[0] = standard;

	ret = libusb_control_transfer(devh->usb_devh, REQ_TYPE_SET, UVC_SET_CUR,
			UVC_PU_ANALOG_VIDEO_STANDARD_CONTROL << 8,
			devh->info->ctrl_if.processing_unit_descs->request,
			data, sizeof(data), CTRL_TIMEOUT_MILLIS);

	if (LIKELY(ret == sizeof(data)))
		return UVC_SUCCESS;
	else
		return ret;
}

uvc_error_t uvc_get_analog_video_lockstate(uvc_device_handle_t *devh, uint8_t *lock_state,
		enum uvc_req_code req_code) {
	uint8_t data[1];
	uvc_error_t ret;

	ret = libusb_control_transfer(devh->usb_devh, REQ_TYPE_GET, req_code,
			UVC_PU_ANALOG_LOCK_STATUS_CONTROL << 8,
			devh->info->ctrl_if.processing_unit_descs->request,
			data, sizeof(data), CTRL_TIMEOUT_MILLIS);

	if (LIKELY(ret == sizeof(data))) {
		*lock_state = data[0];
		return UVC_SUCCESS;
	} else {
		return ret;
	}
	RETURN(-1, uvc_error_t);
}

uvc_error_t uvc_set_analog_video_lockstate(uvc_device_handle_t *devh, uint8_t lock_state) {
	uint8_t data[1];
	uvc_error_t ret;

	data[0] = lock_state;

	ret = libusb_control_transfer(devh->usb_devh, REQ_TYPE_SET, UVC_SET_CUR,
			UVC_PU_ANALOG_LOCK_STATUS_CONTROL << 8,
			devh->info->ctrl_if.processing_unit_descs->request,
			data, sizeof(data), CTRL_TIMEOUT_MILLIS);

	if (LIKELY(ret == sizeof(data)))
		return UVC_SUCCESS;
	else
		return ret;
}
