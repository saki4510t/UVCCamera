//
// Created by saki on 15/10/06.
//

#ifndef PUPILMOBILE_PUBLISHER_PIPELINE_H
#define PUPILMOBILE_PUBLISHER_PIPELINE_H

#pragma interface

#include <string>
#include "Mutex.h"
#include "Timers.h"
#include "zmq.hpp"

#include "pupilmobile_defs.h"
#include "AbstractBufferedPipeline.h"

using namespace android;

class PublisherPipeline : virtual public AbstractBufferedPipeline {
private:
protected:
	const std::string host;
	const std::string subscription_id;
	size_t data_bytes;
	zmq::context_t *context;
	zmq::socket_t *publisher;
	mutable Mutex publisher_mutex;
	virtual void on_start();
	virtual void on_stop();
	virtual int handle_frame(uvc_frame_t *frame);
public:
	PublisherPipeline(const size_t &_data_bytes = DEFAULT_FRAME_SZ, const char *addr = NULL, const char *subscription_id = NULL);
	PublisherPipeline(const char *addr, const char *subscription_id);
	virtual ~PublisherPipeline();
	virtual int queueFrame(uvc_frame_t *frame);
};

#endif //PUPILMOBILE_PUBLISHER_PIPELINE_H
