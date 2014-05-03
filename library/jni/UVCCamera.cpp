/*
 * UVCCamera
 * library and sample to access to UVC web camera on non-rooted Android device
 *
 * Copyright (c) 2014 saki t_saki@serenegiant.com
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
 * Files in the jni/libjpeg, jni/libusb and jin/libuvc folder may have a different license, see the respective files.
*/

//**********************************************************************
//
//**********************************************************************
#include <stdlib.h>
#include <linux/time.h>
#include <unistd.h>
#include <string.h>
#include "UVCCamera.h"
#include "libuvc_internal.h"

#define	LOCAL_DEBUG 1

//**********************************************************************
//
//**********************************************************************
UVCCamera::UVCCamera()
:	  mDevice(NULL),
	  mDeviceHandle(NULL),
	  mPreview(NULL) {

	ENTER();
	uvc_error_t result = uvc_init(&mContext, NULL);
	if (UNLIKELY(result < 0)) {
		LOGD("failed to init libuvc");
	}
	EXIT();
}

UVCCamera::~UVCCamera() {
	ENTER();
	release();
	uvc_exit(mContext);
	EXIT();
}

int UVCCamera::connect(int vid, int pid, int fd) {
	ENTER();
	uvc_error_t result = UVC_ERROR_BUSY;
	if (!mDeviceHandle) {
		result = uvc_find_device2(mContext, &mDevice, vid, pid, NULL, fd);
		if (LIKELY(!result)) {
			result = uvc_open(mDevice, &mDeviceHandle);
			if (LIKELY(!result)) {
#if LOCAL_DEBUG
				uvc_print_diag(mDeviceHandle, stderr);
#endif
				mPreview = new UVCPreview(mDeviceHandle);
			} else {
				LOGE("could not open camera:err=%d", result);
				uvc_unref_device(mDevice);
				SAFE_DELETE(mDevice);
				mDeviceHandle = NULL;
			}
		} else {
			LOGE("could not find camera:err=%d", result);
		}
	} else {
		LOGW("camera is already opened. you should release first");
	}
	RETURN(result, int);
}

int UVCCamera::release() {
	ENTER();
	stopPreview();
	if (LIKELY(mDeviceHandle)) {
		SAFE_DELETE(mPreview);
		uvc_close(mDeviceHandle);
		mDeviceHandle = NULL;
	}
	if (LIKELY(mDevice)) {
		uvc_unref_device(mDevice);
		mDevice = NULL;
	}
	RETURN(0, int);
}

int UVCCamera::setPreviewDisplay(ANativeWindow *preview_window) {
	ENTER();
	if (mPreview) {
		mPreview->setPreviewDisplay(preview_window);
	}
	RETURN(0, int);
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

