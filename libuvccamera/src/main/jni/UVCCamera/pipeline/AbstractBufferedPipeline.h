//
// Created by saki on 15/11/05.
//

#ifndef PUPILMOBILE_ABSTRACTBUFFEREDPIPELINE_H
#define PUPILMOBILE_ABSTRACTBUFFEREDPIPELINE_H

#include <stdlib.h>
#include <pthread.h>
#include <list>
#include "Mutex.h"
#include "Condition.h"

#include "libUVCCamera.h"
#include "IPipeline.h"

#pragma interface

#define DEFAULT_INIT_FRAME_POOL_SZ 2
#define DEFAULT_MAX_FRAME_NUM 8

using namespace android;

class AbstractBufferedPipeline;

class AbstractBufferedPipeline : virtual public IPipeline {
private:
	const uint32_t max_buffer_num;
	const uint32_t init_pool_num;
	const bool drop_frames;
	volatile uint32_t total_frame_num;

// frame buffer pool to improve performance by reducing memory allocation
	mutable Mutex pool_mutex;
	Condition pool_sync;
	std::list<uvc_frame_t *> frame_pool;
// frame buffers
	pthread_t handler_thread;
	mutable Mutex buffer_mutex;
	Condition buffer_sync;
	std::list<uvc_frame_t *> frame_buffers;
	static void *handler_thread_func(void *vptr_args);

protected:
// frame buffer pool
	uvc_frame_t *get_frame(const size_t &data_bytes);
	void recycle_frame(uvc_frame_t *frame);
	void init_pool(const size_t &data_bytes);
	void clear_pool();
// frame buffers
	void clear_frames();
	int add_frame(uvc_frame_t *frame);
	uvc_frame_t *wait_frame();
	uint32_t get_frame_count();
	virtual void do_loop();
	virtual void on_start() = 0;
	virtual void on_stop() = 0;
	virtual int handle_frame(uvc_frame_t *frame) = 0;
public:
	AbstractBufferedPipeline(const int &_max_buffer_num = DEFAULT_MAX_FRAME_NUM, const int &init_pool_num = DEFAULT_INIT_FRAME_POOL_SZ,
		const size_t &default_frame_size = DEFAULT_FRAME_SZ, const bool &drop_frames_when_buffer_empty = true);
	virtual ~AbstractBufferedPipeline();
	virtual int release();
	virtual int start();
	virtual int stop();
	virtual int queueFrame(uvc_frame_t *frame);
};


#endif //PUPILMOBILE_ABSTRACTBUFFEREDPIPELINE_H
