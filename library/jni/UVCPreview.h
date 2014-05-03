/*
 * UVCCamera
 * library and sample to access to UVC web camera on non-rooted Android device
 *
 * Copyright (c) 2014 saki t_saki@serenegiant.com
 *
 * File name: UVCPreview.h
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

#ifndef UVCPREVIEW_H_
#define UVCPREVIEW_H_

#include "libUVCCamera.h"
#include <pthread.h>
#include <android/native_window.h>
#include "objectarray.h"

#pragma interface

#define DEFAULT_PREVIEW_WIDTH 640
#define DEFAULT_PREVIEW_HEIGHT 480
#define DEFAULT_PREVIEW_FPS 30

typedef uvc_error_t (*convFunc_t)(uvc_frame_t *in, uvc_frame_t *out);

class UVCPreview {
private:
	uvc_device_handle_t *mDeviceHandle;
	pthread_mutex_t window_mutex;
	ANativeWindow *mPreviewWindow;
	volatile bool mIsRunning;
	int requestWidth, requestHeight, requestFps;
	int frameWidth, frameHeight;
	size_t frameBytes;
	pthread_t preview_thread;
	pthread_mutex_t preview_mutex;
	pthread_cond_t preview_sync;
	ObjectArray<uvc_frame_t *> previewFrames;
	int previewFormat;
	size_t previewBytes;
	void clearDisplay();
	static void uvc_preview_frame_callback(uvc_frame_t *frame, void *vptr_args);
	void addPreviewFrame(uvc_frame_t *frame);
	uvc_frame_t *waitPreviewFrame();
	void clearPreviewFrame();
	static void *preview_thread_func(void *vptr_args);
	int prepare_preview(uvc_stream_ctrl_t *ctrl);
	void do_preview(uvc_stream_ctrl_t *ctrl);
	void draw_frame_one(uvc_frame_t *frame, ANativeWindow **window, convFunc_t func, int pixelBytes);
public:
	UVCPreview(uvc_device_handle_t *devh);
	~UVCPreview();

	inline const bool isRunning() const;
	int setPreviewSize(int width, int height, int fps);
	int setPreviewDisplay(ANativeWindow *preview_window);
	int startPreview();
	int stopPreview();
};

#endif /* UVCPREVIEW_H_ */
