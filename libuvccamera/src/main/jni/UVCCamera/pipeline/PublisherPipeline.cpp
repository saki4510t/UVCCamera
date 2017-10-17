//
// Created by saki on 15/10/06.
//

#define MEAS_TIME 0

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

#if MEAS_TIME
#define MEAS_TIME_INIT	nsecs_t _meas_time_ = 0; int _meas_count_ = 0;
#define MEAS_TIME_START	const nsecs_t _meas_t_ = systemTime();
#define MEAS_TIME_STOP \
	_meas_time_ += (systemTime() - _meas_t_); \
	if UNLIKELY((++_meas_count_ % 100) == 0) { \
		const float d = _meas_time_ / (1000000.f * _meas_count_); \
		LOGI("meas time=%5.2f[msec]", d); \
	}
#else
#define MEAS_TIME_INIT
#define MEAS_TIME_START
#define MEAS_TIME_STOP
#endif

#include <stdlib.h>
#include <linux/time.h>
#include <unistd.h>

#include "utilbase.h"
#include "common_utils.h"

#include "libUVCCamera.h"
#include "endian_unaligned.h"

#include "pipeline_helper.h"
#include "PublisherPipeline.h"
#include "pupilmobile_defs.h"

#define INIT_FRAME_POOL_SZ 2
#define MAX_FRAME_NUM 8
#define RETRY_INTERVALS_US 25000

/* public */
PublisherPipeline::PublisherPipeline(const size_t &_data_bytes, const char *addr, const char *_subscription_id)
:	AbstractBufferedPipeline(MAX_FRAME_NUM, INIT_FRAME_POOL_SZ, _data_bytes),
	host(addr),
	subscription_id(_subscription_id),
	data_bytes(_data_bytes),
	context(NULL),
	publisher(NULL)
{
	ENTER();

	setState(PIPELINE_STATE_INITIALIZED);

	EXIT();
}

/* public */
PublisherPipeline::PublisherPipeline(const char *addr, const char *_subscription_id)
:	PublisherPipeline(DEFAULT_FRAME_SZ, addr, _subscription_id)
{
}

/* public */
PublisherPipeline::~PublisherPipeline() {
	ENTER();

	LOGI("destructor finished");

	EXIT();
}

/*public*/
int PublisherPipeline::queueFrame(uvc_frame_t *frame) {
//	ENTER();

	int result = AbstractBufferedPipeline::queueFrame(frame);
	chain_frame(frame);

	return result; // 	RETURN(result, int);
}

//********************************************************************************
//
//********************************************************************************
/**
 * build transfer header.
 * all multi bytes fields are little endian.
 */
static void build_header(publish_header_t &header, uvc_frame_t *frame) {
	// build header
	switch (frame->frame_format) {
	case UVC_FRAME_FORMAT_YUYV:
		header.format_le = htole32(VIDEO_FRAME_FORMAT_YUYV);
		break;
	case UVC_FRAME_FORMAT_MJPEG:
		header.format_le = htole32(VIDEO_FRAME_FORMAT_MJPEG);
		break;
//	case UVC_FRAME_FORMAT_H264:	// XXX this definition does not exist now
//		header.format_le = htole32(VIDEO_FRAME_FORMAT_H264);
//		break;
	default:
		header.format_le = htole32(VIDEO_FRAME_FORMAT_UNKNOWN);
	}
	header.width_le = htole32(frame->width);
	header.height_le = htole32(frame->height);
	header.sequence_le = htole32(frame->sequence);
//	header.presentation_time_us_le = htole64(systemTime() / 1000);
	header.presentation_time_us_le = htole64(nsecs_t(frame->capture_time.tv_sec)*1000000LL + nsecs_t(frame->capture_time.tv_usec));
	header.data_bytes_le = htole32(frame->actual_bytes);
}

/* override protected */
void PublisherPipeline::on_start() {
	ENTER();

	Mutex::Autolock lock(publisher_mutex);

	context = new zmq::context_t();
	publisher = new zmq::socket_t(*context, ZMQ_PAIR/*ZMQ_PUB*/);
	LOGV("set timeout value");
	publisher->setsockopt(ZMQ_SNDTIMEO, 1000);
	publisher->setsockopt(ZMQ_LINGER, 100);
	LOGV("bind");
	publisher->bind(host.c_str());

	EXIT();
}

/* override protected */
void PublisherPipeline::on_stop() {
	ENTER();

	Mutex::Autolock lock(publisher_mutex);
	LOGI("on_stop:finished");
	LOGI("stop publisher zmq::socket");
	if (publisher) {
		publisher->close();
		SAFE_DELETE(publisher);
	}
	LOGI("stop publisher zmq::context");
	if (context) {
		context->close();
		SAFE_DELETE(context);
	}
	LOGI("on_stop:finished");

	EXIT();
}

/* override protected */
int PublisherPipeline::handle_frame(uvc_frame_t *frame) {
//	ENTER();

	// local cache
	const int sub_sz = subscription_id.size();
	const char *sub_str = subscription_id.c_str();
//	LOGD("sub_str=%s,sub_sz=%d", sub_str, sub_sz);

	publish_header_t header;
	// create payload
	zmq::message_t payload(frame->actual_bytes + sizeof(publish_header_t));
	// copy header to payload
	build_header(header, frame);
	memcpy(payload.data(), &header, sizeof(publish_header_t));
	// copy frame data to payload
	memcpy(((uint8_t *)payload.data()) + sizeof(publish_header_t), frame->data, frame->actual_bytes);
/*
* zmq::socket_t#send(backed by zmq_msg_send of libzmq) just add message to the internal queue.
* and we can't know how many entries we can send without exceeding queue
* and we also can't know when the queue exceed/overflow and message disappeared.
* As my current test result, if the subscribing speed is slower than publisher
* or bandwidth of network is not enough, zmq drops messages periodically without any error.
* Even worse, queueing to zmq socket always success and no error.
* As I can see the API and source code of zmq, there are no way to handle this now.
* Only one way is receive number of subscribed frames from subscriber via other connection
* and wait for publishing if the difference between number of publishing frames
* and number of subscribed frames become bigger than the limit.
* Unfortunately this way only works well if only one subscriber exist.
* If we actually need to handle this issue, I assume it will be better to use other protocol
* or make own protocol that supports handshake instead of using zmq.
*
* 1280x720p:
* Nexus7(2013, Android5.1.1) => 11n(2.4GHz) => router => 11n(5Ghz) => Nexus9(Android5.1.1) : OK
* Nexus7(2013, Android5.1.1) => 11n(2.4GHz) => router => 11n(2.4GHz) => GALAXY S5(Android5.0) : NG, slow, periodically drops frames.
* Nexus9(Android5.1.1) => 11n(5GHz) => router => 11n(2.4GHz) => GALAXY S5(Android5.0) : OK
* Nexus9(Android5.1.1) => 11n(5GHz) => router => 11n(2.4GHz) => MemoPad7(Android4.4.2) : NG, sometimes become slow, periodically drops frames.
* GALAXY S5(Android5.0) => 11n(2.4GHz) => router => 11n(5Ghz) => Nexus9(Android5.1.1) : OK
* GALAXY S5(Android5.0) => 11n(2.4GHz) => router => 11n(2.4Ghz) => GALAXY note2(Android4.4.2) : NG, slow, periodically drops frames.
* saki
*/
	for ( ; LIKELY(isRunning()) ; ) {
		try {
			// set subscribe id
			if ((LIKELY(publisher->send(sub_str, sub_sz, ZMQ_SNDMORE)))) {
				// send payload
				if (LIKELY(publisher->send(payload))) {
					// success to send
					break;
				}
			}
			LOGD("failed to send");
			usleep(RETRY_INTERVALS_US);
		} catch (zmq::error_t e) {
			// when error occur other than EAGAIN
			LOGW("failed to send:%d", e.num());
			break;
		} catch (...) {
			LOGW("publishing error");
			break;
		}
	}

	return 1; // RETURN(1, int);
}

//********************************************************************************
//
//********************************************************************************
static ID_TYPE nativeCreate(JNIEnv *env, jobject thiz,
	jstring publisher_addr_str, jstring subscription_id_str) {

	ENTER();

	const char *c_addr = env->GetStringUTFChars(publisher_addr_str, JNI_FALSE);
	const char *c_sub_id = env->GetStringUTFChars(subscription_id_str, JNI_FALSE);
	PublisherPipeline *pipeline = new PublisherPipeline(DEFAULT_FRAME_SZ, c_addr, c_sub_id);
	env->ReleaseStringUTFChars(publisher_addr_str, c_addr);
	env->ReleaseStringUTFChars(subscription_id_str, c_sub_id);
	setField_long(env, thiz, "mNativePtr", reinterpret_cast<ID_TYPE>(pipeline));

	RETURN(reinterpret_cast<ID_TYPE>(pipeline), ID_TYPE);
}

static void nativeDestroy(JNIEnv *env, jobject thiz,
	ID_TYPE id_pipeline) {

	ENTER();
	setField_long(env, thiz, "mNativePtr", 0);
	PublisherPipeline *pipeline = reinterpret_cast<PublisherPipeline *>(id_pipeline);
	if (LIKELY(pipeline)) {
		pipeline->release();
		SAFE_DELETE(pipeline);
	}
	EXIT();
}

static jint nativeGetState(JNIEnv *env, jobject thiz,
	ID_TYPE id_pipeline) {

	ENTER();
	jint result = 0;
	PublisherPipeline *pipeline = reinterpret_cast<PublisherPipeline *>(id_pipeline);
	if (LIKELY(pipeline)) {
		result = pipeline->getState();
	}
	RETURN(result, jint);
}

static jint nativeSetPipeline(JNIEnv *env, jobject thiz,
	ID_TYPE id_pipeline, jobject pipeline_obj) {

	ENTER();
	jint result = JNI_ERR;
	PublisherPipeline *pipeline = reinterpret_cast<PublisherPipeline *>(id_pipeline);
	if (pipeline) {
		IPipeline *target_pipeline = getPipeline(env, pipeline_obj);
		result = pipeline->setPipeline(target_pipeline);
	}

	RETURN(result, jint);
}

static jint nativeStart(JNIEnv *env, jobject thiz,
	ID_TYPE id_pipeline) {

	ENTER();

	int result = JNI_ERR;
	PublisherPipeline *pipeline = reinterpret_cast<PublisherPipeline *>(id_pipeline);
	if (LIKELY(pipeline)) {
		result = pipeline->start();
	}

	RETURN(result, jint);
}

static jint nativeStop(JNIEnv *env, jobject thiz,
	ID_TYPE id_pipeline) {

	ENTER();

	jint result = JNI_ERR;
	PublisherPipeline *pipeline = reinterpret_cast<PublisherPipeline *>(id_pipeline);
	if (LIKELY(pipeline)) {
		result = pipeline->stop();
	}

	RETURN(result, jint);
}

//================================================================================
static JNINativeMethod methods_publisher_pipeline[] = {
	{ "nativeCreate", 		"(Ljava/lang/String;Ljava/lang/String;)J", (void *) nativeCreate},
	{ "nativeDestroy",		"(J)V", (void *) nativeDestroy},
	{ "nativeSetPipeline",	"(JLcom/serenegiant/usb/IPipeline;)I", (void *) nativeSetPipeline },

	{ "nativeGetState",		"(J)I", (void *) nativeGetState },
	{ "nativeStart",		"(J)I", (void *) nativeStart },
	{ "nativeStop",			"(J)I", (void *) nativeStop },
};

int register_publisher_pipeline(JNIEnv *env) {
	LOGV("register PublisherPipeline:");
	if (registerNativeMethods(env,
		"com/serenegiant/usb/PublisherPipeline",
		methods_publisher_pipeline, NUM_ARRAY_ELEMENTS(methods_publisher_pipeline)) < 0) {
		return -1;
	}
    return 0;
}
