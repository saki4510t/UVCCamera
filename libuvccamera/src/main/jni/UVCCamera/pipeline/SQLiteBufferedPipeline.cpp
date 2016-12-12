//
// Created by saki on 15/11/09.
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

#include <vector>

#include "utilbase.h"
#include "common_utils.h"

#include "libUVCCamera.h"
#include "pipeline_helper.h"
#include "IPipeline.h"
#include "Timers.h"
#include "SQLiteBufferedPipeline.h"

#define CHECK_INTERVAL_NSEC 5000000000LL	// every 5sec
#define TABLE_NAME "backend"
#define INSERT_FIELDS "dtime, format, width, height, sequence, data_bytes, data"
#define ALL_FIELDS "id, dtime, format, width, height, sequence, data_bytes, data"

/*public*/
SQLiteBufferedPipeline::SQLiteBufferedPipeline(const char *database_name, const bool &clear_table)
:	IPipeline(0),db(NULL),
	sql_insert_one(NULL),
	sql_query_oldest_10(NULL),
	sql_delete_one(NULL),
	sql_delete_older(NULL),
	sql_count(NULL)
{
	ENTER();

	db = new sqlite3pp::database(database_name);
	// 0:id, 1:dtime, 2:format, 3:width, 4:height, 5:sequence, 6:data_bytes, 7:data
	sqlite3pp::command cmd(*db,
		"CREATE TABLE IF NOT EXISTS " TABLE_NAME " ("
	    "id INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT, "
		"dtime INTEGER NOT NULL UNIQUE, "
		"format INTEGER NOT NULL, "
		"width INTEGER NOT NULL, "
		"height INTEGER NOT NULL, "
		"sequence INTEGER NOT NULL, "
		"data_bytes INTEGER NOT NULL, "
		"data BLOB NOT NULL"
		");"
	);
	cmd.execute();

	if (clear_table) {
		clear();
	}
	// sqlite3pp::statement and it's descendants that accessing table can only create when the specific table already exists.
	sql_insert_one = new sqlite3pp::command(*db, "INSERT INTO " TABLE_NAME
		" (" INSERT_FIELDS ") VALUES (?, ?, ?, ?, ?, ?, ?);");
	sql_query_oldest_10 = new sqlite3pp::query(*db, "SELECT " ALL_FIELDS " FROM " TABLE_NAME " ORDER BY dtime LIMIT 10;");
	sql_delete_one = new sqlite3pp::command(*db, "DELETE FROM " TABLE_NAME " WHERE (id=?);");
	sql_delete_older = new sqlite3pp::command(*db, "DELETE FROM " TABLE_NAME " WHERE (dtime<?);");
	sql_count = new sqlite3pp::query(*db, "SELECT count(id) FROM " TABLE_NAME ";");

	setState(PIPELINE_STATE_INITIALIZED);
	EXIT();
}

/*public*/
SQLiteBufferedPipeline::~SQLiteBufferedPipeline() {
	ENTER();

	LOGD("deleting sql_count");
	if (sql_count) {
		sql_count->finish();
		SAFE_DELETE(sql_count);
	}
	LOGD("deleting sql_delete_older");
	if (sql_delete_older) {
		sql_delete_older->finish();
		SAFE_DELETE(sql_delete_older);
	}
	LOGD("deleting sql_delete_one");
	if (sql_delete_one) {
		sql_delete_one->finish();
		SAFE_DELETE(sql_delete_one);
	}
	LOGD("deleting sql_query_oldest_10");
	if (sql_query_oldest_10) {
		sql_query_oldest_10->finish();
		SAFE_DELETE(sql_query_oldest_10);
	}
	LOGD("deleting sql_insert_one");
	if (sql_insert_one) {
		sql_insert_one->finish();
		SAFE_DELETE(sql_insert_one);
	}
	LOGD("deleting db");
	if (db) {
		db->disconnect();
		SAFE_DELETE(db);
	}

	EXIT();
};

/*public*/
int SQLiteBufferedPipeline::release() {
	ENTER();

	setState(PIPELINE_STATE_RELEASING);
	stop();

	RETURN(0, int);
}

/*public*/
int SQLiteBufferedPipeline::start() {
	ENTER();

	int result = EXIT_FAILURE;
	if (!isRunning()) {
		LOGD("start handler thread");
		setState(PIPELINE_STATE_STARTING);
		mIsRunning = true;
		result = pthread_create(&handler_thread, NULL, handler_thread_func, (void *) this);
		if (UNLIKELY(result != EXIT_SUCCESS)) {
			LOGW("PublisherPipeline::already running/could not create thread etc.");
			setState(PIPELINE_STATE_INITIALIZED);
			mIsRunning = false;
			handler_sync.signal();
		}
	}
	RETURN(result, int);
}

/*public*/
int SQLiteBufferedPipeline::stop() {
	ENTER();
	bool b = isRunning();
	if (LIKELY(b)) {
		LOGD("waiting SQLiteBufferedPipeline thread");
		setState(PIPELINE_STATE_STOPPING);
		mIsRunning = false;
		handler_sync.broadcast();
		if (pthread_join(handler_thread, NULL) != EXIT_SUCCESS) {
			LOGW("SQLiteBufferedPipeline::terminate SQLiteBufferedPipeline thread: pthread_join failed");
		}
		LOGD("SQLiteBufferedPipeline thread finished");
	}
	RETURN(0, int);
}

/*public*/
int SQLiteBufferedPipeline::queueFrame(uvc_frame_t *frame) {
	ENTER();

	uvc_error_t ret = UVC_ERROR_OTHER;
	Mutex::Autolock lock(handler_mutex);

	if (LIKELY(frame && isRunning())) {
		// FIXME if the number of record exceeds specific limit, need to delete old records or drop newer frames
		// FIXME otherwise device storage will become empty at some point.
		try {
			sql_insert_one->reset();
			sql_insert_one->bind(1, nsecs_t(frame->capture_time.tv_sec) * 1000000LL +
								nsecs_t(frame->capture_time.tv_usec));
			sql_insert_one->bind(2, (int) frame->frame_format);
			sql_insert_one->bind(3, (int) frame->width);
			sql_insert_one->bind(4, (int) frame->height);
			sql_insert_one->bind(5, (int) frame->sequence);
			sql_insert_one->bind(6, (int) frame->actual_bytes);
			sql_insert_one->bind(7, (void *) frame->data, frame->actual_bytes, false);
			sql_insert_one->execute();
			ret = UVC_SUCCESS;
		} catch (...) {
			LOGW("failed insert frame");
		}
		handler_sync.broadcast();
	}

	RETURN(ret, int);
}

/*public*/
void SQLiteBufferedPipeline::clear() {
	ENTER();

	sqlite3pp::command sql_truncate(*db, "TRUNCATE TABLE " TABLE_NAME ";");
	sql_truncate.execute();

	EXIT();
}

/**
 * delete record(s) older than specific dtime.
 * if you want to delete all record(s), use clear instead
 */
/*protected*/
int SQLiteBufferedPipeline::delete_older(const nsecs_t &dtime) {
	ENTER();

#ifndef NDEBUG
	LOGI("before=%d", getCount());
#endif
	int result = -1;
	try {
		sql_delete_older->reset();
		sql_delete_older->bind(1, dtime);
		sql_delete_older->execute();
		result = 0;
	} catch (...) {
		LOGW("failed to delete older:%lld", dtime);
	}

#ifndef NDEBUG
	LOGI("after=%d", getCount());
#endif

	RETURN(result, int);
}

/*protected*/
int SQLiteBufferedPipeline::purge_older(const nsecs_t &limit_rel_nsec) {
	ENTER();

	int result = -1;

	if (LIKELY(limit_rel_nsec)) {
		result = delete_older(systemTime() - limit_rel_nsec);
	}

	RETURN(result, int);
}

int SQLiteBufferedPipeline::getCount() {
	ENTER();

	int result = 0;
	if (LIKELY(sql_count)) {
		sql_count->reset();
		for (auto iter = sql_count->begin(); iter != sql_count->end(); ++iter) {
			result = (*iter).get<int>(0);
			break;	// sql_count statement always return only one record and this will s redundant.
		}
	}

	RETURN(result, int);
}

/*private*/
void *SQLiteBufferedPipeline::handler_thread_func(void *vptr_args) {

	ENTER();
	SQLiteBufferedPipeline *pipeline = reinterpret_cast<SQLiteBufferedPipeline *>(vptr_args);
	if (LIKELY(pipeline)) {
		pipeline->do_loop();
	}
	PRE_EXIT();
	pthread_exit(NULL);
}

/*private*/
void SQLiteBufferedPipeline::do_loop() {
	ENTER();

	std::vector<int> queued_ids;
	struct timespec ts, ws;
	ts.tv_sec = 5; ts.tv_nsec = 0;	// wait maximum 5 sec
//	ws.tv_sec = 0; ws.tv_nsec = 3000000;	// 3msec
	uvc_frame_t *frame = uvc_allocate_frame(DEFAULT_FRAME_SZ);
	if (LIKELY(frame)) {
		setState(PIPELINE_STATE_RUNNING);
		nsecs_t prev_time = systemTime();
		for (; LIKELY(isRunning());) {
			handler_mutex.lock();
			{
				// wait for new arriving frame data
				handler_sync.waitRelative(handler_mutex, 3000000);
			}
			handler_mutex.unlock();

			if (LIKELY(isRunning())) {

				if (next_pipeline) {
					queued_ids.clear();
					for (auto iter = sql_query_oldest_10->begin(); iter != sql_query_oldest_10->end(); ++iter) {
						if (UNLIKELY(!isRunning())) {
							break;
						}
						// 0:id, 1:dtime, 2:format, 3:width, 4:height, 5:sequence, 6:data_bytes, 7:data
						const int64_t id = (*iter).get<int64_t>(0);	// XXX int64_t(long long int) may be better
						const nsecs_t dtime = (*iter).get <nsecs_t> (1);
						const uvc_frame_format format = (uvc_frame_format)(*iter).get<int>(2);
						const uint32_t width = (uint32_t)(*iter).get<int>(3);
						const uint32_t height = (uint32_t)(*iter).get<int>(4);
						const uint32_t sequence = (uint32_t)((*iter).get <int> (5));
						const size_t actual_bytes = (size_t)(*iter).get<int>(6);
						const void *data = (*iter).get<const void *>(7);
						if (LIKELY(!uvc_ensure_frame_size(frame, actual_bytes))) {
							frame->capture_time.tv_sec = dtime / 1000000LL;
							frame->capture_time.tv_usec = dtime % 1000000LL;
							frame->frame_format = format;
							frame->width = width;
							frame->height = height;
							frame->sequence = sequence;
							frame->actual_bytes = actual_bytes;
							memcpy(frame->data, data, actual_bytes);
							if (!chain_frame(frame)) {
								// if queueing success, delete the record
								queued_ids.push_back(id);
//								LOGI("chain_frame:%lld,%lld,(%d,%d),actual_bytes=%d", id, dtime, width, height, actual_bytes);
								if (iter != sql_query_oldest_10->end()) {
									// wait several msecs here if the record is not a last one, otherwise after pipeline will exceed buffer and drop frame(s)
									handler_mutex.lock();
									{
										handler_sync.waitRelative(handler_mutex, 5000000000L);
									}
									handler_mutex.unlock();
								}
							}
						} else {
							LOGW("uvc_ensure_frame_size failed:%lld,%lld,(%d,%d),actual_bytes=%d", id, dtime, width, height, actual_bytes);
						}
					} // end of for
					// delete chained record(s) if exist
					sqlite3pp::transaction xct(*db);
					try {
						for (auto iter = queued_ids.begin(); iter != queued_ids.end(); iter++) {
							sql_delete_one->reset();
							sql_delete_one->bind(1, *iter);
							sql_delete_one->execute();
						}
						xct.commit();
					} catch (std::exception &e) {
						LOGI("exception: failed to delate");
						xct.rollback();    // this may redundant
					}
				} // end of if (next_pipeline)
				if (UNLIKELY(systemTime() > prev_time + CHECK_INTERVAL_NSEC)) {
					prev_time = systemTime();
					purge_older();
				}
			}
		}
		setState(PIPELINE_STATE_STOPPING);
		uvc_free_frame(frame);
	} else {
		LOGW("uvc_allocate_frame failed");
	}
	setState(PIPELINE_STATE_INITIALIZED);
	mIsRunning = false;

	EXIT();
}

//********************************************************************************
//
//********************************************************************************
static ID_TYPE nativeCreate(JNIEnv *env, jobject thiz,
	jstring database_path_str, jboolean clear_table) {

	ENTER();

	const char *c_database_path = env->GetStringUTFChars(database_path_str, JNI_FALSE);
	SQLiteBufferedPipeline *pipeline = NULL;
	try {
		pipeline = new SQLiteBufferedPipeline(c_database_path, clear_table);
	} catch (...) {
		LOGW("failed to create SQLiteBufferedPipeline");
	}
	env->ReleaseStringUTFChars(database_path_str, c_database_path);

	setField_long(env, thiz, "mNativePtr", reinterpret_cast<ID_TYPE>(pipeline));
	RETURN(reinterpret_cast<ID_TYPE>(pipeline), ID_TYPE);
}

static void nativeDestroy(JNIEnv *env, jobject thiz,
	ID_TYPE id_pipeline) {

	ENTER();
	setField_long(env, thiz, "mNativePtr", 0);
	SQLiteBufferedPipeline *pipeline = reinterpret_cast<SQLiteBufferedPipeline *>(id_pipeline);
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
	SQLiteBufferedPipeline *pipeline = reinterpret_cast<SQLiteBufferedPipeline *>(id_pipeline);
	if (pipeline) {
		result = pipeline->getState();
	}
	RETURN(result, jint);
}

static jint nativeSetPipeline(JNIEnv *env, jobject thiz,
	ID_TYPE id_pipeline, jobject pipeline_obj) {

	ENTER();
	jint result = JNI_ERR;
	SQLiteBufferedPipeline *pipeline = reinterpret_cast<SQLiteBufferedPipeline *>(id_pipeline);
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
	SQLiteBufferedPipeline *pipeline = reinterpret_cast<SQLiteBufferedPipeline *>(id_pipeline);
	if (LIKELY(pipeline)) {
		result = pipeline->start();
	}
	RETURN(result, jint);
}

static jint nativeStop(JNIEnv *env, jobject thiz,
	ID_TYPE id_pipeline) {

	jint result = JNI_ERR;
	ENTER();
	SQLiteBufferedPipeline *pipeline = reinterpret_cast<SQLiteBufferedPipeline *>(id_pipeline);
	if (LIKELY(pipeline)) {
		result = pipeline->stop();
	}
	RETURN(result, jint);
}

//**********************************************************************
//
//**********************************************************************
static JNINativeMethod methods[] = {
	{"nativeCreate", "(Ljava/lang/String;Z)J", (void *) nativeCreate},
	{ "nativeDestroy",					"(J)V", (void *) nativeDestroy },

	{ "nativeGetState",					"(J)I", (void *) nativeGetState },
	{ "nativeSetPipeline",				"(JLcom/serenegiant/usb/IPipeline;)I", (void *) nativeSetPipeline },

	{ "nativeStart",					"(J)I", (void *) nativeStart },
	{ "nativeStop",						"(J)I", (void *) nativeStop },

};

int register_sqlite_buffered_pipeline(JNIEnv *env) {
	LOGV("register PublisherPipeline:");
	if (registerNativeMethods(env,
		"com/serenegiant/usb/SQLiteBufferedPipeline",
		methods, NUM_ARRAY_ELEMENTS(methods)) < 0) {
		return -1;
	}
    return 0;
}
