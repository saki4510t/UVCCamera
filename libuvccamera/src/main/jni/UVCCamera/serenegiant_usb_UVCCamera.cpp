/*
 * UVCCamera
 * library and sample to access to UVC web camera on non-rooted Android device
 *
 * Copyright (c) 2014-2017 saki t_saki@serenegiant.com
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

#if 1	// デバッグ情報を出さない時
	#ifndef LOG_NDEBUG
		#define	LOG_NDEBUG		// LOGV/LOGD/MARKを出力しない時
		#endif
	#undef USE_LOGALL			// 指定したLOGxだけを出力
#else
	#define USE_LOGALL
	#undef LOG_NDEBUG
	#undef NDEBUG
#endif

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
	RETURN(reinterpret_cast<ID_TYPE>(camera), ID_TYPE);
}

// native側のカメラオブジェクトを破棄
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

//======================================================================
// カメラへ接続
static jint nativeConnect(JNIEnv *env, jobject thiz,
	ID_TYPE id_camera,
	jint vid, jint pid, jint fd,
	jint busNum, jint devAddr, jstring usbfs_str) {

	ENTER();
	int result = JNI_ERR;
	UVCCamera *camera = reinterpret_cast<UVCCamera *>(id_camera);
	const char *c_usbfs = env->GetStringUTFChars(usbfs_str, JNI_FALSE);
	if (LIKELY(camera && (fd > 0))) {
//		libusb_set_debug(NULL, LIBUSB_LOG_LEVEL_DEBUG);
		result =  camera->connect(vid, pid, fd, busNum, devAddr, c_usbfs);
	}
	env->ReleaseStringUTFChars(usbfs_str, c_usbfs);
	RETURN(result, jint);
}

// カメラとの接続を解除
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

//======================================================================
static jint nativeSetStatusCallback(JNIEnv *env, jobject thiz,
	ID_TYPE id_camera, jobject jIStatusCallback) {

	jint result = JNI_ERR;
	ENTER();
	UVCCamera *camera = reinterpret_cast<UVCCamera *>(id_camera);
	if (LIKELY(camera)) {
		jobject status_callback_obj = env->NewGlobalRef(jIStatusCallback);
		result = camera->setStatusCallback(env, status_callback_obj);
	}
	RETURN(result, jint);
}

static jint nativeSetButtonCallback(JNIEnv *env, jobject thiz,
	ID_TYPE id_camera, jobject jIButtonCallback) {

	jint result = JNI_ERR;
	ENTER();
	UVCCamera *camera = reinterpret_cast<UVCCamera *>(id_camera);
	if (LIKELY(camera)) {
		jobject button_callback_obj = env->NewGlobalRef(jIButtonCallback);
		result = camera->setButtonCallback(env, button_callback_obj);
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

//======================================================================
// プレビュー画面の大きさをセット
static jint nativeSetPreviewSize(JNIEnv *env, jobject thiz,
	ID_TYPE id_camera, jint width, jint height, jint min_fps, jint max_fps, jint mode, jfloat bandwidth) {

	ENTER();
	UVCCamera *camera = reinterpret_cast<UVCCamera *>(id_camera);
	if (LIKELY(camera)) {
		return camera->setPreviewSize(width, height, min_fps, max_fps, mode, bandwidth);
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

// プレビューを停止
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
// カメラコントロールでサポートしている機能を取得する
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

// プロセッシングユニットでサポートしている機能を取得する
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
// Java mnethod correspond to this function should not be a static mathod
static jint nativeUpdateScanningModeLimit(JNIEnv *env, jobject thiz,
	ID_TYPE id_camera) {
	jint result = JNI_ERR;
	ENTER();
	UVCCamera *camera = reinterpret_cast<UVCCamera *>(id_camera);
	if (LIKELY(camera)) {
		int min, max, def;
		result = camera->updateScanningModeLimit(min, max, def);
		if (!result) {
			// Java側へ書き込む
			setField_int(env, thiz, "mScanningModeMin", min);
			setField_int(env, thiz, "mScanningModeMax", max);
			setField_int(env, thiz, "mScanningModeDef", def);
		}
	}
	RETURN(result, jint);
}

static jint nativeSetScanningMode(JNIEnv *env, jobject thiz,
	ID_TYPE id_camera, jint scanningMode) {

	jint result = JNI_ERR;
	ENTER();
	UVCCamera *camera = reinterpret_cast<UVCCamera *>(id_camera);
	if (LIKELY(camera)) {
		result = camera->setScanningMode(scanningMode);
	}
	RETURN(result, jint);
}

static jint nativeGetScanningMode(JNIEnv *env, jobject thiz,
	ID_TYPE id_camera) {

	jint result = JNI_ERR;
	ENTER();
	UVCCamera *camera = reinterpret_cast<UVCCamera *>(id_camera);
	if (LIKELY(camera)) {
		result = camera->getScanningMode();
	}
	RETURN(result, jint);
}

//======================================================================
// Java mnethod correspond to this function should not be a static mathod
static jint nativeUpdateExposureModeLimit(JNIEnv *env, jobject thiz,
	ID_TYPE id_camera) {
	jint result = JNI_ERR;
	ENTER();
	UVCCamera *camera = reinterpret_cast<UVCCamera *>(id_camera);
	if (LIKELY(camera)) {
		int min, max, def;
		result = camera->updateExposureModeLimit(min, max, def);
		if (!result) {
			// Java側へ書き込む
			setField_int(env, thiz, "mExposureModeMin", min);
			setField_int(env, thiz, "mExposureModeMax", max);
			setField_int(env, thiz, "mExposureModeDef", def);
		}
	}
	RETURN(result, jint);
}

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
// Java mnethod correspond to this function should not be a static mathod
static jint nativeUpdateExposurePriorityLimit(JNIEnv *env, jobject thiz,
	ID_TYPE id_camera) {
	jint result = JNI_ERR;
	ENTER();
	UVCCamera *camera = reinterpret_cast<UVCCamera *>(id_camera);
	if (LIKELY(camera)) {
		int min, max, def;
		result = camera->updateExposurePriorityLimit(min, max, def);
		if (!result) {
			// Java側へ書き込む
			setField_int(env, thiz, "mExposurePriorityMin", min);
			setField_int(env, thiz, "mExposurePriorityMax", max);
			setField_int(env, thiz, "mExposurePriorityDef", def);
		}
	}
	RETURN(result, jint);
}

static jint nativeSetExposurePriority(JNIEnv *env, jobject thiz,
	ID_TYPE id_camera, int priority) {

	jint result = JNI_ERR;
	ENTER();
	UVCCamera *camera = reinterpret_cast<UVCCamera *>(id_camera);
	if (LIKELY(camera)) {
		result = camera->setExposurePriority(priority);
	}
	RETURN(result, jint);
}

static jint nativeGetExposurePriority(JNIEnv *env, jobject thiz,
	ID_TYPE id_camera) {

	jint result = JNI_ERR;
	ENTER();
	UVCCamera *camera = reinterpret_cast<UVCCamera *>(id_camera);
	if (LIKELY(camera)) {
		result = camera->getExposurePriority();
	}
	RETURN(result, jint);
}

//======================================================================
// Java mnethod correspond to this function should not be a static mathod
static jint nativeUpdateExposureLimit(JNIEnv *env, jobject thiz,
	ID_TYPE id_camera) {
	jint result = JNI_ERR;
	ENTER();
	UVCCamera *camera = reinterpret_cast<UVCCamera *>(id_camera);
	if (LIKELY(camera)) {
		int min, max, def;
		result = camera->updateExposureLimit(min, max, def);
		if (!result) {
			// Java側へ書き込む
			setField_int(env, thiz, "mExposureMin", min);
			setField_int(env, thiz, "mExposureMax", max);
			setField_int(env, thiz, "mExposureDef", def);
		}
	}
	RETURN(result, jint);
}

static jint nativeSetExposure(JNIEnv *env, jobject thiz,
	ID_TYPE id_camera, int exposure) {

	jint result = JNI_ERR;
	ENTER();
	UVCCamera *camera = reinterpret_cast<UVCCamera *>(id_camera);
	if (LIKELY(camera)) {
		result = camera->setExposure(exposure);
	}
	RETURN(result, jint);
}

static jint nativeGetExposure(JNIEnv *env, jobject thiz,
	ID_TYPE id_camera) {

	jint result = JNI_ERR;
	ENTER();
	UVCCamera *camera = reinterpret_cast<UVCCamera *>(id_camera);
	if (LIKELY(camera)) {
		result = camera->getExposure();
	}
	RETURN(result, jint);
}

//======================================================================
// Java mnethod correspond to this function should not be a static mathod
static jint nativeUpdateExposureRelLimit(JNIEnv *env, jobject thiz,
	ID_TYPE id_camera) {
	jint result = JNI_ERR;
	ENTER();
	UVCCamera *camera = reinterpret_cast<UVCCamera *>(id_camera);
	if (LIKELY(camera)) {
		int min, max, def;
		result = camera->updateExposureRelLimit(min, max, def);
		if (!result) {
			// Java側へ書き込む
			setField_int(env, thiz, "mExposureRelMin", min);
			setField_int(env, thiz, "mExposureRelMax", max);
			setField_int(env, thiz, "mExposureRelDef", def);
		}
	}
	RETURN(result, jint);
}

static jint nativeSetExposureRel(JNIEnv *env, jobject thiz,
	ID_TYPE id_camera, jint exposure_rel) {

	jint result = JNI_ERR;
	ENTER();
	UVCCamera *camera = reinterpret_cast<UVCCamera *>(id_camera);
	if (LIKELY(camera)) {
		result = camera->setExposureRel(exposure_rel);
	}
	RETURN(result, jint);
}

static jint nativeGetExposureRel(JNIEnv *env, jobject thiz,
	ID_TYPE id_camera) {

	jint result = JNI_ERR;
	ENTER();
	UVCCamera *camera = reinterpret_cast<UVCCamera *>(id_camera);
	if (LIKELY(camera)) {
		result = camera->getExposureRel();
	}
	RETURN(result, jint);
}

//======================================================================
// Java mnethod correspond to this function should not be a static mathod
static jint nativeUpdateAutoFocusLimit(JNIEnv *env, jobject thiz,
	ID_TYPE id_camera) {
	jint result = JNI_ERR;
	ENTER();
	UVCCamera *camera = reinterpret_cast<UVCCamera *>(id_camera);
	if (LIKELY(camera)) {
		int min, max, def;
		result = camera->updateAutoFocusLimit(min, max, def);
		if (!result) {
			// Java側へ書き込む
			setField_int(env, thiz, "mAutoFocusMin", min);
			setField_int(env, thiz, "mAutoFocusMax", max);
			setField_int(env, thiz, "mAutoFocusDef", def);
		}
	}
	RETURN(result, jint);
}

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
// Java mnethod correspond to this function should not be a static mathod
static jint nativeUpdateAutoWhiteBlanceLimit(JNIEnv *env, jobject thiz,
	ID_TYPE id_camera) {
	jint result = JNI_ERR;
	ENTER();
	UVCCamera *camera = reinterpret_cast<UVCCamera *>(id_camera);
	if (LIKELY(camera)) {
		int min, max, def;
		result = camera->updateAutoWhiteBlanceLimit(min, max, def);
		if (!result) {
			// Java側へ書き込む
			setField_int(env, thiz, "mAutoWhiteBlanceMin", min);
			setField_int(env, thiz, "mAutoWhiteBlanceMax", max);
			setField_int(env, thiz, "mAutoWhiteBlanceDef", def);
		}
	}
	RETURN(result, jint);
}

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
static jint nativeUpdateAutoWhiteBlanceCompoLimit(JNIEnv *env, jobject thiz,
	ID_TYPE id_camera) {
	jint result = JNI_ERR;
	ENTER();
	UVCCamera *camera = reinterpret_cast<UVCCamera *>(id_camera);
	if (LIKELY(camera)) {
		int min, max, def;
		result = camera->updateAutoWhiteBlanceCompoLimit(min, max, def);
		if (!result) {
			// Java側へ書き込む
			setField_int(env, thiz, "mAutoWhiteBlanceCompoMin", min);
			setField_int(env, thiz, "mAutoWhiteBlanceCompoMax", max);
			setField_int(env, thiz, "mAutoWhiteBlanceCompoDef", def);
		}
	}
	RETURN(result, jint);
}

static jint nativeSetAutoWhiteBlanceCompo(JNIEnv *env, jobject thiz,
	ID_TYPE id_camera, jboolean autofocus_compo) {

	jint result = JNI_ERR;
	ENTER();
	UVCCamera *camera = reinterpret_cast<UVCCamera *>(id_camera);
	if (LIKELY(camera)) {
		result = camera->setAutoWhiteBlanceCompo(autofocus_compo);
	}
	RETURN(result, jint);
}

static jint nativeGetAutoWhiteBlanceCompo(JNIEnv *env, jobject thiz,
	ID_TYPE id_camera) {

	jint result = JNI_ERR;
	ENTER();
	UVCCamera *camera = reinterpret_cast<UVCCamera *>(id_camera);
	if (LIKELY(camera)) {
		result = camera->getAutoWhiteBlanceCompo();
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
static jint nativeUpdateFocusRelLimit(JNIEnv *env, jobject thiz,
	ID_TYPE id_camera) {
	jint result = JNI_ERR;
	ENTER();
	UVCCamera *camera = reinterpret_cast<UVCCamera *>(id_camera);
	if (LIKELY(camera)) {
		int min, max, def;
		result = camera->updateFocusRelLimit(min, max, def);
		if (!result) {
			// Java側へ書き込む
			setField_int(env, thiz, "mFocusRelMin", min);
			setField_int(env, thiz, "mFocusRelMax", max);
			setField_int(env, thiz, "mFocusRelDef", def);
		}
	}
	RETURN(result, jint);
}

static jint nativeSetFocusRel(JNIEnv *env, jobject thiz,
	ID_TYPE id_camera, jint focus_rel) {

	jint result = JNI_ERR;
	ENTER();
	UVCCamera *camera = reinterpret_cast<UVCCamera *>(id_camera);
	if (LIKELY(camera)) {
		result = camera->setFocusRel(focus_rel);
	}
	RETURN(result, jint);
}

static jint nativeGetFocusRel(JNIEnv *env, jobject thiz,
	ID_TYPE id_camera) {

	jint result = 0;
	ENTER();
	UVCCamera *camera = reinterpret_cast<UVCCamera *>(id_camera);
	if (LIKELY(camera)) {
		result = camera->getFocusRel();
	}
	RETURN(result, jint);
}

//======================================================================
// Java mnethod correspond to this function should not be a static mathod
static jint nativeUpdateIrisLimit(JNIEnv *env, jobject thiz,
	ID_TYPE id_camera) {
	jint result = JNI_ERR;
	ENTER();
	UVCCamera *camera = reinterpret_cast<UVCCamera *>(id_camera);
	if (LIKELY(camera)) {
		int min, max, def;
		result = camera->updateIrisLimit(min, max, def);
		if (!result) {
			// Java側へ書き込む
			setField_int(env, thiz, "mIrisMin", min);
			setField_int(env, thiz, "mIrisMax", max);
			setField_int(env, thiz, "mIrisDef", def);
		}
	}
	RETURN(result, jint);
}

static jint nativeSetIris(JNIEnv *env, jobject thiz,
	ID_TYPE id_camera, jint iris) {

	jint result = JNI_ERR;
	ENTER();
	UVCCamera *camera = reinterpret_cast<UVCCamera *>(id_camera);
	if (LIKELY(camera)) {
		result = camera->setIris(iris);
	}
	RETURN(result, jint);
}

static jint nativeGetIris(JNIEnv *env, jobject thiz,
	ID_TYPE id_camera) {

	jint result = 0;
	ENTER();
	UVCCamera *camera = reinterpret_cast<UVCCamera *>(id_camera);
	if (LIKELY(camera)) {
		result = camera->getIris();
	}
	RETURN(result, jint);
}

//======================================================================
// Java mnethod correspond to this function should not be a static mathod
static jint nativeUpdateIrisRelLimit(JNIEnv *env, jobject thiz,
	ID_TYPE id_camera) {
	jint result = JNI_ERR;
	ENTER();
	UVCCamera *camera = reinterpret_cast<UVCCamera *>(id_camera);
	if (LIKELY(camera)) {
		int min, max, def;
		result = camera->updateIrisRelLimit(min, max, def);
		if (!result) {
			// Java側へ書き込む
			setField_int(env, thiz, "mIrisRelMin", min);
			setField_int(env, thiz, "mIrisRelMax", max);
			setField_int(env, thiz, "mIrisRelDef", def);
		}
	}
	RETURN(result, jint);
}

static jint nativeSetIrisRel(JNIEnv *env, jobject thiz,
	ID_TYPE id_camera, jint iris_rel) {

	jint result = JNI_ERR;
	ENTER();
	UVCCamera *camera = reinterpret_cast<UVCCamera *>(id_camera);
	if (LIKELY(camera)) {
		result = camera->setIrisRel(iris_rel);
	}
	RETURN(result, jint);
}

static jint nativeGetIrisRel(JNIEnv *env, jobject thiz,
	ID_TYPE id_camera) {

	jint result = 0;
	ENTER();
	UVCCamera *camera = reinterpret_cast<UVCCamera *>(id_camera);
	if (LIKELY(camera)) {
		result = camera->getIrisRel();
	}
	RETURN(result, jint);
}

//======================================================================
// Java mnethod correspond to this function should not be a static mathod
static jint nativeUpdatePanLimit(JNIEnv *env, jobject thiz,
	ID_TYPE id_camera) {
	jint result = JNI_ERR;
	ENTER();
	UVCCamera *camera = reinterpret_cast<UVCCamera *>(id_camera);
	if (LIKELY(camera)) {
		int min, max, def;
		result = camera->updatePanLimit(min, max, def);
		if (!result) {
			// Java側へ書き込む
			setField_int(env, thiz, "mPanMin", min);
			setField_int(env, thiz, "mPanMax", max);
			setField_int(env, thiz, "mPanDef", def);
		}
	}
	RETURN(result, jint);
}

static jint nativeSetPan(JNIEnv *env, jobject thiz,
	ID_TYPE id_camera, jint pan) {

	jint result = JNI_ERR;
	ENTER();
	UVCCamera *camera = reinterpret_cast<UVCCamera *>(id_camera);
	if (LIKELY(camera)) {
		result = camera->setPan(pan);
	}
	RETURN(result, jint);
}

static jint nativeGetPan(JNIEnv *env, jobject thiz,
	ID_TYPE id_camera) {

	jint result = 0;
	ENTER();
	UVCCamera *camera = reinterpret_cast<UVCCamera *>(id_camera);
	if (LIKELY(camera)) {
		result = camera->getPan();
	}
	RETURN(result, jint);
}

//======================================================================
// Java mnethod correspond to this function should not be a static mathod
static jint nativeUpdateTiltLimit(JNIEnv *env, jobject thiz,
	ID_TYPE id_camera) {
	jint result = JNI_ERR;
	ENTER();
	UVCCamera *camera = reinterpret_cast<UVCCamera *>(id_camera);
	if (LIKELY(camera)) {
		int min, max, def;
		result = camera->updateTiltLimit(min, max, def);
		if (!result) {
			// Java側へ書き込む
			setField_int(env, thiz, "mTiltMin", min);
			setField_int(env, thiz, "mTiltMax", max);
			setField_int(env, thiz, "mTiltDef", def);
		}
	}
	RETURN(result, jint);
}

static jint nativeSetTilt(JNIEnv *env, jobject thiz,
	ID_TYPE id_camera, jint tilt) {

	jint result = JNI_ERR;
	ENTER();
	UVCCamera *camera = reinterpret_cast<UVCCamera *>(id_camera);
	if (LIKELY(camera)) {
		result = camera->setTilt(tilt);
	}
	RETURN(result, jint);
}

static jint nativeGetTilt(JNIEnv *env, jobject thiz,
	ID_TYPE id_camera) {

	jint result = 0;
	ENTER();
	UVCCamera *camera = reinterpret_cast<UVCCamera *>(id_camera);
	if (LIKELY(camera)) {
		result = camera->getTilt();
	}
	RETURN(result, jint);
}

//======================================================================
// Java mnethod correspond to this function should not be a static mathod
static jint nativeUpdateRollLimit(JNIEnv *env, jobject thiz,
	ID_TYPE id_camera) {
	jint result = JNI_ERR;
	ENTER();
	UVCCamera *camera = reinterpret_cast<UVCCamera *>(id_camera);
	if (LIKELY(camera)) {
		int min, max, def;
		result = camera->updateRollLimit(min, max, def);
		if (!result) {
			// Java側へ書き込む
			setField_int(env, thiz, "mRollMin", min);
			setField_int(env, thiz, "mRollMax", max);
			setField_int(env, thiz, "mRollDef", def);
		}
	}
	RETURN(result, jint);
}

static jint nativeSetRoll(JNIEnv *env, jobject thiz,
	ID_TYPE id_camera, jint roll) {

	jint result = JNI_ERR;
	ENTER();
	UVCCamera *camera = reinterpret_cast<UVCCamera *>(id_camera);
	if (LIKELY(camera)) {
		result = camera->setRoll(roll);
	}
	RETURN(result, jint);
}

static jint nativeGetRoll(JNIEnv *env, jobject thiz,
	ID_TYPE id_camera) {

	jint result = 0;
	ENTER();
	UVCCamera *camera = reinterpret_cast<UVCCamera *>(id_camera);
	if (LIKELY(camera)) {
		result = camera->getRoll();
	}
	RETURN(result, jint);
}

//======================================================================
// Java mnethod correspond to this function should not be a static mathod
static jint nativeUpdatePanRelLimit(JNIEnv *env, jobject thiz,
	ID_TYPE id_camera) {
	jint result = JNI_ERR;
	ENTER();
	UVCCamera *camera = reinterpret_cast<UVCCamera *>(id_camera);
	if (LIKELY(camera)) {
		int min, max, def;
		result = camera->updatePanRelLimit(min, max, def);
		if (!result) {
			// Java側へ書き込む
			setField_int(env, thiz, "mPanRelMin", min);
			setField_int(env, thiz, "mPanRelMax", max);
			setField_int(env, thiz, "mPanRelDef", def);
		}
	}
	RETURN(result, jint);
}

static jint nativeSetPanRel(JNIEnv *env, jobject thiz,
	ID_TYPE id_camera, jint pan_rel) {

	jint result = JNI_ERR;
	ENTER();
	UVCCamera *camera = reinterpret_cast<UVCCamera *>(id_camera);
	if (LIKELY(camera)) {
		result = camera->setPanRel(pan_rel);
	}
	RETURN(result, jint);
}

static jint nativeGetPanRel(JNIEnv *env, jobject thiz,
	ID_TYPE id_camera) {

	jint result = 0;
	ENTER();
	UVCCamera *camera = reinterpret_cast<UVCCamera *>(id_camera);
	if (LIKELY(camera)) {
		result = camera->getPanRel();
	}
	RETURN(result, jint);
}

//======================================================================
// Java mnethod correspond to this function should not be a static mathod
static jint nativeUpdateTiltRelLimit(JNIEnv *env, jobject thiz,
	ID_TYPE id_camera) {
	jint result = JNI_ERR;
	ENTER();
	UVCCamera *camera = reinterpret_cast<UVCCamera *>(id_camera);
	if (LIKELY(camera)) {
		int min, max, def;
		result = camera->updateTiltRelLimit(min, max, def);
		if (!result) {
			// Java側へ書き込む
			setField_int(env, thiz, "mTiltRelMin", min);
			setField_int(env, thiz, "mTiltRelMax", max);
			setField_int(env, thiz, "mTiltRelDef", def);
		}
	}
	RETURN(result, jint);
}

static jint nativeSetTiltRel(JNIEnv *env, jobject thiz,
	ID_TYPE id_camera, jint tilt_rel) {

	jint result = JNI_ERR;
	ENTER();
	UVCCamera *camera = reinterpret_cast<UVCCamera *>(id_camera);
	if (LIKELY(camera)) {
		result = camera->setTiltRel(tilt_rel);
	}
	RETURN(result, jint);
}

static jint nativeGetTiltRel(JNIEnv *env, jobject thiz,
	ID_TYPE id_camera) {

	jint result = 0;
	ENTER();
	UVCCamera *camera = reinterpret_cast<UVCCamera *>(id_camera);
	if (LIKELY(camera)) {
		result = camera->getTiltRel();
	}
	RETURN(result, jint);
}

//======================================================================
// Java mnethod correspond to this function should not be a static mathod
static jint nativeUpdateRollRelLimit(JNIEnv *env, jobject thiz,
	ID_TYPE id_camera) {
	jint result = JNI_ERR;
	ENTER();
	UVCCamera *camera = reinterpret_cast<UVCCamera *>(id_camera);
	if (LIKELY(camera)) {
		int min, max, def;
		result = camera->updateRollRelLimit(min, max, def);
		if (!result) {
			// Java側へ書き込む
			setField_int(env, thiz, "mRollRelMin", min);
			setField_int(env, thiz, "mRollRelMax", max);
			setField_int(env, thiz, "mRollRelDef", def);
		}
	}
	RETURN(result, jint);
}

static jint nativeSetRollRel(JNIEnv *env, jobject thiz,
	ID_TYPE id_camera, jint roll_rel) {

	jint result = JNI_ERR;
	ENTER();
	UVCCamera *camera = reinterpret_cast<UVCCamera *>(id_camera);
	if (LIKELY(camera)) {
		result = camera->setRollRel(roll_rel);
	}
	RETURN(result, jint);
}

static jint nativeGetRollRel(JNIEnv *env, jobject thiz,
	ID_TYPE id_camera) {

	jint result = 0;
	ENTER();
	UVCCamera *camera = reinterpret_cast<UVCCamera *>(id_camera);
	if (LIKELY(camera)) {
		result = camera->getRollRel();
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
// Java method correspond to this function should not be a static mathod
static jint nativeUpdateAutoContrastLimit(JNIEnv *env, jobject thiz,
	ID_TYPE id_camera) {
	jint result = JNI_ERR;
	ENTER();
	UVCCamera *camera = reinterpret_cast<UVCCamera *>(id_camera);
	if (LIKELY(camera)) {
		int min, max, def;
		result = camera->updateAutoContrastLimit(min, max, def);
		if (!result) {
			// Java側へ書き込む
			setField_int(env, thiz, "mAutoContrastMin", min);
			setField_int(env, thiz, "mAutoContrastMax", max);
			setField_int(env, thiz, "mAutoContrastDef", def);
		}
	}
	RETURN(result, jint);
}

static jint nativeSetAutoContrast(JNIEnv *env, jobject thiz,
	ID_TYPE id_camera, jboolean autocontrast) {

	jint result = JNI_ERR;
	ENTER();
	UVCCamera *camera = reinterpret_cast<UVCCamera *>(id_camera);
	if (LIKELY(camera)) {
		result = camera->setAutoContrast(autocontrast);
	}
	RETURN(result, jint);
}

static jint nativeGetAutoContrast(JNIEnv *env, jobject thiz,
	ID_TYPE id_camera) {

	jint result = JNI_ERR;
	ENTER();
	UVCCamera *camera = reinterpret_cast<UVCCamera *>(id_camera);
	if (LIKELY(camera)) {
		result = camera->getAutoContrast();
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
static jint nativeUpdateWhiteBlanceCompoLimit(JNIEnv *env, jobject thiz,
	ID_TYPE id_camera) {
	jint result = JNI_ERR;
	ENTER();
	UVCCamera *camera = reinterpret_cast<UVCCamera *>(id_camera);
	if (LIKELY(camera)) {
		int min, max, def;
		result = camera->updateWhiteBlanceCompoLimit(min, max, def);
		if (!result) {
			// Java側へ書き込む
			setField_int(env, thiz, "mWhiteBlanceCompoMin", min);
			setField_int(env, thiz, "mWhiteBlanceCompoMax", max);
			setField_int(env, thiz, "mWhiteBlanceCompoDef", def);
		}
	}
	RETURN(result, jint);
}

static jint nativeSetWhiteBlanceCompo(JNIEnv *env, jobject thiz,
	ID_TYPE id_camera, jint whiteBlance_compo) {

	jint result = JNI_ERR;
	ENTER();
	UVCCamera *camera = reinterpret_cast<UVCCamera *>(id_camera);
	if (LIKELY(camera)) {
		result = camera->setWhiteBlanceCompo(whiteBlance_compo);
	}
	RETURN(result, jint);
}

static jint nativeGetWhiteBlanceCompo(JNIEnv *env, jobject thiz,
	ID_TYPE id_camera) {

	jint result = 0;
	ENTER();
	UVCCamera *camera = reinterpret_cast<UVCCamera *>(id_camera);
	if (LIKELY(camera)) {
		result = camera->getWhiteBlanceCompo();
	}
	RETURN(result, jint);
}

//======================================================================
// Java mnethod correspond to this function should not be a static mathod
static jint nativeUpdateBacklightCompLimit(JNIEnv *env, jobject thiz,
	ID_TYPE id_camera) {
	jint result = JNI_ERR;
	ENTER();
	UVCCamera *camera = reinterpret_cast<UVCCamera *>(id_camera);
	if (LIKELY(camera)) {
		int min, max, def;
		result = camera->updateBacklightCompLimit(min, max, def);
		if (!result) {
			// Java側へ書き込む
			setField_int(env, thiz, "mBacklightCompMin", min);
			setField_int(env, thiz, "mBacklightCompMax", max);
			setField_int(env, thiz, "mBacklightCompDef", def);
		}
	}
	RETURN(result, jint);
}

static jint nativeSetBacklightComp(JNIEnv *env, jobject thiz,
	ID_TYPE id_camera, jint backlight_comp) {

	jint result = JNI_ERR;
	ENTER();
	UVCCamera *camera = reinterpret_cast<UVCCamera *>(id_camera);
	if (LIKELY(camera)) {
		result = camera->setBacklightComp(backlight_comp);
	}
	RETURN(result, jint);
}

static jint nativeGetBacklightComp(JNIEnv *env, jobject thiz,
	ID_TYPE id_camera) {

	jint result = 0;
	ENTER();
	UVCCamera *camera = reinterpret_cast<UVCCamera *>(id_camera);
	if (LIKELY(camera)) {
		result = camera->getBacklightComp();
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
// Java method correspond to this function should not be a static mathod
static jint nativeUpdateAutoHueLimit(JNIEnv *env, jobject thiz,
	ID_TYPE id_camera) {
	jint result = JNI_ERR;
	ENTER();
	UVCCamera *camera = reinterpret_cast<UVCCamera *>(id_camera);
	if (LIKELY(camera)) {
		int min, max, def;
		result = camera->updateAutoHueLimit(min, max, def);
		if (!result) {
			// Java側へ書き込む
			setField_int(env, thiz, "mAutoHueMin", min);
			setField_int(env, thiz, "mAutoHueMax", max);
			setField_int(env, thiz, "mAutoHueDef", def);
		}
	}
	RETURN(result, jint);
}

static jint nativeSetAutoHue(JNIEnv *env, jobject thiz,
	ID_TYPE id_camera, jboolean autohue) {

	jint result = JNI_ERR;
	ENTER();
	UVCCamera *camera = reinterpret_cast<UVCCamera *>(id_camera);
	if (LIKELY(camera)) {
		result = camera->setAutoHue(autohue);
	}
	RETURN(result, jint);
}

static jint nativeGetAutoHue(JNIEnv *env, jobject thiz,
	ID_TYPE id_camera) {

	jint result = JNI_ERR;
	ENTER();
	UVCCamera *camera = reinterpret_cast<UVCCamera *>(id_camera);
	if (LIKELY(camera)) {
		result = camera->getAutoHue();
	}
	RETURN(result, jint);
}

//======================================================================
// Java mnethod correspond to this function should not be a static mathod
static jint nativeUpdatePowerlineFrequencyLimit(JNIEnv *env, jobject thiz,
	ID_TYPE id_camera) {
	jint result = JNI_ERR;
	ENTER();
	UVCCamera *camera = reinterpret_cast<UVCCamera *>(id_camera);
	if (LIKELY(camera)) {
		int min, max, def;
		result = camera->updatePowerlineFrequencyLimit(min, max, def);
		if (!result) {
			// Java側へ書き込む
			setField_int(env, thiz, "mPowerlineFrequencyMin", min);
			setField_int(env, thiz, "mPowerlineFrequencyMax", max);
			setField_int(env, thiz, "mPowerlineFrequencyDef", def);
		}
	}
	RETURN(result, jint);
}

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

//======================================================================
// Java mnethod correspond to this function should not be a static mathod
static jint nativeUpdateZoomRelLimit(JNIEnv *env, jobject thiz,
	ID_TYPE id_camera) {
	jint result = JNI_ERR;
	ENTER();
	UVCCamera *camera = reinterpret_cast<UVCCamera *>(id_camera);
	if (LIKELY(camera)) {
		int min, max, def;
		result = camera->updateZoomRelLimit(min, max, def);
		if (!result) {
			// Java側へ書き込む
			setField_int(env, thiz, "mZoomRelMin", min);
			setField_int(env, thiz, "mZoomRelMax", max);
			setField_int(env, thiz, "mZoomRelDef", def);
		}
	}
	RETURN(result, jint);
}

static jint nativeSetZoomRel(JNIEnv *env, jobject thiz,
	ID_TYPE id_camera, jint zoom_rel) {

	jint result = JNI_ERR;
	ENTER();
	UVCCamera *camera = reinterpret_cast<UVCCamera *>(id_camera);
	if (LIKELY(camera)) {
		result = camera->setZoomRel(zoom_rel);
	}
	RETURN(result, jint);
}

static jint nativeGetZoomRel(JNIEnv *env, jobject thiz,
	ID_TYPE id_camera) {

	jint result = 0;
	ENTER();
	UVCCamera *camera = reinterpret_cast<UVCCamera *>(id_camera);
	if (LIKELY(camera)) {
		result = camera->getZoomRel();
	}
	RETURN(result, jint);
}

//======================================================================
// Java mnethod correspond to this function should not be a static mathod
static jint nativeUpdateDigitalMultiplierLimit(JNIEnv *env, jobject thiz,
	ID_TYPE id_camera) {
	jint result = JNI_ERR;
	ENTER();
	UVCCamera *camera = reinterpret_cast<UVCCamera *>(id_camera);
	if (LIKELY(camera)) {
		int min, max, def;
		result = camera->updateDigitalMultiplierLimit(min, max, def);
		if (!result) {
			// Java側へ書き込む
			setField_int(env, thiz, "mDigitalMultiplierMin", min);
			setField_int(env, thiz, "mDigitalMultiplierMax", max);
			setField_int(env, thiz, "mDigitalMultiplierDef", def);
		}
	}
	RETURN(result, jint);
}

static jint nativeSetDigitalMultiplier(JNIEnv *env, jobject thiz,
	ID_TYPE id_camera, jint multiplier) {

	jint result = JNI_ERR;
	ENTER();
	UVCCamera *camera = reinterpret_cast<UVCCamera *>(id_camera);
	if (LIKELY(camera)) {
		result = camera->setDigitalMultiplier(multiplier);
	}
	RETURN(result, jint);
}

static jint nativeGetDigitalMultiplier(JNIEnv *env, jobject thiz,
	ID_TYPE id_camera) {

	jint result = 0;
	ENTER();
	UVCCamera *camera = reinterpret_cast<UVCCamera *>(id_camera);
	if (LIKELY(camera)) {
		result = camera->getDigitalMultiplier();
	}
	RETURN(result, jint);
}

//======================================================================
// Java mnethod correspond to this function should not be a static mathod
static jint nativeUpdateDigitalMultiplierLimitLimit(JNIEnv *env, jobject thiz,
	ID_TYPE id_camera) {
	jint result = JNI_ERR;
	ENTER();
	UVCCamera *camera = reinterpret_cast<UVCCamera *>(id_camera);
	if (LIKELY(camera)) {
		int min, max, def;
		result = camera->updateDigitalMultiplierLimitLimit(min, max, def);
		if (!result) {
			// Java側へ書き込む
			setField_int(env, thiz, "mDigitalMultiplierLimitMin", min);
			setField_int(env, thiz, "mDigitalMultiplierLimitMax", max);
			setField_int(env, thiz, "mDigitalMultiplierLimitDef", def);
		}
	}
	RETURN(result, jint);
}

static jint nativeSetDigitalMultiplierLimit(JNIEnv *env, jobject thiz,
	ID_TYPE id_camera, jint multiplier_limit) {

	jint result = JNI_ERR;
	ENTER();
	UVCCamera *camera = reinterpret_cast<UVCCamera *>(id_camera);
	if (LIKELY(camera)) {
		result = camera->setDigitalMultiplierLimit(multiplier_limit);
	}
	RETURN(result, jint);
}

static jint nativeGetDigitalMultiplierLimit(JNIEnv *env, jobject thiz,
	ID_TYPE id_camera) {

	jint result = 0;
	ENTER();
	UVCCamera *camera = reinterpret_cast<UVCCamera *>(id_camera);
	if (LIKELY(camera)) {
		result = camera->getDigitalMultiplierLimit();
	}
	RETURN(result, jint);
}

//======================================================================
// Java mnethod correspond to this function should not be a static mathod
static jint nativeUpdateAnalogVideoStandardLimit(JNIEnv *env, jobject thiz,
	ID_TYPE id_camera) {
	jint result = JNI_ERR;
	ENTER();
	UVCCamera *camera = reinterpret_cast<UVCCamera *>(id_camera);
	if (LIKELY(camera)) {
		int min, max, def;
		result = camera->updateAnalogVideoStandardLimit(min, max, def);
		if (!result) {
			// Java側へ書き込む
			setField_int(env, thiz, "mAnalogVideoStandardMin", min);
			setField_int(env, thiz, "mAnalogVideoStandardMax", max);
			setField_int(env, thiz, "mAnalogVideoStandardDef", def);
		}
	}
	RETURN(result, jint);
}

static jint nativeSetAnalogVideoStandard(JNIEnv *env, jobject thiz,
	ID_TYPE id_camera, jint standard) {

	jint result = JNI_ERR;
	ENTER();
	UVCCamera *camera = reinterpret_cast<UVCCamera *>(id_camera);
	if (LIKELY(camera)) {
		result = camera->setAnalogVideoStandard(standard);
	}
	RETURN(result, jint);
}

static jint nativeGetAnalogVideoStandard(JNIEnv *env, jobject thiz,
	ID_TYPE id_camera) {

	jint result = 0;
	ENTER();
	UVCCamera *camera = reinterpret_cast<UVCCamera *>(id_camera);
	if (LIKELY(camera)) {
		result = camera->getAnalogVideoStandard();
	}
	RETURN(result, jint);
}

//======================================================================
// Java mnethod correspond to this function should not be a static mathod
static jint nativeUpdateAnalogVideoLockStateLimit(JNIEnv *env, jobject thiz,
	ID_TYPE id_camera) {
	jint result = JNI_ERR;
	ENTER();
	UVCCamera *camera = reinterpret_cast<UVCCamera *>(id_camera);
	if (LIKELY(camera)) {
		int min, max, def;
		result = camera->updateAnalogVideoLockStateLimit(min, max, def);
		if (!result) {
			// Java側へ書き込む
			setField_int(env, thiz, "mAnalogVideoLockStateMin", min);
			setField_int(env, thiz, "mAnalogVideoLockStateMax", max);
			setField_int(env, thiz, "mAnalogVideoLockStateDef", def);
		}
	}
	RETURN(result, jint);
}

static jint nativeSetAnalogVideoLockState(JNIEnv *env, jobject thiz,
	ID_TYPE id_camera, jint state) {

	jint result = JNI_ERR;
	ENTER();
	UVCCamera *camera = reinterpret_cast<UVCCamera *>(id_camera);
	if (LIKELY(camera)) {
		result = camera->setAnalogVideoLockState(state);
	}
	RETURN(result, jint);
}

static jint nativeGetAnalogVideoLockState(JNIEnv *env, jobject thiz,
	ID_TYPE id_camera) {

	jint result = 0;
	ENTER();
	UVCCamera *camera = reinterpret_cast<UVCCamera *>(id_camera);
	if (LIKELY(camera)) {
		result = camera->getAnalogVideoLockState();
	}
	RETURN(result, jint);
}

//======================================================================
// Java method correspond to this function should not be a static mathod
static jint nativeUpdatePrivacyLimit(JNIEnv *env, jobject thiz,
	ID_TYPE id_camera) {
	jint result = JNI_ERR;
	ENTER();
	UVCCamera *camera = reinterpret_cast<UVCCamera *>(id_camera);
	if (LIKELY(camera)) {
		int min, max, def;
		result = camera->updatePrivacyLimit(min, max, def);
		if (!result) {
			// Java側へ書き込む
			setField_int(env, thiz, "mPrivacyMin", min);
			setField_int(env, thiz, "mPrivacyMax", max);
			setField_int(env, thiz, "mPrivacyDef", def);
		}
	}
	RETURN(result, jint);
}

static jint nativeSetPrivacy(JNIEnv *env, jobject thiz,
	ID_TYPE id_camera, jboolean privacy) {

	jint result = JNI_ERR;
	ENTER();
	UVCCamera *camera = reinterpret_cast<UVCCamera *>(id_camera);
	if (LIKELY(camera)) {
		result = camera->setPrivacy(privacy ? 1: 0);
	}
	RETURN(result, jint);
}

static jint nativeGetPrivacy(JNIEnv *env, jobject thiz,
	ID_TYPE id_camera) {

	jint result = JNI_ERR;
	ENTER();
	UVCCamera *camera = reinterpret_cast<UVCCamera *>(id_camera);
	if (LIKELY(camera)) {
		result = camera->getPrivacy();
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
	//
	{ "nativeConnect",					"(JIIIIILjava/lang/String;)I", (void *) nativeConnect },
	{ "nativeRelease",					"(J)I", (void *) nativeRelease },

	{ "nativeSetStatusCallback",		"(JLcom/serenegiant/usb/IStatusCallback;)I", (void *) nativeSetStatusCallback },
	{ "nativeSetButtonCallback",		"(JLcom/serenegiant/usb/IButtonCallback;)I", (void *) nativeSetButtonCallback },

	{ "nativeGetSupportedSize",			"(J)Ljava/lang/String;", (void *) nativeGetSupportedSize },
	{ "nativeSetPreviewSize",			"(JIIIIIF)I", (void *) nativeSetPreviewSize },
	{ "nativeStartPreview",				"(J)I", (void *) nativeStartPreview },
	{ "nativeStopPreview",				"(J)I", (void *) nativeStopPreview },
	{ "nativeSetPreviewDisplay",		"(JLandroid/view/Surface;)I", (void *) nativeSetPreviewDisplay },
	{ "nativeSetFrameCallback",			"(JLcom/serenegiant/usb/IFrameCallback;I)I", (void *) nativeSetFrameCallback },

	{ "nativeSetCaptureDisplay",		"(JLandroid/view/Surface;)I", (void *) nativeSetCaptureDisplay },

	{ "nativeGetCtrlSupports",			"(J)J", (void *) nativeGetCtrlSupports },
	{ "nativeGetProcSupports",			"(J)J", (void *) nativeGetProcSupports },

	{ "nativeUpdateScanningModeLimit",	"(J)I", (void *) nativeUpdateScanningModeLimit },
	{ "nativeSetScanningMode",			"(JI)I", (void *) nativeSetScanningMode },
	{ "nativeGetScanningMode",			"(J)I", (void *) nativeGetScanningMode },
			
	{ "nativeUpdateExposureModeLimit",	"(J)I", (void *) nativeUpdateExposureModeLimit },
	{ "nativeSetExposureMode",			"(JI)I", (void *) nativeSetExposureMode },
	{ "nativeGetExposureMode",			"(J)I", (void *) nativeGetExposureMode },

	{ "nativeUpdateExposurePriorityLimit","(J)I", (void *) nativeUpdateExposurePriorityLimit },
	{ "nativeSetExposurePriority",		"(JI)I", (void *) nativeSetExposurePriority },
	{ "nativeGetExposurePriority",		"(J)I", (void *) nativeGetExposurePriority },
			
	{ "nativeUpdateExposureLimit",		"(J)I", (void *) nativeUpdateExposureLimit },
	{ "nativeSetExposure",				"(JI)I", (void *) nativeSetExposure },
	{ "nativeGetExposure",				"(J)I", (void *) nativeGetExposure },
			
	{ "nativeUpdateExposureRelLimit",	"(J)I", (void *) nativeUpdateExposureRelLimit },
	{ "nativeSetExposureRel",			"(JI)I", (void *) nativeSetExposureRel },
	{ "nativeGetExposureRel",			"(J)I", (void *) nativeGetExposureRel },
			
	{ "nativeUpdateAutoFocusLimit",		"(J)I", (void *) nativeUpdateAutoFocusLimit },
	{ "nativeSetAutoFocus",				"(JZ)I", (void *) nativeSetAutoFocus },
	{ "nativeGetAutoFocus",				"(J)I", (void *) nativeGetAutoFocus },

	{ "nativeUpdateFocusLimit",			"(J)I", (void *) nativeUpdateFocusLimit },
	{ "nativeSetFocus",					"(JI)I", (void *) nativeSetFocus },
	{ "nativeGetFocus",					"(J)I", (void *) nativeGetFocus },

	{ "nativeUpdateFocusRelLimit",		"(J)I", (void *) nativeUpdateFocusRelLimit },
	{ "nativeSetFocusRel",				"(JI)I", (void *) nativeSetFocusRel },
	{ "nativeGetFocusRel",				"(J)I", (void *) nativeGetFocusRel },
	
//	{ "nativeUpdateFocusSimpleLimit",	"(J)I", (void *) nativeUpdateFocusSimpleLimit },
//	{ "nativeSetFocusSimple",			"(JI)I", (void *) nativeSetFocusSimple },
//	{ "nativeGetFocusSimple",			"(J)I", (void *) nativeGetFocusSimple },
			
	{ "nativeUpdateIrisLimit",			"(J)I", (void *) nativeUpdateIrisLimit },
	{ "nativeSetIris",					"(JI)I", (void *) nativeSetIris },
	{ "nativeGetIris",					"(J)I", (void *) nativeGetIris },
	
	{ "nativeUpdateIrisRelLimit",		"(J)I", (void *) nativeUpdateIrisRelLimit },
	{ "nativeSetIrisRel",				"(JI)I", (void *) nativeSetIrisRel },
	{ "nativeGetIrisRel",				"(J)I", (void *) nativeGetIrisRel },
	
	{ "nativeUpdatePanLimit",			"(J)I", (void *) nativeUpdatePanLimit },
	{ "nativeSetPan",					"(JI)I", (void *) nativeSetPan },
	{ "nativeGetPan",					"(J)I", (void *) nativeGetPan },
	
	{ "nativeUpdateTiltLimit",			"(J)I", (void *) nativeUpdateTiltLimit },
	{ "nativeSetTilt",					"(JI)I", (void *) nativeSetTilt },
	{ "nativeGetTilt",					"(J)I", (void *) nativeGetTilt },
	
	{ "nativeUpdateRollLimit",			"(J)I", (void *) nativeUpdateRollLimit },
	{ "nativeSetRoll",					"(JI)I", (void *) nativeSetRoll },
	{ "nativeGetRoll",					"(J)I", (void *) nativeGetRoll },
	
	{ "nativeUpdatePanRelLimit",		"(J)I", (void *) nativeUpdatePanRelLimit },
	{ "nativeSetPanRel",				"(JI)I", (void *) nativeSetPanRel },
	{ "nativeGetPanRel",				"(J)I", (void *) nativeGetPanRel },
	
	{ "nativeUpdateTiltRelLimit",		"(J)I", (void *) nativeUpdateTiltRelLimit },
	{ "nativeSetTiltRel",				"(JI)I", (void *) nativeSetTiltRel },
	{ "nativeGetTiltRel",				"(J)I", (void *) nativeGetTiltRel },
	
	{ "nativeUpdateRollRelLimit",		"(J)I", (void *) nativeUpdateRollRelLimit },
	{ "nativeSetRollRel",				"(JI)I", (void *) nativeSetRollRel },
	{ "nativeGetRollRel",				"(J)I", (void *) nativeGetRollRel },
	
	{ "nativeUpdateAutoWhiteBlanceLimit","(J)I", (void *) nativeUpdateAutoWhiteBlanceLimit },
	{ "nativeSetAutoWhiteBlance",		"(JZ)I", (void *) nativeSetAutoWhiteBlance },
	{ "nativeGetAutoWhiteBlance",		"(J)I", (void *) nativeGetAutoWhiteBlance },

	{ "nativeUpdateAutoWhiteBlanceCompoLimit","(J)I", (void *) nativeUpdateAutoWhiteBlanceCompoLimit },
	{ "nativeSetAutoWhiteBlanceCompo",		"(JZ)I", (void *) nativeSetAutoWhiteBlanceCompo },
	{ "nativeGetAutoWhiteBlanceCompo",		"(J)I", (void *) nativeGetAutoWhiteBlanceCompo },
	
	{ "nativeUpdateWhiteBlanceLimit",	"(J)I", (void *) nativeUpdateWhiteBlanceLimit },
	{ "nativeSetWhiteBlance",			"(JI)I", (void *) nativeSetWhiteBlance },
	{ "nativeGetWhiteBlance",			"(J)I", (void *) nativeGetWhiteBlance },

	{ "nativeUpdateWhiteBlanceCompoLimit","(J)I", (void *) nativeUpdateWhiteBlanceCompoLimit },
	{ "nativeSetWhiteBlanceCompo",		"(JI)I", (void *) nativeSetWhiteBlanceCompo },
	{ "nativeGetWhiteBlanceCompo",		"(J)I", (void *) nativeGetWhiteBlanceCompo },
	
	{ "nativeUpdateBacklightCompLimit",	"(J)I", (void *) nativeUpdateBacklightCompLimit },
	{ "nativeSetBacklightComp",			"(JI)I", (void *) nativeSetBacklightComp },
	{ "nativeGetBacklightComp",			"(J)I", (void *) nativeGetBacklightComp },

	{ "nativeUpdateBrightnessLimit",	"(J)I", (void *) nativeUpdateBrightnessLimit },
	{ "nativeSetBrightness",			"(JI)I", (void *) nativeSetBrightness },
	{ "nativeGetBrightness",			"(J)I", (void *) nativeGetBrightness },

	{ "nativeUpdateContrastLimit",		"(J)I", (void *) nativeUpdateContrastLimit },
	{ "nativeSetContrast",				"(JI)I", (void *) nativeSetContrast },
	{ "nativeGetContrast",				"(J)I", (void *) nativeGetContrast },

	{ "nativeUpdateAutoContrastLimit",	"(J)I", (void *) nativeUpdateAutoContrastLimit },
	{ "nativeSetAutoContrast",			"(JZ)I", (void *) nativeSetAutoContrast },
	{ "nativeGetAutoContrast",			"(J)I", (void *) nativeGetAutoContrast },

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

	{ "nativeUpdateAutoHueLimit",		"(J)I", (void *) nativeUpdateAutoHueLimit },
	{ "nativeSetAutoHue",				"(JZ)I", (void *) nativeSetAutoHue },
	{ "nativeGetAutoHue",				"(J)I", (void *) nativeGetAutoHue },
			
	{ "nativeUpdatePowerlineFrequencyLimit","(J)I", (void *) nativeUpdatePowerlineFrequencyLimit },
	{ "nativeSetPowerlineFrequency",	"(JI)I", (void *) nativeSetPowerlineFrequency },
	{ "nativeGetPowerlineFrequency",	"(J)I", (void *) nativeGetPowerlineFrequency },

	{ "nativeUpdateZoomLimit",			"(J)I", (void *) nativeUpdateZoomLimit },
	{ "nativeSetZoom",					"(JI)I", (void *) nativeSetZoom },
	{ "nativeGetZoom",					"(J)I", (void *) nativeGetZoom },
	
	{ "nativeUpdateZoomRelLimit",		"(J)I", (void *) nativeUpdateZoomRelLimit },
	{ "nativeSetZoomRel",				"(JI)I", (void *) nativeSetZoomRel },
	{ "nativeGetZoomRel",				"(J)I", (void *) nativeGetZoomRel },
	
	{ "nativeUpdateDigitalMultiplierLimit","(J)I", (void *) nativeUpdateDigitalMultiplierLimit },
	{ "nativeSetDigitalMultiplier","(JI)I", (void *) nativeSetDigitalMultiplier },
	{ "nativeGetDigitalMultiplier","(J)I", (void *) nativeGetDigitalMultiplier },
	
	{ "nativeUpdateDigitalMultiplierLimitLimit","(J)I", (void *) nativeUpdateDigitalMultiplierLimitLimit },
	{ "nativeSetDigitalMultiplierLimit","(JI)I", (void *) nativeSetDigitalMultiplierLimit },
	{ "nativeGetDigitalMultiplierLimit","(J)I", (void *) nativeGetDigitalMultiplierLimit },
	
	{ "nativeUpdateAnalogVideoStandardLimit","(J)I", (void *) nativeUpdateAnalogVideoStandardLimit },
	{ "nativeSetAnalogVideoStandard",		"(JI)I", (void *) nativeSetAnalogVideoStandard },
	{ "nativeGetAnalogVideoStandard",		"(J)I", (void *) nativeGetAnalogVideoStandard },
	
	{ "nativeUpdateAnalogVideoLockStateLimit","(J)I", (void *) nativeUpdateAnalogVideoLockStateLimit },
	{ "nativeSetAnalogVideoLoackState",	"(JI)I", (void *) nativeSetAnalogVideoLockState },
	{ "nativeGetAnalogVideoLoackState",	"(J)I", (void *) nativeGetAnalogVideoLockState },
	
	{ "nativeUpdatePrivacyLimit",		"(J)I", (void *) nativeUpdatePrivacyLimit },
	{ "nativeSetPrivacy",				"(JZ)I", (void *) nativeSetPrivacy },
	{ "nativeGetPrivacy",				"(J)I", (void *) nativeGetPrivacy },
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
