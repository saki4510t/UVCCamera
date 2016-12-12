//
// Created by saki on 15/11/09.
//

#ifndef PUPILMOBILE_SQLITEBUFFEREDPIPELINE_H
#define PUPILMOBILE_SQLITEBUFFEREDPIPELINE_H

#include <stdlib.h>
#include <pthread.h>
#include <list>
#include <string>
#include "Mutex.h"
#include "Condition.h"

#include "libUVCCamera.h"
#include "IPipeline.h"
#include "sqlite3pp.h"

#pragma interface

using namespace android;

#define DTIME_LIMIT_NSEC 30000000000LL		// 30sec

class SQLiteBufferedPipeline : virtual public IPipeline {
private:
	sqlite3pp::database *db;
	// precompile statements
	sqlite3pp::command *sql_insert_one;
	sqlite3pp::query *sql_query_oldest_10;
	sqlite3pp::command *sql_delete_one;
	sqlite3pp::command *sql_delete_older;
	sqlite3pp::query *sql_count;

	pthread_t handler_thread;
	mutable Mutex handler_mutex;
	Condition handler_sync;
	static void *handler_thread_func(void *vptr_args);
	void do_loop();
protected:
	int getCount();
	/**
	 * delete record(s) older than specific dtime.
	 * if you want to delete all record(s), use clear instead
	 */
	int delete_older(const nsecs_t &dtime);
	/** helper of delete_older */
	int purge_older(const nsecs_t &limit_rel_nsec = DTIME_LIMIT_NSEC);
public:
	SQLiteBufferedPipeline(const char *database_name, const bool &clear = false);
	virtual ~SQLiteBufferedPipeline();
	virtual int release();
	virtual int start();
	virtual int stop();
	virtual int queueFrame(uvc_frame_t *frame);
	virtual void clear();
};


#endif //PUPILMOBILE_SQLITEBUFFEREDPIPELINE_H
