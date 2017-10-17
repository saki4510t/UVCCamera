//
// Created by saki on 15/11/05.
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

#include "utilbase.h"
#include "common_utils.h"

#include "libUVCCamera.h"
#include "pipeline_helper.h"
#include "IPipeline.h"
#include "ConvertPipeline.h"

#define INIT_FRAME_POOL_SZ 2
#define MAX_FRAME_NUM 8

/* public */
ConvertPipeline::ConvertPipeline(const size_t &_data_bytes, const int &_target_pixel_format)
:	AbstractBufferedPipeline(MAX_FRAME_NUM, INIT_FRAME_POOL_SZ, _data_bytes),
	target_pixel_format(_target_pixel_format),
	mFrameConvFunc(NULL)
{
	ENTER();

	updateConvFunc();
	setState(PIPELINE_STATE_INITIALIZED);

	EXIT();
}

/* public */
ConvertPipeline::~ConvertPipeline() {
	ENTER();

	EXIT();
}

void ConvertPipeline::updateConvFunc() {
	ENTER();

	Mutex::Autolock lock(pipeline_mutex);
	mFrameConvFunc = NULL;
	switch (target_pixel_format) {
		case PIXEL_FORMAT_RAW:
			LOGI("PIXEL_FORMAT_RAW:");
			break;
		case PIXEL_FORMAT_YUV:
			LOGI("PIXEL_FORMAT_YUV:");
			mFrameConvFunc = uvc_any2yuyv;
			break;
		case PIXEL_FORMAT_RGB565:
			LOGI("PIXEL_FORMAT_RGB565:");
			mFrameConvFunc = uvc_any2rgb565;
			break;
		case PIXEL_FORMAT_RGBX:
			LOGI("PIXEL_FORMAT_RGBX:");
			mFrameConvFunc = uvc_any2rgbx;
			break;
		case PIXEL_FORMAT_YUV20SP:
			LOGI("PIXEL_FORMAT_YUV20SP:");
			mFrameConvFunc = uvc_any2yuv420SP;
			break;
		case PIXEL_FORMAT_NV21:
			LOGI("PIXEL_FORMAT_NV21:");
			mFrameConvFunc = uvc_any2iyuv420SP;
			break;
	}

	EXIT();
};

void ConvertPipeline::on_start() {
	ENTER();

	updateConvFunc();

	EXIT();
}

void ConvertPipeline::on_stop() {
	ENTER();

	EXIT();
}

int ConvertPipeline::handle_frame(uvc_frame_t *frame) {
	ENTER();

	Mutex::Autolock lock(pipeline_mutex);

	if (next_pipeline) {
		uvc_frame_t *copy = frame;
		if (mFrameConvFunc) {
			copy = get_frame(frame->actual_bytes);
			if (LIKELY(copy)) {
				const uvc_error_t r = mFrameConvFunc(frame, copy);
				if (UNLIKELY(r)) {
					LOGW("failed to convert:%d", r);
					recycle_frame(copy);
					copy = frame;
				}
			}
		}
		next_pipeline->queueFrame(copy);
	}

	RETURN(1, int);
}

//**********************************************************************
//
//**********************************************************************
static ID_TYPE nativeCreate(JNIEnv *env, jobject thiz, jint pixel_format) {

	ENTER();
	ConvertPipeline *pipeline = new ConvertPipeline(DEFAULT_FRAME_SZ, pixel_format);
	setField_long(env, thiz, "mNativePtr", reinterpret_cast<ID_TYPE>(pipeline));
	RETURN(reinterpret_cast<ID_TYPE>(pipeline), ID_TYPE);
}

static void nativeDestroy(JNIEnv *env, jobject thiz,
	ID_TYPE id_pipeline) {

	ENTER();
	setField_long(env, thiz, "mNativePtr", 0);
	ConvertPipeline *pipeline = reinterpret_cast<ConvertPipeline *>(id_pipeline);
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
	ConvertPipeline *pipeline = reinterpret_cast<ConvertPipeline *>(id_pipeline);
	if (pipeline) {
		result = pipeline->getState();
	}
	RETURN(result, jint);
}

static jint nativeSetPipeline(JNIEnv *env, jobject thiz,
	ID_TYPE id_pipeline, jobject pipeline_obj) {

	ENTER();
	jint result = JNI_ERR;
	ConvertPipeline *pipeline = reinterpret_cast<ConvertPipeline *>(id_pipeline);
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
	ConvertPipeline *pipeline = reinterpret_cast<ConvertPipeline *>(id_pipeline);
	if (LIKELY(pipeline)) {
		result = pipeline->start();
	}
	RETURN(result, jint);
}

static jint nativeStop(JNIEnv *env, jobject thiz,
	ID_TYPE id_pipeline) {

	jint result = JNI_ERR;
	ENTER();
	ConvertPipeline *pipeline = reinterpret_cast<ConvertPipeline *>(id_pipeline);
	if (LIKELY(pipeline)) {
		result = pipeline->stop();
	}
	RETURN(result, jint);
}

//**********************************************************************
//
//**********************************************************************
static JNINativeMethod methods[] = {
	{ "nativeCreate",					"(I)J", (void *) nativeCreate },
	{ "nativeDestroy",					"(J)V", (void *) nativeDestroy },

	{ "nativeGetState",					"(J)I", (void *) nativeGetState },
	{ "nativeSetPipeline",				"(JLcom/serenegiant/usb/IPipeline;)I", (void *) nativeSetPipeline },

	{ "nativeStart",					"(J)I", (void *) nativeStart },
	{ "nativeStop",						"(J)I", (void *) nativeStop },

};

int register_convert_pipeline(JNIEnv *env) {
	LOGV("register_convert_pipeline:");
	if (registerNativeMethods(env,
		"com/serenegiant/usb/ConvertPipeline",
		methods, NUM_ARRAY_ELEMENTS(methods)) < 0) {
		return -1;
	}
    return 0;
}
