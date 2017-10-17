//
// Created by saki on 15/11/07.
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

#include "CaptureBasePipeline.h"

#define INIT_FRAME_POOL_SZ 2
#define MAX_FRAME_NUM 8

CaptureBasePipeline::CaptureBasePipeline(const size_t &_data_bytes)
:	AbstractBufferedPipeline(MAX_FRAME_NUM, INIT_FRAME_POOL_SZ, _data_bytes),
	mIsCapturing(false),
	captureQueue(NULL),
	frameWidth(0),
	frameHeight(0)
{
	ENTER();

	EXIT();
}

CaptureBasePipeline::CaptureBasePipeline(const int &_max_buffer_num, const int &init_pool_num, const size_t &default_frame_size)
:	AbstractBufferedPipeline(_max_buffer_num, init_pool_num, default_frame_size),
	mIsCapturing(false),
	captureQueue(NULL),
	frameWidth(0),
	frameHeight(0)
{
	ENTER();

	EXIT();
}


CaptureBasePipeline::~CaptureBasePipeline() {
	ENTER();

	clearCaptureFrame();

	EXIT();
}

//********************************************************************************
//
//********************************************************************************
const bool CaptureBasePipeline::isCapturing() const { return mIsCapturing; }

/**
 * clear frame data for capturing
 */
void CaptureBasePipeline::clearCaptureFrame() {
	Mutex::Autolock lock(capture_mutex);

	if (captureQueue)
		recycle_frame(captureQueue);
	captureQueue = NULL;
}

void CaptureBasePipeline::addCaptureFrame(uvc_frame_t *frame) {
//	ENTER();

	Mutex::Autolock lock(capture_mutex);

	// keep only latest one
	if (captureQueue) {
		recycle_frame(captureQueue);
		captureQueue = NULL;
	}
	if (LIKELY(isRunning())) {
		captureQueue = frame;
		capture_sync.signal();
	} else {
		recycle_frame(frame);
	}

//	EXIT();
}

/**
 * get frame data for capturing, if not exist, block and wait
 */
uvc_frame_t *CaptureBasePipeline::waitCaptureFrame() {
	uvc_frame_t *frame = NULL;
	Mutex::Autolock lock(capture_mutex);

	if (!captureQueue) {
		capture_sync.wait(capture_mutex);
	}
	if (LIKELY(isRunning() && captureQueue)) {
		frame = captureQueue;
		captureQueue = NULL;
	}
	return frame;
}

/* override protected */
void CaptureBasePipeline::on_start() {
	ENTER();

	mIsCapturing = true;
	pthread_create(&capture_thread, NULL, capture_thread_func, (void *)this);

	EXIT();
}

/* override protected */
void CaptureBasePipeline::on_stop() {
	ENTER();

	mIsCapturing = false;
	capture_sync.broadcast();
	if (pthread_join(capture_thread, NULL) != EXIT_SUCCESS) {
		LOGW("UVCCameraControl::terminate capture thread: pthread_join failed");
	}
	clearCaptureFrame();

	EXIT();
}

/* override protected */
int CaptureBasePipeline::handle_frame(uvc_frame_t *frame) {
//	ENTER();

	if (LIKELY(frame)) {
		// get empty frame from frame pool
		uvc_frame_t *copy = get_frame(frame->data_bytes);
		if (LIKELY(copy)) {
			// duplicate frame buffer and pass copy
			uvc_error_t ret = uvc_duplicate_frame(frame, copy);
			if (LIKELY(!ret)) {
				addCaptureFrame(copy);
			} else {
				LOGW("uvc_duplicate_frame failed:%d", ret);
				recycle_frame(copy);
			}
		} else {
			LOGW("buffer pool is empty and exceeds the limit, drop frame");
		}
	}

	return 0; // 	RETURN(0, int);
}

/*
 * thread function
 * @param vptr_args pointer to UVCCameraControl instance
 */
// static
void *CaptureBasePipeline::capture_thread_func(void *vptr_args) {

	ENTER();

	CaptureBasePipeline *pipeline = reinterpret_cast<CaptureBasePipeline *>(vptr_args);
	if (LIKELY(pipeline)) {
		JavaVM *vm = getVM();
		JNIEnv *env;
		// attach to JavaVM
		vm->AttachCurrentThread(&env, NULL);
		pipeline->internal_do_capture(env);	// never return until finish streaming
		// detach from JavaVM
		vm->DetachCurrentThread();
		MARK("DetachCurrentThread");
	}

	PRE_EXIT();
	pthread_exit(NULL);
}

/**
 * the actual function for capturing
 */
void CaptureBasePipeline::internal_do_capture(JNIEnv *env) {

	ENTER();

	clearCaptureFrame();
	for (; isRunning() ;) {
		mIsCapturing = true;
		do_capture(env);
		capture_sync.broadcast();
	}	// end of for (; isRunning() ;)

	EXIT();
}
