/*
 * UVCCamera
 * library and sample to access to UVC web camera on non-rooted Android device
 *
 * Copyright (c) 2014-2017 saki t_saki@serenegiant.com
 *
 * File name: UVCCamera.h
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

#pragma interface

#ifndef UVCCAMERA_H_
#define UVCCAMERA_H_

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <android/native_window.h>
#include "UVCStatusCallback.h"
#include "UVCButtonCallback.h"
#include "UVCPreview.h"

#define	CTRL_SCANNING		0x000001	// D0:  Scanning Mode
#define	CTRL_AE				0x000002	// D1:  Auto-Exposure Mode
#define	CTRL_AE_PRIORITY	0x000004	// D2:  Auto-Exposure Priority
#define	CTRL_AE_ABS			0x000008	// D3:  Exposure Time (Absolute)
#define	CTRL_AE_REL			0x000010	// D4:  Exposure Time (Relative)
#define CTRL_FOCUS_ABS    	0x000020	// D5:  Focus (Absolute)
#define CTRL_FOCUS_REL		0x000040	// D6:  Focus (Relative)
#define CTRL_IRIS_ABS		0x000080	// D7:  Iris (Absolute)
#define	CTRL_IRIS_REL		0x000100	// D8:  Iris (Relative)
#define	CTRL_ZOOM_ABS		0x000200	// D9:  Zoom (Absolute)
#define CTRL_ZOOM_REL		0x000400	// D10: Zoom (Relative)
#define	CTRL_PANTILT_ABS	0x000800	// D11: PanTilt (Absolute)
#define CTRL_PANTILT_REL	0x001000	// D12: PanTilt (Relative)
#define CTRL_ROLL_ABS		0x002000	// D13: Roll (Absolute)
#define CTRL_ROLL_REL		0x004000	// D14: Roll (Relative)
//#define CTRL_D15			0x008000	// D15: Reserved
//#define CTRL_D16			0x010000	// D16: Reserved
#define CTRL_FOCUS_AUTO		0x020000	// D17: Focus, Auto
#define CTRL_PRIVACY		0x040000	// D18: Privacy
#define CTRL_FOCUS_SIMPLE	0x080000	// D19: Focus, Simple
#define CTRL_WINDOW			0x100000	// D20: Window

#define PU_BRIGHTNESS		0x000001	// D0: Brightness
#define PU_CONTRAST			0x000002	// D1: Contrast
#define PU_HUE				0x000004	// D2: Hue
#define	PU_SATURATION		0x000008	// D3: Saturation
#define PU_SHARPNESS		0x000010	// D4: Sharpness
#define PU_GAMMA			0x000020	// D5: Gamma
#define	PU_WB_TEMP			0x000040	// D6: White Balance Temperature
#define	PU_WB_COMPO			0x000080	// D7: White Balance Component
#define	PU_BACKLIGHT		0x000100	// D8: Backlight Compensation
#define PU_GAIN				0x000200	// D9: Gain
#define PU_POWER_LF			0x000400	// D10: Power Line Frequency
#define PU_HUE_AUTO			0x000800	// D11: Hue, Auto
#define PU_WB_TEMP_AUTO		0x001000	// D12: White Balance Temperature, Auto
#define PU_WB_COMPO_AUTO	0x002000	// D13: White Balance Component, Auto
#define PU_DIGITAL_MULT		0x004000	// D14: Digital Multiplier
#define PU_DIGITAL_LIMIT	0x008000	// D15: Digital Multiplier Limit
#define PU_AVIDEO_STD		0x010000	// D16: Analog Video Standard
#define PU_AVIDEO_LOCK		0x020000	// D17: Analog Video Lock Status
#define PU_CONTRAST_AUTO	0x040000	// D18: Contrast, Auto

typedef struct control_value {
	int res;	// unused
	int min;
	int max;
	int def;
	int current;
} control_value_t;

typedef uvc_error_t (*paramget_func_i8)(uvc_device_handle_t *devh, int8_t *value, enum uvc_req_code req_code);
typedef uvc_error_t (*paramget_func_i16)(uvc_device_handle_t *devh, int16_t *value, enum uvc_req_code req_code);
typedef uvc_error_t (*paramget_func_i32)(uvc_device_handle_t *devh, int32_t *value, enum uvc_req_code req_code);
typedef uvc_error_t (*paramget_func_u8)(uvc_device_handle_t *devh, uint8_t *value, enum uvc_req_code req_code);
typedef uvc_error_t (*paramget_func_u16)(uvc_device_handle_t *devh, uint16_t *value, enum uvc_req_code req_code);
typedef uvc_error_t (*paramget_func_u32)(uvc_device_handle_t *devh, uint32_t *value, enum uvc_req_code req_code);
typedef uvc_error_t (*paramget_func_u8u8)(uvc_device_handle_t *devh, uint8_t *value1, uint8_t *value2, enum uvc_req_code req_code);
typedef uvc_error_t (*paramget_func_i8u8)(uvc_device_handle_t *devh, int8_t *value1, uint8_t *value2, enum uvc_req_code req_code);
typedef uvc_error_t (*paramget_func_i8u8u8)(uvc_device_handle_t *devh, int8_t *value1, uint8_t *value2, uint8_t *value3, enum uvc_req_code req_code);
typedef uvc_error_t (*paramget_func_i32i32)(uvc_device_handle_t *devh, int32_t *value1, int32_t *value2, enum uvc_req_code req_code);

typedef uvc_error_t (*paramset_func_i8)(uvc_device_handle_t *devh, int8_t value);
typedef uvc_error_t (*paramset_func_i16)(uvc_device_handle_t *devh, int16_t value);
typedef uvc_error_t (*paramset_func_i32)(uvc_device_handle_t *devh, int32_t value);
typedef uvc_error_t (*paramset_func_u8)(uvc_device_handle_t *devh, uint8_t value);
typedef uvc_error_t (*paramset_func_u16)(uvc_device_handle_t *devh, uint16_t value);
typedef uvc_error_t (*paramset_func_u32)(uvc_device_handle_t *devh, uint32_t value);
typedef uvc_error_t (*paramset_func_u8u8)(uvc_device_handle_t *devh, uint8_t value1, uint8_t value2);
typedef uvc_error_t (*paramset_func_i8u8)(uvc_device_handle_t *devh, int8_t value1, uint8_t value2);
typedef uvc_error_t (*paramset_func_i8u8u8)(uvc_device_handle_t *devh, int8_t value1, uint8_t value2, uint8_t value3);
typedef uvc_error_t (*paramset_func_i32i32)(uvc_device_handle_t *devh, int32_t value1, int32_t value2);

class UVCCamera {
	char *mUsbFs;
	uvc_context_t *mContext;
	int mFd;
	uvc_device_t *mDevice;
	uvc_device_handle_t *mDeviceHandle;
	UVCStatusCallback *mStatusCallback;
	UVCButtonCallback *mButtonCallback;
	// プレビュー用
	UVCPreview *mPreview;
	uint64_t mCtrlSupports;
	uint64_t mPUSupports;
	control_value_t mScanningMode;
	control_value_t mExposureMode;
	control_value_t mExposurePriority;
	control_value_t mExposureAbs;
	control_value_t mAutoFocus;
	control_value_t mAutoWhiteBlance;
	control_value_t mAutoWhiteBlanceCompo;
	control_value_t mWhiteBlance;
	control_value_t mWhiteBlanceCompo;
	control_value_t mBacklightComp;
	control_value_t mBrightness;
	control_value_t mContrast;
	control_value_t mAutoContrast;
	control_value_t mSharpness;
	control_value_t mGain;
	control_value_t mGamma;
	control_value_t mSaturation;
	control_value_t mHue;
	control_value_t mAutoHue;
	control_value_t mZoom;
	control_value_t mZoomRel;
	control_value_t mFocus;
	control_value_t mFocusRel;
	control_value_t mFocusSimple;
	control_value_t mIris;
	control_value_t mIrisRel;
	control_value_t mPan;
	control_value_t mTilt;
	control_value_t mRoll;
	control_value_t mPanRel;
	control_value_t mTiltRel;
	control_value_t mRollRel;
	control_value_t mPrivacy;
	control_value_t mPowerlineFrequency;
	control_value_t mMultiplier;
	control_value_t mMultiplierLimit;
	control_value_t mAnalogVideoStandard;
	control_value_t mAnalogVideoLockState;

	void clearCameraParams();
	int internalSetCtrlValue(control_value_t &values, int8_t value,
		paramget_func_i8 get_func, paramset_func_i8 set_func);
	int internalSetCtrlValue(control_value_t &values, uint8_t value,
		paramget_func_u8 get_func, paramset_func_u8 set_func);
	int internalSetCtrlValue(control_value_t &values, uint8_t value1, uint8_t value2,
		paramget_func_u8u8 get_func, paramset_func_u8u8 set_func);
	int internalSetCtrlValue(control_value_t &values, int8_t value1, uint8_t value2,
		paramget_func_i8u8 get_func, paramset_func_i8u8 set_func);
	int internalSetCtrlValue(control_value_t &values, int8_t value1, uint8_t value2, uint8_t value3,
		paramget_func_i8u8u8 get_func, paramset_func_i8u8u8 set_func);
	int internalSetCtrlValue(control_value_t &values, int16_t value,
		paramget_func_i16 get_func, paramset_func_i16 set_func);
	int internalSetCtrlValue(control_value_t &values, uint16_t value,
		paramget_func_u16 get_func, paramset_func_u16 set_func);
	int internalSetCtrlValue(control_value_t &values, int32_t value,
		paramget_func_i32 get_func, paramset_func_i32 set_func);
	int internalSetCtrlValue(control_value_t &values, uint32_t value,
		paramget_func_u32 get_func, paramset_func_u32 set_func);
public:
	UVCCamera();
	~UVCCamera();

	int connect(int vid, int pid, int fd, int busnum, int devaddr, const char *usbfs);
	int release();

	int setStatusCallback(JNIEnv *env, jobject status_callback_obj);
	int setButtonCallback(JNIEnv *env, jobject button_callback_obj);

	char *getSupportedSize();
	int setPreviewSize(int width, int height, int min_fps, int max_fps, int mode, float bandwidth = DEFAULT_BANDWIDTH);
	int setPreviewDisplay(ANativeWindow *preview_window);
	int setFrameCallback(JNIEnv *env, jobject frame_callback_obj, int pixel_format);
	int startPreview();
	int stopPreview();
	int setCaptureDisplay(ANativeWindow *capture_window);

	int getCtrlSupports(uint64_t *supports);
	int getProcSupports(uint64_t *supports);

	int updateScanningModeLimit(int &min, int &max, int &def);
	int setScanningMode(int mode);
	int getScanningMode();
	
	int updateExposureModeLimit(int &min, int &max, int &def);
	int setExposureMode(int mode);
	int getExposureMode();

	int updateExposurePriorityLimit(int &min, int &max, int &def);
	int setExposurePriority(int priority);
	int getExposurePriority();
	
	int updateExposureLimit(int &min, int &max, int &def);
	int setExposure(int ae_abs);
	int getExposure();
	
	int updateExposureRelLimit(int &min, int &max, int &def);
	int setExposureRel(int ae_rel);
	int getExposureRel();
	
	int updateAutoFocusLimit(int &min, int &max, int &def);
	int setAutoFocus(bool autoFocus);
	bool getAutoFocus();

	int updateFocusLimit(int &min, int &max, int &def);
	int setFocus(int focus);
	int getFocus();

	int updateFocusRelLimit(int &min, int &max, int &def);
	int setFocusRel(int focus);
	int getFocusRel();
	
/*	int updateFocusSimpleLimit(int &min, int &max, int &def);
	int setFocusSimple(int focus);
	int getFocusSimple(); */
	
	int updateIrisLimit(int &min, int &max, int &def);
	int setIris(int iris);
	int getIris();
	
	int updateIrisRelLimit(int &min, int &max, int &def);
	int setIrisRel(int iris);
	int getIrisRel();
	
	int updatePanLimit(int &min, int &max, int &def);
	int setPan(int pan);
	int getPan();
	
	int updateTiltLimit(int &min, int &max, int &def);
	int setTilt(int tilt);
	int getTilt();
	
	int updateRollLimit(int &min, int &max, int &def);
	int setRoll(int roll);
	int getRoll();
	
	int updatePanRelLimit(int &min, int &max, int &def);
	int setPanRel(int pan_rel);
	int getPanRel();
	
	int updateTiltRelLimit(int &min, int &max, int &def);
	int setTiltRel(int tilt_rel);
	int getTiltRel();
	
	int updateRollRelLimit(int &min, int &max, int &def);
	int setRollRel(int roll_rel);
	int getRollRel();
	
	int updatePrivacyLimit(int &min, int &max, int &def);
	int setPrivacy(int privacy);
	int getPrivacy();
	
	int updateAutoWhiteBlanceLimit(int &min, int &max, int &def);
	int setAutoWhiteBlance(bool autoWhiteBlance);
	bool getAutoWhiteBlance();

	int updateAutoWhiteBlanceCompoLimit(int &min, int &max, int &def);
	int setAutoWhiteBlanceCompo(bool autoWhiteBlanceCompo);
	bool getAutoWhiteBlanceCompo();
	
	int updateWhiteBlanceLimit(int &min, int &max, int &def);
	int setWhiteBlance(int temp);
	int getWhiteBlance();

	int updateWhiteBlanceCompoLimit(int &min, int &max, int &def);
	int setWhiteBlanceCompo(int white_blance_compo);
	int getWhiteBlanceCompo();
	
	int updateBacklightCompLimit(int &min, int &max, int &def);
	int setBacklightComp(int backlight);
	int getBacklightComp();
	
	int updateBrightnessLimit(int &min, int &max, int &def);
	int setBrightness(int brightness);
	int getBrightness();

	int updateContrastLimit(int &min, int &max, int &def);
	int setContrast(uint16_t contrast);
	int getContrast();

	int updateAutoContrastLimit(int &min, int &max, int &def);
	int setAutoContrast(bool autoFocus);
	bool getAutoContrast();

	int updateSharpnessLimit(int &min, int &max, int &def);
	int setSharpness(int sharpness);
	int getSharpness();

	int updateGainLimit(int &min, int &max, int &def);
	int setGain(int gain);
	int getGain();

	int updateGammaLimit(int &min, int &max, int &def);
	int setGamma(int gamma);
	int getGamma();

	int updateSaturationLimit(int &min, int &max, int &def);
	int setSaturation(int saturation);
	int getSaturation();

	int updateHueLimit(int &min, int &max, int &def);
	int setHue(int hue);
	int getHue();

	int updateAutoHueLimit(int &min, int &max, int &def);
	int setAutoHue(bool autoFocus);
	bool getAutoHue();
	
	int updatePowerlineFrequencyLimit(int &min, int &max, int &def);
	int setPowerlineFrequency(int frequency);
	int getPowerlineFrequency();

	int updateZoomLimit(int &min, int &max, int &def);
	int setZoom(int zoom);
	int getZoom();
	
	int updateZoomRelLimit(int &min, int &max, int &def);
	int setZoomRel(int zoom);
	int getZoomRel();
	
	int updateDigitalMultiplierLimit(int &min, int &max, int &def);
	int setDigitalMultiplier(int multiplier);
	int getDigitalMultiplier();
	
	int updateDigitalMultiplierLimitLimit(int &min, int &max, int &def);
	int setDigitalMultiplierLimit(int multiplier_limit);
	int getDigitalMultiplierLimit();
	
	int updateAnalogVideoStandardLimit(int &min, int &max, int &def);
	int setAnalogVideoStandard(int standard);
	int getAnalogVideoStandard();
	
	int updateAnalogVideoLockStateLimit(int &min, int &max, int &def);
	int setAnalogVideoLockState(int status);
	int getAnalogVideoLockState();
};

#endif /* UVCCAMERA_H_ */
