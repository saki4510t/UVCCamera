/*
 * UVCCamera
 * library and sample to access to UVC web camera on non-rooted Android device
 *
 * Copyright (c) 2014 saki t_saki@serenegiant.com
 *
 * File name: utilbase.h
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 * All files in the folder are under this Apache License, Version 2.0.
 * Files in the jni/libjpeg, jni/libusb and jin/libuvc folder may have a different license, see the respective files.
*/

#ifndef UTILBASE_H_
#define UTILBASE_H_

#ifdef __ANDROID__
#include <android/log.h>
#endif
#include <libgen.h>
#include "localdefines.h"

#ifndef NULL
#define	NULL	0
#endif

#if defined(USE_LOGALL) && defined(__ANDROID__)
	#define LOGV(...) __android_log_print(ANDROID_LOG_VERBOSE, LOG_TAG, __VA_ARGS__)
	#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
	#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
	#define LOGW(...) __android_log_print(ANDROID_LOG_WARN, LOG_TAG, __VA_ARGS__)
	#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)
	#define LOGF(...) __android_log_print(ANDROID_LOG_FATAL, LOG_TAG, __VA_ARGS__)
#else
	#if defined(USE_LOGV) && defined(__ANDROID__)
		#define LOGV(...) __android_log_print(ANDROID_LOG_VERBOSE, LOG_TAG, __VA_ARGS__)
	#else
		#define LOGV(...)
	#endif
	#if defined(USE_LOGD) && defined(__ANDROID__)
		#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
	#else
		#define LOGD(...)
	#endif
	#if defined(USE_LOGI) && defined(__ANDROID__)
		#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
	#else
		#define LOGI(...)
	#endif
	#if defined(USE_LOGW) && defined(__ANDROID__)
		#define LOGW(...) __android_log_print(ANDROID_LOG_WARN, LOG_TAG, __VA_ARGS__)
	#else
		#define LOGW(...)
	#endif
	#if defined(USE_LOGE) && defined(__ANDROID__)
		#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)
	#else
		#define LOGE(...)
	#endif
	#if defined(USE_LOGF) && defined(__ANDROID__)
		#define LOGF(...) __android_log_print(ANDROID_LOG_FATAL, LOG_TAG, __VA_ARGS__)
	#else
		#define LOGF(...)
	#endif
#endif

#define		ENTER()			LOGD("[%s:%d] begin %s", basename(__FILE__), __LINE__, __FUNCTION__)
#define		RETURN(code,type)	{type RESULT = code; LOGD("[%s:%d] end %s (%d)", basename(__FILE__), __LINE__, __FUNCTION__, (int)RESULT); return RESULT;}
#define		EXIT()			{LOGD("[%s:%d] end %s", basename(__FILE__), __LINE__, __FUNCTION__); return;}
#define		PRE_EXIT()		LOGD("[%s:%d] end %s", basename(__FILE__), __LINE__, __FUNCTION__)
#define		MARK(msg)		LOGI("[%s:%d] %s:%s", basename(__FILE__), __LINE__, __FUNCTION__, msg)

#define		SAFE_DELETE(p)				{ if (p) { delete (p); (p) = NULL; } }
#define		SAFE_DELETE_ARRAY(p)		{ if (p) { delete [](p); (p) = NULL; } }
#define		NUM_ARRAY_ELEMENTS(p)		((int) sizeof(p) / sizeof(p[0]))

#if defined(__GNUC__)
// the macro for branch prediction optimaization for gcc(-O2/-O3 required)
#define		LIKELY(x)		__builtin_expect(!!(x), 1)	// x is likely true
#define		UNLIKELY(x)		__builtin_expect(!!(x), 0)	// x is likely false
#else
#define		LIKELY(x)		(x)
#define		UNLIKELY(x)		(x)
#endif

#endif /* UTILBASE_H_ */
