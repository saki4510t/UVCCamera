//
// Created by saki on 15/11/23.
//

#ifndef PUPILMOBILE_SIMPLEBUFFEREDPIPELINE_H
#define PUPILMOBILE_SIMPLEBUFFEREDPIPELINE_H

#include "AbstractBufferedPipeline.h"

class SimpleBufferedPipeline : virtual public AbstractBufferedPipeline {
protected:
	virtual void on_start();
	virtual void on_stop();
	virtual int handle_frame(uvc_frame_t *frame);
public:
	SimpleBufferedPipeline(const int &_max_buffer_num = DEFAULT_MAX_FRAME_NUM, const int &init_pool_num = DEFAULT_INIT_FRAME_POOL_SZ,
			const size_t &default_frame_size = DEFAULT_FRAME_SZ, const bool &drop_frames_when_buffer_empty = true);
	virtual ~SimpleBufferedPipeline();
};


#endif //PUPILMOBILE_SIMPLEBUFFEREDPIPELINE_H
