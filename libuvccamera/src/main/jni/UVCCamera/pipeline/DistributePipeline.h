//
// Created by saki on 15/11/25.
//

#ifndef PUPILMOBILE_DISTRIBUTEPIPELINE_H
#define PUPILMOBILE_DISTRIBUTEPIPELINE_H

#include "AbstractBufferedPipeline.h"

#pragma interface

class DistributePipeline : virtual public AbstractBufferedPipeline {
private:
	std::list<IPipeline *> pipelines;
protected:
	virtual void on_start();
	virtual void on_stop();
	virtual int handle_frame(uvc_frame_t *frame);
public:
	DistributePipeline(const int &_max_buffer_num = DEFAULT_MAX_FRAME_NUM, const int &init_pool_num = DEFAULT_INIT_FRAME_POOL_SZ,
			const size_t &default_frame_size = DEFAULT_FRAME_SZ, const bool &drop_frames_when_buffer_empty = true);
	virtual ~DistributePipeline();
	virtual int addPipeline(IPipeline *pipeline);
	virtual int removePipeline(IPipeline *pipeline);
};

#endif //PUPILMOBILE_DISTRIBUTEPIPELINE_H
