//
// Created by saki on 15/11/07.
//

#ifndef PUPILMOBILE_CALLBACKPIPELINE_H
#define PUPILMOBILE_CALLBACKPIPELINE_H

#include "libUVCCamera.h"
#include "CaptureBasePipeline.h"

class CallbackPipeline : virtual public CaptureBasePipeline {
private:
	jobject mFrameCallbackObj;
	convFunc_t mFrameCallbackFunc;
	Fields_iframecallback iframecallback_fields;
	int mPixelFormat;
	size_t callbackPixelBytes;
	void callbackPixelFormatChanged(const uint32_t &width, const uint32_t &height);
protected:
	virtual void do_capture(JNIEnv *env);
public:
	CallbackPipeline(const size_t &_data_bytes = DEFAULT_FRAME_SZ);
	virtual ~CallbackPipeline();
	int setFrameCallback(JNIEnv *env, jobject frame_callback_obj, int pixel_format);
};

#endif //PUPILMOBILE_CALLBACKPIPELINE_H
