/*
******************* Copyright (c) ***********************\
**
**         (c) Copyright 2017, 蒋朋, china, sxkj. sd
**                  All Rights Reserved
**
**                 By(青岛世新科技有限公司)
**                    www.qdsxkj.com
**
**                       _oo0oo_
**                      o8888888o
**                      88" . "88
**                      (| -_- |)
**                      0\  =  /0
**                    ___/`---'\___
**                  .' \\|     |// '.
**                 / \\|||  :  |||// \
**                / _||||| -:- |||||- \
**               |   | \\\  -  /// |   |
**               | \_|  ''\---/''  |_/ |
**               \  .-\__  '-'  ___/-. /
**             ___'. .'  /--.--\  `. .'___
**          ."" '<  `.___\_<|>_/___.' >' "".
**         | | :  `- \`.;`\ _ /`;.`/ - ` : | |
**         \  \ `_.   \_ __\ /__ _/   .-` /  /
**     =====`-.____`.___ \_____/___.-`___.-'=====
**                       `=---='
**
**
**     ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
**
**               佛祖保佑         永无BUG
**
**
**                   南无本师释迦牟尼佛
**

**----------------------版本信息------------------------
** 版    本: V0.1
**
******************* End of Head **********************\
*/

package com.serenegiant.service;

import android.app.AlarmManager;
import android.app.PendingIntent;
import android.app.Service;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.os.Environment;
import android.os.IBinder;
import android.os.StatFs;
import android.os.StrictMode;
import android.os.SystemClock;
import android.text.TextUtils;
import android.util.Log;

import com.serenegiant.usbcameracommon.BuildConfig;
import com.serenegiant.utils.Constants;
import com.serenegiant.utils.FileUtil;

import java.io.BufferedReader;
import java.io.File;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.text.ParseException;
import java.text.SimpleDateFormat;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Calendar;
import java.util.Comparator;
import java.util.Date;
import java.util.List;
import java.util.Locale;


/**
 * @class LogService
 * @description
 * 日志服务，日志默认会存储在内存中的安装目录下面并且系统每隔一段时间就会检测一下log文件的大小，
 * 如果log文件超过指定的大小则会执行如下动作：
 * 1.SD卡不可用，所有log文件都保存在内存中，当log文件数超出指定的数量就会删除较早的log
 * 2.SD卡可用而且存储空间足够则将文件移动SD卡中，并检测SD中保存的文件是否都在指定时间日期内，如果不是则删除
 * 3.SD卡可用但存储空间小于指定的大小，在操作log前会先删除SD卡中的所有log并重新检测空间，如果空间仍不足则按1操作否则按2进行
 */

public class LogService extends Service {

	private static final String TAG = "LOGSERVICELOG";

	private static final int MEMORY_LOG_FILE_MAX_SIZE = 10 * 1024; // 内存中日志文件最大值, 10K
	private static final int MEMORY_LOG_FILE_MONITOR_INTERVAL = 60 * 1000; // 内存中的日志文件大小监控时间间隔，1分钟
	private static final int MEMORY_LOG_FILE_MAX_NUMBER = 2; // 内存中允许保存的最大文件个数

	//一天删除一次
	private static final int INTERVAL_DELETE_LOG_SD = 24 * 3600 * 1000 / MEMORY_LOG_FILE_MONITOR_INTERVAL;
	private int time2DeleteLogSd;

	// sd卡中日志文件的最多保存天数, 3天
	private static final int SDCARD_LOG_FILE_SAVE_DAYS = 3;
	private static final String LOG_FILE_NAME = ".txt";	//日志文件名称后缀
	private static final String MONITOR_LOG_PRIORITY = "V"; // 监听日志的最低优先级
	private static final int SLEEPTIME = 600; 		// LogService 文件夹创建后等待时间

	private static final String MONITOR_LOG_SIZE_ACTION = "MONITOR_LOG_SIZE"; // 日志文件大小监测action

	/**
	 * logcat -v time *:D | grep -n '^.*\/.*(  828):' >> /sdcard/download/Log.log
	 */
	private static final String LOG_FILTER_COMMAND_FORMAT =
			"logcat -v time *:%s | grep '^.*\\/.*(%s):' >> %s";
			//过滤日志, 不包含dequeue, 由摄像头造成的有两处, egl_platform_dequeue_buffer和dequeueBuffer
//			"logcat -v time *:%s | grep '^.*[^(.*dequeue.*)+]\\/.*(%s):' >> %s";
//	"logcat -v time *:%s | grep '^.*\\/.*(%s):.*(?!(.*))' >> %s";

	// 日志名称格式, 注意要用年月日, 否则删除七天之前的日志导致立即删除
	private static final SimpleDateFormat DATE_FORMAT = new SimpleDateFormat("yyyy-MM-dd HH-mm-ss", Locale.getDefault());

	private String mLogFileDirMemory; // 日志文件在内存中的路径(日志文件在安装目录中的路径)

	private String mCurrLogFileName; // 如果当前的日志写在内存中，记录当前的日志文件名称

	private Process mProcess;

	private PendingIntent mMemoryLogFileSizeMonitor;
	private AlarmManager mAlarmManager;

	@Override
	public IBinder onBind(Intent intent) {
		return null;
	}

	@Override
	public void onCreate() {
		super.onCreate();
		Log.w(TAG, "-- onCreate() --");
		if (BuildConfig.DEBUG) {
			StrictMode.ThreadPolicy policy = new StrictMode.ThreadPolicy.Builder().permitAll().build();
			StrictMode.setThreadPolicy(policy);
		}

		final IntentFilter logTaskFilter = new IntentFilter();
		logTaskFilter.addAction(MONITOR_LOG_SIZE_ACTION); //注册日志大小监听广播
		//如果时间被重新设置，要判断当前时间是否在有效时间内，
		//如果超出了范围需要将日志保存到新文件中，否则会被清除
		logTaskFilter.addAction(Intent.ACTION_TIME_CHANGED);
		registerReceiver(mLogTaskReceiver, logTaskFilter);
		
//		部署日志大小监控任务
		final Intent intent = new Intent(MONITOR_LOG_SIZE_ACTION);
		mMemoryLogFileSizeMonitor = PendingIntent.getBroadcast(this, 0, intent, 0);
		mAlarmManager = (AlarmManager) getSystemService(ALARM_SERVICE);
		mAlarmManager.setRepeating(AlarmManager.RTC_WAKEUP, System.currentTimeMillis(),
				MEMORY_LOG_FILE_MONITOR_INTERVAL, mMemoryLogFileSizeMonitor);

		new LogCollectorThread().start();
	}

	@Override
	public void onDestroy() {
		super.onDestroy();
		Log.w(TAG, "-- onDestroy() --");
		if (mProcess != null) {
			mProcess.destroy();
			mProcess = null;
		}
		
//		取消日志大小监控任务
		mAlarmManager.cancel(mMemoryLogFileSizeMonitor);
		
		unregisterReceiver(mLogTaskReceiver);
	}

	/**
	 * 日志收集 
	 * 1.清除日志缓存 
	 * 2.杀死应用程序已开启的Logcat进程防止多个进程写入一个日志文件 
	 * 3.开启日志收集进程 
	 * 4.处理日志文件 移动 OR 删除
	 */
	private final class LogCollectorThread extends Thread {

		private LogCollectorThread() {
			super("LogCollectorThread");
			Log.w(TAG, "LogCollectorThread is create");
		}

		@Override
		public void run() {
			try {
				clearLogCache();
				final List<String> orgProcessList = getAllProcess();
				final List<ProcessInfo> processInfoList = getProcessInfoList(orgProcessList);
				killLogcatProc(processInfoList);
				mCurrLogFileName = dateToFileName(new Date());//获取新的日志文件名称
				createLogCollector(getMemoryFilePath(mCurrLogFileName));

				// 延时一会再处理，创建文件，然后处理文件，不然该文件还没创建，会影响文件删除
				SystemClock.sleep(SLEEPTIME);
				handleLog(mCurrLogFileName);


			} catch (Exception e) {
				e.printStackTrace();
			}
		}
	}

	/**
	 * 每次记录日志之前先清除日志的缓存, 不然会在两个日志文件中记录重复的日志
	 */
	private void clearLogCache() {
		Log.w(TAG, "-- clearLogCache() --");
		Process process = null;
		final List<String> commandList = new ArrayList<>();
		commandList.add("logcat");
		commandList.add("-c");
		try {
			process = Runtime.getRuntime().exec(commandList.toArray(new String[commandList.size()]));
			final StreamConsumer errorGobbler = new StreamConsumer(process.getErrorStream());
			final StreamConsumer outputGobbler = new StreamConsumer(process.getInputStream());
			errorGobbler.start();
			outputGobbler.start();
			if (process.waitFor() != 0) {
				Log.e(TAG, " clearLogCache mProcess.waitFor() != 0");
			}
		} catch (Exception e) {
			Log.e(TAG, "clearLogCache failed", e);
		} finally {
			try {
				if (process != null) {
					process.destroy();
				}
			} catch (Exception e) {
				Log.e(TAG, "clearLogCache failed", e);
			}
		}
	}

	/**
	 * 关闭由本程序开启的logcat进程： 根据用户名称杀死进程(如果是本程序进程开启的Logcat收集进程那么两者的USER一致)
	 * 如果不关闭会有多个进程读取logcat日志缓存信息写入日志文件
	 * @param allProcList
	 * @return
	 */
	private void killLogcatProc(List<ProcessInfo> allProcList) {
		Log.w(TAG, "-- killLogcatProc() --");

		if (mProcess != null) {
			Log.w(TAG, "-- killLogcatProc() -- mProcess.destroy();");

			mProcess.destroy();
			mProcess = null;
		}

		final String packName = this.getPackageName();
		final String myUser = getAppUser(packName, allProcList);
		for (ProcessInfo processInfo : allProcList) {
			if (processInfo.name.equalsIgnoreCase("logcat")
					&& processInfo.user.equalsIgnoreCase(myUser)
					//20170628添加不要杀死系统进程
					&& !processInfo.user.equalsIgnoreCase("system")) {
				Log.w(TAG, "-- killLogcatProc() -- myUser: " + myUser);

				android.os.Process.killProcess(Integer.parseInt(processInfo.pid));
			}
		}
	}

	/**
	 * 获取本程序的用户名称
	 * @param packName
	 * @param allProcList
	 * @return
	 */
	private String getAppUser(String packName, List<ProcessInfo> allProcList) {
		for (ProcessInfo processInfo : allProcList) {
			if (processInfo.name.equals(packName)) {
				return processInfo.user;
			}
		}
		return null;
	}

	/**
	 * 根据ps命令得到的内容获取PID，User，name等信息
	 * @param orgProcessList
	 * @return
	 */
	private List<ProcessInfo> getProcessInfoList(List<String> orgProcessList) {
		final List<ProcessInfo> procInfoList = new ArrayList<>();
		for (int i = 1; i < orgProcessList.size(); i++) {
			final String processInfo = orgProcessList.get(i);
			final String[] proStr = processInfo.split(" ");
			// USER PID PPID VSIZE RSS WCHAN PC NAME
			// root 1 0 416 300 c00d4b28 0000cd5c S /init
			final List<String> orgInfo = new ArrayList<>();
			for (String str : proStr) {
				if (!TextUtils.isEmpty(str)) {
					orgInfo.add(str);
				}
			}
			if (orgInfo.size() == 9) {
				final ProcessInfo pInfo = new ProcessInfo();
				pInfo.user = orgInfo.get(0);
				pInfo.pid = orgInfo.get(1);
				pInfo.ppid = orgInfo.get(2);
				pInfo.name = orgInfo.get(8);
				procInfoList.add(pInfo);
			}
		}
		return procInfoList;
	}

	/**
	 * 运行PS命令得到进程信息
	 * @return USER PID PPID VSIZE RSS WCHAN    PC         NAME 
	 * 		   root 1   0    416   300 c00d4b28 0000cd5c S /init
	 */
	private List<String> getAllProcess() {
		List<String> orgProcList = new ArrayList<>();
		Process proc = null;
		try {
			proc = Runtime.getRuntime().exec("ps");
			final StreamConsumer errorConsumer = new StreamConsumer(proc.getErrorStream());
			final StreamConsumer outputConsumer = new StreamConsumer(proc.getInputStream(), orgProcList);
			errorConsumer.start();
			outputConsumer.start();
			if (proc.waitFor() != 0) {
				Log.e(TAG, "getAllProcess proc.waitFor() != 0");
			}
		} catch (Exception e) {
			Log.e(TAG, "getAllProcess failed", e);
		} finally {
			try {
                if (proc != null) {
                    proc.destroy();
                }
			} catch (Exception e) {
				Log.e(TAG, "getAllProcess failed", e);
			}
		}
		return orgProcList;
	}
	
	/**
     * 把命令结果写入文件
	 * eg: logcat -v time *:D | grep -n '^.*\/.*(  828):' >> /sdcard/download/Log.log
	 * @param priority
	 * @param path
	 * @return
	 */
	private String getLogFilterCommand(String priority, String path) {
		Log.w(TAG, "-- getLogFilterCommand() --");
		String sid = "" + android.os.Process.myPid();
		for (int i = sid.length(); i < 5; i++) {
			sid = " " + sid;
		}
		return String.format(LOG_FILTER_COMMAND_FORMAT, priority, sid, path);
	}

	/**
	 * 开始收集日志信息
	 */
	public void createLogCollector(final String path) {
		Log.w(TAG, "-- createLogCollector() --: path: " + path);
		//日志保存到内存命令
		String command = getLogFilterCommand(MONITOR_LOG_PRIORITY, path.replace(" ", "\\ "));
		Log.w(TAG, "日志保存命令: " + command);
		List<String> commandList = new ArrayList<>();
		commandList.add("sh");
		commandList.add("-c");
		commandList.add(command);
		try {
			mProcess = Runtime.getRuntime().exec(commandList.toArray(new String[commandList.size()]));
			// mProcess.waitFor();
		} catch (Exception e) {
			Log.e(TAG, "CollectorThread == >" + e.getMessage(), e);
		}
	}

	/**
	 * 处理日志文件 
	 */
	public void handleLog(String currFileName) {
		Log.w(TAG, "-- handleLog() --");
		moveLogfile(currFileName);//将内存中日志文件转移到SD中，当前正在记录的日志文件除外
		deleteMemoryExpiredLog(MEMORY_LOG_FILE_MAX_NUMBER, currFileName);//删除内存中过期的日志文件

		//一天执行一次
		if (time2DeleteLogSd <= 0) {
			time2DeleteLogSd = INTERVAL_DELETE_LOG_SD;
			deleteSdcardExpiredLog();//删除SD中过期的日志文件
		}
	}

	/**
	 * 检查日志文件大小是否超过了规定大小 如果超过了重新开启一个日志收集进程
	 */
	private void checLogSize() {
		Log.w(TAG, "-- checLogSize() --");
		if (!TextUtils.isEmpty(mCurrLogFileName)) {
			String path = getMemoryFilePath(mCurrLogFileName);
			File file = new File(path);
			if (!file.exists()) {
				return;
			}
			if (file.length() >= MEMORY_LOG_FILE_MAX_SIZE) {
				Log.w(TAG, "The log's size is too big, 开始收集日志...");
				new LogCollectorThread().start();
			}
		}
	}
	
	/**
	 * 将日志文件转移到SD卡下面
	 */
	private void moveLogfile(String currFileName) {
		Log.w(TAG, "move file start, currFileName: " + currFileName);

		if (!isSdcardAvailable()) {
			return;
		}
		File file = new File(getMemoryDirPath());
		if (!file.isDirectory()) {
			return;
		}
		File[] allFiles = file.listFiles();
		for (File logFile : allFiles) {
			String fileName = logFile.getName();
			if (currFileName.equals(fileName)) {
				continue;
			}
			if (logFile.length() >= getSdcardAvailableSize()) {
//				deleteSdcardAllLog();
			}
			if (logFile.length() >= getSdcardAvailableSize()) {
				return;
			}
			if (FileUtil.moveFile(logFile, new File(getSdcardFilePath(fileName)))) {
				Log.w(TAG, "move file success, srcFile: " + logFile + ", destFile:" + getSdcardFilePath(fileName));
			}
		}
	}

	/**
	 * 删除SD内过期的日志
	 */
	private void deleteSdcardExpiredLog() {
		Log.w(TAG, "-- deleteSdcardExpiredLog() --");
		if (isSdcardAvailable()) {
			File timeDir = new File(getSdcardDirPath());

			File[] allFiles = timeDir.listFiles();
			for (File logFile : allFiles) {
				if (logFile.isDirectory()) {
					Log.w(TAG, "-- deleteSdcardExpiredLog() -- logFileDir: " + logFile);

					final String[] tmps = logFile.toString().split(Constants.LOG_FOLDER + File.separator);
					if (tmps.length > 1) {
						final String tmpDir = tmps[1].replace("-", "");
						Log.w(TAG, "deleteSdcardExpiredLog: tmpDir: " + tmpDir);

						final String deadTime = "20" + getMediaDateBeforeDay(SDCARD_LOG_FILE_SAVE_DAYS)
								.substring(0, 6);
						Log.w(TAG, "deleteSdcardExpiredLog: deadLine: " + deadTime);

						//删除早期日志
						if (TextUtils.isDigitsOnly(tmpDir)) {
							if (Integer.valueOf(tmpDir) < Integer.valueOf(deadTime)) {
								FileUtil.deleteDir(logFile);
							}
						} else {
							FileUtil.deleteDir(logFile);
						}

					}

				}

			}
		}
	}

	/**
	 * 获取index天之前的多媒体文件日期
	 * @param index
	 * @return
	 */
	public static String getMediaDateBeforeDay(int index) {
		return getDateBeforeDay(index, "yyMMddHHmmss");
	}


	/**
	 * 获取index天之前的日期
	 * @param index 提前天数
	 * @param pattern　日期格式
	 * @return
	 */
	public static String getDateBeforeDay(int index, String pattern) {
		Calendar cal = Calendar.getInstance();
		cal.add(Calendar.DAY_OF_YEAR, -index);
		SimpleDateFormat sdf = new SimpleDateFormat(pattern, Locale.getDefault());
		return sdf.format(cal.getTime());
	}

	/**
	 * 判断sdcard上的日志文件是否可以删除，规则：文件创建时间是否在days天数内
	 * @param fileName 文件名
	 * @param days 天数
	 * @return
	 */
	public boolean canDeleteSDLog(String fileName, int days) {
		Calendar calendar = Calendar.getInstance();
		calendar.add(Calendar.DAY_OF_MONTH, -1 * days);// 删除n天之前日志
		Date expiredDate = calendar.getTime();
		try {
			Date createDate = fileNameToDate(fileName);
			Log.w(TAG, "createDate: " + createDate.toString() + ", expireTime: " + expiredDate.toString()
					+ ", isBefore: " + createDate.before(expiredDate));
			return createDate.before(expiredDate);
		} catch (ParseException e) {
			Log.e(TAG, e.getMessage());
		}
		return false;
	}

	/**
	 * 删除内存中的过期日志，删除规则： 保存最近的number个日志文件
	 * @param number 保留的文件个数
	 */
	private void deleteMemoryExpiredLog(int number, String currFileName) {
		Log.w(TAG, "-- deleteMemoryExpiredLog() --");
		File file = new File(getMemoryDirPath());
		if (file.isDirectory()) {
			File[] allFiles = file.listFiles();
			Arrays.sort(allFiles, new FileComparator());
			for (int i = 0; i < allFiles.length - number; i++) {
				File f = allFiles[i];
				if (f.getName().equals(currFileName)) {
					continue;
				}
				f.delete();
				Log.w(TAG, "delete file success, file name is:" + f.getName());
			}
		}
	}

	private class ProcessInfo {
		public String user;
		public String pid;
		public String ppid;
		public String name;

		@Override
		public String toString() {
			return "user=" + user + " pid=" + pid + " ppid=" + ppid + " name=" + name;
		}
	}

	private final class StreamConsumer extends Thread {
		InputStream is;
		List<String> list;

		StreamConsumer(InputStream is) {
			super("StreamConsumerThread");
			this.is = is;
		}

		StreamConsumer(InputStream is, List<String> list) {
			super("StreamConsumerThread");
			this.is = is;
			this.list = list;
		}

		public void run() {
			InputStreamReader isr = null;
			BufferedReader br = null;
			try {
				isr = new InputStreamReader(is);
				br = new BufferedReader(isr);
				String line;
				while ((line = br.readLine()) != null) {
					if (list != null) {
                        //此处打印top指令的输出
						list.add(line);
					}
				}
			} catch (IOException ioe) {
				ioe.printStackTrace();
			} finally {
				FileUtil.closeIO(br, isr);
			}
		}
	}

	/**
	 * 监控日志大小
	 */
	private BroadcastReceiver mLogTaskReceiver = new BroadcastReceiver() {
		
		public void onReceive(Context context, Intent intent) {
			String action = intent.getAction();
			Log.w(TAG, "mLogTaskReceiver： " + action);
			if (MONITOR_LOG_SIZE_ACTION.equals(action)) {
				time2DeleteLogSd--;
				checLogSize();
			} else {
				if (Intent.ACTION_TIME_CHANGED.equals(action)) {
					if (canDeleteSDLog(mCurrLogFileName, SDCARD_LOG_FILE_SAVE_DAYS)) {
						Log.w(TAG, "The log is out of date !");
						new LogCollectorThread().start();
					}
				}
			}
		}
		
	};

	private class FileComparator implements Comparator<File> {
		
		public int compare(File file1, File file2) {
			try {
				Date create1 = fileNameToDate(file1.getName());
				Date create2 = fileNameToDate(file2.getName());
				if (create1.before(create2)) {
					return -1;
				} else {
					return 1;
				}
			} catch (ParseException e) {
				return 0;
			}
		}
		
	}
	
	/**
	 * SD卡是否可用
	 * @return
	 */
	private boolean isSdcardAvailable() {
		return Environment.getExternalStorageState().equals(Environment.MEDIA_MOUNTED);
	}
	
	/**
	 * SD卡剩余空间
	 * @return
	 */
	private long getSdcardAvailableSize() {
		if (isSdcardAvailable()) {
			String path = Environment.getExternalStorageDirectory().getAbsolutePath();
			StatFs fileStats = new StatFs(path);
			fileStats.restat(path);
			return fileStats.getAvailableBlocksLong() * fileStats.getBlockSizeLong();
		}
		return 0;
	}
	
	public boolean creatDir(String path) {
		File f = new File(path);
		f.mkdir();
		return f.exists() && f.isDirectory() || f.mkdirs();
	}
	
	public boolean creatFile(String path) {
		if (path != null && path.contains(File.separator) 
				&& creatDir(path.substring(0, path.lastIndexOf(File.separator)))) {
			File f = new File(path);
			try {
				return f.exists() && f.isFile() || f.createNewFile();
			} catch (IOException e) {
				Log.w(TAG, e);
			}
		}
		return false;
	}

	/**
	 * 日志文件名转日期
	 *
	 * @param fileName
	 * @return
	 * @throws ParseException
	 */
	private Date fileNameToDate(String fileName) throws ParseException {
		fileName = fileName.substring(0, fileName.indexOf("."));//去除文件的扩展类型（.log）
		Log.w(TAG, "fileName: " + fileName);
		return DATE_FORMAT.parse(fileName);
	}

	/**
	 * 日期转日志文件名
	 * @param date
	 * @return
	 */
	private String dateToFileName(Date date) {
		return DATE_FORMAT.format(date) + LOG_FILE_NAME;//日志文件名称
	}
	
	/**
	 * 文件在内存中的路径
	 * @param fileName
	 * @return
	 */
	private String getMemoryFilePath(String fileName) {
		return getMemoryDirPath() + File.separator + fileName;
	}
	
	/**
	 * 内存中日志文件保存目录
	 * @return
	 */
	private String getMemoryDirPath() {
		if (mLogFileDirMemory == null) {
			mLogFileDirMemory = getFilesDir().getAbsolutePath() + File.separator + Constants.LOG_FOLDER;
			Log.w(TAG, "memory dir: " + mLogFileDirMemory);
			creatDir(mLogFileDirMemory);
		}
		return mLogFileDirMemory;
	}
	
	/**
	 * 文件在SD中的路径
	 * @param fileName
	 * @return
	 */
	private String getSdcardFilePath(String fileName) {
		String timeDir = getSdcardDirPath() + File.separator
				+ getIsuMediaDirTime();
		creatDir(timeDir);
		return timeDir + File.separator + fileName;
	}

	/**
	 * 当前年月日时间作为音视频文件夹
	 * @return yyyy-MM-dd
	 */
	public static String getIsuMediaDirTime() {
		Calendar c = Calendar.getInstance();
		SimpleDateFormat sdf = new SimpleDateFormat("yyyy-MM-dd", Locale.getDefault());
		return sdf.format(c.getTime());
	}

	/**
	 * SD卡中日志文件保存的主目录目录
	 *
	 * @return
	 */
	private String getSdcardDirPath() {
		String logFileDir = Environment.getExternalStorageDirectory().getAbsolutePath()
				+ File.separator
				+ Constants.LOG_FOLDER;
		Log.w(TAG, "sdpath: " + logFileDir);
		creatDir(logFileDir);
		return logFileDir;
	}
	
}