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
#include "AbstractBufferedPipeline.h"

/*public*/
AbstractBufferedPipeline::AbstractBufferedPipeline(const int &_max_buffer_num, const int &_init_pool_num,
	const size_t &_default_frame_size, const bool &drop_frames_when_buffer_empty)
:	IPipeline(_default_frame_size),
	max_buffer_num(_max_buffer_num),
	init_pool_num(_init_pool_num),
	drop_frames(drop_frames_when_buffer_empty),
	total_frame_num(0)
{
	ENTER();

	EXIT();
}

/*public*/
AbstractBufferedPipeline::~AbstractBufferedPipeline() {
	ENTER();

	release();
	setState(PIPELINE_STATE_UNINITIALIZED);

	EXIT();
}

/*public*/
int AbstractBufferedPipeline::release() {
	ENTER();

	setState(PIPELINE_STATE_RELEASING);
	stop();
	clear_frames();
	clear_pool();
	setState(PIPELINE_STATE_UNINITIALIZED);

	RETURN(0, int);
}

/*public*/
int AbstractBufferedPipeline::start() {
	ENTER();

	int result = EXIT_FAILURE;
	if (!isRunning()) {
		mIsRunning = true;
		setState(PIPELINE_STATE_STARTING);
		buffer_mutex.lock();
		{
			result = pthread_create(&handler_thread, NULL, handler_thread_func, (void *) this);
		}
		buffer_mutex.unlock();
		if (UNLIKELY(result != EXIT_SUCCESS)) {
			LOGW("AbstractBufferedPipeline::already running/could not create thread etc.");
			setState(PIPELINE_STATE_INITIALIZED);
			mIsRunning = false;
			buffer_mutex.lock();
			{
				pool_sync.broadcast();
				buffer_sync.broadcast();
			}
			buffer_mutex.unlock();
		}
	}
	RETURN(result, int);
}

/*public*/
int AbstractBufferedPipeline::stop() {
	ENTER();

	bool b = isRunning();
	if (LIKELY(b)) {
		setState(PIPELINE_STATE_STOPPING);
		mIsRunning = false;
		pool_sync.broadcast();
		buffer_sync.broadcast();
		LOGD("pthread_join:handler_thread");
		if (pthread_join(handler_thread, NULL) != EXIT_SUCCESS) {
			LOGW("PublisherPipeline::terminate publisher thread: pthread_join failed");
		}
		setState(PIPELINE_STATE_INITIALIZED);
		LOGD("handler_thread finished");
	}
	clear_frames();

	RETURN(0, int);
}

/*public*/
int AbstractBufferedPipeline::queueFrame(uvc_frame_t *frame) {
	ENTER();

	int ret = UVC_ERROR_OTHER;
	if (LIKELY(frame)) {
		// get empty frame from frame pool
		uvc_frame_t *copy = get_frame(frame->data_bytes);
		if (UNLIKELY(!copy)) {
			LOGD("buffer pool is empty and exceeds the limit, drop frame");
			RETURN(UVC_ERROR_NO_MEM, int);
		}
		// duplicate frame buffer and pass copy to publisher
		ret = uvc_duplicate_frame(frame, copy);
		if (LIKELY(!ret)) {
			ret = add_frame(copy);
		} else {
			LOGW("uvc_duplicate_frame failed:%d", ret);
			recycle_frame(copy);
		}
	}

	RETURN(ret, int);
}

//********************************************************************************
//
//********************************************************************************
/**
 * get uvc_frame_t from frame pool
 * if pool is empty, create new frame
 * this function does not confirm the frame size
 * and you may need to confirm the size
 */
uvc_frame_t *AbstractBufferedPipeline::get_frame(const size_t &data_bytes) {
	uvc_frame_t *frame = NULL;
	Mutex::Autolock lock(pool_mutex);

	if (UNLIKELY(frame_pool.empty() && (total_frame_num < max_buffer_num))) {
		uint32_t n = total_frame_num * 2;
		if (n > max_buffer_num) {
			n = max_buffer_num;
		}
		n -= total_frame_num;
		if (LIKELY(n > 0)) {
			for (int i = 0; i < n; i++) {
				frame = uvc_allocate_frame(data_bytes);
				total_frame_num++;
			}
			LOGW("allocate new frame:%d", total_frame_num);
		} else {
			LOGW("number of allocated frame exceeds limit");
		}
	}
	if (UNLIKELY(frame_pool.empty() && !drop_frames)) {
		// if pool is empty and need to block(avoid dropping frames), wait frame recycling.
		for (; mIsRunning && frame_pool.empty() ; ) {
			pool_sync.wait(pool_mutex);
		}
	}
	if (!frame_pool.empty()) {
		frame = frame_pool.front();
		frame_pool.pop_front();
	}

	return frame;
}

void AbstractBufferedPipeline::recycle_frame(uvc_frame_t *frame) {
	ENTER();

	if (LIKELY(frame)) {
		Mutex::Autolock lock(pool_mutex);
		if (LIKELY(frame_pool.size() < max_buffer_num)) {
			frame_pool.push_back(frame);
			frame = NULL;
		}
		if (UNLIKELY(frame)) {
			// if pool overflowed
			total_frame_num--;
			uvc_free_frame(frame);
		}
		pool_sync.signal();
	}

	EXIT();
}

void AbstractBufferedPipeline::init_pool(const size_t &data_bytes) {
	ENTER();

	uvc_frame_t *frame = NULL;

	clear_pool();
	pool_mutex.lock();
	{

		size_t frame_sz = data_bytes / 4;	// expects 25%, this will be able to much lower
		if (!frame_sz) {
			frame_sz = DEFAULT_FRAME_SZ;
		}
		for (uint32_t i = 0; i < init_pool_num; i++) {
			frame = uvc_allocate_frame(frame_sz);
			if (LIKELY(frame)) {
				frame_pool.push_back(frame);
				total_frame_num++;
			} else {
				LOGW("failed to allocate new frame:%d", total_frame_num);
				break;
			}
		}
	}
	pool_mutex.unlock();

	EXIT();
}

void AbstractBufferedPipeline::clear_pool() {
	ENTER();

	Mutex::Autolock lock(pool_mutex);

	for (auto iter = frame_pool.begin(); iter != frame_pool.end(); iter++) {
		total_frame_num--;
		uvc_free_frame(*iter);
	}
	frame_pool.clear();
	EXIT();
}

//********************************************************************************
//
//********************************************************************************

void AbstractBufferedPipeline::clear_frames() {
	Mutex::Autolock lock(buffer_mutex);

	for (auto iter = frame_buffers.begin(); iter != frame_buffers.end(); iter++) {
		recycle_frame(*iter);
	}
	frame_buffers.clear();
}

int AbstractBufferedPipeline::add_frame(uvc_frame_t *frame) {
	ENTER();

	buffer_mutex.lock();
	{
		// FIXME as current implementation, transferring frame data on my device is slower than that coming from UVC camera... just drop them now
		if (frame_buffers.size() > max_buffer_num) {
			// erase old frames
			int cnt = 0;
			for (auto iter = frame_buffers.begin();
				 (iter != frame_buffers.end()) && (cnt < 5); iter++, cnt++) {
				recycle_frame(*iter);
				iter = frame_buffers.erase(iter);
			}
			LOGW("droped frame data");
		}
		if (isRunning() && (frame_buffers.size() < max_buffer_num)) {
			frame_buffers.push_back(frame);
			frame = NULL;
		}
		buffer_sync.signal();
	}
	buffer_mutex.unlock();
	if (frame) {
		recycle_frame(frame);
	}

	RETURN(0, int);
}

uvc_frame_t *AbstractBufferedPipeline::wait_frame() {
	uvc_frame_t *frame = NULL;

	Mutex::Autolock lock(buffer_mutex);

	if (!frame_buffers.size()) {
		buffer_sync.wait(buffer_mutex);
	}
	if (LIKELY(isRunning() && frame_buffers.size() > 0)) {
		frame = frame_buffers.front();
		frame_buffers.pop_front();
	}
	return frame;
}

uint32_t AbstractBufferedPipeline::get_frame_count() {
	ENTER();

	Mutex::Autolock lock(buffer_mutex);
	uint32_t result = frame_buffers.size();

	RETURN(result, uint32_t);
}

//********************************************************************************
//
//********************************************************************************
void *AbstractBufferedPipeline::handler_thread_func(void *vptr_args) {

	ENTER();
	AbstractBufferedPipeline *pipeline = reinterpret_cast<AbstractBufferedPipeline *>(vptr_args);
	if (LIKELY(pipeline)) {
		pipeline->do_loop();
	}
	PRE_EXIT();
	pthread_exit(NULL);
}

void AbstractBufferedPipeline::do_loop() {
	ENTER();

	clear_frames();
	init_pool(default_frame_size);

	on_start();
	setState(PIPELINE_STATE_RUNNING);
	for ( ; LIKELY(isRunning()) ; ) {
		uvc_frame_t *frame = wait_frame();
		if ((LIKELY(frame))) {
			try {
				if (!handle_frame(frame)) {
					chain_frame(frame);
				}
			} catch (...) {
				LOGE("exception");
			}
			recycle_frame(frame);
		}
	}
	setState(PIPELINE_STATE_STOPPING);
	mIsRunning = false;
	on_stop();
	setState(PIPELINE_STATE_INITIALIZED);

	EXIT();
}
