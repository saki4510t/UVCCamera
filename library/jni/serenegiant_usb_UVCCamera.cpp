/*
 * UVCCamera
 * library and sample to access to UVC web camera on non-rooted Android device
 *
 * Copyright (c) 2014 saki t_saki@serenegiant.com
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
 * Files in the jni/libjpeg, jni/libusb and jin/libuvc folder may have a different license, see the respective files.
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
	jint vid, jint pid, jint fd) {

	ENTER();
	int result = JNI_ERR;
	UVCCamera *camera = reinterpret_cast<UVCCamera *>(id_camera);
	if (LIKELY(camera && (fd > 0))) {
		 result =  camera->connect(vid, pid, fd);
	}
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
	{ "nativeCreate",			"()J", (void *) nativeCreate },
	{ "nativeDestroy",			"(J)V", (void *) nativeDestroy },

	{ "nativeConnect",			"(JIII)I", (void *) nativeConnect },
	{ "nativeRelease",			"(J)I", (void *) nativeRelease },

	{ "nativeStartPreview",		"(J)I", (void *) nativeStartPreview },
	{ "nativeStopPreview",		"(J)I", (void *) nativeStopPreview },
	{ "nativeSetPreviewDisplay","(JLandroid/view/Surface;)I", (void *) nativeSetPreviewDisplay },
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
