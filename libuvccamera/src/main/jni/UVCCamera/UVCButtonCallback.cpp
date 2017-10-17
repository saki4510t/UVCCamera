#include <stdlib.h>
#include <linux/time.h>
#include <unistd.h>
#include "utilbase.h"
#include "UVCButtonCallback.h"
#include "libuvc_internal.h"

#define	LOCAL_DEBUG 0

UVCButtonCallback::UVCButtonCallback(uvc_device_handle_t *devh)
:	mDeviceHandle(devh),
	mButtonCallbackObj(NULL) {

	ENTER();
	pthread_mutex_init(&button_mutex, NULL);

	uvc_set_button_callback(mDeviceHandle, uvc_button_callback, (void *)this);
	EXIT();
}

UVCButtonCallback::~UVCButtonCallback() {

	ENTER();
	pthread_mutex_destroy(&button_mutex);
	EXIT();
}

int UVCButtonCallback::setCallback(JNIEnv *env, jobject button_callback_obj) {
	
	ENTER();
	pthread_mutex_lock(&button_mutex);
	{
		if (!env->IsSameObject(mButtonCallbackObj, button_callback_obj)) {
			ibuttoncallback_fields.onButton = NULL;
			if (mButtonCallbackObj) {
				env->DeleteGlobalRef(mButtonCallbackObj);
			}
			mButtonCallbackObj = button_callback_obj;
			if (button_callback_obj) {
				// get method IDs of Java object for callback
				jclass clazz = env->GetObjectClass(button_callback_obj);
				if (LIKELY(clazz)) {
					ibuttoncallback_fields.onButton = env->GetMethodID(clazz,
						"onButton",	"(II)V");
				} else {
					LOGW("failed to get object class");
				}
				env->ExceptionClear();
				if (!ibuttoncallback_fields.onButton) {
					LOGE("Can't find IButtonCallback#onButton");
					env->DeleteGlobalRef(button_callback_obj);
					mButtonCallbackObj = button_callback_obj = NULL;
				}
			}
		}
	}
	pthread_mutex_unlock(&button_mutex);
	RETURN(0, int);
}

void UVCButtonCallback::notifyButtonCallback(JNIEnv* env, int button, int state) {

	pthread_mutex_lock(&button_mutex);
	{
		if (mButtonCallbackObj) {
			env->CallVoidMethod(mButtonCallbackObj, ibuttoncallback_fields.onButton, button, state);
			env->ExceptionClear();
		}
	}
	pthread_mutex_unlock(&button_mutex);
}

void UVCButtonCallback::uvc_button_callback(int button, int state, void *user_ptr) {

	UVCButtonCallback *buttonCallback = reinterpret_cast<UVCButtonCallback *>(user_ptr);

	JavaVM *vm = getVM();
	JNIEnv *env;
	// attach to JavaVM
	vm->AttachCurrentThread(&env, NULL);

	buttonCallback->notifyButtonCallback(env, button, state);
	
	vm->DetachCurrentThread();
}
