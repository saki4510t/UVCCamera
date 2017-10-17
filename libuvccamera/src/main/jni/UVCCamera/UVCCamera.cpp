/*
 * UVCCamera
 * library and sample to access to UVC web camera on non-rooted Android device
 *
 * Copyright (c) 2014-2017 saki t_saki@serenegiant.com
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

#define LOG_TAG "UVCCamera"
#if 1	// デバッグ情報を出さない時1
	#ifndef LOG_NDEBUG
		#define	LOG_NDEBUG		// LOGV/LOGD/MARKを出力しない時
		#endif
	#undef USE_LOGALL			// 指定したLOGxだけを出力
#else
	#define USE_LOGALL
	#undef LOG_NDEBUG
	#undef NDEBUG
	#define GET_RAW_DESCRIPTOR
#endif

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
/**
 * コンストラクタ
 */
UVCCamera::UVCCamera()
:	mFd(0),
	mUsbFs(NULL),
	mContext(NULL),
	mDevice(NULL),
	mDeviceHandle(NULL),
	mStatusCallback(NULL),
	mButtonCallback(NULL),
	mPreview(NULL),
	mCtrlSupports(0),
	mPUSupports(0) {

	ENTER();
	clearCameraParams();
	EXIT();
}

/**
 * デストラクタ
 */
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
	mScanningMode.min = mScanningMode.max = mScanningMode.def = 0;
	mExposureMode.min = mExposureMode.max = mExposureMode.def = 0;
	mExposurePriority.min = mExposurePriority.max = mExposurePriority.def = 0;
	mExposureAbs.min = mExposureAbs.max = mExposureAbs.def = 0;
	mAutoFocus.min = mAutoFocus.max = mAutoFocus.def = 0;
	mAutoWhiteBlance.min = mAutoWhiteBlance.max = mAutoWhiteBlance.def = 0;
	mWhiteBlance.min = mWhiteBlance.max = mWhiteBlance.def = 0;
	mAutoWhiteBlanceCompo.min = mAutoWhiteBlanceCompo.max = mAutoWhiteBlanceCompo.def = 0;
	mWhiteBlanceCompo.min = mWhiteBlanceCompo.max = mWhiteBlanceCompo.def = 0;
	mBacklightComp.min = mBacklightComp.max = mBacklightComp.def = 0;
	mBrightness.min = mBrightness.max = mBrightness.def = 0;
	mContrast.min = mContrast.max = mContrast.def = 0;
	mAutoContrast.min = mAutoContrast.max = mAutoContrast.def = 0;
	mSharpness.min = mSharpness.max = mSharpness.def = 0;
	mGain.min = mGain.max = mGain.def = 0;
	mGamma.min = mGamma.max = mGamma.def = 0;
	mSaturation.min = mSaturation.max = mSaturation.def = 0;
	mHue.min = mHue.max = mHue.def = 0;
	mAutoHue.min = mAutoHue.max = mAutoHue.def = 0;
	mZoom.min = mZoom.max = mZoom.def = 0;
	mZoomRel.min = mZoomRel.max = mZoomRel.def = 0;
	mFocus.min = mFocus.max = mFocus.def = 0;
	mFocusRel.min = mFocusRel.max = mFocusRel.def = 0;
	mFocusSimple.min = mFocusSimple.max = mFocusSimple.def = 0;
	mIris.min = mIris.max = mIris.def = 0;
	mIrisRel.min = mIrisRel.max = mIrisRel.def = 0;
	mPan.min = mPan.max = mPan.def = 0; mPan.current = -1;
	mTilt.min = mTilt.max = mTilt.def = 0; mTilt.current = -1;
	mRoll.min = mRoll.max = mRoll.def = 0;
	mPanRel.min = mPanRel.max = mPanRel.def = 0; mPanRel.current = -1;
	mTiltRel.min = mTiltRel.max = mTiltRel.def = 0; mTiltRel.current = -1;
	mRollRel.min = mRollRel.max = mRollRel.def = 0;
	mPrivacy.min = mPrivacy.max = mPrivacy.def = 0;
	mPowerlineFrequency.min = mPowerlineFrequency.max = mPowerlineFrequency.def = 0;
	mMultiplier.min = mMultiplier.max = mMultiplier.def = 0;
	mMultiplierLimit.min = mMultiplierLimit.max = mMultiplierLimit.def = 0;
	mAnalogVideoStandard.min = mAnalogVideoStandard.max = mAnalogVideoStandard.def = 0;
	mAnalogVideoLockState.min = mAnalogVideoLockState.max = mAnalogVideoLockState.def = 0;
}

//======================================================================
/**
 * カメラへ接続する
 */
int UVCCamera::connect(int vid, int pid, int fd, int busnum, int devaddr, const char *usbfs) {
	ENTER();
	uvc_error_t result = UVC_ERROR_BUSY;
	if (!mDeviceHandle && fd) {
		if (mUsbFs)
			free(mUsbFs);
		mUsbFs = strdup(usbfs);
		if (UNLIKELY(!mContext)) {
			result = uvc_init2(&mContext, NULL, mUsbFs);
//			libusb_set_debug(mContext->usb_ctx, LIBUSB_LOG_LEVEL_DEBUG);
			if (UNLIKELY(result < 0)) {
				LOGD("failed to init libuvc");
				RETURN(result, int);
			}
		}
		// カメラ機能フラグをクリア
		clearCameraParams();
		fd = dup(fd);
		// 指定したvid,idを持つデバイスを検索, 見つかれば0を返してmDeviceに見つかったデバイスをセットする(既に1回uvc_ref_deviceを呼んである)
//		result = uvc_find_device2(mContext, &mDevice, vid, pid, NULL, fd);
		result = uvc_get_device_with_fd(mContext, &mDevice, vid, pid, NULL, fd, busnum, devaddr);
		if (LIKELY(!result)) {
			// カメラのopen処理
			result = uvc_open(mDevice, &mDeviceHandle);
			if (LIKELY(!result)) {
				// open出来た時
#if LOCAL_DEBUG
				uvc_print_diag(mDeviceHandle, stderr);
#endif
				mFd = fd;
				mStatusCallback = new UVCStatusCallback(mDeviceHandle);
				mButtonCallback = new UVCButtonCallback(mDeviceHandle);
				mPreview = new UVCPreview(mDeviceHandle);
			} else {
				// open出来なかった時
				LOGE("could not open camera:err=%d", result);
				uvc_unref_device(mDevice);
//				SAFE_DELETE(mDevice);	// 参照カウンタが0ならuvc_unref_deviceでmDeviceがfreeされるから不要 XXX クラッシュ, 既に破棄されているのを再度破棄しようとしたからみたい
				mDevice = NULL;
				mDeviceHandle = NULL;
				close(fd);
			}
		} else {
			LOGE("could not find camera:err=%d", result);
			close(fd);
		}
	} else {
		// カメラが既にopenしている時
		LOGW("camera is already opened. you should release first");
	}
	RETURN(result, int);
}

// カメラを開放する
int UVCCamera::release() {
	ENTER();
	stopPreview();
	// カメラのclose処理
	if (LIKELY(mDeviceHandle)) {
		MARK("カメラがopenしていたら開放する");
		// ステータスコールバックオブジェクトを破棄
		SAFE_DELETE(mStatusCallback);
		SAFE_DELETE(mButtonCallback);
		// プレビューオブジェクトを破棄
		SAFE_DELETE(mPreview);
		// カメラをclose
		uvc_close(mDeviceHandle);
		mDeviceHandle = NULL;
	}
	if (LIKELY(mDevice)) {
		MARK("カメラを開放");
		uvc_unref_device(mDevice);
		mDevice = NULL;
	}
	// カメラ機能フラグをクリア
	clearCameraParams();
	if (mUsbFs) {
		close(mFd);
		mFd = 0;
		free(mUsbFs);
		mUsbFs = NULL;
	}
	RETURN(0, int);
}

int UVCCamera::setStatusCallback(JNIEnv *env, jobject status_callback_obj) {
	ENTER();
	int result = EXIT_FAILURE;
	if (mStatusCallback) {
		result = mStatusCallback->setCallback(env, status_callback_obj);
	}
	RETURN(result, int);
}

int UVCCamera::setButtonCallback(JNIEnv *env, jobject button_callback_obj) {
	ENTER();
	int result = EXIT_FAILURE;
	if (mButtonCallback) {
		result = mButtonCallback->setCallback(env, button_callback_obj);
	}
	RETURN(result, int);
}

char *UVCCamera::getSupportedSize() {
	ENTER();
	if (mDeviceHandle) {
		UVCDiags params;
		RETURN(params.getSupportedSize(mDeviceHandle), char *)
	}
	RETURN(NULL, char *);
}

int UVCCamera::setPreviewSize(int width, int height, int min_fps, int max_fps, int mode, float bandwidth) {
	ENTER();
	int result = EXIT_FAILURE;
	if (mPreview) {
		result = mPreview->setPreviewSize(width, height, min_fps, max_fps, mode, bandwidth);
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

//======================================================================
#define CTRL_BRIGHTNESS		0
#define CTRL_CONTRAST		1
#define	CTRL_SHARPNESS		2
#define CTRL_GAIN			3
#define CTRL_WHITEBLANCE	4
#define CTRL_FOCUS			5

static uvc_error_t update_ctrl_values(uvc_device_handle_t *devh, control_value_t &values,
	paramget_func_i16 get_func) {

	ENTER();

	uvc_error_t ret = UVC_SUCCESS;
	if (!values.min && !values.max) {
		int16_t value;
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
	paramget_func_u16 get_func) {

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

static uvc_error_t update_ctrl_values(uvc_device_handle_t *devh, control_value_t &values,
	paramget_func_i8 get_func) {

	ENTER();

	uvc_error_t ret = UVC_SUCCESS;
	if (!values.min && !values.max) {
		int8_t value;
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
	paramget_func_u8 get_func) {

	ENTER();

	uvc_error_t ret = UVC_SUCCESS;
	if (!values.min && !values.max) {
		uint8_t value;
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
	paramget_func_u8u8 get_func) {

	ENTER();

	uvc_error_t ret = UVC_SUCCESS;
	if (!values.min && !values.max) {
		uint8_t value1, value2;
		ret = get_func(devh, &value1, &value2, UVC_GET_MIN);
		if (LIKELY(!ret)) {
			values.min = (value1 << 8) + value2;
			LOGV("update_params:min value1=%d,value2=%d,min=%d", value1, value2, values.min);
			ret = get_func(devh, &value1, &value2, UVC_GET_MAX);
			if (LIKELY(!ret)) {
				values.max = (value1 << 8) + value2;
				LOGV("update_params:max value1=%d,value2=%d,max=%d", value1, value2, values.max);
				ret = get_func(devh, &value1, &value2, UVC_GET_DEF);
				if (LIKELY(!ret)) {
					values.def = (value1 << 8) + value2;
					LOGV("update_params:def value1=%d,value2=%ddef=%d", value1, value2, values.def);
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
	paramget_func_i8u8 get_func) {

	ENTER();

	uvc_error_t ret = UVC_SUCCESS;
	if (!values.min && !values.max) {
		int8_t value1;
		uint8_t value2;
		ret = get_func(devh, &value1, &value2, UVC_GET_MIN);
		if (LIKELY(!ret)) {
			values.min = (value1 << 8) + value2;
			LOGV("update_params:min value1=%d,value2=%d,min=%d", value1, value2, values.min);
			ret = get_func(devh, &value1, &value2, UVC_GET_MAX);
			if (LIKELY(!ret)) {
				values.max = (value1 << 8) + value2;
				LOGV("update_params:max value1=%d,value2=%d,max=%d", value1, value2, values.max);
				ret = get_func(devh, &value1, &value2, UVC_GET_DEF);
				if (LIKELY(!ret)) {
					values.def = (value1 << 8) + value2;
					LOGV("update_params:def value1=%d,value2=%ddef=%d", value1, value2, values.def);
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
	paramget_func_i8u8u8 get_func) {

	ENTER();

	uvc_error_t ret = UVC_SUCCESS;
	if (!values.min && !values.max) {
		int8_t value1;
		uint8_t value2;
		uint8_t value3;
		ret = get_func(devh, &value1, &value2, &value3, UVC_GET_MIN);
		if (LIKELY(!ret)) {
			values.min = (value1 << 16) + (value2 <<8) +value3;
			LOGV("update_params:min value1=%d,value2=%d,value3=%d,min=%d", value1, value2, value3, values.min);
			ret = get_func(devh, &value1, &value2, &value3, UVC_GET_MAX);
			if (LIKELY(!ret)) {
				values.max = (value1 << 16) + (value2 <<8) +value3;
				LOGV("update_params:max value1=%d,value2=%d,value3=%d,max=%d", value1, value2, value3, values.max);
				ret = get_func(devh, &value1, &value2, &value3, UVC_GET_DEF);
				if (LIKELY(!ret)) {
					values.def = (value1 << 16) + (value2 <<8) +value3;
					LOGV("update_params:def value1=%d,value2=%d,value3=%d,def=%d", value1, value2, value3, values.def);
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
	paramget_func_i32 get_func) {

	ENTER();

	uvc_error_t ret = UVC_SUCCESS;
	if (!values.min && !values.max) {
		int32_t value;
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
	paramget_func_u32 get_func) {

	ENTER();

	uvc_error_t ret = UVC_SUCCESS;
	if (!values.min && !values.max) {
		uint32_t value;
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

static uvc_error_t update_ctrl_values(uvc_device_handle_t *devh, control_value_t &values1, control_value_t &values2,
	paramget_func_i32i32 get_func) {

	ENTER();

	uvc_error_t ret = UVC_SUCCESS;
	if ((!values1.min && !values1.max) ||(!values2.min && !values2.max)) {
		int32_t value1, value2;
		ret = get_func(devh, &value1, &value2, UVC_GET_MIN);
		if (LIKELY(!ret)) {
			values1.min = value1;
			values2.min = value2;
			LOGV("update_params:min value1=%d,value2=%d", value1, value2);
			ret = get_func(devh, &value1, &value2, UVC_GET_MAX);
			if (LIKELY(!ret)) {
				values1.max = value1;
				values2.max = value2;
				LOGV("update_params:max value1=%d,value2=%d", value1, value2);
				ret = get_func(devh, &value1, &value2, UVC_GET_DEF);
				if (LIKELY(!ret)) {
					values1.def = value1;
					values2.def = value2;
					LOGV("update_params:def value1=%d,value2=%d", value1, value2);
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
int UVCCamera::internalSetCtrlValue(control_value_t &values, int8_t value,
		paramget_func_i8 get_func, paramset_func_i8 set_func) {
	int ret = update_ctrl_values(mDeviceHandle, values, get_func);
	if (LIKELY(!ret)) {	// 正常に最小・最大値を取得出来た時
		value = value < values.min
			? values.min
			: (value > values.max ? values.max : value);
		set_func(mDeviceHandle, value);
	}
	RETURN(ret, int);
}

int UVCCamera::internalSetCtrlValue(control_value_t &values, uint8_t value,
		paramget_func_u8 get_func, paramset_func_u8 set_func) {
	int ret = update_ctrl_values(mDeviceHandle, values, get_func);
	if (LIKELY(!ret)) {	// 正常に最小・最大値を取得出来た時
		value = value < values.min
			? values.min
			: (value > values.max ? values.max : value);
		set_func(mDeviceHandle, value);
	}
	RETURN(ret, int);
}

int UVCCamera::internalSetCtrlValue(control_value_t &values, uint8_t value1, uint8_t value2,
		paramget_func_u8u8 get_func, paramset_func_u8u8 set_func) {
	int ret = update_ctrl_values(mDeviceHandle, values, get_func);
	if (LIKELY(!ret)) {	// 正常に最小・最大値を取得出来た時
		uint8_t v1min = (uint8_t)((values.min >> 8) & 0xff);
		uint8_t v2min = (uint8_t)(values.min & 0xff);
		uint8_t v1max = (uint8_t)((values.max >> 8) & 0xff);
		uint8_t v2max = (uint8_t)(values.max & 0xff);
		value1 = value1 < v1min
			? v1min
			: (value1 > v1max ? v1max : value1); 
		value2 = value2 < v2min
			? v2min
			: (value2 > v2max ? v2max : value2); 
		set_func(mDeviceHandle, value1, value2);
	}
	RETURN(ret, int);
}

int UVCCamera::internalSetCtrlValue(control_value_t &values, int8_t value1, uint8_t value2,
		paramget_func_i8u8 get_func, paramset_func_i8u8 set_func) {
	int ret = update_ctrl_values(mDeviceHandle, values, get_func);
	if (LIKELY(!ret)) {	// 正常に最小・最大値を取得出来た時
		int8_t v1min = (int8_t)((values.min >> 8) & 0xff);
		uint8_t v2min = (uint8_t)(values.min & 0xff);
		int8_t v1max = (int8_t)((values.max >> 8) & 0xff);
		uint8_t v2max = (uint8_t)(values.max & 0xff);
		value1 = value1 < v1min
			? v1min
			: (value1 > v1max ? v1max : value1); 
		value2 = value2 < v2min
			? v2min
			: (value2 > v2max ? v2max : value2); 
		set_func(mDeviceHandle, value1, value2);
	}
	RETURN(ret, int);
}

int UVCCamera::internalSetCtrlValue(control_value_t &values, int8_t value1, uint8_t value2, uint8_t value3,
		paramget_func_i8u8u8 get_func, paramset_func_i8u8u8 set_func) {
	int ret = update_ctrl_values(mDeviceHandle, values, get_func);
	if (LIKELY(!ret)) {	// 正常に最小・最大値を取得出来た時
		int8_t v1min = (int8_t)((values.min >> 16) & 0xff);
		uint8_t v2min = (uint8_t)((values.min >> 8) & 0xff);
		uint8_t v3min = (uint8_t)(values.min & 0xff);
		int8_t v1max = (int8_t)((values.max >> 16) & 0xff);
		uint8_t v2max = (uint8_t)((values.max >> 8) & 0xff);
		uint8_t v3max = (uint8_t)(values.max & 0xff);
		value1 = value1 < v1min
			? v1min
			: (value1 > v1max ? v1max : value1); 
		value2 = value2 < v2min
			? v2min
			: (value2 > v2max ? v2max : value2); 
		value3 = value3 < v3min
			? v3min
			: (value3 > v3max ? v3max : value3); 
		set_func(mDeviceHandle, value1, value2, value3);
	}
	RETURN(ret, int);
}

/**
 * カメラコントロール設定の下請け
 */
int UVCCamera::internalSetCtrlValue(control_value_t &values, int16_t value,
		paramget_func_i16 get_func, paramset_func_i16 set_func) {
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
		paramget_func_u16 get_func, paramset_func_u16 set_func) {
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
int UVCCamera::internalSetCtrlValue(control_value_t &values, int32_t value,
		paramget_func_i32 get_func, paramset_func_i32 set_func) {
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
int UVCCamera::internalSetCtrlValue(control_value_t &values, uint32_t value,
		paramget_func_u32 get_func, paramset_func_u32 set_func) {
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
// スキャニングモード
int UVCCamera::updateScanningModeLimit(int &min, int &max, int &def) {
	ENTER();
	int ret = UVC_ERROR_IO;
	if (mPUSupports & CTRL_SCANNING) {
		UPDATE_CTRL_VALUES(mScanningMode, uvc_get_scanning_mode);
	}
	RETURN(ret, int);
}

// スキャニングモードをセット
int UVCCamera::setScanningMode(int mode) {
	ENTER();
	int r = UVC_ERROR_ACCESS;
	if LIKELY((mDeviceHandle) && (mCtrlSupports & CTRL_SCANNING)) {
//		LOGI("ae:%d", mode);
		r = uvc_set_scanning_mode(mDeviceHandle, mode/* & 0xff*/);
	}
	RETURN(r, int);
}

// スキャニングモード設定を取得
int UVCCamera::getScanningMode() {

	ENTER();
	int r = UVC_ERROR_ACCESS;
	if LIKELY((mDeviceHandle) && (mCtrlSupports & CTRL_SCANNING)) {
		uint8_t mode;
		r = uvc_get_scanning_mode(mDeviceHandle, &mode, UVC_GET_CUR);
//		LOGI("ae:%d", mode);
		if (LIKELY(!r)) {
			r = mode;
		}
	}
	RETURN(r, int);
}

//======================================================================
// 露出モード
int UVCCamera::updateExposureModeLimit(int &min, int &max, int &def) {
	ENTER();
	int ret = UVC_ERROR_IO;
	if (mPUSupports & CTRL_AE) {
		UPDATE_CTRL_VALUES(mExposureMode, uvc_get_ae_mode);
	}
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
		uint8_t mode;
		r = uvc_get_ae_mode(mDeviceHandle, &mode, UVC_GET_CUR);
//		LOGI("ae:%d", mode);
		if (LIKELY(!r)) {
			r = mode;
		}
	}
	RETURN(r, int);
}

//======================================================================
// 露出優先設定
int UVCCamera::updateExposurePriorityLimit(int &min, int &max, int &def) {
	ENTER();
	int ret = UVC_ERROR_IO;
	if (mPUSupports & CTRL_AE_PRIORITY) {
		UPDATE_CTRL_VALUES(mExposurePriority, uvc_get_ae_priority);
	}
	RETURN(ret, int);
}

// 露出優先設定をセット
int UVCCamera::setExposurePriority(int priority) {
	ENTER();
	int r = UVC_ERROR_ACCESS;
	if LIKELY((mDeviceHandle) && (mCtrlSupports & CTRL_AE_PRIORITY)) {
//		LOGI("ae priority:%d", priority);
		r = uvc_set_ae_priority(mDeviceHandle, priority/* & 0xff*/);
	}
	RETURN(r, int);
}

// 露出優先設定を取得
int UVCCamera::getExposurePriority() {

	ENTER();
	int r = UVC_ERROR_ACCESS;
	if LIKELY((mDeviceHandle) && (mCtrlSupports & CTRL_AE_PRIORITY)) {
		uint8_t priority;
		r = uvc_get_ae_priority(mDeviceHandle, &priority, UVC_GET_CUR);
//		LOGI("ae priority:%d", priority);
		if (LIKELY(!r)) {
			r = priority;
		}
	}
	RETURN(r, int);
}

//======================================================================
// 露出(絶対値)設定
int UVCCamera::updateExposureLimit(int &min, int &max, int &def) {
	ENTER();
	int ret = UVC_ERROR_IO;
	if (mPUSupports & CTRL_AE_ABS) {
		UPDATE_CTRL_VALUES(mExposureAbs, uvc_get_exposure_abs);
	}
	RETURN(ret, int);
}

// 露出(絶対値)設定をセット
int UVCCamera::setExposure(int ae_abs) {
	ENTER();
	int r = UVC_ERROR_ACCESS;
	if LIKELY((mDeviceHandle) && (mCtrlSupports & CTRL_AE_ABS)) {
//		LOGI("ae_abs:%d", ae_abs);
		r = uvc_set_exposure_abs(mDeviceHandle, ae_abs/* & 0xff*/);
	}
	RETURN(r, int);
}

// 露出(絶対値)設定を取得
int UVCCamera::getExposure() {

	ENTER();
	int r = UVC_ERROR_ACCESS;
	if LIKELY((mDeviceHandle) && (mCtrlSupports & CTRL_AE_ABS)) {
		int ae_abs;
		r = uvc_get_exposure_abs(mDeviceHandle, &ae_abs, UVC_GET_CUR);
//		LOGI("ae_abs:%d", ae_abs);
		if (LIKELY(!r)) {
			r = ae_abs;
		}
	}
	RETURN(r, int);
}

//======================================================================
// 露出(相対値)設定
int UVCCamera::updateExposureRelLimit(int &min, int &max, int &def) {
	ENTER();
	int ret = UVC_ERROR_IO;
	if (mPUSupports & CTRL_AE_REL) {
		UPDATE_CTRL_VALUES(mExposureAbs, uvc_get_exposure_rel);
	}
	RETURN(ret, int);
}

// 露出(相対値)設定をセット
int UVCCamera::setExposureRel(int ae_rel) {
	ENTER();
	int r = UVC_ERROR_ACCESS;
	if LIKELY((mDeviceHandle) && (mCtrlSupports & CTRL_AE_REL)) {
//		LOGI("ae_rel:%d", ae_rel);
		r = uvc_set_exposure_rel(mDeviceHandle, ae_rel/* & 0xff*/);
	}
	RETURN(r, int);
}

// 露出(相対値)設定を取得
int UVCCamera::getExposureRel() {

	ENTER();
	int r = UVC_ERROR_ACCESS;
	if LIKELY((mDeviceHandle) && (mCtrlSupports & CTRL_AE_REL)) {
		int ae_rel;
		r = uvc_get_exposure_rel(mDeviceHandle, &ae_rel, UVC_GET_CUR);
//		LOGI("ae_rel:%d", ae_rel);
		if (LIKELY(!r)) {
			r = ae_rel;
		}
	}
	RETURN(r, int);
}

//======================================================================
// オートフォーカス
int UVCCamera::updateAutoFocusLimit(int &min, int &max, int &def) {
	ENTER();
	int ret = UVC_ERROR_IO;
	if (mPUSupports & CTRL_FOCUS_AUTO) {
		UPDATE_CTRL_VALUES(mAutoFocus, uvc_get_focus_auto);
	}
	RETURN(ret, int);
}

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
// フォーカス(絶対値)調整
int UVCCamera::updateFocusLimit(int &min, int &max, int &def) {
	ENTER();
	int ret = UVC_ERROR_ACCESS;
	if (mCtrlSupports & CTRL_FOCUS_ABS) {
		UPDATE_CTRL_VALUES(mFocus, uvc_get_focus_abs);
	}
	RETURN(ret, int);
}

// フォーカス(絶対値)を設定
int UVCCamera::setFocus(int focus) {
	ENTER();
	int ret = UVC_ERROR_ACCESS;
	if (mCtrlSupports & CTRL_FOCUS_ABS) {
		ret = internalSetCtrlValue(mFocus, focus, uvc_get_focus_abs, uvc_set_focus_abs);
	}
	RETURN(ret, int);
}

// フォーカス(絶対値)の現在値を取得
int UVCCamera::getFocus() {
	ENTER();
	if (mCtrlSupports & CTRL_FOCUS_ABS) {
		int ret = update_ctrl_values(mDeviceHandle, mFocus, uvc_get_focus_abs);
		if (LIKELY(!ret)) {	// 正常に最小・最大値を取得出来た時
			int16_t value;
			ret = uvc_get_focus_abs(mDeviceHandle, &value, UVC_GET_CUR);
			if (LIKELY(!ret))
				return value;
		}
	}
	RETURN(0, int);
}

//======================================================================
// フォーカス(相対値)調整
int UVCCamera::updateFocusRelLimit(int &min, int &max, int &def) {
	ENTER();
	int ret = UVC_ERROR_ACCESS;
	if (mCtrlSupports & CTRL_FOCUS_REL) {
		UPDATE_CTRL_VALUES(mFocusRel, uvc_get_focus_rel);
	}
	RETURN(ret, int);
}

// フォーカス(相対値)を設定
int UVCCamera::setFocusRel(int focus_rel) {
	ENTER();
	int ret = UVC_ERROR_ACCESS;
	if (mCtrlSupports & CTRL_FOCUS_REL) {
		ret = internalSetCtrlValue(mFocusRel, (int8_t)((focus_rel >> 8) & 0xff), (uint8_t)(focus_rel &0xff), uvc_get_focus_rel, uvc_set_focus_rel);
	}
	RETURN(ret, int);
}

// フォーカス(相対値)の現在値を取得
int UVCCamera::getFocusRel() {
	ENTER();
	if (mCtrlSupports & CTRL_FOCUS_REL) {
		int ret = update_ctrl_values(mDeviceHandle, mFocusRel, uvc_get_focus_abs);
		if (LIKELY(!ret)) {	// 正常に最小・最大値を取得出来た時
			int8_t focus;
			uint8_t speed;
			ret = uvc_get_focus_rel(mDeviceHandle, &focus, &speed, UVC_GET_CUR);
			if (LIKELY(!ret))
				return (focus <<8) + speed;
		}
	}
	RETURN(0, int);
}

//======================================================================
/*
// フォーカス(シンプル)調整
int UVCCamera::updateFocusSimpleLimit(int &min, int &max, int &def) {
	ENTER();
	int ret = UVC_ERROR_ACCESS;
	if (mCtrlSupports & CTRL_FOCUS_SIMPLE) {
		UPDATE_CTRL_VALUES(mFocusSimple, uvc_get_focus_simple_range);
	}
	RETURN(ret, int);
}

// フォーカス(シンプル)を設定
int UVCCamera::setFocusSimple(int focus) {
	ENTER();
	int ret = UVC_ERROR_ACCESS;
	if (mCtrlSupports & CTRL_FOCUS_SIMPLE) {
		ret = internalSetCtrlValue(mFocusSimple, focus, uvc_get_focus_simple_range, uvc_set_focus_simple_range);
	}
	RETURN(ret, int);
}

// フォーカス(シンプル)の現在値を取得
int UVCCamera::getFocusSimple() {
	ENTER();
	if (mCtrlSupports & CTRL_FOCUS_SIMPLE) {
		int ret = update_ctrl_values(mDeviceHandle, mFocusSimple, uvc_get_focus_abs);
		if (LIKELY(!ret)) {	// 正常に最小・最大値を取得出来た時
			uint8_t value;
			ret = uvc_get_focus_simple_range(mDeviceHandle, &value, UVC_GET_CUR);
			if (LIKELY(!ret))
				return value;
		}
	}
	RETURN(0, int);
}
*/

//======================================================================
// 絞り(絶対値)調整
int UVCCamera::updateIrisLimit(int &min, int &max, int &def) {
	ENTER();
	int ret = UVC_ERROR_ACCESS;
	if (mCtrlSupports & CTRL_IRIS_ABS) {
		UPDATE_CTRL_VALUES(mIris, uvc_get_iris_abs);
	}
	RETURN(ret, int);
}

// 絞り(絶対値)を設定
int UVCCamera::setIris(int iris) {
	ENTER();
	int ret = UVC_ERROR_ACCESS;
	if (mCtrlSupports & CTRL_IRIS_ABS) {
		ret = internalSetCtrlValue(mIris, iris, uvc_get_iris_abs, uvc_set_iris_abs);
	}
	RETURN(ret, int);
}

// 絞り(絶対値)の現在値を取得
int UVCCamera::getIris() {
	ENTER();
	if (mCtrlSupports & CTRL_IRIS_ABS) {
		int ret = update_ctrl_values(mDeviceHandle, mIris, uvc_get_iris_abs);
		if (LIKELY(!ret)) {	// 正常に最小・最大値を取得出来た時
			uint16_t value;
			ret = uvc_get_iris_abs(mDeviceHandle, &value, UVC_GET_CUR);
			if (LIKELY(!ret))
				return value;
		}
	}
	RETURN(0, int);
}

//======================================================================
// 絞り(相対値)調整
int UVCCamera::updateIrisRelLimit(int &min, int &max, int &def) {
	ENTER();
	int ret = UVC_ERROR_ACCESS;
	if (mCtrlSupports & CTRL_IRIS_REL) {
		UPDATE_CTRL_VALUES(mIris, uvc_get_iris_rel);
	}
	RETURN(ret, int);
}

// 絞り(相対値)を設定
int UVCCamera::setIrisRel(int iris_rel) {
	ENTER();
	int ret = UVC_ERROR_ACCESS;
	if (mCtrlSupports & CTRL_IRIS_REL) {
		ret = internalSetCtrlValue(mIris, iris_rel, uvc_get_iris_rel, uvc_set_iris_rel);
	}
	RETURN(ret, int);
}

// 絞り(相対値)の現在値を取得
int UVCCamera::getIrisRel() {
	ENTER();
	if (mCtrlSupports & CTRL_IRIS_REL) {
		int ret = update_ctrl_values(mDeviceHandle, mIris, uvc_get_iris_rel);
		if (LIKELY(!ret)) {	// 正常に最小・最大値を取得出来た時
			uint8_t iris_rel;
			ret = uvc_get_iris_rel(mDeviceHandle, &iris_rel, UVC_GET_CUR);
			if (LIKELY(!ret))
				return iris_rel;
		}
	}
	RETURN(0, int);
}

//======================================================================
// Pan(絶対値)調整
int UVCCamera::updatePanLimit(int &min, int &max, int &def) {
	ENTER();
	int ret = UVC_ERROR_ACCESS;
	if (mCtrlSupports & CTRL_PANTILT_ABS) {
		update_ctrl_values(mDeviceHandle, mPan, mTilt, uvc_get_pantilt_abs);
	}
	RETURN(ret, int);
}

// Pan(絶対値)を設定
int UVCCamera::setPan(int pan) {
	ENTER();
	int ret = UVC_ERROR_ACCESS;
	if (mCtrlSupports & CTRL_PANTILT_ABS) {
		ret = update_ctrl_values(mDeviceHandle, mPan, mTilt, uvc_get_pantilt_abs);
		if (LIKELY(!ret)) {
			pan = pan < mPan.min
					? mPan.min
					: (pan > mPan.max ? mPan.max : pan);
			int tilt = mTilt.current < 0 ? mTilt.def : mTilt.current;
			ret = uvc_set_pantilt_abs(mDeviceHandle, pan, tilt);
			if (LIKELY(!ret)) {
				mPan.current = pan;
				mTilt.current = tilt;
			}
		}
	}
	RETURN(ret, int);
}

// Pan(絶対値)の現在値を取得
int UVCCamera::getPan() {
	ENTER();
	if (mCtrlSupports & CTRL_PANTILT_ABS) {
		int ret = update_ctrl_values(mDeviceHandle, mPan, mTilt, uvc_get_pantilt_abs);
		if (LIKELY(!ret)) {	// 正常に最小・最大値を取得出来た時
			int32_t pan, tilt;
			ret = uvc_get_pantilt_abs(mDeviceHandle, &pan, &tilt, UVC_GET_CUR);
			if (LIKELY(!ret)) {
				mPan.current = pan;
				mTilt.current = tilt;
				return pan;
			}
		}
	}
	RETURN(0, int);
}

//======================================================================
// Tilt(絶対値)調整
int UVCCamera::updateTiltLimit(int &min, int &max, int &def) {
	ENTER();
	int ret = UVC_ERROR_ACCESS;
	if (mCtrlSupports & CTRL_PANTILT_ABS) {
		update_ctrl_values(mDeviceHandle, mPan, mTilt, uvc_get_pantilt_abs);
	}
	RETURN(ret, int);
}

// Tilt(絶対値)を設定
int UVCCamera::setTilt(int tilt) {
	ENTER();
	int ret = UVC_ERROR_ACCESS;
	if (mCtrlSupports & CTRL_PANTILT_ABS) {
		ret = update_ctrl_values(mDeviceHandle, mPan, mTilt, uvc_get_pantilt_abs);
		if (LIKELY(!ret)) {
			tilt = tilt < mTilt.min
					? mTilt.min
					: (tilt > mTilt.max ? mTilt.max : tilt);
			int pan = mPan.current < 0 ? mPan.def : mPan.current;
			ret = uvc_set_pantilt_abs(mDeviceHandle, pan, tilt);
			if (LIKELY(!ret)) {
				mPan.current = pan;
				mTilt.current = tilt;
			}
		}
	}
	RETURN(ret, int);
}

// Tilt(絶対値)の現在値を取得
int UVCCamera::getTilt() {
	ENTER();
	if (mCtrlSupports & CTRL_PANTILT_ABS) {
		int ret = update_ctrl_values(mDeviceHandle, mPan, mTilt, uvc_get_pantilt_abs);
		if (LIKELY(!ret)) {	// 正常に最小・最大値を取得出来た時
			int32_t pan, tilt;
			ret = uvc_get_pantilt_abs(mDeviceHandle, &pan, &tilt, UVC_GET_CUR);
			if (LIKELY(!ret)) {
				mPan.current = pan;
				mTilt.current = tilt;
				return tilt;
			}
		}
	}
	RETURN(0, int);
}

//======================================================================
// Roll(絶対値)調整
int UVCCamera::updateRollLimit(int &min, int &max, int &def) {
	ENTER();
	int ret = UVC_ERROR_ACCESS;
	if (mCtrlSupports & CTRL_ROLL_ABS) {
		UPDATE_CTRL_VALUES(mRoll, uvc_get_roll_abs);
	}
	RETURN(ret, int);
}

// Roll(絶対値)を設定
int UVCCamera::setRoll(int roll) {
	ENTER();
	int ret = UVC_ERROR_ACCESS;
	if (mCtrlSupports & CTRL_ROLL_ABS) {
		ret = internalSetCtrlValue(mRoll, roll, uvc_get_roll_abs, uvc_set_roll_abs);
	}
	RETURN(ret, int);
}

// Roll(絶対値)の現在値を取得
int UVCCamera::getRoll() {
	ENTER();
	if (mCtrlSupports & CTRL_ROLL_ABS) {
		int ret = update_ctrl_values(mDeviceHandle, mRoll, uvc_get_roll_abs);
		if (LIKELY(!ret)) {	// 正常に最小・最大値を取得出来た時
			int16_t roll;
			ret = uvc_get_roll_abs(mDeviceHandle, &roll, UVC_GET_CUR);
			if (LIKELY(!ret)) {
				mRoll.current = roll;
				return roll;
			}
		}
	}
	RETURN(0, int);
}

//======================================================================
int UVCCamera::updatePanRelLimit(int &min, int &max, int &def) {
	ENTER();
	// FIXME not implemented yet
	RETURN(UVC_ERROR_ACCESS, int);
}

int UVCCamera::setPanRel(int pan_rel) {
	ENTER();
	// FIXME not implemented yet
	RETURN(UVC_ERROR_ACCESS, int);
}

int UVCCamera::getPanRel() {
	ENTER();
	// FIXME not implemented yet
	RETURN(UVC_ERROR_ACCESS, int);
}
	
//======================================================================
int UVCCamera::updateTiltRelLimit(int &min, int &max, int &def) {
	ENTER();
	// FIXME not implemented yet
	RETURN(UVC_ERROR_ACCESS, int);
}

int UVCCamera::setTiltRel(int tilt_rel) {
	ENTER();
	// FIXME not implemented yet
	RETURN(UVC_ERROR_ACCESS, int);
}

int UVCCamera::getTiltRel() {
	ENTER();
	// FIXME not implemented yet
	RETURN(UVC_ERROR_ACCESS, int);
}
	
//======================================================================
int UVCCamera::updateRollRelLimit(int &min, int &max, int &def) {
	ENTER();
	// FIXME not implemented yet
	RETURN(UVC_ERROR_ACCESS, int);
}

int UVCCamera::setRollRel(int roll_rel) {
	ENTER();
	// FIXME not implemented yet
	RETURN(UVC_ERROR_ACCESS, int);
}

int UVCCamera::getRollRel() {
	ENTER();
	// FIXME not implemented yet
	RETURN(UVC_ERROR_ACCESS, int);
}

//======================================================================
// プライバシーモード
int UVCCamera::updatePrivacyLimit(int &min, int &max, int &def) {
	ENTER();
	int ret = UVC_ERROR_ACCESS;
	if (mCtrlSupports & CTRL_PRIVACY) {
		UPDATE_CTRL_VALUES(mPrivacy, uvc_get_focus_abs);
	}
	RETURN(ret, int);
}

// プライバシーモードを設定
int UVCCamera::setPrivacy(int privacy) {
	ENTER();
	int ret = UVC_ERROR_ACCESS;
	if (mCtrlSupports & CTRL_PRIVACY) {
		ret = internalSetCtrlValue(mPrivacy, privacy, uvc_get_privacy, uvc_set_privacy);
	}
	RETURN(ret, int);
}

// プライバシーモードの現在値を取得
int UVCCamera::getPrivacy() {
	ENTER();
	if (mCtrlSupports & CTRL_PRIVACY) {
		int ret = update_ctrl_values(mDeviceHandle, mPrivacy, uvc_get_privacy);
		if (LIKELY(!ret)) {	// 正常に最小・最大値を取得出来た時
			uint8_t privacy;
			ret = uvc_get_privacy(mDeviceHandle, &privacy, UVC_GET_CUR);
			if (LIKELY(!ret))
				return privacy;
		}
	}
	RETURN(0, int);
}

//======================================================================
/*
// DigitalWindow
int UVCCamera::updateDigitalWindowLimit(...not defined...) {
	ENTER();
	// FIXME not implemented yet
	RETURN(UVC_ERROR_ACCESS, int);
}

// DigitalWindowを設定
int UVCCamera::setDigitalWindow(int top, int reft, int bottom, int right) {
	ENTER();
	// FIXME not implemented yet
	RETURN(UVC_ERROR_ACCESS, int);
}

// DigitalWindowの現在値を取得
int UVCCamera::getDigitalWindow(int &top, int &reft, int &bottom, int &right) {
	ENTER();
	// FIXME not implemented yet
	RETURN(UVC_ERROR_ACCESS, int);
}
*/

//======================================================================
/*
// DigitalRoi
int UVCCamera::updateDigitalRoiLimit(...not defined...) {
	ENTER();
	// FIXME not implemented yet
	RETURN(UVC_ERROR_ACCESS, int);
}

// DigitalRoiを設定
int UVCCamera::setDigitalRoi(int top, int reft, int bottom, int right) {
	ENTER();
	// FIXME not implemented yet
	RETURN(UVC_ERROR_ACCESS, int);
}

// DigitalRoiの現在値を取得
int UVCCamera::getDigitalRoi(int &top, int &reft, int &bottom, int &right) {
	ENTER();
	// FIXME not implemented yet
	RETURN(UVC_ERROR_ACCESS, int);
}
*/

//======================================================================
// backlight_compensation
int UVCCamera::updateBacklightCompLimit(int &min, int &max, int &def) {
	ENTER();
	int ret = UVC_ERROR_IO;
	if (mPUSupports & PU_BACKLIGHT) {
		UPDATE_CTRL_VALUES(mBacklightComp, uvc_get_backlight_compensation);
	}
	RETURN(ret, int);
}

// backlight_compensationを設定
int UVCCamera::setBacklightComp(int backlight) {
	ENTER();
	int ret = UVC_ERROR_IO;
	if (mPUSupports & PU_BACKLIGHT) {
		ret = internalSetCtrlValue(mBacklightComp, backlight, uvc_get_backlight_compensation, uvc_set_backlight_compensation);
	}
	RETURN(ret, int);
}

// backlight_compensationの現在値を取得
int UVCCamera::getBacklightComp() {
	ENTER();
	if (mPUSupports & PU_BACKLIGHT) {
		int ret = update_ctrl_values(mDeviceHandle, mBacklightComp, uvc_get_backlight_compensation);
		if (LIKELY(!ret)) {	// 正常に最小・最大値を取得出来た時
			int16_t value;
			ret = uvc_get_backlight_compensation(mDeviceHandle, &value, UVC_GET_CUR);
			if (LIKELY(!ret))
				return value;
		}
	}
	RETURN(0, int);
}


//======================================================================
// 明るさ
int UVCCamera::updateBrightnessLimit(int &min, int &max, int &def) {
	ENTER();
	int ret = UVC_ERROR_IO;
	if (mPUSupports & PU_BRIGHTNESS) {
		UPDATE_CTRL_VALUES(mBrightness, uvc_get_brightness);
	}
	RETURN(ret, int);
}

int UVCCamera::setBrightness(int brightness) {
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
			int16_t value;
			ret = uvc_get_brightness(mDeviceHandle, &value, UVC_GET_CUR);
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
// オートコントラスト
int UVCCamera::updateAutoContrastLimit(int &min, int &max, int &def) {
	ENTER();
	int ret = UVC_ERROR_IO;
	if (mPUSupports & PU_CONTRAST_AUTO) {
		UPDATE_CTRL_VALUES(mAutoFocus, uvc_get_contrast_auto);
	}
	RETURN(ret, int);
}

// オートコントラストをon/off
int UVCCamera::setAutoContrast(bool autoContrast) {
	ENTER();

	int r = UVC_ERROR_ACCESS;
	if LIKELY((mDeviceHandle) && (mPUSupports & PU_CONTRAST_AUTO)) {
		r = uvc_set_contrast_auto(mDeviceHandle, autoContrast);
	}
	RETURN(r, int);
}

// オートコントラストのon/off状態を取得
bool UVCCamera::getAutoContrast() {
	ENTER();
	int r = UVC_ERROR_ACCESS;
	if LIKELY((mDeviceHandle) && (mPUSupports & PU_CONTRAST_AUTO)) {
		uint8_t autoContrast;
		r = uvc_get_contrast_auto(mDeviceHandle, &autoContrast, UVC_GET_CUR);
		if (LIKELY(!r))
			r = autoContrast;
	}
	RETURN(r, int);
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
// オートホワイトバランス(temp)
int UVCCamera::updateAutoWhiteBlanceLimit(int &min, int &max, int &def) {
	ENTER();
	int ret = UVC_ERROR_IO;
	if (mPUSupports & PU_WB_TEMP_AUTO) {
		UPDATE_CTRL_VALUES(mAutoWhiteBlance, uvc_get_white_balance_temperature_auto);
	}
	RETURN(ret, int);
}

// オートホワイトバランス(temp)をon/off
int UVCCamera::setAutoWhiteBlance(bool autoWhiteBlance) {
	ENTER();
	int r = UVC_ERROR_ACCESS;
	if LIKELY((mDeviceHandle) && (mPUSupports & PU_WB_TEMP_AUTO)) {
		r = uvc_set_white_balance_temperature_auto(mDeviceHandle, autoWhiteBlance);
	}
	RETURN(r, int);
}

// オートホワイトバランス(temp)のon/off状態を取得
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
// オートホワイトバランス(compo)
int UVCCamera::updateAutoWhiteBlanceCompoLimit(int &min, int &max, int &def) {
	ENTER();
	int ret = UVC_ERROR_IO;
	if (mPUSupports & PU_WB_COMPO_AUTO) {
		UPDATE_CTRL_VALUES(mAutoWhiteBlanceCompo, uvc_get_white_balance_component_auto);
	}
	RETURN(ret, int);
}

// オートホワイトバランス(compo)をon/off
int UVCCamera::setAutoWhiteBlanceCompo(bool autoWhiteBlanceCompo) {
	ENTER();
	int r = UVC_ERROR_ACCESS;
	if LIKELY((mDeviceHandle) && (mPUSupports & PU_WB_COMPO_AUTO)) {
		r = uvc_set_white_balance_component_auto(mDeviceHandle, autoWhiteBlanceCompo);
	}
	RETURN(r, int);
}

// オートホワイトバランス(compo)のon/off状態を取得
bool UVCCamera::getAutoWhiteBlanceCompo() {
	ENTER();
	int r = UVC_ERROR_ACCESS;
	if LIKELY((mDeviceHandle) && (mPUSupports & PU_WB_COMPO_AUTO)) {
		uint8_t autoWhiteBlanceCompo;
		r = uvc_get_white_balance_component_auto(mDeviceHandle, &autoWhiteBlanceCompo, UVC_GET_CUR);
		if (LIKELY(!r))
			r = autoWhiteBlanceCompo;
	}
	RETURN(r, int);
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
// ホワイトバランスcompo調整
int UVCCamera::updateWhiteBlanceCompoLimit(int &min, int &max, int &def) {
	ENTER();
	int ret = UVC_ERROR_IO;
	if (mPUSupports & PU_WB_COMPO) {
		UPDATE_CTRL_VALUES(mWhiteBlanceCompo, uvc_get_white_balance_component)
	}
	RETURN(ret, int);
}

// ホワイトバランスcompoを設定
int UVCCamera::setWhiteBlanceCompo(int white_blance_compo) {
	ENTER();
	int ret = UVC_ERROR_IO;
	if (mPUSupports & PU_WB_COMPO) {
		ret = internalSetCtrlValue(mWhiteBlanceCompo, white_blance_compo,
			uvc_get_white_balance_component, uvc_set_white_balance_component);
	}
	RETURN(ret, int);
}

// ホワイトバランスcompoの現在値を取得
int UVCCamera::getWhiteBlanceCompo() {
	ENTER();
	if (mPUSupports & PU_WB_COMPO) {
		int ret = update_ctrl_values(mDeviceHandle, mWhiteBlanceCompo, uvc_get_white_balance_component);
		if (LIKELY(!ret)) {	// 正常に最小・最大値を取得出来た時
			uint32_t white_blance_compo;
			ret = uvc_get_white_balance_component(mDeviceHandle, &white_blance_compo, UVC_GET_CUR);
			if (LIKELY(!ret))
				return white_blance_compo;
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
			int16_t value;
			ret = uvc_get_hue(mDeviceHandle, &value, UVC_GET_CUR);
			if (LIKELY(!ret))
				return value;
		}
	}
	RETURN(0, int);
}

//======================================================================
// オート色相
int UVCCamera::updateAutoHueLimit(int &min, int &max, int &def) {
	ENTER();
	int ret = UVC_ERROR_IO;
	if (mPUSupports & PU_HUE_AUTO) {
		UPDATE_CTRL_VALUES(mAutoHue, uvc_get_hue_auto);
	}
	RETURN(ret, int);
}

// オート色相をon/off
int UVCCamera::setAutoHue(bool autoHue) {
	ENTER();

	int r = UVC_ERROR_ACCESS;
	if LIKELY((mDeviceHandle) && (mPUSupports & PU_HUE_AUTO)) {
		r = uvc_set_hue_auto(mDeviceHandle, autoHue);
	}
	RETURN(r, int);
}

// オート色相のon/off状態を取得
bool UVCCamera::getAutoHue() {
	ENTER();
	int r = UVC_ERROR_ACCESS;
	if LIKELY((mDeviceHandle) && (mPUSupports & PU_HUE_AUTO)) {
		uint8_t autoHue;
		r = uvc_get_hue_auto(mDeviceHandle, &autoHue, UVC_GET_CUR);
		if (LIKELY(!r))
			r = autoHue;
	}
	RETURN(r, int);
}

//======================================================================
// 電源周波数によるチラつき補正
int UVCCamera::updatePowerlineFrequencyLimit(int &min, int &max, int &def) {
	ENTER();
	int ret = UVC_ERROR_IO;
	if (mCtrlSupports & PU_POWER_LF) {
		UPDATE_CTRL_VALUES(mPowerlineFrequency, uvc_get_powerline_freqency)
	}
	RETURN(ret, int);
}

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

//======================================================================
// ズーム(相対値)調整
int UVCCamera::updateZoomRelLimit(int &min, int &max, int &def) {
	ENTER();
	int ret = UVC_ERROR_IO;
	if (mCtrlSupports & CTRL_ZOOM_REL) {
		UPDATE_CTRL_VALUES(mZoomRel, uvc_get_zoom_rel)
	}
	RETURN(ret, int);
}

// ズーム(相対値)を設定
int UVCCamera::setZoomRel(int zoom) {
	ENTER();
	int ret = UVC_ERROR_IO;
	if (mCtrlSupports & CTRL_ZOOM_REL) {
		ret = internalSetCtrlValue(mZoomRel,
			(int8_t)((zoom >> 16) & 0xff), (uint8_t)((zoom >> 8) & 0xff), (uint8_t)(zoom & 0xff),
			uvc_get_zoom_rel, uvc_set_zoom_rel);
	}
	RETURN(ret, int);
}

// ズーム(相対値)の現在値を取得
int UVCCamera::getZoomRel() {
	ENTER();
	if (mCtrlSupports & CTRL_ZOOM_REL) {
		int ret = update_ctrl_values(mDeviceHandle, mZoomRel, uvc_get_zoom_rel);
		if (LIKELY(!ret)) {	// 正常に最小・最大値を取得出来た時
			int8_t zoom;
			uint8_t isdigital;
			uint8_t speed;
			ret = uvc_get_zoom_rel(mDeviceHandle, &zoom, &isdigital, &speed, UVC_GET_CUR);
			if (LIKELY(!ret))
				return (zoom << 16) +(isdigital << 8) + speed;
		}
	}
	RETURN(0, int);
}

//======================================================================
// digital multiplier調整
int UVCCamera::updateDigitalMultiplierLimit(int &min, int &max, int &def) {
	ENTER();
	int ret = UVC_ERROR_IO;
	if (mPUSupports & PU_DIGITAL_MULT) {
		UPDATE_CTRL_VALUES(mMultiplier, uvc_get_digital_multiplier)
	}
	RETURN(ret, int);
}

// digital multiplierを設定
int UVCCamera::setDigitalMultiplier(int multiplier) {
	ENTER();
	int ret = UVC_ERROR_IO;
	if (mPUSupports & PU_DIGITAL_MULT) {
//		LOGI("multiplier:%d", multiplier);
		ret = internalSetCtrlValue(mMultiplier, multiplier, uvc_get_digital_multiplier, uvc_set_digital_multiplier);
	}
	RETURN(ret, int);
}

// digital multiplierの現在値を取得
int UVCCamera::getDigitalMultiplier() {
	ENTER();
	if (mPUSupports & PU_DIGITAL_MULT) {
		int ret = update_ctrl_values(mDeviceHandle, mMultiplier, uvc_get_digital_multiplier);
		if (LIKELY(!ret)) {	// 正常に最小・最大値を取得出来た時
			uint16_t multiplier;
			ret = uvc_get_digital_multiplier(mDeviceHandle, &multiplier, UVC_GET_CUR);
//			LOGI("multiplier:%d", multiplier);
			if (LIKELY(!ret))
				return multiplier;
		}
	}
	RETURN(0, int);
}

//======================================================================
// digital multiplier limit調整
int UVCCamera::updateDigitalMultiplierLimitLimit(int &min, int &max, int &def) {
	ENTER();
	int ret = UVC_ERROR_IO;
	if (mPUSupports & PU_DIGITAL_LIMIT) {
		UPDATE_CTRL_VALUES(mMultiplierLimit, uvc_get_digital_multiplier_limit)
	}
	RETURN(ret, int);
}

// digital multiplier limitを設定
int UVCCamera::setDigitalMultiplierLimit(int multiplier_limit) {
	ENTER();
	int ret = UVC_ERROR_IO;
	if (mPUSupports & PU_DIGITAL_LIMIT) {
//		LOGI("multiplier limit:%d", multiplier_limit);
		ret = internalSetCtrlValue(mMultiplierLimit, multiplier_limit, uvc_get_digital_multiplier_limit, uvc_set_digital_multiplier_limit);
	}
	RETURN(ret, int);
}

// digital multiplier limitの現在値を取得
int UVCCamera::getDigitalMultiplierLimit() {
	ENTER();
	if (mPUSupports & PU_DIGITAL_LIMIT) {
		int ret = update_ctrl_values(mDeviceHandle, mMultiplierLimit, uvc_get_digital_multiplier_limit);
		if (LIKELY(!ret)) {	// 正常に最小・最大値を取得出来た時
			uint16_t multiplier_limit;
			ret = uvc_get_digital_multiplier_limit(mDeviceHandle, &multiplier_limit, UVC_GET_CUR);
//			LOGI("multiplier_limit:%d", multiplier_limit);
			if (LIKELY(!ret))
				return multiplier_limit;
		}
	}
	RETURN(0, int);
}

//======================================================================
// AnalogVideoStandard
int UVCCamera::updateAnalogVideoStandardLimit(int &min, int &max, int &def) {
	ENTER();
	int ret = UVC_ERROR_IO;
	if (mPUSupports & PU_AVIDEO_STD) {
		UPDATE_CTRL_VALUES(mAnalogVideoStandard, uvc_get_analog_video_standard)
	}
	RETURN(ret, int);
}

int UVCCamera::setAnalogVideoStandard(int standard) {
	ENTER();
	int ret = UVC_ERROR_IO;
	if (mPUSupports & PU_AVIDEO_STD) {
//		LOGI("standard:%d", standard);
		ret = internalSetCtrlValue(mAnalogVideoStandard, standard, uvc_get_analog_video_standard, uvc_set_analog_video_standard);
	}
	RETURN(ret, int);
}

int UVCCamera::getAnalogVideoStandard() {
	ENTER();
	if (mPUSupports & PU_AVIDEO_STD) {
		int ret = update_ctrl_values(mDeviceHandle, mAnalogVideoStandard, uvc_get_analog_video_standard);
		if (LIKELY(!ret)) {	// 正常に最小・最大値を取得出来た時
			uint8_t standard;
			ret = uvc_get_analog_video_standard(mDeviceHandle, &standard, UVC_GET_CUR);
//			LOGI("standard:%d", standard);
			if (LIKELY(!ret))
				return standard;
		}
	}
	RETURN(0, int);
}

//======================================================================
// AnalogVideoLoackStatus
int UVCCamera::updateAnalogVideoLockStateLimit(int &min, int &max, int &def) {
	ENTER();
	int ret = UVC_ERROR_IO;
	if (mPUSupports & PU_AVIDEO_LOCK) {
		UPDATE_CTRL_VALUES(mAnalogVideoLockState, uvc_get_analog_video_lockstate)
	}
	RETURN(ret, int);
}

int UVCCamera::setAnalogVideoLockState(int state) {
	ENTER();
	int ret = UVC_ERROR_IO;
	if (mPUSupports & PU_AVIDEO_LOCK) {
//		LOGI("status:%d", status);
		ret = internalSetCtrlValue(mAnalogVideoLockState, state, uvc_get_analog_video_lockstate, uvc_set_analog_video_lockstate);
	}
	RETURN(ret, int);
}

int UVCCamera::getAnalogVideoLockState() {
	ENTER();
	if (mPUSupports & PU_AVIDEO_LOCK) {
		int ret = update_ctrl_values(mDeviceHandle, mAnalogVideoLockState, uvc_get_analog_video_lockstate);
		if (LIKELY(!ret)) {	// 正常に最小・最大値を取得出来た時
			uint8_t status;
			ret = uvc_get_analog_video_lockstate(mDeviceHandle, &status, UVC_GET_CUR);
//			LOGI("status:%d", status);
			if (LIKELY(!ret))
				return status;
		}
	}
	RETURN(0, int);
}
