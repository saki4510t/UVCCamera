#ifndef UVCBUTTONCALLBACK_H_
#define UVCBUTTONCALLBACK_H_

#include "libUVCCamera.h"
#include <pthread.h>
#include <android/native_window.h>
#include "objectarray.h"

#pragma interface

// for callback to Java object
typedef struct {
	jmethodID onButton;
} Fields_ibuttoncallback;

class UVCButtonCallback {
private:
	uvc_device_handle_t *mDeviceHandle;
 	pthread_mutex_t button_mutex;
 	jobject mButtonCallbackObj;
 	Fields_ibuttoncallback ibuttoncallback_fields;
 	void notifyButtonCallback(JNIEnv *env, int button, int state);
 	static void uvc_button_callback(int button, int state, void *user_ptr);
public:
	UVCButtonCallback(uvc_device_handle_t *devh);
	~UVCButtonCallback();

	int setCallback(JNIEnv *env, jobject button_callback_obj);
};

#endif /* UVCBUTTONCALLBACK_H_ */
