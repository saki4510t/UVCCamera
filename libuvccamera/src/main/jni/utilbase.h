/*
 * UVCCamera
 * library and sample to access to UVC web camera on non-rooted Android device
 *
 * Copyright (c) 2014-2017 saki t_saki@serenegiant.com
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
 * Files in the jni/libjpeg, jni/libusb, jin/libuvc, jni/rapidjson folder may have a different license, see the respective files.
*/

#ifndef UTILBASE_H_
#define UTILBASE_H_

#include <jni.h>
#ifdef __ANDROID__
#include <android/log.h>
#endif
#include <unistd.h>
#include <libgen.h>
#include "localdefines.h"

#define		SAFE_FREE(p)				{ if (p) { free((p)); (p) = NULL; } }
#define		SAFE_DELETE(p)				{ if (p) { delete (p); (p) = NULL; } }
#define		SAFE_DELETE_ARRAY(p)		{ if (p) { delete [](p); (p) = NULL; } }
#define		NUM_ARRAY_ELEMENTS(p)		((int) sizeof(p) / sizeof(p[0]))

#if defined(__GNUC__)
// the macro for branch prediction optimaization for gcc(-O2/-O3 required)
#define		CONDITION(cond)				((__builtin_expect((cond)!=0, 0)))
#define		LIKELY(x)					((__builtin_expect(!!(x), 1)))	// x is likely true
#define		UNLIKELY(x)					((__builtin_expect(!!(x), 0)))	// x is likely false
#else
#define		CONDITION(cond)				((cond))
#define		LIKELY(x)					((x))
#define		UNLIKELY(x)					((x))
#endif

// XXX assertはNDEBUGが定義されていたら引数を含めて丸ごと削除されてしまうので
// 関数実行を直接assertの引数にするとその関数はNDEBUGの時に実行されなくなるので注意
#include <assert.h>
#define CHECK(CONDITION) { bool RES = (CONDITION); assert(RES); }
#define CHECK_EQ(X, Y) { bool RES = (X == Y); assert(RES); }
#define CHECK_NE(X, Y) { bool RES = (X != Y); assert(RES); }
#define CHECK_GE(X, Y) { bool RES = (X >= Y); assert(RES); }
#define CHECK_GT(X, Y) { bool RES = (X > Y); assert(RES); }
#define CHECK_LE(X, Y) { bool RES = (X <= Y); assert(RES); }
#define CHECK_LT(X, Y) { bool RES = (X < Y); assert(RES); }

#if defined(USE_LOGALL) && defined(__ANDROID__) && !defined(LOG_NDEBUG)
	#define LOGV(FMT, ...) __android_log_print(ANDROID_LOG_VERBOSE, LOG_TAG, "[%d*%s:%d:%s]:" FMT,	\
							gettid(), basename(__FILE__), __LINE__, __FUNCTION__, ## __VA_ARGS__)
	#define LOGD(FMT, ...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, "[%d*%s:%d:%s]:" FMT,	\
							gettid(), basename(__FILE__), __LINE__, __FUNCTION__, ## __VA_ARGS__)
	#define LOGI(FMT, ...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, "[%d*%s:%d:%s]:" FMT,	\
							gettid(), basename(__FILE__), __LINE__, __FUNCTION__, ## __VA_ARGS__)
	#define LOGW(FMT, ...) __android_log_print(ANDROID_LOG_WARN, LOG_TAG, "[%d*%s:%d:%s]:" FMT,	\
							gettid(), basename(__FILE__), __LINE__, __FUNCTION__, ## __VA_ARGS__)
	#define LOGE(FMT, ...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, "[%d*%s:%d:%s]:" FMT,	\
							gettid(), basename(__FILE__), __LINE__, __FUNCTION__, ## __VA_ARGS__)
	#define LOGF(FMT, ...) __android_log_print(ANDROID_LOG_FATAL, LOG_TAG, "[%d*%s:%d:%s]:" FMT,	\
							gettid(), basename(__FILE__), __LINE__, __FUNCTION__, ## __VA_ARGS__)
	#define LOGV_IF(cond, ...) \
		( (CONDITION(cond)) \
			? LOGV(__VA_ARGS__) \
			: (0) )
	#define LOGD_IF(cond, ...) \
		( (CONDITION(cond)) \
			? LOGD(__VA_ARGS__) \
			: (0) )
	#define LOGI_IF(cond, ...) \
		( (CONDITION(cond)) \
			? LOGI(__VA_ARGS__) \
			: (0) )
	#define LOGW_IF(cond, ...) \
		( (CONDITION(cond)) \
			? LOGW(__VA_ARGS__) \
			: (0) )
	#define LOGE_IF(cond, ...) \
		( (CONDITION(cond)) \
			? LOGE(__VA_ARGS__) \
			: (0) )
	#define LOGF_IF(cond, ...) \
		( (CONDITION(cond)) \
			? LOGF(__VA_ARGS__) \
			: (0) )
#else
	#if defined(USE_LOGV) && defined(__ANDROID__) && !defined(LOG_NDEBUG)
		#define LOGV(FMT, ...) __android_log_print(ANDROID_LOG_VERBOSE, LOG_TAG, "[%d*%s:%d:%s]:" FMT,	\
         	 	 	 	 	 	 gettid(), basename(__FILE__), __LINE__, __FUNCTION__, ## __VA_ARGS__)
		#define LOGV_IF(cond, ...) \
			( (CONDITION(cond)) \
			? LOGV(__VA_ARGS__) \
			: (0) )
		#else
		#define LOGV(...)
		#define LOGV_IF(cond, ...)
	#endif
	#if defined(USE_LOGD) && defined(__ANDROID__) && !defined(LOG_NDEBUG)
		#define LOGD(FMT, ...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, "[%d*%s:%d:%s]:" FMT,	\
         	 	 	 	 	 	 gettid(), basename(__FILE__), __LINE__, __FUNCTION__, ## __VA_ARGS__)
		#define LOGD_IF(cond, ...) \
			( (CONDITION(cond)) \
			? LOGD(__VA_ARGS__) \
			: (0) )
	#else
		#define LOGD(...)
		#define LOGD_IF(cond, ...)
	#endif
	#if defined(USE_LOGI) && defined(__ANDROID__)
		#define LOGI(FMT, ...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, "[%d*%s:%d:%s]:" FMT,	\
         	 	 	 	 	 	 gettid(), basename(__FILE__), __LINE__, __FUNCTION__, ## __VA_ARGS__)
		#define LOGI_IF(cond, ...) \
			( (CONDITION(cond)) \
			? LOGI(__VA_ARGS__) \
			: (0) )
	#else
		#define LOGI(...)
		#define LOGI_IF(cond, ...)
	#endif
	#if defined(USE_LOGW) && defined(__ANDROID__)
		#define LOGW(FMT, ...) __android_log_print(ANDROID_LOG_WARN, LOG_TAG, "[%d*%s:%d:%s]:" FMT,	\
         	 	 	 	 	 	 gettid(), basename(__FILE__), __LINE__, __FUNCTION__, ## __VA_ARGS__)
		#define LOGW_IF(cond, ...) \
			( (CONDITION(cond)) \
			? LOGW(__VA_ARGS__) \
			: (0) )
	#else
		#define LOGW(...)
		#define LOGW_IF(cond, ...)
	#endif
	#if defined(USE_LOGE) && defined(__ANDROID__)
		#define LOGE(FMT, ...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, "[%d*%s:%d:%s]:" FMT,	\
         	 	 	 	 	 	 gettid(), basename(__FILE__), __LINE__, __FUNCTION__, ## __VA_ARGS__)
		#define LOGE_IF(cond, ...) \
			( (CONDITION(cond)) \
			? LOGE(__VA_ARGS__) \
			: (0) )
	#else
		#define LOGE(...)
		#define LOGE_IF(cond, ...)
	#endif
	#if defined(USE_LOGF) && defined(__ANDROID__)
		#define LOGF(FMT, ...) __android_log_print(ANDROID_LOG_FATAL, LOG_TAG, "[%d*%s:%d:%s]:" FMT,	\
         	 	 	 	 	 	 gettid(), basename(__FILE__), __LINE__, __FUNCTION__, ## __VA_ARGS__)
		#define LOGF_IF(cond, ...) \
			( (CONDITION(cond)) \
			? LOGF(__VA_ARGS__) \
			: (0) )
	#else
		#define LOGF(...)
		#define LOGF_IF(cond, ...)
	#endif
#endif

#ifndef		LOG_ALWAYS_FATAL_IF
#define		LOG_ALWAYS_FATAL_IF(cond, ...) \
				( (CONDITION(cond)) \
				? ((void)__android_log_assert(#cond, LOG_TAG, ## __VA_ARGS__)) \
				: (void)0 )
#endif

#ifndef		LOG_ALWAYS_FATAL
#define		LOG_ALWAYS_FATAL(...) \
				( ((void)__android_log_assert(NULL, LOG_TAG, ## __VA_ARGS__)) )
#endif

#ifndef		LOG_ASSERT
#define		LOG_ASSERT(cond, ...) LOG_FATAL_IF(!(cond), ## __VA_ARGS__)
#endif

#ifdef LOG_NDEBUG

#ifndef		LOG_FATAL_IF
#define		LOG_FATAL_IF(cond, ...) ((void)0)
#endif
#ifndef		LOG_FATAL
#define		LOG_FATAL(...) ((void)0)
#endif

#else

#ifndef		LOG_FATAL_IF
#define		LOG_FATAL_IF(cond, ...) LOG_ALWAYS_FATAL_IF(cond, ## __VA_ARGS__)
#endif
#ifndef		LOG_FATAL
#define		LOG_FATAL(...) LOG_ALWAYS_FATAL(__VA_ARGS__)
#endif

#endif

#define		ENTER()				LOGD("begin")
#define		RETURN(code,type)	{type RESULT = code; LOGD("end (%d)", (int)RESULT); return RESULT;}
#define		RET(code)			{LOGD("end"); return code;}
#define		EXIT()				{LOGD("end"); return;}
#define		PRE_EXIT()			LOGD("end")

#if defined(__ANDROID__) && (defined(USE_LOGALL) || defined(USE_LOGI)) && !defined(LOG_NDEBUG)
#define MARK(FMT, ...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, "[%s:%d:%s]:" FMT,	\
						basename(__FILE__), __LINE__, __FUNCTION__, ## __VA_ARGS__)
#else
#define		MARK(...)
#endif

#define LITERAL_TO_STRING_INTERNAL(x)    #x
#define LITERAL_TO_STRING(x) LITERAL_TO_STRING_INTERNAL(x)

#define TRESPASS() \
		LOG_ALWAYS_FATAL(                                       \
			__FILE__ ":" LITERAL_TO_STRING(__LINE__)            \
			" Should not be here.");

void setVM(JavaVM *);
JavaVM *getVM();
JNIEnv *getEnv();

#endif /* UTILBASE_H_ */
