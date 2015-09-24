/*
 * UVCCamera
 * library and sample to access to UVC web camera on non-rooted Android device
 *
 * Copyright (c) 2014-2015 saki t_saki@serenegiant.com
 *
 * File name: serenegiant_usb_UVCCamera.cpp
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

#include <jni.h>
#include <android/native_window_jni.h>

#include "libUVCCamera.h"
#include "UVCCamera.h"

/**
 * set the value into the long field
 * @param env: this param should not be null
 * @param bullet_obj: this param should not be null
 * @param field_name
 * @params val
 */
static jlong setField_long(JNIEnv *env, jobject java_obj, const char *field_name, jlong val) {
#if LOCAL_DEBUG
	LOGV("setField_long:");
#endif

	jclass clazz = env->GetObjectClass(java_obj);
	jfieldID field = env->GetFieldID(clazz, field_name, "J");
	if (LIKELY(field))
		env->SetLongField(java_obj, field, val);
	else {
		LOGE("__setField_long:field '%s' not found", field_name);
	}
#ifdef ANDROID_NDK
	env->DeleteLocalRef(clazz);
#endif
	return val;
}

/**
 * @param env: this param should not be null
 * @param bullet_obj: this param should not be null
 */
static jlong __setField_long(JNIEnv *env, jobject java_obj, jclass clazz, const char *field_name, jlong val) {
#if LOCAL_DEBUG
	LOGV("__setField_long:");
#endif

	jfieldID field = env->GetFieldID(clazz, field_name, "J");
	if (LIKELY(field))
		env->SetLongField(java_obj, field, val);
	else {
		LOGE("__setField_long:field '%s' not found", field_name);
	}
	return val;
}

/**
 * @param env: this param should not be null
 * @param bullet_obj: this param should not be null
 */
jint __setField_int(JNIEnv *env, jobject java_obj, jclass clazz, const char *field_name, jint val) {
	LOGV("__setField_int:");

	jfieldID id = env->GetFieldID(clazz, field_name, "I");
	if (LIKELY(id))
		env->SetIntField(java_obj, id, val);
	else {
		LOGE("__setField_int:field '%s' not found", field_name);
		env->ExceptionClear();	// clear java.lang.NoSuchFieldError exception
	}
	return val;
}

/**
 * set the value into int field
 * @param env: this param should not be null
 * @param java_obj: this param should not be null
 * @param field_name
 * @params val
 */
jint setField_int(JNIEnv *env, jobject java_obj, const char *field_name, jint val) {
	LOGV("setField_int:");

	jclass clazz = env->GetObjectClass(java_obj);
	__setField_int(env, java_obj, clazz, field_name, val);
#ifdef ANDROID_NDK
	env->DeleteLocalRef(clazz);
#endif
	return val;
}

static ID_TYPE nativeCreate(JNIEnv *env, jobject thiz) {

	ENTER();
	UVCCamera *camera = new UVCCamera();
	setField_long(env, thiz, "mNativePtr", reinterpret_cast<ID_TYPE>(camera));
	RETURN(reinterpret_cast<ID_TYPE>(camera), ID_TYPE)
}

static void nativeDestroy(JNIEnv *env, jobject thiz,
	ID_TYPE id_camera) {

	ENTER();
	setField_long(env, thiz, "mNativePtr", 0);
	UVCCamera *camera = reinterpret_cast<UVCCamera *>(id_camera);
	if (LIKELY(camera)) {
		SAFE_DELETE(camera);
	}
	EXIT();
}

static jint nativeConnect(JNIEnv *env, jobject thiz,
	ID_TYPE id_camera,
	jint vid, jint pid, jint fd,  jstring usbfs_str) {

	ENTER();
	int result = JNI_ERR;
	UVCCamera *camera = reinterpret_cast<UVCCamera *>(id_camera);
	const char *c_usbfs = env->GetStringUTFChars(usbfs_str, JNI_FALSE);
	if (LIKELY(camera && (fd > 0))) {
		 result =  camera->connect(vid, pid, fd, c_usbfs);
	}
	env->ReleaseStringUTFChars(usbfs_str, c_usbfs);
	RETURN(result, jint);
}

static jint nativeRelease(JNIEnv *env, jobject thiz,
	ID_TYPE id_camera) {

	ENTER();
	int result = JNI_ERR;
	UVCCamera *camera = reinterpret_cast<UVCCamera *>(id_camera);
	if (LIKELY(camera)) {
		result = camera->release();
	}
	RETURN(result, jint);
}

static jobject nativeGetSupportedSize(JNIEnv *env, jobject thiz,
	ID_TYPE id_camera) {

	ENTER();
	jstring result = NULL;
	UVCCamera *camera = reinterpret_cast<UVCCamera *>(id_camera);
	if (LIKELY(camera)) {
		char *c_str = camera->getSupportedSize();
		if (LIKELY(c_str)) {
			result = env->NewStringUTF(c_str);
			free(c_str);
		}
	}
	RETURN(result, jobject);
}

static jint nativeSetPreviewSize(JNIEnv *env, jobject thiz,
	ID_TYPE id_camera, jint width, jint height, jint mode, jfloat bandwidth) {

	ENTER();
	UVCCamera *camera = reinterpret_cast<UVCCamera *>(id_camera);
	if (LIKELY(camera)) {
		return camera->setPreviewSize(width, height, mode, bandwidth);
	}
	RETURN(JNI_ERR, jint);
}

static jint nativeStartPreview(JNIEnv *env, jobject thiz,
	ID_TYPE id_camera) {

	ENTER();
	UVCCamera *camera = reinterpret_cast<UVCCamera *>(id_camera);
	if (LIKELY(camera)) {
		return camera->startPreview();
	}
	RETURN(JNI_ERR, jint);
}

static jint nativeStopPreview(JNIEnv *env, jobject thiz,
	ID_TYPE id_camera) {

	jint result = JNI_ERR;
	ENTER();
	UVCCamera *camera = reinterpret_cast<UVCCamera *>(id_camera);
	if (LIKELY(camera)) {
		result = camera->stopPreview();
	}
	RETURN(result, jint);
}

static jint nativeSetPreviewDisplay(JNIEnv *env, jobject thiz,
	ID_TYPE id_camera, jobject jSurface) {

	jint result = JNI_ERR;
	ENTER();
	UVCCamera *camera = reinterpret_cast<UVCCamera *>(id_camera);
	if (LIKELY(camera)) {
		ANativeWindow *preview_window = jSurface ? ANativeWindow_fromSurface(env, jSurface) : NULL;
		result = camera->setPreviewDisplay(preview_window);
	}
	RETURN(result, jint);
}

static jint nativeSetFrameCallback(JNIEnv *env, jobject thiz,
	ID_TYPE id_camera, jobject jIFrameCallback, jint pixel_format) {

	jint result = JNI_ERR;
	ENTER();
	UVCCamera *camera = reinterpret_cast<UVCCamera *>(id_camera);
	if (LIKELY(camera)) {
		jobject frame_callback_obj = env->NewGlobalRef(jIFrameCallback);
		result = camera->setFrameCallback(env, frame_callback_obj, pixel_format);
	}
	RETURN(result, jint);
}

static jint nativeSetCaptureDisplay(JNIEnv *env, jobject thiz,
	ID_TYPE id_camera, jobject jSurface) {

	jint result = JNI_ERR;
	ENTER();
	UVCCamera *camera = reinterpret_cast<UVCCamera *>(id_camera);
	if (LIKELY(camera)) {
		ANativeWindow *capture_window = jSurface ? ANativeWindow_fromSurface(env, jSurface) : NULL;
		result = camera->setCaptureDisplay(capture_window);
	}
	RETURN(result, jint);
}

//======================================================================
static jlong nativeGetCtrlSupports(JNIEnv *env, jobject thiz,
	ID_TYPE id_camera) {

	jlong result = 0;
	ENTER();
	UVCCamera *camera = reinterpret_cast<UVCCamera *>(id_camera);
	if (LIKELY(camera)) {
		uint64_t supports;
		int r = camera->getCtrlSupports(&supports);
		if (!r)
			result = supports;
	}
	RETURN(result, jlong);
}

static jlong nativeGetProcSupports(JNIEnv *env, jobject thiz,
	ID_TYPE id_camera) {

	jlong result = 0;
	ENTER();
	UVCCamera *camera = reinterpret_cast<UVCCamera *>(id_camera);
	if (LIKELY(camera)) {
		uint64_t supports;
		int r = camera->getProcSupports(&supports);
		if (!r)
			result = supports;
	}
	RETURN(result, jlong);
}

//======================================================================
static jint nativeSetExposureMode(JNIEnv *env, jobject thiz,
	ID_TYPE id_camera, int exposureMode) {

	jint result = JNI_ERR;
	ENTER();
	UVCCamera *camera = reinterpret_cast<UVCCamera *>(id_camera);
	if (LIKELY(camera)) {
		result = camera->setExposureMode(exposureMode);
	}
	RETURN(result, jint);
}

static jint nativeGetExposureMode(JNIEnv *env, jobject thiz,
	ID_TYPE id_camera) {

	jint result = JNI_ERR;
	ENTER();
	UVCCamera *camera = reinterpret_cast<UVCCamera *>(id_camera);
	if (LIKELY(camera)) {
		result = camera->getExposureMode();
	}
	RETURN(result, jint);
}

//======================================================================
static jint nativeSetAutoFocus(JNIEnv *env, jobject thiz,
	ID_TYPE id_camera, jboolean autofocus) {

	jint result = JNI_ERR;
	ENTER();
	UVCCamera *camera = reinterpret_cast<UVCCamera *>(id_camera);
	if (LIKELY(camera)) {
		result = camera->setAutoFocus(autofocus);
	}
	RETURN(result, jint);
}

static jint nativeGetAutoFocus(JNIEnv *env, jobject thiz,
	ID_TYPE id_camera) {

	jint result = JNI_ERR;
	ENTER();
	UVCCamera *camera = reinterpret_cast<UVCCamera *>(id_camera);
	if (LIKELY(camera)) {
		result = camera->getAutoFocus();
	}
	RETURN(result, jint);
}

//======================================================================
static jint nativeSetAutoWhiteBlance(JNIEnv *env, jobject thiz,
	ID_TYPE id_camera, jboolean autofocus) {

	jint result = JNI_ERR;
	ENTER();
	UVCCamera *camera = reinterpret_cast<UVCCamera *>(id_camera);
	if (LIKELY(camera)) {
		result = camera->setAutoWhiteBlance(autofocus);
	}
	RETURN(result, jint);
}

static jint nativeGetAutoWhiteBlance(JNIEnv *env, jobject thiz,
	ID_TYPE id_camera) {

	jint result = JNI_ERR;
	ENTER();
	UVCCamera *camera = reinterpret_cast<UVCCamera *>(id_camera);
	if (LIKELY(camera)) {
		result = camera->getAutoWhiteBlance();
	}
	RETURN(result, jint);
}

//======================================================================
// Java mnethod correspond to this function should not be a static mathod
static jint nativeUpdateBrightnessLimit(JNIEnv *env, jobject thiz,
	ID_TYPE id_camera) {
	jint result = JNI_ERR;
	ENTER();
	UVCCamera *camera = reinterpret_cast<UVCCamera *>(id_camera);
	if (LIKELY(camera)) {
		int min, max, def;
		result = camera->updateBrightnessLimit(min, max, def);
		if (!result) {
			// Java側へ書き込む
			setField_int(env, thiz, "mBrightnessMin", min);
			setField_int(env, thiz, "mBrightnessMax", max);
			setField_int(env, thiz, "mBrightnessDef", def);
		}
	}
	RETURN(result, jint);
}

static jint nativeSetBrightness(JNIEnv *env, jobject thiz,
	ID_TYPE id_camera, jint brightness) {

	jint result = JNI_ERR;
	ENTER();
	UVCCamera *camera = reinterpret_cast<UVCCamera *>(id_camera);
	if (LIKELY(camera)) {
		result = camera->setBrightness(brightness);
	}
	RETURN(result, jint);
}

static jint nativeGetBrightness(JNIEnv *env, jobject thiz,
	ID_TYPE id_camera) {

	jint result = 0;
	ENTER();
	UVCCamera *camera = reinterpret_cast<UVCCamera *>(id_camera);
	if (LIKELY(camera)) {
		result = camera->getBrightness();
	}
	RETURN(result, jint);
}

//======================================================================
// Java mnethod correspond to this function should not be a static mathod
static jint nativeUpdateFocusLimit(JNIEnv *env, jobject thiz,
	ID_TYPE id_camera) {
	jint result = JNI_ERR;
	ENTER();
	UVCCamera *camera = reinterpret_cast<UVCCamera *>(id_camera);
	if (LIKELY(camera)) {
		int min, max, def;
		result = camera->updateFocusLimit(min, max, def);
		if (!result) {
			// Java側へ書き込む
			setField_int(env, thiz, "mFocusMin", min);
			setField_int(env, thiz, "mFocusMax", max);
			setField_int(env, thiz, "mFocusDef", def);
		}
	}
	RETURN(result, jint);
}

static jint nativeSetFocus(JNIEnv *env, jobject thiz,
	ID_TYPE id_camera, jint focus) {

	jint result = JNI_ERR;
	ENTER();
	UVCCamera *camera = reinterpret_cast<UVCCamera *>(id_camera);
	if (LIKELY(camera)) {
		result = camera->setFocus(focus);
	}
	RETURN(result, jint);
}

static jint nativeGetFocus(JNIEnv *env, jobject thiz,
	ID_TYPE id_camera) {

	jint result = 0;
	ENTER();
	UVCCamera *camera = reinterpret_cast<UVCCamera *>(id_camera);
	if (LIKELY(camera)) {
		result = camera->getFocus();
	}
	RETURN(result, jint);
}

//======================================================================
// Java mnethod correspond to this function should not be a static mathod
static jint nativeUpdateContrastLimit(JNIEnv *env, jobject thiz,
	ID_TYPE id_camera) {
	jint result = JNI_ERR;
	ENTER();
	UVCCamera *camera = reinterpret_cast<UVCCamera *>(id_camera);
	if (LIKELY(camera)) {
		int min, max, def;
		result = camera->updateContrastLimit(min, max, def);
		if (!result) {
			// Java側へ書き込む
			setField_int(env, thiz, "mContrastMin", min);
			setField_int(env, thiz, "mContrastMax", max);
			setField_int(env, thiz, "mContrastDef", def);
		}
	}
	RETURN(result, jint);
}

static jint nativeSetContrast(JNIEnv *env, jobject thiz,
	ID_TYPE id_camera, jint contrast) {

	jint result = JNI_ERR;
	ENTER();
	UVCCamera *camera = reinterpret_cast<UVCCamera *>(id_camera);
	if (LIKELY(camera)) {
		result = camera->setContrast(contrast);
	}
	RETURN(result, jint);
}

static jint nativeGetContrast(JNIEnv *env, jobject thiz,
	ID_TYPE id_camera) {

	jint result = 0;
	ENTER();
	UVCCamera *camera = reinterpret_cast<UVCCamera *>(id_camera);
	if (LIKELY(camera)) {
		result = camera->getContrast();
	}
	RETURN(result, jint);
}

//======================================================================
// Java mnethod correspond to this function should not be a static mathod
static jint nativeUpdateSharpnessLimit(JNIEnv *env, jobject thiz,
	ID_TYPE id_camera) {
	jint result = JNI_ERR;
	ENTER();
	UVCCamera *camera = reinterpret_cast<UVCCamera *>(id_camera);
	if (LIKELY(camera)) {
		int min, max, def;
		result = camera->updateSharpnessLimit(min, max, def);
		if (!result) {
			// Java側へ書き込む
			setField_int(env, thiz, "mSharpnessMin", min);
			setField_int(env, thiz, "mSharpnessMax", max);
			setField_int(env, thiz, "mSharpnessDef", def);
		}
	}
	RETURN(result, jint);
}

static jint nativeSetSharpness(JNIEnv *env, jobject thiz,
	ID_TYPE id_camera, jint sharpness) {

	jint result = JNI_ERR;
	ENTER();
	UVCCamera *camera = reinterpret_cast<UVCCamera *>(id_camera);
	if (LIKELY(camera)) {
		result = camera->setSharpness(sharpness);
	}
	RETURN(result, jint);
}

static jint nativeGetSharpness(JNIEnv *env, jobject thiz,
	ID_TYPE id_camera) {

	jint result = 0;
	ENTER();
	UVCCamera *camera = reinterpret_cast<UVCCamera *>(id_camera);
	if (LIKELY(camera)) {
		result = camera->getSharpness();
	}
	RETURN(result, jint);
}

//======================================================================
// Java mnethod correspond to this function should not be a static mathod
static jint nativeUpdateGainLimit(JNIEnv *env, jobject thiz,
	ID_TYPE id_camera) {
	jint result = JNI_ERR;
	ENTER();
	UVCCamera *camera = reinterpret_cast<UVCCamera *>(id_camera);
	if (LIKELY(camera)) {
		int min, max, def;
		result = camera->updateGainLimit(min, max, def);
		if (!result) {
			// Java側へ書き込む
			setField_int(env, thiz, "mGainMin", min);
			setField_int(env, thiz, "mGainMax", max);
			setField_int(env, thiz, "mGainDef", def);
		}
	}
	RETURN(result, jint);
}

static jint nativeSetGain(JNIEnv *env, jobject thiz,
	ID_TYPE id_camera, jint gain) {

	jint result = JNI_ERR;
	ENTER();
	UVCCamera *camera = reinterpret_cast<UVCCamera *>(id_camera);
	if (LIKELY(camera)) {
		result = camera->setGain(gain);
	}
	RETURN(result, jint);
}

static jint nativeGetGain(JNIEnv *env, jobject thiz,
	ID_TYPE id_camera) {

	jint result = 0;
	ENTER();
	UVCCamera *camera = reinterpret_cast<UVCCamera *>(id_camera);
	if (LIKELY(camera)) {
		result = camera->getGain();
	}
	RETURN(result, jint);
}

//======================================================================
// Java mnethod correspond to this function should not be a static mathod
static jint nativeUpdateGammaLimit(JNIEnv *env, jobject thiz,
	ID_TYPE id_camera) {
	jint result = JNI_ERR;
	ENTER();
	UVCCamera *camera = reinterpret_cast<UVCCamera *>(id_camera);
	if (LIKELY(camera)) {
		int min, max, def;
		result = camera->updateGammaLimit(min, max, def);
		if (!result) {
			// Java側へ書き込む
			setField_int(env, thiz, "mGammaMin", min);
			setField_int(env, thiz, "mGammaMax", max);
			setField_int(env, thiz, "mGammaDef", def);
		}
	}
	RETURN(result, jint);
}

static jint nativeSetGamma(JNIEnv *env, jobject thiz,
	ID_TYPE id_camera, jint gamma) {

	jint result = JNI_ERR;
	ENTER();
	UVCCamera *camera = reinterpret_cast<UVCCamera *>(id_camera);
	if (LIKELY(camera)) {
		result = camera->setGamma(gamma);
	}
	RETURN(result, jint);
}

static jint nativeGetGamma(JNIEnv *env, jobject thiz,
	ID_TYPE id_camera) {

	jint result = 0;
	ENTER();
	UVCCamera *camera = reinterpret_cast<UVCCamera *>(id_camera);
	if (LIKELY(camera)) {
		result = camera->getGamma();
	}
	RETURN(result, jint);
}

//======================================================================
// Java mnethod correspond to this function should not be a static mathod
static jint nativeUpdateWhiteBlanceLimit(JNIEnv *env, jobject thiz,
	ID_TYPE id_camera) {
	jint result = JNI_ERR;
	ENTER();
	UVCCamera *camera = reinterpret_cast<UVCCamera *>(id_camera);
	if (LIKELY(camera)) {
		int min, max, def;
		result = camera->updateWhiteBlanceLimit(min, max, def);
		if (!result) {
			// Java側へ書き込む
			setField_int(env, thiz, "mWhiteBlanceMin", min);
			setField_int(env, thiz, "mWhiteBlanceMax", max);
			setField_int(env, thiz, "mWhiteBlanceDef", def);
		}
	}
	RETURN(result, jint);
}

static jint nativeSetWhiteBlance(JNIEnv *env, jobject thiz,
	ID_TYPE id_camera, jint whiteBlance) {

	jint result = JNI_ERR;
	ENTER();
	UVCCamera *camera = reinterpret_cast<UVCCamera *>(id_camera);
	if (LIKELY(camera)) {
		result = camera->setWhiteBlance(whiteBlance);
	}
	RETURN(result, jint);
}

static jint nativeGetWhiteBlance(JNIEnv *env, jobject thiz,
	ID_TYPE id_camera) {

	jint result = 0;
	ENTER();
	UVCCamera *camera = reinterpret_cast<UVCCamera *>(id_camera);
	if (LIKELY(camera)) {
		result = camera->getWhiteBlance();
	}
	RETURN(result, jint);
}

//======================================================================
// Java mnethod correspond to this function should not be a static mathod
static jint nativeUpdateSaturationLimit(JNIEnv *env, jobject thiz,
	ID_TYPE id_camera) {
	jint result = JNI_ERR;
	ENTER();
	UVCCamera *camera = reinterpret_cast<UVCCamera *>(id_camera);
	if (LIKELY(camera)) {
		int min, max, def;
		result = camera->updateSaturationLimit(min, max, def);
		if (!result) {
			// Java側へ書き込む
			setField_int(env, thiz, "mSaturationMin", min);
			setField_int(env, thiz, "mSaturationMax", max);
			setField_int(env, thiz, "mSaturationDef", def);
		}
	}
	RETURN(result, jint);
}

static jint nativeSetSaturation(JNIEnv *env, jobject thiz,
	ID_TYPE id_camera, jint saturation) {

	jint result = JNI_ERR;
	ENTER();
	UVCCamera *camera = reinterpret_cast<UVCCamera *>(id_camera);
	if (LIKELY(camera)) {
		result = camera->setSaturation(saturation);
	}
	RETURN(result, jint);
}

static jint nativeGetSaturation(JNIEnv *env, jobject thiz,
	ID_TYPE id_camera) {

	jint result = 0;
	ENTER();
	UVCCamera *camera = reinterpret_cast<UVCCamera *>(id_camera);
	if (LIKELY(camera)) {
		result = camera->getSaturation();
	}
	RETURN(result, jint);
}

//======================================================================
// Java mnethod correspond to this function should not be a static mathod
static jint nativeUpdateHueLimit(JNIEnv *env, jobject thiz,
	ID_TYPE id_camera) {
	jint result = JNI_ERR;
	ENTER();
	UVCCamera *camera = reinterpret_cast<UVCCamera *>(id_camera);
	if (LIKELY(camera)) {
		int min, max, def;
		result = camera->updateHueLimit(min, max, def);
		if (!result) {
			// Java側へ書き込む
			setField_int(env, thiz, "mHueMin", min);
			setField_int(env, thiz, "mHueMax", max);
			setField_int(env, thiz, "mHueDef", def);
		}
	}
	RETURN(result, jint);
}

static jint nativeSetHue(JNIEnv *env, jobject thiz,
	ID_TYPE id_camera, jint hue) {

	jint result = JNI_ERR;
	ENTER();
	UVCCamera *camera = reinterpret_cast<UVCCamera *>(id_camera);
	if (LIKELY(camera)) {
		result = camera->setHue(hue);
	}
	RETURN(result, jint);
}

static jint nativeGetHue(JNIEnv *env, jobject thiz,
	ID_TYPE id_camera) {

	jint result = 0;
	ENTER();
	UVCCamera *camera = reinterpret_cast<UVCCamera *>(id_camera);
	if (LIKELY(camera)) {
		result = camera->getHue();
	}
	RETURN(result, jint);
}

//======================================================================
static jint nativeSetPowerlineFrequency(JNIEnv *env, jobject thiz,
	ID_TYPE id_camera, jint frequency) {

	jint result = JNI_ERR;
	ENTER();
	UVCCamera *camera = reinterpret_cast<UVCCamera *>(id_camera);
	if (LIKELY(camera)) {
		result = camera->setPowerlineFrequency(frequency);
	}
	RETURN(result, jint);
}

static jint nativeGetPowerlineFrequency(JNIEnv *env, jobject thiz,
	ID_TYPE id_camera) {

	jint result = 0;
	ENTER();
	UVCCamera *camera = reinterpret_cast<UVCCamera *>(id_camera);
	if (LIKELY(camera)) {
		result = camera->getPowerlineFrequency();
	}
	RETURN(result, jint);
}

//======================================================================
// Java mnethod correspond to this function should not be a static mathod
static jint nativeUpdateZoomLimit(JNIEnv *env, jobject thiz,
	ID_TYPE id_camera) {
	jint result = JNI_ERR;
	ENTER();
	UVCCamera *camera = reinterpret_cast<UVCCamera *>(id_camera);
	if (LIKELY(camera)) {
		int min, max, def;
		result = camera->updateZoomLimit(min, max, def);
		if (!result) {
			// Java側へ書き込む
			setField_int(env, thiz, "mZoomMin", min);
			setField_int(env, thiz, "mZoomMax", max);
			setField_int(env, thiz, "mZoomDef", def);
		}
	}
	RETURN(result, jint);
}

static jint nativeSetZoom(JNIEnv *env, jobject thiz,
	ID_TYPE id_camera, jint zoom) {

	jint result = JNI_ERR;
	ENTER();
	UVCCamera *camera = reinterpret_cast<UVCCamera *>(id_camera);
	if (LIKELY(camera)) {
		result = camera->setZoom(zoom);
	}
	RETURN(result, jint);
}

static jint nativeGetZoom(JNIEnv *env, jobject thiz,
	ID_TYPE id_camera) {

	jint result = 0;
	ENTER();
	UVCCamera *camera = reinterpret_cast<UVCCamera *>(id_camera);
	if (LIKELY(camera)) {
		result = camera->getZoom();
	}
	RETURN(result, jint);
}

//**********************************************************************
//
//**********************************************************************
jint registerNativeMethods(JNIEnv* env, const char *class_name, JNINativeMethod *methods, int num_methods) {
	int result = 0;

	jclass clazz = env->FindClass(class_name);
	if (LIKELY(clazz)) {
		int result = env->RegisterNatives(clazz, methods, num_methods);
		if (UNLIKELY(result < 0)) {
			LOGE("registerNativeMethods failed(class=%s)", class_name);
		}
	} else {
		LOGE("registerNativeMethods: class'%s' not found", class_name);
	}
	return result;
}

static JNINativeMethod methods[] = {
	{ "nativeCreate",					"()J", (void *) nativeCreate },
	{ "nativeDestroy",					"(J)V", (void *) nativeDestroy },

	{ "nativeConnect",					"(JIIILjava/lang/String;)I", (void *) nativeConnect },
	{ "nativeRelease",					"(J)I", (void *) nativeRelease },

	{ "nativeGetSupportedSize",			"(J)Ljava/lang/String;", (void *) nativeGetSupportedSize },
	{ "nativeSetPreviewSize",			"(JIIIF)I", (void *) nativeSetPreviewSize },
	{ "nativeStartPreview",				"(J)I", (void *) nativeStartPreview },
	{ "nativeStopPreview",				"(J)I", (void *) nativeStopPreview },
	{ "nativeSetPreviewDisplay",		"(JLandroid/view/Surface;)I", (void *) nativeSetPreviewDisplay },
	{ "nativeSetFrameCallback",			"(JLcom/serenegiant/usb/IFrameCallback;I)I", (void *) nativeSetFrameCallback },

	{ "nativeSetCaptureDisplay",		"(JLandroid/view/Surface;)I", (void *) nativeSetCaptureDisplay },

	{ "nativeGetCtrlSupports",			"(J)J", (void *) nativeGetCtrlSupports },
	{ "nativeGetProcSupports",			"(J)J", (void *) nativeGetProcSupports },

	{ "nativeSetExposureMode",			"(JI)I", (void *) nativeSetExposureMode },
	{ "nativeGetExposureMode",			"(J)I", (void *) nativeGetExposureMode },

	{ "nativeSetAutoFocus",				"(JZ)I", (void *) nativeSetAutoFocus },
	{ "nativeGetAutoFocus",				"(J)I", (void *) nativeGetAutoFocus },

	{ "nativeUpdateFocusLimit",			"(J)I", (void *) nativeUpdateFocusLimit },
	{ "nativeSetFocus",					"(JI)I", (void *) nativeSetFocus },
	{ "nativeGetFocus",					"(J)I", (void *) nativeGetFocus },

	{ "nativeSetAutoWhiteBlance",		"(JZ)I", (void *) nativeSetAutoWhiteBlance },
	{ "nativeGetAutoWhiteBlance",		"(J)I", (void *) nativeGetAutoWhiteBlance },

	{ "nativeUpdateWhiteBlanceLimit",	"(J)I", (void *) nativeUpdateWhiteBlanceLimit },
	{ "nativeSetWhiteBlance",			"(JI)I", (void *) nativeSetWhiteBlance },
	{ "nativeGetWhiteBlance",			"(J)I", (void *) nativeGetWhiteBlance },

	{ "nativeUpdateBrightnessLimit",	"(J)I", (void *) nativeUpdateBrightnessLimit },
	{ "nativeSetBrightness",			"(JI)I", (void *) nativeSetBrightness },
	{ "nativeGetBrightness",			"(J)I", (void *) nativeGetBrightness },

	{ "nativeUpdateContrastLimit",		"(J)I", (void *) nativeUpdateContrastLimit },
	{ "nativeSetContrast",				"(JI)I", (void *) nativeSetContrast },
	{ "nativeGetContrast",				"(J)I", (void *) nativeGetContrast },

	{ "nativeUpdateSharpnessLimit",		"(J)I", (void *) nativeUpdateSharpnessLimit },
	{ "nativeSetSharpness",				"(JI)I", (void *) nativeSetSharpness },
	{ "nativeGetSharpness",				"(J)I", (void *) nativeGetSharpness },

	{ "nativeUpdateGainLimit",			"(J)I", (void *) nativeUpdateGainLimit },
	{ "nativeSetGain",					"(JI)I", (void *) nativeSetGain },
	{ "nativeGetGain",					"(J)I", (void *) nativeGetGain },

	{ "nativeUpdateGammaLimit",			"(J)I", (void *) nativeUpdateGammaLimit },
	{ "nativeSetGamma",					"(JI)I", (void *) nativeSetGamma },
	{ "nativeGetGamma",					"(J)I", (void *) nativeGetGamma },

	{ "nativeUpdateSaturationLimit",	"(J)I", (void *) nativeUpdateSaturationLimit },
	{ "nativeSetSaturation",			"(JI)I", (void *) nativeSetSaturation },
	{ "nativeGetSaturation",			"(J)I", (void *) nativeGetSaturation },

	{ "nativeUpdateHueLimit",			"(J)I", (void *) nativeUpdateHueLimit },
	{ "nativeSetHue",					"(JI)I", (void *) nativeSetHue },
	{ "nativeGetHue",					"(J)I", (void *) nativeGetHue },

	{ "nativeSetPowerlineFrequency",	"(JI)I", (void *) nativeSetPowerlineFrequency },
	{ "nativeGetPowerlineFrequency",	"(J)I", (void *) nativeGetPowerlineFrequency },

	{ "nativeUpdateZoomLimit",			"(J)I", (void *) nativeUpdateZoomLimit },
	{ "nativeSetZoom",					"(JI)I", (void *) nativeSetZoom },
	{ "nativeGetZoom",					"(J)I", (void *) nativeGetZoom },
};

int register_uvccamera(JNIEnv *env) {
	LOGV("register_uvccamera:");
	if (registerNativeMethods(env,
		"com/serenegiant/usb/UVCCamera",
		methods, NUM_ARRAY_ELEMENTS(methods)) < 0) {
		return -1;
	}
    return 0;
}
