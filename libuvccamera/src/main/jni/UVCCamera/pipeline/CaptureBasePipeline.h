//
// Created by saki on 15/11/07.
//

#ifndef PUPILMOBILE_CAPTUREBASEPIPELINE_H
#define PUPILMOBILE_CAPTUREBASEPIPELINE_H

#include "Mutex.h"
#include "Condition.h"

#include "libUVCCamera.h"
#include "AbstractBufferedPipeline.h"

using namespace android;

class CaptureBasePipeline : virtual public AbstractBufferedPipeline {
private:
	static void *capture_thread_func(void *vptr_args);
	void internal_do_capture(JNIEnv *env);
protected:
	volatile bool mIsCapturing;
	mutable Mutex capture_mutex;
	Condition capture_sync;
	pthread_t capture_thread;
	uvc_frame_t *captureQueue;			// keep latest one frame only
	uint32_t frameWidth;
	uint32_t frameHeight;
	void clearCaptureFrame();
	void addCaptureFrame(uvc_frame_t *frame);
	uvc_frame_t *waitCaptureFrame();

	virtual void on_start();
	virtual void on_stop();
	virtual int handle_frame(uvc_frame_t *frame);
	virtual void do_capture(JNIEnv *env) = 0;
public:
	CaptureBasePipeline(const size_t &_data_bytes = DEFAULT_FRAME_SZ);
	CaptureBasePipeline(const int &_max_buffer_num = DEFAULT_MAX_FRAME_NUM, const int &init_pool_num = DEFAULT_INIT_FRAME_POOL_SZ, const size_t &default_frame_size = DEFAULT_FRAME_SZ);
	virtual ~CaptureBasePipeline();
	const bool isCapturing() const;
};


#endif //PUPILMOBILE_CAPTUREBASEPIPELINE_H
