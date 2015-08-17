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
 * @defgroup ctrl Video capture and processing control
 */

#include "libuvc/libuvc.h"
#include "libuvc/libuvc_internal.h"

static const int REQ_TYPE_SET = 0x21;
static const int REQ_TYPE_GET = 0xa1;

uvc_error_t uvc_get_power_mode(uvc_device_handle_t *devh, enum uvc_device_power_mode *mode, enum uvc_req_code req_code) {
  uint8_t mode_char;
  uvc_error_t ret;

  ret = libusb_control_transfer(
    devh->usb_devh,
    REQ_TYPE_GET, req_code,
    UVC_VC_VIDEO_POWER_MODE_CONTROL << 8,
    0,
    &mode_char,
    sizeof(mode_char),
    0);

  if (ret == 1) {
    *mode = mode_char;
    return UVC_SUCCESS;
  } else {
    return ret;
  }
}

uvc_error_t uvc_set_power_mode(uvc_device_handle_t *devh, enum uvc_device_power_mode mode) {
  uint8_t mode_char = mode;
  uvc_error_t ret;

  ret = libusb_control_transfer(
    devh->usb_devh,
    REQ_TYPE_SET, UVC_SET_CUR,
    UVC_VC_VIDEO_POWER_MODE_CONTROL << 8,
    0,
    &mode_char,
    sizeof(mode_char),
    0);

  if (ret == 1)
    return UVC_SUCCESS;
  else
    return ret;
}

/***** CAMERA TERMINAL CONTROLS *****/

uvc_error_t uvc_get_ae_mode(uvc_device_handle_t *devh, int *mode, enum uvc_req_code req_code) {
  uint8_t data[1];
  uvc_error_t ret;

  ret = libusb_control_transfer(
    devh->usb_devh,
    REQ_TYPE_GET, req_code,
    UVC_CT_AE_MODE_CONTROL << 8,
    1 << 8,
    data,
    sizeof(data),
    0);

  if (ret == sizeof(data)) {
    *mode = data[0];
    return UVC_SUCCESS;
  } else {
    return ret;
  }
}

uvc_error_t uvc_set_ae_mode(uvc_device_handle_t *devh, int mode) {
  uint8_t data[1];
  uvc_error_t ret;

  data[0] = mode;

  ret = libusb_control_transfer(
    devh->usb_devh,
    REQ_TYPE_SET, UVC_SET_CUR,
    UVC_CT_AE_MODE_CONTROL << 8,
    1 << 8,
    data,
    sizeof(data),
    0);

  if (ret == sizeof(data))
    return UVC_SUCCESS;
  else
    return ret;
}

uvc_error_t uvc_get_ae_priority(uvc_device_handle_t *devh, uint8_t *priority, enum uvc_req_code req_code) {
  uint8_t data[1];
  uvc_error_t ret;

  ret = libusb_control_transfer(
    devh->usb_devh,
    REQ_TYPE_GET, req_code,
    UVC_CT_AE_PRIORITY_CONTROL << 8,
    1 << 8,
    data,
    sizeof(data),
    0);

  if (ret == sizeof(data)) {
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

  ret = libusb_control_transfer(
    devh->usb_devh,
    REQ_TYPE_SET, UVC_SET_CUR,
    UVC_CT_AE_PRIORITY_CONTROL << 8,
    1 << 8,
    data,
    sizeof(data),
    0);

  if (ret == sizeof(data))
    return UVC_SUCCESS;
  else
    return ret;
}

uvc_error_t uvc_get_exposure_abs(uvc_device_handle_t *devh, int *time, enum uvc_req_code req_code) {
  uint8_t data[4];
  uvc_error_t ret;

  ret = libusb_control_transfer(
    devh->usb_devh,
    REQ_TYPE_GET, req_code,
    UVC_CT_EXPOSURE_TIME_ABSOLUTE_CONTROL << 8,
    1 << 8,
    data,
    sizeof(data),
    0);

  if (ret == sizeof(data)) {
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

  ret = libusb_control_transfer(
    devh->usb_devh,
    REQ_TYPE_SET, UVC_SET_CUR,
    UVC_CT_EXPOSURE_TIME_ABSOLUTE_CONTROL << 8,
    1 << 8,
    data,
    sizeof(data),
    0);

  if (ret == sizeof(data))
    return UVC_SUCCESS;
  else
    return ret;
}

uvc_error_t uvc_get_exposure_rel(uvc_device_handle_t *devh, int *step, enum uvc_req_code req_code) {
  uint8_t data[1];
  uvc_error_t ret;

  ret = libusb_control_transfer(
    devh->usb_devh,
    REQ_TYPE_GET, req_code,
    UVC_CT_EXPOSURE_TIME_RELATIVE_CONTROL << 8,
    1 << 8,
    data,
    sizeof(data),
    0);

  if (ret == sizeof(data)) {
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

  ret = libusb_control_transfer(
    devh->usb_devh,
    REQ_TYPE_SET, UVC_SET_CUR,
    UVC_CT_EXPOSURE_TIME_RELATIVE_CONTROL << 8,
    1 << 8,
    data,
    sizeof(data),
    0);

  if (ret == sizeof(data))
    return UVC_SUCCESS;
  else
    return ret;
}

uvc_error_t uvc_get_scanning_mode(uvc_device_handle_t *devh, int *step, enum uvc_req_code req_code) {
  uint8_t data[1];
  uvc_error_t ret;

  ret = libusb_control_transfer(
    devh->usb_devh,
    REQ_TYPE_GET, req_code,
    UVC_CT_SCANNING_MODE_CONTROL << 8,
    1 << 8,
    data,
    sizeof(data),
    0);

  if (ret == sizeof(data)) {
    *step = data[0];
    return UVC_SUCCESS;
  } else {
    return ret;
  }
}

uvc_error_t uvc_set_scanning_mode(uvc_device_handle_t *devh, int mode) {
  uint8_t data[1];
  uvc_error_t ret;

  data[0] = mode;

  ret = libusb_control_transfer(
    devh->usb_devh,
    REQ_TYPE_SET, UVC_SET_CUR,
    UVC_CT_SCANNING_MODE_CONTROL << 8,
    1 << 8,
    data,
    sizeof(data),
    0);

  if (ret == sizeof(data))
    return UVC_SUCCESS;
  else
    return ret;
}

uvc_error_t uvc_get_focus_abs(uvc_device_handle_t *devh, short *focus, enum uvc_req_code req_code) {
  uint8_t data[2];
  uvc_error_t ret;

  ret = libusb_control_transfer(
    devh->usb_devh,
    REQ_TYPE_GET, req_code,
    UVC_CT_FOCUS_ABSOLUTE_CONTROL << 8,
    1 << 8,
    data,
    sizeof(data),
    0);

  if (ret == sizeof(data)) {
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

  ret = libusb_control_transfer(
    devh->usb_devh,
    REQ_TYPE_SET, UVC_SET_CUR,
    UVC_CT_FOCUS_ABSOLUTE_CONTROL << 8,
    1 << 8,
    data,
    sizeof(data),
    0);

  if (ret == sizeof(data))
    return UVC_SUCCESS;
  else
    return ret;
}

/** @todo focus_rel, focus_auto_control */
/** @todo iris_abs_ctrl, iris_rel_ctrl */
/** @todo zoom_abs, zoom_rel */

uvc_error_t uvc_get_pantilt_abs(uvc_device_handle_t *devh, int *pan, int *tilt, enum uvc_req_code req_code) {
  uint8_t data[8];
  uvc_error_t ret;

  ret = libusb_control_transfer(
    devh->usb_devh,
    REQ_TYPE_GET, req_code,
    UVC_CT_PANTILT_ABSOLUTE_CONTROL << 8,
    1 << 8,
    data,
    sizeof(data),
    0);

  if (ret == sizeof(data)) {
    *pan = DW_TO_INT(data);
    *tilt = DW_TO_INT(data + 4);
    return UVC_SUCCESS;
  } else {
    return ret;
  }
}

uvc_error_t uvc_set_pantilt_abs(uvc_device_handle_t *devh, int pan, int tilt) {
  uint8_t data[8];
  uvc_error_t ret;

  INT_TO_DW(pan, data);
  INT_TO_DW(tilt, data + 4);

  ret = libusb_control_transfer(
    devh->usb_devh,
    REQ_TYPE_SET, UVC_SET_CUR,
    UVC_CT_PANTILT_ABSOLUTE_CONTROL << 8,
    1 << 8,
    data,
    sizeof(data),
    0);

  if (ret == sizeof(data))
    return UVC_SUCCESS;
  else
    return ret;
}

/** @todo pantilt_rel */

/** @todo roll_abs, roll_rel */

/** @todo privacy */

/***** SELECTOR UNIT CONTROLS *****/

/** @todo input_select */

/***** PROCESSING UNIT CONTROLS *****/

/***** GENERIC CONTROLS *****/
/**
 * @brief Get the length of a control on a terminal or unit.
 * 
 * @param devh UVC device handle
 * @param unit Unit or Terminal ID; obtain this from the uvc_extension_unit_t describing the extension unit
 * @param ctrl Vendor-specific control number to query
 * @return On success, the length of the control as reported by the device. Otherwise,
 *   a uvc_error_t error describing the error encountered.
 */
int uvc_get_ctrl_len(uvc_device_handle_t *devh, uint8_t unit, uint8_t ctrl) {
  unsigned char buf[2];

  int ret = libusb_control_transfer(
    devh->usb_devh,
    REQ_TYPE_GET, UVC_GET_LEN,
    ctrl << 8,
    unit << 8,
    buf,
    2,
    0 /* timeout */);

  if (ret < 0)
    return ret;
  else
    return (unsigned short)SW_TO_SHORT(buf);
}

/**
 * @brief Perform a GET_* request from an extension unit.
 * 
 * @param devh UVC device handle
 * @param unit Unit ID; obtain this from the uvc_extension_unit_t describing the extension unit
 * @param ctrl Control number to query
 * @param data Data buffer to be filled by the device
 * @param len Size of data buffer
 * @param req_code GET_* request to execute
 * @return On success, the number of bytes actually transferred. Otherwise,
 *   a uvc_error_t error describing the error encountered.
 */
int uvc_get_ctrl(uvc_device_handle_t *devh, uint8_t unit, uint8_t ctrl, void *data, int len, enum uvc_req_code req_code) {
  return libusb_control_transfer(
    devh->usb_devh,
    REQ_TYPE_GET, req_code,
    ctrl << 8,
    unit << 8,
    data,
    len,
    0 /* timeout */);
}

/**
 * @brief Perform a SET_CUR request to a terminal or unit.
 * 
 * @param devh UVC device handle
 * @param unit Unit or Terminal ID
 * @param ctrl Control number to set
 * @param data Data buffer to be sent to the device
 * @param len Size of data buffer
 * @return On success, the number of bytes actually transferred. Otherwise,
 *   a uvc_error_t error describing the error encountered.
 */
int uvc_set_ctrl(uvc_device_handle_t *devh, uint8_t unit, uint8_t ctrl, void *data, int len) {
  return libusb_control_transfer(
    devh->usb_devh,
    REQ_TYPE_SET, UVC_SET_CUR,
    ctrl << 8,
    unit << 8,
    data,
    len,
    0 /* timeout */);
}
