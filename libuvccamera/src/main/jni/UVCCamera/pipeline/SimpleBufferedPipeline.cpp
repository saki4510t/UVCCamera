//
// Created by saki on 15/11/23.
//

#include "utilbase.h"
#include "common_utils.h"

#include "libUVCCamera.h"
#include "pipeline_helper.h"
#include "IPipeline.h"
#include "SimpleBufferedPipeline.h"

SimpleBufferedPipeline::SimpleBufferedPipeline(const int &_max_buffer_num, const int &init_pool_num,
		const size_t &default_frame_size, const bool &drop_frames_when_buffer_empty)
:	AbstractBufferedPipeline(_max_buffer_num, init_pool_num, default_frame_size, drop_frames_when_buffer_empty)
{
	ENTER();

	setState(PIPELINE_STATE_INITIALIZED);

	EXIT();
}

SimpleBufferedPipeline::~SimpleBufferedPipeline() {
	ENTER();
	EXIT();
}

void SimpleBufferedPipeline::on_start() {
	ENTER();
	EXIT();
}

void SimpleBufferedPipeline::on_stop() {
	ENTER();
	EXIT();
}

int SimpleBufferedPipeline::handle_frame(uvc_frame_t *frame) {
	ENTER();

	RETURN(0, int);
}

//**********************************************************************
//
//**********************************************************************
static ID_TYPE nativeCreate(JNIEnv *env, jobject thiz) {

	ENTER();
	SimpleBufferedPipeline *pipeline = new SimpleBufferedPipeline();
	setField_long(env, thiz, "mNativePtr", reinterpret_cast<ID_TYPE>(pipeline));
	RETURN(reinterpret_cast<ID_TYPE>(pipeline), ID_TYPE);
}

static void nativeDestroy(JNIEnv *env, jobject thiz,
	ID_TYPE id_pipeline) {

	ENTER();
	setField_long(env, thiz, "mNativePtr", 0);
	SimpleBufferedPipeline *pipeline = reinterpret_cast<SimpleBufferedPipeline *>(id_pipeline);
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
	SimpleBufferedPipeline *pipeline = reinterpret_cast<SimpleBufferedPipeline *>(id_pipeline);
	if (pipeline) {
		result = pipeline->getState();
	}
	RETURN(result, jint);
}

static jint nativeSetPipeline(JNIEnv *env, jobject thiz,
	ID_TYPE id_pipeline, jobject pipeline_obj) {

	ENTER();
	jint result = JNI_ERR;
	SimpleBufferedPipeline *pipeline = reinterpret_cast<SimpleBufferedPipeline *>(id_pipeline);
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
	SimpleBufferedPipeline *pipeline = reinterpret_cast<SimpleBufferedPipeline *>(id_pipeline);
	if (LIKELY(pipeline)) {
		result = pipeline->start();
	}
	RETURN(result, jint);
}

static jint nativeStop(JNIEnv *env, jobject thiz,
	ID_TYPE id_pipeline) {

	jint result = JNI_ERR;
	ENTER();
	SimpleBufferedPipeline *pipeline = reinterpret_cast<SimpleBufferedPipeline *>(id_pipeline);
	if (LIKELY(pipeline)) {
		result = pipeline->stop();
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
};

int register_simple_buffered_pipeline(JNIEnv *env) {
	LOGV("register_simple_buffered_pipeline:");
	if (registerNativeMethods(env,
		"com/serenegiant/usb/SimpleBufferedPipeline",
		methods, NUM_ARRAY_ELEMENTS(methods)) < 0) {
		return -1;
	}
    return 0;
}
