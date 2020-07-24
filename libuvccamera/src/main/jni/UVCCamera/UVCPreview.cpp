/*
 * UVCCamera
 * library and sample to access to UVC web camera on non-rooted Android device
 *
 * Copyright (c) 2014-2017 saki t_saki@serenegiant.com
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
 * Files in the jni/libjpeg, jni/libusb, jin/libuvc, jni/rapidjson folder may have a different license, see the respective files.
*/

#include <stdlib.h>
#include <linux/time.h>
#include <unistd.h>

#if 1	// set 1 if you don't need debug log
	#ifndef LOG_NDEBUG
		#define	LOG_NDEBUG		// w/o LOGV/LOGD/MARK
	#endif
	#undef USE_LOGALL
#else
	#define USE_LOGALL
	#undef LOG_NDEBUG
//	#undef NDEBUG
#endif

#include "utilbase.h"
#include "UVCPreview.h"
#include "libuvc_internal.h"

#define	LOCAL_DEBUG 0
#define MAX_FRAME 4
#define PREVIEW_PIXEL_BYTES 4	// RGBA/RGBX
#define FRAME_POOL_SZ MAX_FRAME + 2

UVCPreview::UVCPreview(uvc_device_handle_t *devh)
:	mPreviewWindow(NULL),
	mCaptureWindow(NULL),
	mDeviceHandle(devh),
	requestWidth(DEFAULT_PREVIEW_WIDTH),
	requestHeight(DEFAULT_PREVIEW_HEIGHT),
	requestMinFps(DEFAULT_PREVIEW_FPS_MIN),
	requestMaxFps(DEFAULT_PREVIEW_FPS_MAX),
	requestMode(DEFAULT_PREVIEW_MODE),
	requestBandwidth(DEFAULT_BANDWIDTH),
	frameWidth(DEFAULT_PREVIEW_WIDTH),
	frameHeight(DEFAULT_PREVIEW_HEIGHT),
	frameRotationAngle(DEFAULT_FRAME_ROTATION_ANGLE),
	frameHorizontalMirror(0),
	frameVerticalMirror(0),
	rotateImage(NULL),
	frameBytes(DEFAULT_PREVIEW_WIDTH * DEFAULT_PREVIEW_HEIGHT * 2),	// YUYV
	frameMode(0),
	previewBytes(DEFAULT_PREVIEW_WIDTH * DEFAULT_PREVIEW_HEIGHT * PREVIEW_PIXEL_BYTES),
	previewFormat(WINDOW_FORMAT_RGBA_8888),
	mIsRunning(false),
	mIsCapturing(false),
	captureQueu(NULL),
	// 帧回调Java对象
	mFrameCallbackObj(NULL),
    // 像素格式转换方法
	mFrameCallbackFunc(NULL),
	callbackPixelBytes(2) {

	ENTER();
	pthread_cond_init(&preview_sync, NULL);
	pthread_mutex_init(&preview_mutex, NULL);
//
	pthread_cond_init(&capture_sync, NULL);
	pthread_mutex_init(&capture_mutex, NULL);
//	
	pthread_mutex_init(&pool_mutex, NULL);

	EXIT();
}

UVCPreview::~UVCPreview() {

	ENTER();
	if(rotateImage){
	    SAFE_DELETE(rotateImage);
    }
	if (mPreviewWindow)
		ANativeWindow_release(mPreviewWindow);
	mPreviewWindow = NULL;
	if (mCaptureWindow)
		ANativeWindow_release(mCaptureWindow);
	mCaptureWindow = NULL;
	clearPreviewFrame();
	clearCaptureFrame();
	clear_pool();
	pthread_mutex_destroy(&preview_mutex);
	pthread_cond_destroy(&preview_sync);
	pthread_mutex_destroy(&capture_mutex);
	pthread_cond_destroy(&capture_sync);
	pthread_mutex_destroy(&pool_mutex);
	EXIT();
}

/**
 * get uvc_frame_t from frame pool
 * if pool is empty, create new frame
 * this function does not confirm the frame size
 * and you may need to confirm the size
 *
 * 从帧池中获取帧
 * 如果池为空，则从帧池中获取uvc_frame_t，创建新帧，此功能无法确认帧大小，您可能需要确认大小
 */
uvc_frame_t *UVCPreview::get_frame(size_t data_bytes) {
	uvc_frame_t *frame = NULL;
	pthread_mutex_lock(&pool_mutex);
	{
		if (!mFramePool.isEmpty()) {
			frame = mFramePool.last();
			if(frame->data_bytes < data_bytes){
			    mFramePool.put(frame);
			    frame = NULL;
			}else{
			    frame->actual_bytes = data_bytes;
			}
		}
	}
	pthread_mutex_unlock(&pool_mutex);
	if UNLIKELY(!frame) {
		//LOGW("allocate new frame");
		// 开辟帧数据内存
		frame = uvc_allocate_frame(data_bytes);
	}
	return frame;
}

/**
 * 将帧放回帧池中
 */
void UVCPreview::recycle_frame(uvc_frame_t *frame) {
	pthread_mutex_lock(&pool_mutex);
	// 如果当前帧池小于最大大小则放回帧池，否则销毁
	if (LIKELY(mFramePool.size() < FRAME_POOL_SZ)) {
		mFramePool.put(frame);
		frame = NULL;
	}
	pthread_mutex_unlock(&pool_mutex);
	if (UNLIKELY(frame)) {
	    // 释放帧数据内存
		uvc_free_frame(frame);
	}
}

/**
 * 初始化帧池
 */
void UVCPreview::init_pool(size_t data_bytes) {
	ENTER();

	clear_pool();
	pthread_mutex_lock(&pool_mutex);
	{
		for (int i = 0; i < FRAME_POOL_SZ; i++) {
			mFramePool.put(uvc_allocate_frame(data_bytes));
		}
	}
	pthread_mutex_unlock(&pool_mutex);

	EXIT();
}

/**
 * 清除帧池
 */
void UVCPreview::clear_pool() {
	ENTER();

	pthread_mutex_lock(&pool_mutex);
	{
		const int n = mFramePool.size();
		for (int i = 0; i < n; i++) {
		    // 释放帧数据内存
			uvc_free_frame(mFramePool[i]);
		}
		mFramePool.clear();
	}
	pthread_mutex_unlock(&pool_mutex);
	EXIT();
}

inline const bool UVCPreview::isRunning() const {return mIsRunning; }

// 设置预览参数
int UVCPreview::setPreviewSize(int width, int height, int cameraAngle, int min_fps, int max_fps, int mode, float bandwidth) {
	ENTER();
	
	int result = 0;
	if ((requestWidth != width) || (requestHeight != height) || (requestMode != mode)) {
		requestWidth = width;
		requestHeight = height;
		requestMinFps = min_fps;
		requestMaxFps = max_fps;
		requestMode = mode;
		requestBandwidth = bandwidth;

		uvc_stream_ctrl_t ctrl;
		result = uvc_get_stream_ctrl_format_size_fps(mDeviceHandle, &ctrl,
			!requestMode ? UVC_FRAME_FORMAT_YUYV : UVC_FRAME_FORMAT_MJPEG,
			requestWidth, requestHeight, requestMinFps, requestMaxFps);
	}

	// 根据摄像头角度计算图像帧需要旋转的角度
	frameRotationAngle = (360 - cameraAngle) % 360;
	LOGW("frameRotationAngle:%d",frameRotationAngle);
	if( (frameHorizontalMirror || frameVerticalMirror || frameRotationAngle) && !rotateImage) {
		rotateImage = new RotateImage();
	}
	
	RETURN(result, int);
}

// 设置预览显示
int UVCPreview::setPreviewDisplay(ANativeWindow *preview_window) {
	ENTER();
	pthread_mutex_lock(&preview_mutex);
	{
		if (mPreviewWindow != preview_window) {
			if (mPreviewWindow)
				ANativeWindow_release(mPreviewWindow);
			mPreviewWindow = preview_window;
			if (LIKELY(mPreviewWindow)) {
				ANativeWindow_setBuffersGeometry(mPreviewWindow,
					frameWidth, frameHeight, previewFormat);
			}
		}
	}
	pthread_mutex_unlock(&preview_mutex);
	RETURN(0, int);
}

// 设置帧回调
int UVCPreview::setFrameCallback(JNIEnv *env, jobject frame_callback_obj, int pixel_format) {
	
	ENTER();
	pthread_mutex_lock(&capture_mutex);
	{
		if (isRunning() && isCapturing()) {
			mIsCapturing = false;
			if (mFrameCallbackObj) {
				pthread_cond_signal(&capture_sync);
				pthread_cond_wait(&capture_sync, &capture_mutex);	// wait finishing capturing
			}
		}
		if (!env->IsSameObject(mFrameCallbackObj, frame_callback_obj))	{
			iframecallback_fields.onFrame = NULL;
			if (mFrameCallbackObj) {
				env->DeleteGlobalRef(mFrameCallbackObj);
			}
			mFrameCallbackObj = frame_callback_obj;
			if (frame_callback_obj) {
				// get method IDs of Java object for callback
				// 获取用于回调的Java对象的方法ID
				jclass clazz = env->GetObjectClass(frame_callback_obj);
				if (LIKELY(clazz)) {
					iframecallback_fields.onFrame = env->GetMethodID(clazz,
						"onFrame",	"(Ljava/nio/ByteBuffer;)V");
				} else {
					LOGW("failed to get object class");
				}
				env->ExceptionClear();
				if (!iframecallback_fields.onFrame) {
					LOGE("Can't find IFrameCallback#onFrame");
					env->DeleteGlobalRef(frame_callback_obj);
					mFrameCallbackObj = frame_callback_obj = NULL;
				}
			}
		}
		if (frame_callback_obj) {
			mPixelFormat = pixel_format;
			callbackPixelFormatChanged();
		}
	}
	pthread_mutex_unlock(&capture_mutex);
	RETURN(0, int);
}

// 像素格式转换
void UVCPreview::callbackPixelFormatChanged() {
	mFrameCallbackFunc = NULL;
	const size_t sz = requestWidth * requestHeight;
	// 帧回调像素格式
	switch (mPixelFormat) {
	  case PIXEL_FORMAT_RAW:
		LOGI("PIXEL_FORMAT_RAW:");
		callbackPixelBytes = sz * 2;
		break;
	  case PIXEL_FORMAT_YUV:
		LOGI("PIXEL_FORMAT_YUV:");
		callbackPixelBytes = sz * 2;
		break;
	  case PIXEL_FORMAT_RGB565:
		LOGI("PIXEL_FORMAT_RGB565:");
		mFrameCallbackFunc = uvc_any2rgb565;
		callbackPixelBytes = sz * 2;
		break;
	  case PIXEL_FORMAT_RGBX:
		LOGI("PIXEL_FORMAT_RGBX:");
		mFrameCallbackFunc = uvc_any2rgbx;
		callbackPixelBytes = sz * 4;
		break;
	  case PIXEL_FORMAT_YUV20SP:
		LOGI("PIXEL_FORMAT_YUV20SP:");
		// NV12: YYYYYYYY UVUV   => YUV420SP
		mFrameCallbackFunc = uvc_yuyv2yuv420SP;
		callbackPixelBytes = (sz * 3) / 2;
		break;
	  case PIXEL_FORMAT_NV21:
		LOGI("PIXEL_FORMAT_NV21:");
	    // NV21: YYYYYYYY VUVU   => YUV420SP
		mFrameCallbackFunc = uvc_yuyv2iyuv420SP;
		callbackPixelBytes = (sz * 3) / 2;
		break;
	}
}

// 清除显示
void UVCPreview::clearDisplay() {
	ENTER();

	ANativeWindow_Buffer buffer;
	pthread_mutex_lock(&capture_mutex);
	{
		if (LIKELY(mCaptureWindow)) {
			if (LIKELY(ANativeWindow_lock(mCaptureWindow, &buffer, NULL) == 0)) {
				uint8_t *dest = (uint8_t *)buffer.bits;
				const size_t bytes = buffer.width * PREVIEW_PIXEL_BYTES;
				const int stride = buffer.stride * PREVIEW_PIXEL_BYTES;
				for (int i = 0; i < buffer.height; i++) {
					memset(dest, 0, bytes);
					dest += stride;
				}
				ANativeWindow_unlockAndPost(mCaptureWindow);
			}
		}
	}
	pthread_mutex_unlock(&capture_mutex);
	pthread_mutex_lock(&preview_mutex);
	{
		if (LIKELY(mPreviewWindow)) {
			if (LIKELY(ANativeWindow_lock(mPreviewWindow, &buffer, NULL) == 0)) {
				uint8_t *dest = (uint8_t *)buffer.bits;
				const size_t bytes = buffer.width * PREVIEW_PIXEL_BYTES;
				const int stride = buffer.stride * PREVIEW_PIXEL_BYTES;
				for (int i = 0; i < buffer.height; i++) {
					memset(dest, 0, bytes);
					dest += stride;
				}
				ANativeWindow_unlockAndPost(mPreviewWindow);
			}
		}
	}
	pthread_mutex_unlock(&preview_mutex);

	EXIT();
}

// 开始预览
int UVCPreview::startPreview() {
	ENTER();

	int result = EXIT_FAILURE;
	if (!isRunning()) {
		mIsRunning = true;
		pthread_mutex_lock(&preview_mutex);
		{
			if (LIKELY(mPreviewWindow)) {
			    // 创建线程执行 preview_thread_func
				result = pthread_create(&preview_thread, NULL, preview_thread_func, (void *)this);
			}
		}
		pthread_mutex_unlock(&preview_mutex);
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

// 停止预览
int UVCPreview::stopPreview() {
	ENTER();
	bool b = isRunning();
	if (LIKELY(b)) {
		mIsRunning = false;
		pthread_cond_signal(&preview_sync);
		pthread_cond_signal(&capture_sync);
		if (pthread_join(capture_thread, NULL) != EXIT_SUCCESS) {
			LOGW("UVCPreview::terminate capture thread: pthread_join failed");
		}
		if (pthread_join(preview_thread, NULL) != EXIT_SUCCESS) {
			LOGW("UVCPreview::terminate preview thread: pthread_join failed");
		}
		clearDisplay();
	}
	clearPreviewFrame();
	clearCaptureFrame();
	pthread_mutex_lock(&preview_mutex);
	if (mPreviewWindow) {
		ANativeWindow_release(mPreviewWindow);
		mPreviewWindow = NULL;
	}
	pthread_mutex_unlock(&preview_mutex);
	pthread_mutex_lock(&capture_mutex);
	if (mCaptureWindow) {
		ANativeWindow_release(mCaptureWindow);
		mCaptureWindow = NULL;
	}
	pthread_mutex_unlock(&capture_mutex);
	RETURN(0, int);
}

//**********************************************************************
// UVC预览帧回调
//**********************************************************************
void UVCPreview::uvc_preview_frame_callback(uvc_frame_t *frame, void *vptr_args) {
	UVCPreview *preview = reinterpret_cast<UVCPreview *>(vptr_args);
	if UNLIKELY(!preview->isRunning() || !frame || !frame->frame_format || !frame->data || !frame->data_bytes || !frame->actual_bytes) return;
	if (UNLIKELY(
		((frame->frame_format != UVC_FRAME_FORMAT_MJPEG) && (frame->actual_bytes < preview->frameBytes))
		|| (frame->width != preview->frameWidth) || (frame->height != preview->frameHeight) )) {

#if LOCAL_DEBUG
		LOGD("broken frame!:format=%d,actual_bytes=%d/%d(%d,%d/%d,%d)",
			frame->frame_format, frame->actual_bytes, preview->frameBytes,
			frame->width, frame->height, preview->frameWidth, preview->frameHeight);
#endif
		return;
	}
	if (LIKELY(preview->isRunning())) {
	    // 从帧池中获取帧
		uvc_frame_t *copy = preview->get_frame(frame->actual_bytes);
		if (UNLIKELY(!copy)) {
#if LOCAL_DEBUG
			LOGE("uvc_callback:unable to allocate duplicate frame!");
#endif
			return;
		}
		// 复制帧，保留色彩格式
		uvc_error_t ret = uvc_duplicate_frame(frame, copy);
		if (UNLIKELY(ret)) {
		    // 放回帧池
			preview->recycle_frame(copy);
			return;
		}
		// 添加预览帧
		preview->addPreviewFrame(copy);
	}
}

// 添加预览帧
void UVCPreview::addPreviewFrame(uvc_frame_t *frame) {
	pthread_mutex_lock(&preview_mutex);
	if (isRunning() && (previewFrames.size() < MAX_FRAME)) {
	    // 添加到预览数组中
		previewFrames.put(frame);
		frame = NULL;
		// 发送信号给另外一个正在处于阻塞等待状态的线程,使其脱离阻塞状态,继续执行
		pthread_cond_signal(&preview_sync);
	}
	pthread_mutex_unlock(&preview_mutex);
	if (frame) {
		// 放回帧池
		recycle_frame(frame);
	}
}

// 等待预览帧
uvc_frame_t *UVCPreview::waitPreviewFrame() {
	uvc_frame_t *frame = NULL;
	pthread_mutex_lock(&preview_mutex);
	{
		if (!previewFrames.size()) {
            // 等待 preview_sync，解锁 preview_mutex
			pthread_cond_wait(&preview_sync, &preview_mutex);
		}
		if (LIKELY(isRunning() && previewFrames.size() > 0)) {
			frame = previewFrames.remove(0);
		}
	}
	pthread_mutex_unlock(&preview_mutex);
	return frame;
}

// 清空预览帧
void UVCPreview::clearPreviewFrame() {
	pthread_mutex_lock(&preview_mutex);
	{
		for (int i = 0; i < previewFrames.size(); i++)
		    // 放回帧池
			recycle_frame(previewFrames[i]);
		previewFrames.clear();
	}
	pthread_mutex_unlock(&preview_mutex);
}

// 预览线程
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

// 准备预览
int UVCPreview::prepare_preview(uvc_stream_ctrl_t *ctrl) {
	uvc_error_t result;

	ENTER();
	result = uvc_get_stream_ctrl_format_size_fps(mDeviceHandle, ctrl,
		!requestMode ? UVC_FRAME_FORMAT_YUYV : UVC_FRAME_FORMAT_MJPEG,
		requestWidth, requestHeight, requestMinFps, requestMaxFps
	);
	if (LIKELY(!result)) {
#if LOCAL_DEBUG
		uvc_print_stream_ctrl(ctrl, stderr);
#endif
		uvc_frame_desc_t *frame_desc;
		result = uvc_get_frame_desc(mDeviceHandle, ctrl, &frame_desc);
		if (LIKELY(!result)) {
			frameWidth = frame_desc->wWidth;
			frameHeight = frame_desc->wHeight;
			LOGI("frameSize=(%d,%d)@%s", frameWidth, frameHeight, (!requestMode ? "YUYV" : "MJPEG"));
			pthread_mutex_lock(&preview_mutex);
			if (LIKELY(mPreviewWindow)) {
				ANativeWindow_setBuffersGeometry(mPreviewWindow,
					frameWidth, frameHeight, previewFormat);
			}
			pthread_mutex_unlock(&preview_mutex);
		} else {
			frameWidth = requestWidth;
			frameHeight = requestHeight;
		}
		frameMode = requestMode;
		frameBytes = frameWidth * frameHeight * (!requestMode ? 2 : 4);
		previewBytes = frameWidth * frameHeight * PREVIEW_PIXEL_BYTES;
	} else {
		LOGE("could not negotiate with camera:err=%d", result);
	}
	RETURN(result, int);
}

// 执行预览
void UVCPreview::do_preview(uvc_stream_ctrl_t *ctrl) {
	ENTER();

	uvc_frame_t *frame = NULL;
	uvc_frame_t *frame_mjpeg = NULL;
	uvc_error_t result = uvc_start_streaming_bandwidth(
		mDeviceHandle, ctrl, uvc_preview_frame_callback, (void *)this, requestBandwidth, 0);

	if (LIKELY(!result)) {
	    // 清空预览帧
		clearPreviewFrame();
		// 创建线程执行 capture_thread_func
		pthread_create(&capture_thread, NULL, capture_thread_func, (void *)this);

#if LOCAL_DEBUG
		LOGI("Streaming...");
#endif
		if (frameMode) {
			// MJPEG mode
			for ( ; LIKELY(isRunning()) ; ) {
			    // 等待预览帧
				frame_mjpeg = waitPreviewFrame();
				if (LIKELY(frame_mjpeg)) {
				    // 从帧池中获取帧
					frame = get_frame(frame_mjpeg->width * frame_mjpeg->height * 2);
					// 将MJPEG转为yuyv
					result = uvc_mjpeg2yuyv(frame_mjpeg, frame);   // MJPEG => yuyv
					// 放回帧池
					recycle_frame(frame_mjpeg);
					if (LIKELY(!result)) {
                        // 需要旋转图像帧
                        if(rotateImage){
                            if(frameRotationAngle==90){
                                rotateImage->rotate_yuyv_90(frame);
                            }else if(frameRotationAngle==180){
                                rotateImage->rotate_yuyv_180(frame);
                            }else if(frameRotationAngle==270){
                                rotateImage->rotate_yuyv_270(frame);
                            }
                            // 需要水平镜像
                            if(frameHorizontalMirror){
                                rotateImage->horizontal_mirror_yuyv(frame);
                            }
                            // 需要垂直镜像
                            if(frameVerticalMirror){
                                rotateImage->vertical_mirror_yuyv(frame);
                            }
                        }

					    // 画预览帧
						frame = draw_preview_one(frame, &mPreviewWindow, uvc_any2rgbx, 4);
						// 设置抓拍帧
						addCaptureFrame(frame);
					} else {
					    // 放回帧池
						recycle_frame(frame);
					}
				}
			}
		} else {
			// yuyv mode
			for ( ; LIKELY(isRunning()) ; ) {
			    // 等待预览帧
				frame = waitPreviewFrame();
				if (LIKELY(frame)) {
				    // 需要旋转图像帧
				    if(rotateImage){
                        if(frameRotationAngle==90){
                            rotateImage->rotate_yuyv_90(frame);
                        }else if(frameRotationAngle==180){
                            rotateImage->rotate_yuyv_180(frame);
                        }else if(frameRotationAngle==270){
                            rotateImage->rotate_yuyv_270(frame);
                        }
                        // 需要水平镜像
                        if(frameHorizontalMirror){
                            rotateImage->horizontal_mirror_yuyv(frame);
                        }
                        // 需要垂直镜像
                        if(frameVerticalMirror){
                            rotateImage->vertical_mirror_yuyv(frame);
                        }
				    }
				    // 画预览帧
					frame = draw_preview_one(frame, &mPreviewWindow, uvc_any2rgbx, 4);
					// 设置抓拍帧
					addCaptureFrame(frame);
				}
			}
		}
		pthread_cond_signal(&capture_sync);
#if LOCAL_DEBUG
		LOGI("preview_thread_func:wait for all callbacks complete");
#endif
		uvc_stop_streaming(mDeviceHandle);
#if LOCAL_DEBUG
		LOGI("Streaming finished");
#endif
	} else {
		uvc_perror(result, "failed start_streaming");
	}

	EXIT();
}

// 复制帧数据
static void copyFrame(const uint8_t *src, uint8_t *dest, const int width, int height, const int stride_src, const int stride_dest) {
	const int h8 = height % 8;
	for (int i = 0; i < h8; i++) {
		memcpy(dest, src, width);
		dest += stride_dest; src += stride_src;
	}
	for (int i = 0; i < height; i += 8) {
		memcpy(dest, src, width);
		dest += stride_dest; src += stride_src;
		memcpy(dest, src, width);
		dest += stride_dest; src += stride_src;
		memcpy(dest, src, width);
		dest += stride_dest; src += stride_src;
		memcpy(dest, src, width);
		dest += stride_dest; src += stride_src;
		memcpy(dest, src, width);
		dest += stride_dest; src += stride_src;
		memcpy(dest, src, width);
		dest += stride_dest; src += stride_src;
		memcpy(dest, src, width);
		dest += stride_dest; src += stride_src;
		memcpy(dest, src, width);
		dest += stride_dest; src += stride_src;
	}
}


// transfer specific frame data to the Surface(ANativeWindow)
// 将特定的帧数据传输到Surface（ANativeWindow）
int copyToSurface(uvc_frame_t *frame, ANativeWindow **window) {
	// ENTER();
	int result = 0;
	if (LIKELY(*window)) {
		ANativeWindow_Buffer buffer;
		if (LIKELY(ANativeWindow_lock(*window, &buffer, NULL) == 0)) {
			// source = frame data
			const uint8_t *src = (uint8_t *)frame->data;
			const int src_w = frame->width * PREVIEW_PIXEL_BYTES;
			const int src_step = frame->width * PREVIEW_PIXEL_BYTES;
			// destination = Surface(ANativeWindow)
			uint8_t *dest = (uint8_t *)buffer.bits;
			const int dest_w = buffer.width * PREVIEW_PIXEL_BYTES;
			const int dest_step = buffer.stride * PREVIEW_PIXEL_BYTES;
			// use lower transfer bytes
			// 使用较低的传输字节
			const int w = src_w < dest_w ? src_w : dest_w;
			// use lower height
			// 使用较低的高度
			const int h = frame->height < buffer.height ? frame->height : buffer.height;
			// transfer from frame data to the Surface
			// 从帧数据传输到Surface
			// 复制帧数据
			copyFrame(src, dest, w, h, src_step, dest_step);
			ANativeWindow_unlockAndPost(*window);
		} else {
			result = -1;
		}
	} else {
		result = -1;
	}
	return result; //RETURN(result, int);
}

// changed to return original frame instead of returning converted frame even if convert_func is not null.
// 画预览帧 更改为返回原始帧，而不是返回转换后的帧，即使convert_func不为null。
uvc_frame_t *UVCPreview::draw_preview_one(uvc_frame_t *frame, ANativeWindow **window, convFunc_t convert_func, int pixcelBytes) {
	// ENTER();

	int b = 0;
	pthread_mutex_lock(&preview_mutex);
	{
		b = *window != NULL;
	}
	pthread_mutex_unlock(&preview_mutex);
	if (LIKELY(b)) {
		uvc_frame_t *converted;
		if (convert_func) {
		    // 从帧池中获取帧
			converted = get_frame(frame->width * frame->height * pixcelBytes);
			if LIKELY(converted) {
			    // 转换帧
				b = convert_func(frame, converted);
				if (!b) {
					pthread_mutex_lock(&preview_mutex);
					// 复制到Surface
					copyToSurface(converted, window);
					pthread_mutex_unlock(&preview_mutex);
				} else {
					LOGE("failed converting");
				}
				// 放回帧池
				recycle_frame(converted);
			}
		} else {
			pthread_mutex_lock(&preview_mutex);
			// 复制到Surface
			copyToSurface(frame, window);
			pthread_mutex_unlock(&preview_mutex);
		}
	}
	return frame; //RETURN(frame, uvc_frame_t *);
}

//======================================================================
// 是否抓拍
//======================================================================
inline const bool UVCPreview::isCapturing() const { return mIsCapturing; }

// 设置抓拍显示
int UVCPreview::setCaptureDisplay(ANativeWindow *capture_window) {
	ENTER();
	pthread_mutex_lock(&capture_mutex);
	{
		if (isRunning() && isCapturing()) {
			mIsCapturing = false;
			if (mCaptureWindow) {
				pthread_cond_signal(&capture_sync);
				pthread_cond_wait(&capture_sync, &capture_mutex);	// wait finishing capturing
			}
		}
		if (mCaptureWindow != capture_window) {
			// release current Surface if already assigned.
			if (UNLIKELY(mCaptureWindow))
				ANativeWindow_release(mCaptureWindow);
			mCaptureWindow = capture_window;
			// if you use Surface came from MediaCodec#createInputSurface
			// you could not change window format at least when you use
			// ANativeWindow_lock / ANativeWindow_unlockAndPost
			// to write frame data to the Surface...
			// So we need check here.
			if (mCaptureWindow) {
				int32_t window_format = ANativeWindow_getFormat(mCaptureWindow);
				if ((window_format != WINDOW_FORMAT_RGB_565)
					&& (previewFormat == WINDOW_FORMAT_RGB_565)) {
					LOGE("window format mismatch, cancelled movie capturing.");
					ANativeWindow_release(mCaptureWindow);
					mCaptureWindow = NULL;
				}
			}
		}
	}
	pthread_mutex_unlock(&capture_mutex);
	RETURN(0, int);
}

// 设置抓拍帧
void UVCPreview::addCaptureFrame(uvc_frame_t *frame) {
	pthread_mutex_lock(&capture_mutex);
	if (LIKELY(isRunning())) {
		// keep only latest one
		if (captureQueu) {
		    // 放回帧池
			recycle_frame(captureQueu);
		}
		captureQueu = frame;
		pthread_cond_broadcast(&capture_sync);
	}else {
	    // 放回帧池
	    recycle_frame(frame);
    }
	pthread_mutex_unlock(&capture_mutex);
}

/**
 * get frame data for capturing, if not exist, block and wait
 * 获取要抓拍的帧数据（如果不存在），阻塞并等待
 */
uvc_frame_t *UVCPreview::waitCaptureFrame() {
	uvc_frame_t *frame = NULL;
	pthread_mutex_lock(&capture_mutex);
	{
		if (!captureQueu) {
			pthread_cond_wait(&capture_sync, &capture_mutex);
		}
		if (LIKELY(isRunning() && captureQueu)) {
			frame = captureQueu;
			captureQueu = NULL;
		}
	}
	pthread_mutex_unlock(&capture_mutex);
	return frame;
}

/**
 * clear drame data for capturing
 * 清除抓拍帧
 */
void UVCPreview::clearCaptureFrame() {
	pthread_mutex_lock(&capture_mutex);
	{
		if (captureQueu)
		    // 放回帧池
			recycle_frame(captureQueu);
		captureQueu = NULL;
	}
	pthread_mutex_unlock(&capture_mutex);
}

//======================================================================
/*
 * thread function
 * @param vptr_args pointer to UVCPreview instance
 * 抓拍线程方法
 */
// static
void *UVCPreview::capture_thread_func(void *vptr_args) {
	int result;

	ENTER();
	UVCPreview *preview = reinterpret_cast<UVCPreview *>(vptr_args);
	if (LIKELY(preview)) {
		JavaVM *vm = getVM();
		JNIEnv *env;
		// attach to JavaVM
		// 附加当前线程到一个Java(Dalvik)虚拟机
		vm->AttachCurrentThread(&env, NULL);
		preview->do_capture(env);	// never return until finish previewing 直到完成预览才返回
		// detach from JavaVM
		// 从一个Java（Dalvik）虚拟机，分离当前线程。
		vm->DetachCurrentThread();
		MARK("DetachCurrentThread");
	}
	PRE_EXIT();
	pthread_exit(NULL);
}

/**
 * the actual function for capturing
 * 执行抓拍
 */
void UVCPreview::do_capture(JNIEnv *env) {

	ENTER();
	// 清除抓拍帧
	clearCaptureFrame();
	// 像素格式转换
	callbackPixelFormatChanged();
	for (; isRunning() ;) {
	    // 标记为抓拍
		mIsCapturing = true;
		if (mCaptureWindow) {
		    // 将帧数据写入Surface以进行捕获
			do_capture_surface(env);
		} else {
		    // 执行抓拍空闲循环
			do_capture_idle_loop(env);
		}
		// 唤醒所有被 capture_sync 阻塞的线程
		pthread_cond_broadcast(&capture_sync);
	}	// end of for (; isRunning() ;)
	EXIT();
}

// 执行抓拍空闲循环
void UVCPreview::do_capture_idle_loop(JNIEnv *env) {
	ENTER();
	
	for (; isRunning() && isCapturing() ;) {
		do_capture_callback(env, waitCaptureFrame());
	}
	
	EXIT();
}

/**
 * write frame data to Surface for capturing
 * 将帧数据写入Surface以进行捕获
 */
void UVCPreview::do_capture_surface(JNIEnv *env) {
	ENTER();

	uvc_frame_t *frame = NULL;
	uvc_frame_t *converted = NULL;
	char *local_picture_path;

	for (; isRunning() && isCapturing() ;) {
	    // 获取要抓拍的帧数据（如果不存在），阻塞并等待
		frame = waitCaptureFrame();
		if (LIKELY(frame)) {
			// frame data is always YUYV format.
			// 帧数据始终为YUYV格式。
			if LIKELY(isCapturing()) {
				if (LIKELY(mCaptureWindow)) {
                    if (UNLIKELY(!converted)) {
                        // 从帧池中获取帧
                        converted = get_frame(previewBytes);
                    }
                    if (LIKELY(converted)) {
                        // 转为RGBA8888
                        int b = uvc_any2rgbx(frame, converted);
                        if (!b) {
                            // 复制到Surface
                            copyToSurface(converted, &mCaptureWindow);
                        }
                    }
				}
			}
			// 回调 IFrameCallback#onFrame
			do_capture_callback(env, frame);
		}
	}
	if (converted) {
	    // 放回帧池
		recycle_frame(converted);
	}
	if (mCaptureWindow) {
		ANativeWindow_release(mCaptureWindow);
		mCaptureWindow = NULL;
	}

	EXIT();
}

/**
 * call IFrameCallback#onFrame if needs
 * 如有需要，回调 IFrameCallback#onFrame
 */
void UVCPreview::do_capture_callback(JNIEnv *env, uvc_frame_t *frame) {
	ENTER();

	if (LIKELY(frame)) {
		uvc_frame_t *callback_frame = frame;
		if (mFrameCallbackObj) {
			if (mFrameCallbackFunc) {
			    // 从帧池中获取帧
				callback_frame = get_frame(callbackPixelBytes);
				if (LIKELY(callback_frame)) {
				    // 像素格式转换
					int b = mFrameCallbackFunc(frame, callback_frame);
					// 放回帧池
					recycle_frame(frame);
					if (UNLIKELY(b)) {
						LOGW("failed to convert for callback frame");
						goto SKIP;
					}
				} else {
					LOGW("failed to allocate for callback frame");
					callback_frame = frame;
					goto SKIP;
				}
			}
			// NewDirectByteBuffer 基于指定内存地址创建指定长度可直接访问的内存
			jobject buf = env->NewDirectByteBuffer(callback_frame->data, callbackPixelBytes);
			env->CallVoidMethod(mFrameCallbackObj, iframecallback_fields.onFrame, buf);
			env->ExceptionClear();
			env->DeleteLocalRef(buf);
		}
 SKIP:
        // 放回帧池
		recycle_frame(callback_frame);
	}
	EXIT();
}

void UVCPreview::setHorizontalMirror(int horizontalMirror){
	frameHorizontalMirror = horizontalMirror;
	if( frameHorizontalMirror && !rotateImage) {
	    rotateImage = new RotateImage();
    }
}

void UVCPreview::setVerticalMirror(int verticalMirror){
	frameVerticalMirror = verticalMirror;
	if( frameVerticalMirror && !rotateImage) {
	    rotateImage = new RotateImage();
    }
}

void UVCPreview::setCameraAngle(int cameraAngle){
	frameRotationAngle = (360 - cameraAngle) % 360;
	LOGW("frameRotationAngle:%d",frameRotationAngle);
	if( frameRotationAngle && !rotateImage) {
	    rotateImage = new RotateImage();
    }
}
