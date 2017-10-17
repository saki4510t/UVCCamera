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

#include <stdlib.h>

#include "utilbase.h"
#include "common_utils.h"

#include "libUVCCamera.h"
#include "pipeline_helper.h"
#include "IPipeline.h"

/*public*/
IPipeline::IPipeline(const size_t &_default_frame_size)
:	state(PIPELINE_STATE_UNINITIALIZED),
	mIsRunning(false),
	default_frame_size(_default_frame_size),
	next_pipeline(NULL)
{
	ENTER();

	EXIT();
}

/*public*/
IPipeline::~IPipeline() {
	ENTER();

	EXIT();
}

/*public*/
const bool IPipeline::isRunning() const { return (mIsRunning); };

/*public*/
const pipeline_state_t IPipeline::getState() const { return (state); };

/*protected*/
void IPipeline::setState(const pipeline_state_t &new_state) { state = new_state; }

/*public*/
int IPipeline::setPipeline(IPipeline *pipeline) {
	ENTER();

	Mutex::Autolock lock(pipeline_mutex);

	// XXX do I need to delete next_pipeline if it is not NULL?
	next_pipeline = pipeline;

	RETURN(0, int);
}

/**
 * set frame to next_pipeline
 * if you don't need this, override this function
 */
int IPipeline::chain_frame(uvc_frame_t *frame) {
	ENTER();

	int result = -1;
	Mutex::Autolock lock(pipeline_mutex);

	if (next_pipeline) {
		next_pipeline->queueFrame(frame);
		result = 0;
	}

	RETURN(result, int);
}
