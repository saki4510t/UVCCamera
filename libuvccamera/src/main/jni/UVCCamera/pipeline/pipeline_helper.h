//
// Created by saki on 15/11/12.
//

#ifndef PUPILMOBILE_PIPELINE_HELPER_H_H
#define PUPILMOBILE_PIPELINE_HELPER_H_H

#include "common_utils.h"

#include "IPipeline.h"

IPipeline *getPipeline(JNIEnv *env, jobject pipeline_obj);

#endif //PUPILMOBILE_PIPELINE_HELPER_H_H
