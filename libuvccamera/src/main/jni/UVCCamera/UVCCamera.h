/*
 * UVCCamera
 * library and sample to access to UVC web camera on non-rooted Android device
 *
 * Copyright (c) 2014-2015 saki t_saki@serenegiant.com
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
#include "UVCPreview.h"

#define	CTRL_SCANNING		0x000001	// D0:  Scanning Mode
#define	CTRL_AE				0x000002	// D1:  Auto-Exposure Mode
#define	CTRL_AE_PRIORITY	0x000004	// D2:  Auto-Exposure Priority
#define	CTRL_AE_ABS			0x000008	// D3:  Exposure Time (Absolute)
#define	CTRL_AR_REL			0x000010	// D4:  Exposure Time (Relative)
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

using namespace std;

typedef struct control_value {
	int res;	// unused
	int min;
	int max;
	int def;
//	int current;
} control_value_t;

typedef uvc_error_t (*paramget_func_short)(uvc_device_handle_t *devh, short *value, enum uvc_req_code req_code);
typedef uvc_error_t (*paramget_func_ushort)(uvc_device_handle_t *devh, uint16_t *value, enum uvc_req_code req_code);
typedef uvc_error_t (*paramset_func_short)(uvc_device_handle_t *devh, short value);
typedef uvc_error_t (*paramset_func_ushort)(uvc_device_handle_t *devh, uint16_t value);

class UVCCamera {
	char *mUsbFs;
	uvc_context_t *mContext;
	int mFd;
	uvc_device_t *mDevice;
	uvc_device_handle_t *mDeviceHandle;
	UVCPreview *mPreview;
	uint64_t mCtrlSupports;
	uint64_t mPUSupports;
	control_value_t mBrightness;
	control_value_t mContrast;
	control_value_t mSharpness;
	control_value_t mGain;
	control_value_t mGamma;
	control_value_t mSaturation;
	control_value_t mHue;
	control_value_t mZoom;
	control_value_t mWhiteBlance;
	control_value_t mFocus;
	void clearCameraParams();
	int internalSetCtrlValue(control_value_t &values, short value,
			paramget_func_short get_func, paramset_func_short set_func);
	int internalSetCtrlValue(control_value_t &values, uint16_t value,
			paramget_func_ushort get_func, paramset_func_ushort set_func);
public:
	UVCCamera();
	~UVCCamera();

	int connect(int vid, int pid, int fd, const char *usbfs);
	int release();
	char *getSupportedSize();
	int setPreviewSize(int width, int height, int mode, float bandwidth = DEFAULT_BANDWIDTH);
	int setPreviewDisplay(ANativeWindow *preview_window);
	int setFrameCallback(JNIEnv *env, jobject frame_callback_obj, int pixel_format);
	int startPreview();
	int stopPreview();
	int setCaptureDisplay(ANativeWindow *capture_window);

	int getCtrlSupports(uint64_t *supports);
	int getProcSupports(uint64_t *supports);

	int setExposureMode(int mode);
	int getExposureMode();

	int setAutoFocus(bool autoFocus);
	bool getAutoFocus();

	int updateFocusLimit(int &min, int &max, int &def);
	int setFocus(int focus);
	int getFocus();

	int setAutoWhiteBlance(bool autoWhiteBlance);
	bool getAutoWhiteBlance();

	int updateWhiteBlanceLimit(int &min, int &max, int &def);
	int setWhiteBlance(int gain);
	int getWhiteBlance();

	int updateBrightnessLimit(int &min, int &max, int &def);
	int setBrightness(short brightness);
	int getBrightness();

	int updateContrastLimit(int &min, int &max, int &def);
	int setContrast(uint16_t contrast);
	int getContrast();

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

	int setPowerlineFrequency(int frequency);
	int getPowerlineFrequency();

	int updateZoomLimit(int &min, int &max, int &def);
	int setZoom(int zoom);
	int getZoom();
};

#endif /* UVCCAMERA_H_ */
