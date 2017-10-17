//
// Created by saki on 15/11/05.
//

#ifndef PUPILMOBILE_IPIPELINE_H
#define PUPILMOBILE_IPIPELINE_H

#include <stdlib.h>
#include <pthread.h>
#include "Mutex.h"

#include "libUVCCamera.h"

#pragma interface

using namespace android;

#define DEFAULT_FRAME_SZ 1024

typedef enum pipeline_type {
	PIPELINE_TYPE_SIMPLE_BUFFERED = 0,
	PIPELINE_TYPE_SQLITE_BUFFERED = 10,
	PIPELINE_TYPE_UVC_CONTROL = 100,
	PIPELINE_TYPE_CALLBACK = 200,
	PIPELINE_TYPE_CONVERT = 300,
	PIPELINE_TYPE_PREVIEW = 400,
	PIPELINE_TYPE_PUBLISHER = 500,
	PIPELINE_TYPE_DISTRIBUTE = 600,
} pipeline_type_t;

typedef enum _pipeline_state {
	PIPELINE_STATE_UNINITIALIZED = 0,
	PIPELINE_STATE_RELEASING = 10,
	PIPELINE_STATE_INITIALIZED = 20,
	PIPELINE_STATE_STARTING = 30,
	PIPELINE_STATE_RUNNING = 40,
	PIPELINE_STATE_STOPPING = 50,
} pipeline_state_t;

class IPipeline;

class IPipeline {
private:
	volatile pipeline_state_t state;
	// force inhibiting copy/assignment
	IPipeline(const IPipeline &src);
	void operator =(const IPipeline &src);
protected:
	volatile bool mIsRunning;
	const size_t default_frame_size;
	mutable Mutex pipeline_mutex;
	IPipeline *next_pipeline;
	void setState(const pipeline_state_t &new_state);
	/**
	 * if handle_frame return 0, handler_thread call this function
	 * set frame to next pipeline
	 * this may block caller thread while the pipeline is full
	 * @return 0: success queueing, other: failed
	 */
	virtual int chain_frame(uvc_frame_t *frame);
public:
	IPipeline(const size_t &default_frame_size = DEFAULT_FRAME_SZ);
	virtual ~IPipeline();
	const pipeline_state_t getState() const;
	const bool isRunning() const;
	virtual int setPipeline(IPipeline *pipeline);
	virtual int release() { return 0; };
	virtual int start() { return 0; };
	virtual int stop() { return 0; };
	virtual int queueFrame(uvc_frame_t *frame) = 0;
};


#endif //PUPILMOBILE_IPIPELINE_H
