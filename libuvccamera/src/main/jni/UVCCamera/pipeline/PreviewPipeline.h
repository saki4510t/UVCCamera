//
// Created by saki on 15/11/06.
//

#ifndef PUPILMOBILE_PREVIEWPIPELINE_H
#define PUPILMOBILE_PREVIEWPIPELINE_H

#include <android/native_window.h>

#include "libUVCCamera.h"
#include "CaptureBasePipeline.h"

class PreviewPipeline : virtual public CaptureBasePipeline {
private:
	ANativeWindow *mCaptureWindow;
protected:
	virtual void do_capture(JNIEnv *env);
public:
	PreviewPipeline(const size_t &_data_bytes = DEFAULT_FRAME_SZ);
	virtual ~PreviewPipeline();
	int setCaptureDisplay(ANativeWindow *capture_window);
};


#endif //PUPILMOBILE_PREVIEWPIPELINE_H
