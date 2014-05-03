/*
 * UVCCamera
 * library and sample to access to UVC web camera on non-rooted Android device
 *
 * Copyright (c) 2014 saki t_saki@serenegiant.com
 *
 * File name: UVCPreview.cpp
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

#include <stdlib.h>
#include <linux/time.h>
#include <unistd.h>
#include "UVCPreview.h"
#include "libuvc_internal.h"

#define	LOCAL_DEBUG 0
#define MAX_FRAME 4

UVCPreview::UVCPreview(uvc_device_handle_t *devh)
:	mPreviewWindow(NULL),
	mDeviceHandle(devh),
	requestWidth(DEFAULT_PREVIEW_WIDTH),
	requestHeight(DEFAULT_PREVIEW_HEIGHT),
	requestFps(DEFAULT_PREVIEW_FPS),
	frameWidth(DEFAULT_PREVIEW_WIDTH),
	frameHeight(DEFAULT_PREVIEW_HEIGHT),
	frameBytes(DEFAULT_PREVIEW_WIDTH * DEFAULT_PREVIEW_HEIGHT * 2),
	previewBytes(DEFAULT_PREVIEW_WIDTH * DEFAULT_PREVIEW_HEIGHT * 2),
	previewFormat(WINDOW_FORMAT_RGB_565),
	mIsRunning(false) {

	ENTER();
	pthread_mutex_init(&window_mutex, NULL);
	pthread_cond_init(&preview_sync, NULL);
	pthread_mutex_init(&preview_mutex, NULL);
	EXIT();
}

UVCPreview::~UVCPreview() {

	ENTER();
	if (mPreviewWindow)
		ANativeWindow_release(mPreviewWindow);
	mPreviewWindow = NULL;
	pthread_mutex_destroy(&preview_mutex);
	pthread_cond_destroy(&preview_sync);
	pthread_mutex_destroy(&window_mutex);
	EXIT();
}

inline const bool UVCPreview::isRunning() const {return mIsRunning; }

int UVCPreview::setPreviewDisplay(ANativeWindow *preview_window) {
	ENTER();
	pthread_mutex_lock(&window_mutex);
	{
		if (mPreviewWindow != preview_window) {
			if (mPreviewWindow)
				ANativeWindow_release(mPreviewWindow);
			mPreviewWindow = preview_window;
			if (LIKELY(mPreviewWindow)) {
				ANativeWindow_acquire(mPreviewWindow);
				ANativeWindow_setBuffersGeometry(mPreviewWindow,
					frameWidth, frameHeight, previewFormat);
			}
		}
	}
	pthread_mutex_unlock(&window_mutex);
	RETURN(0, int);
}

void UVCPreview::clearDisplay() {
	pthread_mutex_lock(&window_mutex);
	{
		ANativeWindow_Buffer buffer;
		if (LIKELY(mPreviewWindow)) {
			if (LIKELY(ANativeWindow_lock(mPreviewWindow, &buffer, NULL) == 0)) {
				uint8_t *dest = (uint8_t *)buffer.bits;
				const size_t bytes = buffer.width * 2;
				const int stride = buffer.stride * 2;
				for (int i = 0; i < buffer.height; i++) {
					memset(dest, 0, bytes);
					dest += stride;
				}
				ANativeWindow_unlockAndPost(mPreviewWindow);
			}
		}
	}
	pthread_mutex_unlock(&window_mutex);
}

int UVCPreview::startPreview() {
	ENTER();

	int result = EXIT_FAILURE;
	if (!isRunning()) {
		pthread_mutex_lock(&window_mutex);
		{
			if (LIKELY(mPreviewWindow)) {
				mIsRunning = true;
				result = pthread_create(&preview_thread, NULL, preview_thread_func, (void *)this);
			}
		}
		pthread_mutex_unlock(&window_mutex);
		if (UNLIKELY(result != EXIT_SUCCESS)) {
			LOGW("UVCCamera::window does not exist/already running/could not create thread etc.");
			mIsRunning = false;
			pthread_mutex_lock(&preview_mutex);
			{
				pthread_cond_signal(&preview_sync);
			}
			pthread_mutex_unlock(&preview_mutex);
		}
	}
	RETURN(result, int);
}

int UVCPreview::stopPreview() {
	ENTER();
	if (LIKELY(isRunning())) {
		pthread_mutex_lock(&preview_mutex);
		{
			mIsRunning = false;
			pthread_cond_signal(&preview_sync);
		}
		pthread_mutex_unlock(&preview_mutex);
		if (pthread_join(preview_thread, NULL) != EXIT_SUCCESS) {
			LOGW("UVCCamera::terminate draw thread: pthread_join failed");
		}
		clearDisplay();
	}
	clearPreviewFrame();
	if (mPreviewWindow) {
		MARK("ANativeWindow_release:mPreviewWindow");
		ANativeWindow_release(mPreviewWindow);
		mPreviewWindow = NULL;
	}
	RETURN(0, int);
}

//**********************************************************************
//
//**********************************************************************
void UVCPreview::uvc_preview_frame_callback(uvc_frame_t *frame, void *vptr_args) {
	UVCPreview *preview = reinterpret_cast<UVCPreview *>(vptr_args);
	if (UNLIKELY(!frame || !frame->frame_format || !frame->data
		|| !frame->width || !frame->height
		|| (frame->data_bytes < preview->frameBytes))) {
#if LOCAL_DEBUG
		LOGW("broken frame!");
#endif
		return;
	}
	if (LIKELY(preview->isRunning())) {
		uvc_frame_t *copy = uvc_allocate_frame(frame->data_bytes);
		if (UNLIKELY(!copy)) {
#if LOCAL_DEBUG
			LOGE("uvc_callback:unable to allocate duplicate frame!");
#endif
			return;
		}
		uvc_error_t ret = uvc_duplicate_frame(frame, copy);
		if (UNLIKELY(ret)) {
			uvc_free_frame(copy);
			return;
		}
		preview->addPreviewFrame(copy);
	}
}

void UVCPreview::addPreviewFrame(uvc_frame_t *frame) {

	pthread_mutex_lock(&preview_mutex);
	if (isRunning() && (previewFrames.size() < MAX_FRAME)) {
		previewFrames.put(frame);
		frame = NULL;
		pthread_cond_signal(&preview_sync);
	}
	pthread_mutex_unlock(&preview_mutex);
	if (frame) {
		uvc_free_frame(frame);
	}
}

uvc_frame_t *UVCPreview::waitPreviewFrame() {
	uvc_frame_t *frame = NULL;
	pthread_mutex_lock(&preview_mutex);
	{
		if (!previewFrames.size()) {
			pthread_cond_wait(&preview_sync, &preview_mutex);
		}
		if (LIKELY(isRunning() && previewFrames.size() > 0)) {
			frame = previewFrames.remove(0);
		}
	}
	return frame;
}

void UVCPreview::clearPreviewFrame() {
	pthread_mutex_lock(&preview_mutex);
	{
		for (int i = 0; i < previewFrames.size(); i++)
			uvc_free_frame(previewFrames[i]);
		previewFrames.clear();
	}
	pthread_mutex_unlock(&preview_mutex);
}

void *UVCPreview::preview_thread_func(void *vptr_args) {
	int result;

	ENTER();
	UVCPreview *preview = reinterpret_cast<UVCPreview *>(vptr_args);
	if (LIKELY(preview)) {
		uvc_stream_ctrl_t ctrl;
		result = preview->prepare_preview(&ctrl);
		if (LIKELY(!result)) {
			preview->do_preview(&ctrl);
		}
	}
	PRE_EXIT();
	pthread_exit(NULL);
}

int UVCPreview::prepare_preview(uvc_stream_ctrl_t *ctrl) {
	uvc_error_t result;

	ENTER();
	result = uvc_get_stream_ctrl_format_size(mDeviceHandle, ctrl,
		UVC_FRAME_FORMAT_YUYV,
		requestWidth, requestHeight, requestFps
	);
	if (LIKELY(!result)) {
		uvc_print_stream_ctrl(ctrl, stderr);

		uvc_frame_desc_t *frame_desc;
		result = uvc_get_frame_desc(mDeviceHandle, ctrl, &frame_desc);
		if (LIKELY(!result)) {
			frameWidth = frame_desc->wWidth;
			frameHeight = frame_desc->wHeight;
			LOGI("frameSize=(%d,%d)", frameWidth, frameHeight);
			pthread_mutex_lock(&window_mutex);
			if (LIKELY(mPreviewWindow)) {
				ANativeWindow_setBuffersGeometry(mPreviewWindow,
					frameWidth, frameHeight, previewFormat);
			}
			pthread_mutex_unlock(&window_mutex);
		} else {
			frameWidth = requestWidth;
			frameHeight = requestHeight;
		}
		frameBytes = frameWidth * frameHeight * 2;
		previewBytes = frameWidth * frameHeight * 2;
	} else {
		LOGE("could not negotiate with camera:err=%d", result);
	}
	RETURN(result, int);
}

void UVCPreview::do_preview(uvc_stream_ctrl_t *ctrl) {
	ENTER();

	uvc_frame_t *frame = NULL;
	uvc_error_t result = uvc_start_iso_streaming(
		mDeviceHandle, ctrl, uvc_preview_frame_callback, (void *)this);

	if (LIKELY(!result)) {
		clearPreviewFrame();
		LOGI("Streaming...");
		while (LIKELY(isRunning())) {
			frame = waitPreviewFrame();
			pthread_mutex_unlock(&preview_mutex);
			if (LIKELY(frame)) {
				draw_frame_one(frame, &mPreviewWindow, uvc_any2rgb565, 2);
			}
		}
		LOGI("preview_thread_func:wait for all callbacks complete");
		uvc_stop_streaming(mDeviceHandle);
		LOGI("Streaming finished");
	} else {
		uvc_perror(result, "failed start_streaming");
	}

	EXIT();
}

void UVCPreview::draw_frame_one(uvc_frame_t *frame, ANativeWindow **window, convFunc_t convert_func, int pixcelBytes) {
	uvc_error_t ret;
	ANativeWindow_Buffer buffer;

	pthread_mutex_lock(&window_mutex);

	if (LIKELY(*window)) {
		uvc_frame_t *converted;
		if (convert_func)
			converted = uvc_allocate_frame(frame->width * frame->height * pixcelBytes);
		else
			converted = frame;
		if (LIKELY(converted)) {
			ret = convert_func(frame, converted);
			if (LIKELY(!ret)) {
				if (LIKELY(ANativeWindow_lock(*window, &buffer, NULL) == 0)) {
					uint8_t *dest = (uint8_t *)buffer.bits;
					uint8_t *src = (uint8_t *)converted->data;
					const int w = (converted->width < buffer.width ? converted->width : buffer.width) * pixcelBytes;
					const int h = converted->height < buffer.height ? converted->height : buffer.height;
					const int src_step = converted->width * pixcelBytes;
					const int dest_step = buffer.stride * pixcelBytes;
					int h8 = h % 8;
					for (int i = 0; i < h8; i++) {
						memcpy(dest, src, w);
						dest += dest_step; src += src_step;
					}
					for (int i = 0; i < h; i += 8) {
						memcpy(dest, src, w);
						dest += dest_step; src += src_step;
						memcpy(dest, src, w);
						dest += dest_step; src += src_step;
						memcpy(dest, src, w);
						dest += dest_step; src += src_step;
						memcpy(dest, src, w);
						dest += dest_step; src += src_step;
						memcpy(dest, src, w);
						dest += dest_step; src += src_step;
						memcpy(dest, src, w);
						dest += dest_step; src += src_step;
						memcpy(dest, src, w);
						dest += dest_step; src += src_step;
						memcpy(dest, src, w);
						dest += dest_step; src += src_step;
					}
					ANativeWindow_unlockAndPost(*window);
				}
			} else {
				LOGW("failed converting");
			}
			if (convert_func)
				uvc_free_frame(converted);
		} else {
			LOGW("draw_frame_one:unable to allocate converted frame!");
		}
	}
	pthread_mutex_unlock(&window_mutex);
}
