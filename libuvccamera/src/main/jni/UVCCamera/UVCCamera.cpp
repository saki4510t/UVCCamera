/*
 * UVCCamera
 * library and sample to access to UVC web camera on non-rooted Android device
 *
 * Copyright (c) 2014-2015 saki t_saki@serenegiant.com
 *
 * File name: UVCCamera.cpp
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 * All files in the folder are under this Apache License, Version 2.0.
 * Files in the jni/libjpeg, jni/libusb, jin/libuvc, jni/rapidjson folder may have a different license, see the respective files.
*/

//**********************************************************************
//
//**********************************************************************
#include <stdlib.h>
#include <linux/time.h>
#include <unistd.h>
#include <string.h>
#include "UVCCamera.h"
#include "Parameters.h"
#include "libuvc_internal.h"

#define	LOCAL_DEBUG 0

//**********************************************************************
//
//**********************************************************************
UVCCamera::UVCCamera()
:	mFd(0),
	mUsbFs(NULL),
	mContext(NULL),
	mDevice(NULL),
	mDeviceHandle(NULL),
	mPreview(NULL),
	mCtrlSupports(0),
	mPUSupports(0) {

	ENTER();
	clearCameraParams();
	EXIT();
}

UVCCamera::~UVCCamera() {
	ENTER();
	release();
	if (mContext) {
		uvc_exit(mContext);
		mContext = NULL;
	}
	if (mUsbFs) {
		free(mUsbFs);
		mUsbFs = NULL;
	}
	EXIT();
}

void UVCCamera::clearCameraParams() {
	mCtrlSupports = mPUSupports = 0;
	mBrightness.min = mBrightness.max = mBrightness.def = 0;
	mContrast.min = mContrast.max = mContrast.def = 0;
	mSharpness.min = mSharpness.max = mSharpness.def = 0;
	mGain.min = mGain.max = mGain.def = 0;
	mGamma.min = mGamma.max = mGamma.def = 0;
	mSaturation.min = mSaturation.max = mSaturation.def = 0;
	mHue.min = mHue.max = mHue.def = 0;
	mZoom.min = mZoom.max = mZoom.def = 0;
	mWhiteBlance.min = mWhiteBlance.max = mWhiteBlance.def = 0;
	mFocus.min = mFocus.max = mFocus.def = 0;
}

int UVCCamera::connect(int vid, int pid, int fd, const char *usbfs) {
	ENTER();
	uvc_error_t result = UVC_ERROR_BUSY;
	if (!mDeviceHandle && fd) {
		if (mUsbFs)
			free(mUsbFs);
		mUsbFs = strdup(usbfs);
		if (UNLIKELY(!mContext)) {
			result = uvc_init2(&mContext, NULL, mUsbFs);
			if (UNLIKELY(result < 0)) {
				LOGD("failed to init libuvc");
				RETURN(result, int);
			}
		}
		clearCameraParams();
		fd = dup(fd);
		result = uvc_find_device2(mContext, &mDevice, vid, pid, NULL, fd);
		if (LIKELY(!result)) {
			result = uvc_open(mDevice, &mDeviceHandle);
			if (LIKELY(!result)) {
#if LOCAL_DEBUG
				uvc_print_diag(mDeviceHandle, stderr);
#endif
				mFd = fd;
				mPreview = new UVCPreview(mDeviceHandle);
			} else {
				LOGE("could not open camera:err=%d", result);
				uvc_unref_device(mDevice);
				mDevice = NULL;		//	SAFE_DELETE(mDevice);
				mDeviceHandle = NULL;
				close(fd);
			}
		} else {
			LOGE("could not find camera:err=%d", result);
			close(fd);
		}
	} else {
		LOGW("camera is already opened. you should release first");
	}
	RETURN(result, int);
}

int UVCCamera::release() {
	ENTER();
	stopPreview();
	if (LIKELY(mDeviceHandle)) {
		SAFE_DELETE(mPreview);
		uvc_close(mDeviceHandle);
		mDeviceHandle = NULL;
	}
	if (LIKELY(mDevice)) {
		uvc_unref_device(mDevice);
		mDevice = NULL;
	}
	clearCameraParams();
	mCtrlSupports = mPUSupports = 0;
	if (LIKELY(mFd)) {
		close(mFd);
		mFd = 0;
	}
	RETURN(0, int);
}

char *UVCCamera::getSupportedSize() {
	ENTER();
	if (mDeviceHandle) {
		UVCDiags params;
		RETURN(params.getSupportedSize(mDeviceHandle), char *)
	}
	RETURN(NULL, char *);
}

int UVCCamera::setPreviewSize(int width, int height, int mode, float bandwidth) {
	ENTER();
	int result = EXIT_FAILURE;
	if (mPreview) {
		result = mPreview->setPreviewSize(width, height, mode, bandwidth);
	}
	RETURN(result, int);
}

int UVCCamera::setPreviewDisplay(ANativeWindow *preview_window) {
	ENTER();
	int result = EXIT_FAILURE;
	if (mPreview) {
		result = mPreview->setPreviewDisplay(preview_window);
	}
	RETURN(result, int);
}

int UVCCamera::setFrameCallback(JNIEnv *env, jobject frame_callback_obj, int pixel_format) {
	ENTER();
	int result = EXIT_FAILURE;
	if (mPreview) {
		result = mPreview->setFrameCallback(env, frame_callback_obj, pixel_format);
	}
	RETURN(result, int);
}

int UVCCamera::startPreview() {
	ENTER();

	int result = EXIT_FAILURE;
	if (mDeviceHandle) {
		return mPreview->startPreview();
	}
	RETURN(result, int);
}

int UVCCamera::stopPreview() {
	ENTER();
	if (LIKELY(mPreview)) {
		mPreview->stopPreview();
	}
	RETURN(0, int);
}

int UVCCamera::setCaptureDisplay(ANativeWindow *capture_window) {
	ENTER();
	int result = EXIT_FAILURE;
	if (mPreview) {
		result = mPreview->setCaptureDisplay(capture_window);
	}
	RETURN(result, int);
}

//======================================================================
// カメラのサポートしているコントロール機能を取得する
int UVCCamera::getCtrlSupports(uint64_t *supports) {
	ENTER();
	uvc_error_t ret = UVC_ERROR_NOT_FOUND;
	if (LIKELY(mDeviceHandle)) {
		if (!mCtrlSupports) {
			// 何個あるのかわからへんねんけど、試した感じは１個みたいやからとりあえず先頭のを返す
			const uvc_input_terminal_t *input_terminals = uvc_get_input_terminals(mDeviceHandle);
			const uvc_input_terminal_t *it;
			DL_FOREACH(input_terminals, it)
			{
				if (it) {
					mCtrlSupports = it->bmControls;
					MARK("getCtrlSupports=%lx", (unsigned long)mCtrlSupports);
					ret = UVC_SUCCESS;
					break;
				}
			}
		} else
			ret = UVC_SUCCESS;
	}
	if (supports)
		*supports = mCtrlSupports;
	RETURN(ret, int);
}

int UVCCamera::getProcSupports(uint64_t *supports) {
	ENTER();
	uvc_error_t ret = UVC_ERROR_NOT_FOUND;
	if (LIKELY(mDeviceHandle)) {
		if (!mPUSupports) {
			// 何個あるのかわからへんねんけど、試した感じは１個みたいやからとりあえず先頭のを返す
			const uvc_processing_unit_t *proc_units = uvc_get_processing_units(mDeviceHandle);
			const uvc_processing_unit_t *pu;
			DL_FOREACH(proc_units, pu)
			{
				if (pu) {
					mPUSupports = pu->bmControls;
					MARK("getProcSupports=%lx", (unsigned long)mPUSupports);
					ret = UVC_SUCCESS;
					break;
				}
			}
		} else
			ret = UVC_SUCCESS;
	}
	if (supports)
		*supports = mPUSupports;
	RETURN(ret, int);
}

// 露出をセット
int UVCCamera::setExposureMode(int mode) {
	ENTER();
	int r = UVC_ERROR_ACCESS;
	if LIKELY((mDeviceHandle) && (mCtrlSupports & CTRL_AE)) {
//		LOGI("ae:%d", mode);
		r = uvc_set_ae_mode(mDeviceHandle, mode/* & 0xff*/);
	}
	RETURN(r, int);
}

// 露出設定を取得
int UVCCamera::getExposureMode() {

	ENTER();
	int r = UVC_ERROR_ACCESS;
	if LIKELY((mDeviceHandle) && (mCtrlSupports & CTRL_AE)) {
		int mode;
		r = uvc_get_ae_mode(mDeviceHandle, &mode, UVC_GET_CUR);
//		LOGI("ae:%d", mode);
		if (LIKELY(!r)) {
			r = mode;
		}
	}
	RETURN(r, int);
}

//======================================================================
// オートフォーカスをon/off
int UVCCamera::setAutoFocus(bool autoFocus) {
	ENTER();

	int r = UVC_ERROR_ACCESS;
	if LIKELY((mDeviceHandle) && (mCtrlSupports & CTRL_FOCUS_AUTO)) {
		r = uvc_set_focus_auto(mDeviceHandle, autoFocus);
	}
	RETURN(r, int);
}

// オートフォーカスのon/off状態を取得
bool UVCCamera::getAutoFocus() {
	ENTER();
	int r = UVC_ERROR_ACCESS;
	if LIKELY((mDeviceHandle) && (mCtrlSupports & CTRL_FOCUS_AUTO)) {
		uint8_t autoFocus;
		r = uvc_get_focus_auto(mDeviceHandle, &autoFocus, UVC_GET_CUR);
		if (LIKELY(!r))
			r = autoFocus;
	}
	RETURN(r, int);
}

//======================================================================
// オートホワイトバランスをon/off
int UVCCamera::setAutoWhiteBlance(bool autoWhiteBlance) {
	ENTER();
	int r = UVC_ERROR_ACCESS;
	if LIKELY((mDeviceHandle) && (mPUSupports & PU_WB_TEMP_AUTO)) {
		r = uvc_set_white_balance_temperature_auto(mDeviceHandle, autoWhiteBlance);
	}
	RETURN(r, int);
}

// オートホワイトバランスのon/off状態を取得
bool UVCCamera::getAutoWhiteBlance() {
	ENTER();
	int r = UVC_ERROR_ACCESS;
	if LIKELY((mDeviceHandle) && (mPUSupports & PU_WB_TEMP_AUTO)) {
		uint8_t autoWhiteBlance;
		r = uvc_get_white_balance_temperature_auto(mDeviceHandle, &autoWhiteBlance, UVC_GET_CUR);
		if (LIKELY(!r))
			r = autoWhiteBlance;
	}
	RETURN(r, int);
}

//======================================================================
#define CTRL_BRIGHTNESS		0
#define CTRL_CONTRAST		1
#define	CTRL_SHARPNESS		2
#define CTRL_GAIN			3
#define CTRL_WHITEBLANCE	4
#define CTRL_FOCUS			5

static uvc_error_t update_ctrl_values(uvc_device_handle_t *devh, control_value_t &values,
	paramget_func_short get_func) {

	ENTER();

	uvc_error_t ret = UVC_SUCCESS;
	if (!values.min && !values.max) {
		short value;
		ret = get_func(devh, &value, UVC_GET_MIN);
		if (LIKELY(!ret)) {
			values.min = value;
			LOGV("update_params:min value=%d,min=%d", value, values.min);
			ret = get_func(devh, &value, UVC_GET_MAX);
			if (LIKELY(!ret)) {
				values.max = value;
				LOGV("update_params:max value=%d,max=%d", value, values.max);
				ret = get_func(devh, &value, UVC_GET_DEF);
				if (LIKELY(!ret)) {
					values.def = value;
					LOGV("update_params:def value=%d,def=%d", value, values.def);
				}
			}
		}
	}
	if (UNLIKELY(ret)) {
		LOGD("update_params failed:err=%d", ret);
	}
	RETURN(ret, uvc_error_t);
}

static uvc_error_t update_ctrl_values(uvc_device_handle_t *devh, control_value_t &values,
	paramget_func_ushort get_func) {

	ENTER();

	uvc_error_t ret = UVC_SUCCESS;
	if (!values.min && !values.max) {
		uint16_t value;
		ret = get_func(devh, &value, UVC_GET_MIN);
		if (LIKELY(!ret)) {
			values.min = value;
			LOGV("update_params:min value=%d,min=%d", value, values.min);
			ret = get_func(devh, &value, UVC_GET_MAX);
			if (LIKELY(!ret)) {
				values.max = value;
				LOGV("update_params:max value=%d,max=%d", value, values.max);
				ret = get_func(devh, &value, UVC_GET_DEF);
				if (LIKELY(!ret)) {
					values.def = value;
					LOGV("update_params:def value=%d,def=%d", value, values.def);
				}
			}
		}
	}
	if (UNLIKELY(ret)) {
		LOGD("update_params failed:err=%d", ret);
	}
	RETURN(ret, uvc_error_t);
}

#define UPDATE_CTRL_VALUES(VAL,FUNC) \
	ret = update_ctrl_values(mDeviceHandle, VAL, FUNC); \
	if (LIKELY(!ret)) { \
		min = VAL.min; \
		max = VAL.max; \
		def = VAL.def; \
	} else { \
		MARK("failed to UPDATE_CTRL_VALUES"); \
	} \


/**
 * カメラコントロール設定の下請け
 */
int UVCCamera::internalSetCtrlValue(control_value_t &values, short value,
		paramget_func_short get_func, paramset_func_short set_func) {
	int ret = update_ctrl_values(mDeviceHandle, values, get_func);
	if (LIKELY(!ret)) {	// 正常に最小・最大値を取得出来た時
		value = value < values.min
			? values.min
			: (value > values.max ? values.max : value);
		set_func(mDeviceHandle, value);
	}
	RETURN(ret, int);
}

/**
 * カメラコントロール設定の下請け
 */
int UVCCamera::internalSetCtrlValue(control_value_t &values, uint16_t value,
		paramget_func_ushort get_func, paramset_func_ushort set_func) {
	int ret = update_ctrl_values(mDeviceHandle, values, get_func);
	if (LIKELY(!ret)) {	// 正常に最小・最大値を取得出来た時
		value = value < values.min
			? values.min
			: (value > values.max ? values.max : value);
		set_func(mDeviceHandle, value);
	}
	RETURN(ret, int);
}

//======================================================================
int UVCCamera::updateBrightnessLimit(int &min, int &max, int &def) {
	ENTER();
	int ret = UVC_ERROR_IO;
	if (mPUSupports & PU_BRIGHTNESS) {
		UPDATE_CTRL_VALUES(mBrightness, uvc_get_brightness);
	}
	RETURN(ret, int);
}

int UVCCamera::setBrightness(short brightness) {
	ENTER();
	int ret = UVC_ERROR_IO;
	if (mPUSupports & PU_BRIGHTNESS) {
		ret = internalSetCtrlValue(mBrightness, brightness, uvc_get_brightness, uvc_set_brightness);
	}
	RETURN(ret, int);
}

// 明るさの現在値を取得
int UVCCamera::getBrightness() {
	ENTER();
	if (mPUSupports & PU_BRIGHTNESS) {
		int ret = update_ctrl_values(mDeviceHandle, mBrightness, uvc_get_brightness);
		if (LIKELY(!ret)) {	// 正常に最小・最大値を取得出来た時
			short value;
			ret = uvc_get_brightness(mDeviceHandle, &value, UVC_GET_CUR);
			if (LIKELY(!ret))
				return value;
		}
	}
	RETURN(0, int);
}

//======================================================================
// フォーカス調整
int UVCCamera::updateFocusLimit(int &min, int &max, int &def) {
	ENTER();
	int ret = UVC_ERROR_ACCESS;
	if (mCtrlSupports & CTRL_FOCUS_ABS) {
		UPDATE_CTRL_VALUES(mFocus, uvc_get_focus_abs);
	}
	RETURN(ret, int);
}

// フォーカスを設定
int UVCCamera::setFocus(int focus) {
	ENTER();
	int ret = UVC_ERROR_ACCESS;
	if (mCtrlSupports & CTRL_FOCUS_ABS) {
		ret = internalSetCtrlValue(mFocus, focus, uvc_get_focus_abs, uvc_set_focus_abs);
	}
	RETURN(ret, int);
}

// フォーカスの現在値を取得
int UVCCamera::getFocus() {
	ENTER();
	if (mCtrlSupports & CTRL_FOCUS_ABS) {
		int ret = update_ctrl_values(mDeviceHandle, mFocus, uvc_get_focus_abs);
		if (LIKELY(!ret)) {	// 正常に最小・最大値を取得出来た時
			short value;
			ret = uvc_get_focus_abs(mDeviceHandle, &value, UVC_GET_CUR);
			if (LIKELY(!ret))
				return value;
		}
	}
	RETURN(0, int);
}

//======================================================================
// コントラスト調整
int UVCCamera::updateContrastLimit(int &min, int &max, int &def) {
	ENTER();
	int ret = UVC_ERROR_IO;
	if (mPUSupports & PU_CONTRAST) {
		UPDATE_CTRL_VALUES(mContrast, uvc_get_contrast);
	}
	RETURN(ret, int);
}

// コントラストを設定
int UVCCamera::setContrast(uint16_t contrast) {
	ENTER();
	int ret = UVC_ERROR_IO;
	if (mPUSupports & PU_CONTRAST) {
		ret = internalSetCtrlValue(mContrast, contrast, uvc_get_contrast, uvc_set_contrast);
	}
	RETURN(ret, int);
}

// コントラストの現在値を取得
int UVCCamera::getContrast() {
	ENTER();
	if (mPUSupports & PU_CONTRAST) {
		int ret = update_ctrl_values(mDeviceHandle, mContrast, uvc_get_contrast);
		if (LIKELY(!ret)) {	// 正常に最小・最大値を取得出来た時
			uint16_t value;
			ret = uvc_get_contrast(mDeviceHandle, &value, UVC_GET_CUR);
			if (LIKELY(!ret))
				return value;
		}
	}
	RETURN(0, int);
}

//======================================================================
// シャープネス調整
int UVCCamera::updateSharpnessLimit(int &min, int &max, int &def) {
	ENTER();
	int ret = UVC_ERROR_IO;
	if (mPUSupports & PU_SHARPNESS) {
		UPDATE_CTRL_VALUES(mSharpness, uvc_get_sharpness);
	}
	RETURN(ret, int);
}

// シャープネスを設定
int UVCCamera::setSharpness(int sharpness) {
	ENTER();
	int ret = UVC_ERROR_IO;
	if (mPUSupports & PU_SHARPNESS) {
		ret = internalSetCtrlValue(mSharpness, sharpness, uvc_get_sharpness, uvc_set_sharpness);
	}
	RETURN(ret, int);
}

// シャープネスの現在値を取得
int UVCCamera::getSharpness() {
	ENTER();
	if (mPUSupports & PU_SHARPNESS) {
		int ret = update_ctrl_values(mDeviceHandle, mSharpness, uvc_get_sharpness);
		if (LIKELY(!ret)) {	// 正常に最小・最大値を取得出来た時
			uint16_t value;
			ret = uvc_get_sharpness(mDeviceHandle, &value, UVC_GET_CUR);
			if (LIKELY(!ret))
				return value;
		}
	}
	RETURN(0, int);
}

//======================================================================
// ゲイン調整
int UVCCamera::updateGainLimit(int &min, int &max, int &def) {
	ENTER();
	int ret = UVC_ERROR_IO;
	if (mPUSupports & PU_GAIN) {
		UPDATE_CTRL_VALUES(mGain, uvc_get_gain)
	}
	RETURN(ret, int);
}

// ゲインを設定
int UVCCamera::setGain(int gain) {
	ENTER();
	int ret = UVC_ERROR_IO;
	if (mPUSupports & PU_GAIN) {
//		LOGI("gain:%d", gain);
		ret = internalSetCtrlValue(mGain, gain, uvc_get_gain, uvc_set_gain);
	}
	RETURN(ret, int);
}

// ゲインの現在値を取得
int UVCCamera::getGain() {
	ENTER();
	if (mPUSupports & PU_GAIN) {
		int ret = update_ctrl_values(mDeviceHandle, mGain, uvc_get_gain);
		if (LIKELY(!ret)) {	// 正常に最小・最大値を取得出来た時
			uint16_t value;
			ret = uvc_get_gain(mDeviceHandle, &value, UVC_GET_CUR);
//			LOGI("gain:%d", value);
			if (LIKELY(!ret))
				return value;
		}
	}
	RETURN(0, int);
}

//======================================================================
// ホワイトバランス色温度調整
int UVCCamera::updateWhiteBlanceLimit(int &min, int &max, int &def) {
	ENTER();
	int ret = UVC_ERROR_IO;
	if (mPUSupports & PU_WB_TEMP) {
		UPDATE_CTRL_VALUES(mWhiteBlance, uvc_get_white_balance_temperature)
	}
	RETURN(ret, int);
}

// ホワイトバランス色温度を設定
int UVCCamera::setWhiteBlance(int white_blance) {
	ENTER();
	int ret = UVC_ERROR_IO;
	if (mPUSupports & PU_WB_TEMP) {
		ret = internalSetCtrlValue(mWhiteBlance, white_blance,
			uvc_get_white_balance_temperature, uvc_set_white_balance_temperature);
	}
	RETURN(ret, int);
}

// ホワイトバランス色温度の現在値を取得
int UVCCamera::getWhiteBlance() {
	ENTER();
	if (mPUSupports & PU_WB_TEMP) {
		int ret = update_ctrl_values(mDeviceHandle, mWhiteBlance, uvc_get_white_balance_temperature);
		if (LIKELY(!ret)) {	// 正常に最小・最大値を取得出来た時
			uint16_t value;
			ret = uvc_get_white_balance_temperature(mDeviceHandle, &value, UVC_GET_CUR);
			if (LIKELY(!ret))
				return value;
		}
	}
	RETURN(0, int);
}

//======================================================================
// ガンマ調整
int UVCCamera::updateGammaLimit(int &min, int &max, int &def) {
	ENTER();
	int ret = UVC_ERROR_IO;
	if (mPUSupports & PU_GAMMA) {
		UPDATE_CTRL_VALUES(mGamma, uvc_get_gamma)
	}
	RETURN(ret, int);
}

// ガンマを設定
int UVCCamera::setGamma(int gamma) {
	ENTER();
	int ret = UVC_ERROR_IO;
	if (mPUSupports & PU_GAMMA) {
//		LOGI("gamma:%d", gamma);
		ret = internalSetCtrlValue(mGamma, gamma, uvc_get_gamma, uvc_set_gamma);
	}
	RETURN(ret, int);
}

// ガンマの現在値を取得
int UVCCamera::getGamma() {
	ENTER();
	if (mPUSupports & PU_GAMMA) {
		int ret = update_ctrl_values(mDeviceHandle, mGamma, uvc_get_gamma);
		if (LIKELY(!ret)) {	// 正常に最小・最大値を取得出来た時
			uint16_t value;
			ret = uvc_get_gamma(mDeviceHandle, &value, UVC_GET_CUR);
//			LOGI("gamma:%d", ret);
			if (LIKELY(!ret))
				return value;
		}
	}
	RETURN(0, int);
}

//======================================================================
// 彩度調整
int UVCCamera::updateSaturationLimit(int &min, int &max, int &def) {
	ENTER();
	int ret = UVC_ERROR_IO;
	if (mPUSupports & PU_SATURATION) {
		UPDATE_CTRL_VALUES(mSaturation, uvc_get_saturation)
	}
	RETURN(ret, int);
}

// 彩度を設定
int UVCCamera::setSaturation(int saturation) {
	ENTER();
	int ret = UVC_ERROR_IO;
	if (mPUSupports & PU_SATURATION) {
		ret = internalSetCtrlValue(mSaturation, saturation, uvc_get_saturation, uvc_set_saturation);
	}
	RETURN(ret, int);
}

// 彩度の現在値を取得
int UVCCamera::getSaturation() {
	ENTER();
	if (mPUSupports & PU_SATURATION) {
		int ret = update_ctrl_values(mDeviceHandle, mSaturation, uvc_get_saturation);
		if (LIKELY(!ret)) {	// 正常に最小・最大値を取得出来た時
			uint16_t value;
			ret = uvc_get_saturation(mDeviceHandle, &value, UVC_GET_CUR);
			if (LIKELY(!ret))
				return value;
		}
	}
	RETURN(0, int);
}

//======================================================================
// 色相調整
int UVCCamera::updateHueLimit(int &min, int &max, int &def) {
	ENTER();
	int ret = UVC_ERROR_IO;
	if (mPUSupports & PU_HUE) {
		UPDATE_CTRL_VALUES(mHue, uvc_get_hue)
	}
	RETURN(ret, int);
}

// 色相を設定
int UVCCamera::setHue(int hue) {
	ENTER();
	int ret = UVC_ERROR_IO;
	if (mPUSupports & PU_HUE) {
		ret = internalSetCtrlValue(mHue, hue, uvc_get_hue, uvc_set_hue);
	}
	RETURN(ret, int);
}

// 色相の現在値を取得
int UVCCamera::getHue() {
	ENTER();
	if (mPUSupports & PU_HUE) {
		int ret = update_ctrl_values(mDeviceHandle, mHue, uvc_get_hue);
		if (LIKELY(!ret)) {	// 正常に最小・最大値を取得出来た時
			short value;
			ret = uvc_get_hue(mDeviceHandle, &value, UVC_GET_CUR);
			if (LIKELY(!ret))
				return value;
		}
	}
	RETURN(0, int);
}

//======================================================================
// 電源周波数によるチラつき補正を設定
int UVCCamera::setPowerlineFrequency(int frequency) {
	ENTER();
	int ret = UVC_ERROR_IO;
	if (mPUSupports & PU_POWER_LF) {
		if (frequency < 0) {
			uint8_t value;
			ret = uvc_get_powerline_freqency(mDeviceHandle, &value, UVC_GET_DEF);
			if LIKELY(ret)
				frequency = value;
			else
				RETURN(ret, int);
		}
		LOGD("frequency:%d", frequency);
		ret = uvc_set_powerline_freqency(mDeviceHandle, frequency);
	}

	RETURN(ret, int);
}

// 電源周波数によるチラつき補正値を取得
int UVCCamera::getPowerlineFrequency() {
	ENTER();
	if (mPUSupports & PU_POWER_LF) {
		uint8_t value;
		int ret = uvc_get_powerline_freqency(mDeviceHandle, &value, UVC_GET_CUR);
		LOGD("frequency:%d", ret);
		if (LIKELY(!ret))
			return value;
	}
	RETURN(0, int);
}

//======================================================================
// ズーム(abs)調整
int UVCCamera::updateZoomLimit(int &min, int &max, int &def) {
	ENTER();
	int ret = UVC_ERROR_IO;
	if (mCtrlSupports & CTRL_ZOOM_ABS) {
		UPDATE_CTRL_VALUES(mZoom, uvc_get_zoom_abs)
	}
	RETURN(ret, int);
}

// ズーム(abs)を設定
int UVCCamera::setZoom(int zoom) {
	ENTER();
	int ret = UVC_ERROR_IO;
	if (mCtrlSupports & CTRL_ZOOM_ABS) {
		ret = internalSetCtrlValue(mZoom, zoom, uvc_get_zoom_abs, uvc_set_zoom_abs);
	}
	RETURN(ret, int);
}

// ズーム(abs)の現在値を取得
int UVCCamera::getZoom() {
	ENTER();
	if (mCtrlSupports & CTRL_ZOOM_ABS) {
		int ret = update_ctrl_values(mDeviceHandle, mZoom, uvc_get_zoom_abs);
		if (LIKELY(!ret)) {	// 正常に最小・最大値を取得出来た時
			uint16_t value;
			ret = uvc_get_zoom_abs(mDeviceHandle, &value, UVC_GET_CUR);
			if (LIKELY(!ret))
				return value;
		}
	}
	RETURN(0, int);
}
