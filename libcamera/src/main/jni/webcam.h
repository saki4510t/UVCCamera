#ifndef __WEBCAM_H__
#define __WEBCAM_H__

#include <jni.h>

#include "util.h"

static int DEVICE_DESCRIPTOR = -1;
int* RGB_BUFFER = NULL;
int* Y_BUFFER = NULL;

// These are documented on the Java side, in NativeWebcam
jint Java_com_ford_openxc_webcam_webcam_NativeWebcam_startCamera(JNIEnv* env,
        jobject thiz, jstring deviceName, jint width, jint height);
void Java_com_ford_openxc_webcam_webcam_NativeWebcam_loadNextFrame(JNIEnv* env,
        jobject thiz, jobject bitmap);
jboolean Java_com_ford_openxc_webcam_webcam_NativeWebcam_cameraAttached(JNIEnv* env,
        jobject thiz);
void Java_com_ford_openxc_webcam_webcam_NativeWebcam_stopCamera(JNIEnv* env,
        jobject thiz);

#endif // __WEBCAM_H__
