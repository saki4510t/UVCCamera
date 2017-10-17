//
// Created by saki on 15/11/06.
//

#if 1	// set 1 if you don't need debug message
	#ifndef LOG_NDEBUG
		#define	LOG_NDEBUG		// ignore LOGV/LOGD/MARK
	#endif
	#undef USE_LOGALL
#else
	#define USE_LOGALL
	#undef LOG_NDEBUG
	#undef NDEBUG		// depends on definition in Android.mk and Application.mk
#endif

#include <android/native_window_jni.h>

#include "utilbase.h"
#include "common_utils.h"

#include "libUVCCamera.h"
#include "pipeline_helper.h"
#include "IPipeline.h"
#include "PreviewPipeline.h"

#define INIT_FRAME_POOL_SZ 2
#define MAX_FRAME_NUM 32	// max approx. 1sec

#define CAPTURE_PIXEL_BYTES 2	// RGB565

PreviewPipeline::PreviewPipeline(const size_t &_data_bytes)
:	CaptureBasePipeline(MAX_FRAME_NUM, INIT_FRAME_POOL_SZ, _data_bytes),
	mCaptureWindow(NULL)
{
	ENTER();

	setState(PIPELINE_STATE_INITIALIZED);

	EXIT();
}

PreviewPipeline::~PreviewPipeline() {
	ENTER();

	if (mCaptureWindow) {
		ANativeWindow_release(mCaptureWindow);
	}
	mCaptureWindow = NULL;
	clearCaptureFrame();

	EXIT();
}

//********************************************************************************
//
//********************************************************************************
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
static int copyToSurface(uvc_frame_t *frame, ANativeWindow **window) {
//	ENTER();
	int result = 0;
	if (LIKELY(*window)) {
		ANativeWindow_Buffer buffer;
		if (LIKELY(ANativeWindow_lock(*window, &buffer, NULL) == 0)) {
			// source = frame data
			const uint8_t *src = (uint8_t *)frame->data;
			const int src_w = frame->width * CAPTURE_PIXEL_BYTES;
			const int src_step = frame->width * CAPTURE_PIXEL_BYTES;
			// destination = Surface(ANativeWindow)
			uint8_t *dest = (uint8_t *)buffer.bits;
			const int dest_w = buffer.width * CAPTURE_PIXEL_BYTES;
			const int dest_step = buffer.stride * CAPTURE_PIXEL_BYTES;
			// use lower transfer bytes
			const int w = src_w < dest_w ? src_w : dest_w;
			// use lower height
			const int h = frame->height < buffer.height ? frame->height : buffer.height;
			// transfer from frame data to the Surface
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

//********************************************************************************
//
//********************************************************************************
int PreviewPipeline::setCaptureDisplay(ANativeWindow *capture_window) {
	ENTER();
	LOGI("setCaptureDisplay:%p", capture_window);

	Mutex::Autolock lock(capture_mutex);

	if (isRunning() && isCapturing()) {
		mIsCapturing = false;
		if (mCaptureWindow) {
			LOGD("wait for finishing capture loop");
			capture_sync.broadcast();
			capture_sync.wait(capture_mutex);	// wait finishing capturing
		}
	}
	if (mCaptureWindow != capture_window) {
		// release current Surface if already assigned.
		if (UNLIKELY(mCaptureWindow)) {
			LOGD("ANativeWindow_release");
			ANativeWindow_release(mCaptureWindow);
		}
		mCaptureWindow = capture_window;
	}

	RETURN(0, int);
}

/**
 * the actual function for capturing
 */
void PreviewPipeline::do_capture(JNIEnv *env) {

//	ENTER();

	uvc_frame_t *frame = NULL;
	uvc_frame_t *rgb565 = get_frame(default_frame_size);

	if (LIKELY(rgb565)) {
		for (; isRunning() && isCapturing() ;) {
			frame = waitCaptureFrame();
			if (LIKELY(frame)) {
				if (LIKELY(isCapturing())) {
					const bool need_update_geometry = (frame->width != frameWidth) || (frame->height != frameHeight);
					capture_mutex.lock();
					{
						ANativeWindow *window = mCaptureWindow;	// local cache
						if (LIKELY(window)) {
							if (UNLIKELY(need_update_geometry)) {
								frameWidth = frame->width;
								frameHeight = frame->height;
								LOGD("ANativeWindow_setBuffersGeometry:(%dx%d)", frameWidth, frameHeight);
								ANativeWindow_setBuffersGeometry(window,
									frameWidth, frameHeight, WINDOW_FORMAT_RGB_565);
								// if you use Surface came from MediaCodec#createInputSurface
								// you could not change window format at least when you use
								// ANativeWindow_lock / ANativeWindow_unlockAndPost
								// to write frame data to the Surface...you should use RGBX8888 instead
								// So we need check here.
								int32_t window_format = ANativeWindow_getFormat(window);
								if (window_format != WINDOW_FORMAT_RGB_565) {
									LOGE("window format mismatch, cancelled movie capturing.");
									ANativeWindow_release(window);
									window = mCaptureWindow = NULL;
									frameWidth = frameHeight = 0;
								}
							}
							if (LIKELY(window)) {
								int b = uvc_any2rgb565(frame, rgb565);
								if (LIKELY(!b)) {
									copyToSurface(rgb565, &window);
								} else {
									LOGE("failed to convert frame: err=%d", b);
								}
							}
						}
					}
					capture_mutex.unlock();
				}
				recycle_frame(frame);
			}
		}
	}
	if (rgb565) {
		recycle_frame(rgb565);
	}
	capture_mutex.lock();
	{
		if (mCaptureWindow) {
			ANativeWindow_release(mCaptureWindow);
			mCaptureWindow = NULL;
		}
	}
	capture_mutex.unlock();
//	EXIT();
}

//**********************************************************************
//
//**********************************************************************
static ID_TYPE nativeCreate(JNIEnv *env, jobject thiz) {

	ENTER();
	PreviewPipeline *pipeline = new PreviewPipeline();
	setField_long(env, thiz, "mNativePtr", reinterpret_cast<ID_TYPE>(pipeline));
	RETURN(reinterpret_cast<ID_TYPE>(pipeline), ID_TYPE);
}

static void nativeDestroy(JNIEnv *env, jobject thiz,
	ID_TYPE id_pipeline) {

	ENTER();
	setField_long(env, thiz, "mNativePtr", 0);
	PreviewPipeline *pipeline = reinterpret_cast<PreviewPipeline *>(id_pipeline);
	if (LIKELY(pipeline)) {
		pipeline->release();
		SAFE_DELETE(pipeline);
	}
	EXIT();
}

static jint nativeGetState(JNIEnv *env, jobject thiz,
	ID_TYPE id_pipeline) {

	ENTER();
	jint result = 0;
	PreviewPipeline *pipeline = reinterpret_cast<PreviewPipeline *>(id_pipeline);
	if (pipeline) {
		result = pipeline->getState();
	}
	RETURN(result, jint);
}

static jint nativeSetPipeline(JNIEnv *env, jobject thiz,
	ID_TYPE id_pipeline, jobject pipeline_obj) {

	ENTER();
	jint result = JNI_ERR;
	PreviewPipeline *pipeline = reinterpret_cast<PreviewPipeline *>(id_pipeline);
	if (pipeline) {
		IPipeline *target_pipeline = getPipeline(env, pipeline_obj);
		result = pipeline->setPipeline(target_pipeline);
	}

	RETURN(result, jint);
}

static jint nativeStart(JNIEnv *env, jobject thiz,
	ID_TYPE id_pipeline) {

	ENTER();

	int result = JNI_ERR;
	PreviewPipeline *pipeline = reinterpret_cast<PreviewPipeline *>(id_pipeline);
	if (LIKELY(pipeline)) {
		result = pipeline->start();
	}
	RETURN(result, jint);
}

static jint nativeStop(JNIEnv *env, jobject thiz,
	ID_TYPE id_pipeline) {

	jint result = JNI_ERR;
	ENTER();
	PreviewPipeline *pipeline = reinterpret_cast<PreviewPipeline *>(id_pipeline);
	if (LIKELY(pipeline)) {
		result = pipeline->stop();
	}
	RETURN(result, jint);
}

static jint nativeSetCaptureDisplay(JNIEnv *env, jobject thiz,
	ID_TYPE id_pipeline, jobject jSurface) {

	jint result = JNI_ERR;
	ENTER();
	PreviewPipeline *pipeline = reinterpret_cast<PreviewPipeline *>(id_pipeline);
	if (LIKELY(pipeline)) {
		ANativeWindow *capture_window = jSurface ? ANativeWindow_fromSurface(env, jSurface) : NULL;
		result = pipeline->setCaptureDisplay(capture_window);
	}
	RETURN(result, jint);
}

//**********************************************************************
//
//**********************************************************************
static JNINativeMethod methods[] = {
	{ "nativeCreate",					"()J", (void *) nativeCreate },
	{ "nativeDestroy",					"(J)V", (void *) nativeDestroy },

	{ "nativeGetState",					"(J)I", (void *) nativeGetState },
	{ "nativeSetPipeline",				"(JLcom/serenegiant/usb/IPipeline;)I", (void *) nativeSetPipeline },

	{ "nativeStart",					"(J)I", (void *) nativeStart },
	{ "nativeStop",						"(J)I", (void *) nativeStop },

	{ "nativeSetCaptureDisplay",		"(JLandroid/view/Surface;)I", (void *) nativeSetCaptureDisplay },

};

int register_preview_pipeline(JNIEnv *env) {
	LOGV("register_preview_pipeline:");
	if (registerNativeMethods(env,
		"com/serenegiant/usb/PreviewPipeline",
		methods, NUM_ARRAY_ELEMENTS(methods)) < 0) {
		return -1;
	}
    return 0;
}
