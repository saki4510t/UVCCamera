#include "webcam.h"
#include "yuv.h"
#include "util.h"
#include "video_device.h"
#include "capture.h"

#include <android/bitmap.h>
#include <malloc.h>

void Java_com_ford_openxc_webcam_webcam_NativeWebcam_loadNextFrame(JNIEnv* env,
        jobject thiz, jobject bitmap) {
    AndroidBitmapInfo info;
    int result;
    if((result = AndroidBitmap_getInfo(env, bitmap, &info)) < 0) {
        LOGE("AndroidBitmap_getInfo() failed, error=%d", result);
        return;
    }

    if(info.format != ANDROID_BITMAP_FORMAT_RGBA_8888) {
        LOGE("Bitmap format is not RGBA_8888 !");
        return;
    }

    int* colors;
    if((result = AndroidBitmap_lockPixels(env, bitmap, (void*)&colors)) < 0) {
        LOGE("AndroidBitmap_lockPixels() failed, error=%d", result);
    }

    if(!RGB_BUFFER || !Y_BUFFER) {
        LOGE("Unable to load frame, buffers not initialized");
        return;
    }

    process_camera(DEVICE_DESCRIPTOR, FRAME_BUFFERS, info.width, info.height,
            RGB_BUFFER, Y_BUFFER);

    int *lrgb = &RGB_BUFFER[0];
    for(int i = 0; i < info.width * info.height; i++) {
        *colors++ = *lrgb++;
    }

    AndroidBitmap_unlockPixels(env, bitmap);
}

jint Java_com_ford_openxc_webcam_webcam_NativeWebcam_startCamera(JNIEnv* env,
        jobject thiz, jstring deviceName, jint width, jint height) {
    const char* dev_name = (*env)->GetStringUTFChars(env, deviceName, 0);
    int result = open_device(dev_name, &DEVICE_DESCRIPTOR);
    (*env)->ReleaseStringUTFChars(env, deviceName, dev_name);
    if(result == ERROR_LOCAL) {
        return result;
    }

    result = init_device(DEVICE_DESCRIPTOR, width, height);
    if(result == ERROR_LOCAL) {
        return result;
    }

    result = start_capture(DEVICE_DESCRIPTOR);
    if(result != SUCCESS_LOCAL) {
        stop_camera(&DEVICE_DESCRIPTOR, RGB_BUFFER, Y_BUFFER);
        LOGE("Unable to start capture, resetting device");
    } else {
        int area = width * height;
        RGB_BUFFER = (int*)malloc(sizeof(int) * area);
        Y_BUFFER = (int*)malloc(sizeof(int) * area);
    }

    return result;
}

void Java_com_ford_openxc_webcam_webcam_NativeWebcam_stopCamera(JNIEnv* env,
        jobject thiz) {
    stop_camera(&DEVICE_DESCRIPTOR, RGB_BUFFER, Y_BUFFER);
}

jboolean Java_com_ford_openxc_webcam_webcam_NativeWebcam_cameraAttached(JNIEnv* env,
        jobject thiz) {
    return DEVICE_DESCRIPTOR != -1;
}

jint JNI_OnLoad(JavaVM* vm, void* reserved) {
    cache_yuv_lookup_table(YUV_TABLE);
    return JNI_VERSION_1_6;
}
