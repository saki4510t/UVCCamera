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

package com.serenegiant.utils;


/**
 * 文 件 名: Constants
 * 创 建 人: 蒋朋
 * 创建日期: 17-1-4 13:33
 * 邮    箱: jp19891017@gmail.com
 * 博    客: https://jp1017.github.io/
 * 描    述: 一些常量定义
 * 修 改 人:
 * 修改时间：
 * 修改备注：
 */

public final class Constants {
	public static final long SIZE_SD_FREE_LIMIT = 500;		//sd卡存储限制, 500M
	public static final int COUNT_FILE_DELETE = 10;			//每次删除的dvr视频文件个数
	public static final int SIZE_PRE_MEDIA = 128;			//图片声音前面的128个字节的905基本信息
	public static final int SIZE_PRE_PACKS = 1000;			//上传图片时每包数据大小

	public static final String UVC_FRONT = "1-1.1";    //前置摄像头设备，　行车记录
	public static final String UVC_BACK = "1-1.2";     //后置摄像头设备，　拍照

	public static final int PIC_WIDTH_DEFAULT = 640;		//默认图片宽度
	public static final int PIC_HEIGHT_DEFAULT = 480;        //默认图片高度


	public static final int BRIGHTNESS_LOW = 100;        //电池供电时低亮度
	public static final int BRIGHTNESS_HIGH = 255;       //最大亮度



    //todo 串口摄像头个数
    public static final int NUM_UVC_CAMERA = 2;
	//摄像头关闭后再打开时间间隔(ms), 不可太小, 否则会导致摄像头编号选择失败
    public static final int INTERVAL_UVC_OFF_ON = 600;

	public static final String DIR_U_PAN = "/storage/usbdisk4";        //默认u盘路径
	public static final String DIR_INTERNAL = "/storage/emulated/legacy";        //系统内部存储
	public static final String DIR_SD_CARD = "/storage/sdcard1";        //默认SD卡路径

	public static final String PICTURE_DIR = "Pictures";		//图片存放主目录
	public static final String MOVIE_DIR = "Movies";			//视频存放主目录
	public static final String RINGTONES_DIR = "Ringtones";	//音频存放主目录
	public static final String INST_DVR_DIR = "行车记录";		//dvr视频存放文件夹

	//世新电话
	@Deprecated
	public static final String PHONE_INST = "0532–88815696";


    //后台视频录制时间间隔(s), 10min一个文件(很多厂商都是10分钟一个, 类似标准)
    public final static int TIME_DVR = 10 * 60;
    public final static int INTERVAL_VIDEO = 1000;	//两个视频录制间隔(ms), 否则第二个会没有录像

	//8802, 8805音视频检索, 每次发送个数
	public final static int MEDIA_LIMIT_SEARCH = 120;

	//串口心跳(s), 用于判断计价器, Led连接是否正常, 计价器心跳5s, 这里稍大点,减小误报
	public static final long SERIAL_CHECK_DELAY = 10;

	//串口设备查询状态时间(ms)
	public final static int DELAY_TIME_SERIAL_CHECK = 1000;

	//gps通知栏id
	public static final int NOTIFICATION_ID_GPS = 1;
	//设备状态通知栏
	public static final int NOTIFICATION_ID_DEVICE = 2;
	//行车记录通知
	public static final int NOTIFICATION_ID_DVR = 3;
	//消息通知
	public static final int NOTIFICATION_ID_SMS = 4;

	//网络时间服务器
	public static final String BJTIME_URL = "http://www.bjtime.cn";//bjTime
	public static final String BAIDU_URL = "http://www.baidu.com";//百度
	public static final String TAOBAO_URL = "http://www.taobao.com";//淘宝
	public static final String NTSC_URL = "http://www.ntsc.ac.cn";//中国科学院国家授时中心
	public static final String _360_URL = "http://www.360.cn";//360
	public static final String BEIJING_TIME_URL = "http://www.beijing-time.org";//beijing-time

	//socket通信地址和端口
	public static final String SOCKET_ADDR = "gps.qdsxkj.com";
//	public static final String SOCKET_ADDR = "139.224.226.23";
	public static final int SOCKET_PORT = 4002;
	public static final String BACKUP_ADDR = "gps.qdsxkj.com";
	public static final int BACKUP_PORT = 4008;
	//推流地址,端口
	public static final String PUSHER_ADDR = "video.qdsxkj.com";
//	public static final String PUSHER_ADDR = "192.168.1.7";
	public static final int PUSHER_PORT = 10554;//或者10554
//	public static final int PUSHER_ID = 4002;
	public static final String PUSHER_ID = "107700000088_11";
	public static final String KEY_EASYPUSHER = "6A36334A743536526D3430416F36685A706C6463532F64685A6E646C59575A6B5A723558444661672F307667523246326157346D516D466962334E68514449774D545A4659584E355247467964326C75564756686257566863336B3D";


	//默认数据库名称
	public static final String DEFAULT_DB_NAME = "instlauncher.db";
	//默认数据库密码
	public static final String DEFAULT_DB_PWD = "INST999";
    //设备维护密码
	public static final String PWD_MAINTAIN = "INST888";

	//终端编号最前面一位, 默认10, 手机号码时为"01"
	public static final String ISUID_PRE = "10";
	//isu id最前面两个字节
	public static final String ISUID_HEAD = "7700";
	public static final String ISU_MARK = "77";		//厂商标示, 透传时使用

	//默认用户级别对应的操作
	public static final int USER_OPERATION_DEFAULT  = 0;

	//两个点最大偏移，以此来过滤误差较大的点，35m/s, 相当于126km/h
	public static final int MAX_DISTANCE_BETWEEN_POINTS = 35;

	//位置汇报补传条数/每次
	public static final int LOCATION_PATCH = 100;
	//位置汇报时间大于10ｓ时，补传时间为30ｓ，不变
	public static final int LOCATION_PATCH_TIME = 30 * 1000;

	//todo 可根据不同用户调整，（比如流量小了可以设置为30ｓ）位置保存时间间隔, 10s　保存一次
	public static final int LOCATION_SAVE_TIME = 10 * 1000;
	public static final int LOCATION_SAVE_DIS = 500;    //500m, 汇报间隔大于500m时, 用0203补传

	//isu休眠后, 180s保存一次位置
	public static final int LOCATION_SAVE_SLEEP = 180 * 1000;

	public static final int CARD_ALERT_TIME = 5000;    //提醒司机刷卡时间间隔

	//ISU 参数默认值
	public static final int HEARTBEAT_TIME = 30 * 1000;    //默认心跳包发送间隔 30s
	public static final int HEARTBEAT_TIME_SLEEP = 120 * 1000;    //休眠时心跳包发送间隔
	public static final int SOCKET_CONN_TIMEOUT = 15 * 1000;    //连接超时时间,可设置 15s
	public static final int SOCKET_CONN_FREQUENCY = 1;    //重连次数，默认1
	public static final int SNS_TIMEOUT = 60 * 1000;    //短信发送超时时间,可设置  60s
	public static final int SNS_FREQUENCY = 1;    //短信重发次数，默认1

	//默认位置汇报时间(定时)
	public static final int REPORT_TIME_MIN = 1000;    //定时最小汇报时间  1s
	public static final int REPORT_DIS_MIN = 50;    //定距最小汇报距离  50m

	public static final int REPORT_DIS_INTERVAL_MAX = 3000;    //定距最大汇报距离  3000m, 距离误差上限
	public static final int REPORT_DIS_INTERVAL_MIN = 20;   //定距最小汇报距离  20m, 距离误差下限

	public static final int REPORT_TIME_DEFAULT = 30 * 1000;    //定时默认汇报时间  30s
	public static final int REPORT_TIME_EMPTY = REPORT_TIME_DEFAULT;    //空车汇报  30s
	public static final int REPORT_TIME_LOAD = 10 * 1000;    //重车上报  30s
	public static final int REPORT_TIME_ACC_OFF = 180 * 1000;    //acc 关闭时  180s
	public static final int REPORT_TIME_ACC_ON = 30 * 1000;    //acc 开时  30s
	public static final int REPORT_TIME_NOLOGIN = 30 * 1000;    //未登录时  30s
	public static final int REPORT_TIME_SLEEP = 360 * 1000;    //休眠时  360s
	//这是一个自定义的休眠时间限制, 默认60s, 注意小于30s时会一边发送心跳一边发送位置
	public static final int REPORT_TIME_SLEEP_LIMIT = 60 * 1000;    //休眠时间限制  60s
	public static final int REPORT_TIME_ALARM = 10 * 1000;    //紧急报警时  10s

	//默认位置汇报距离(定距)
	public static final int REPORT_DIS_DEFAULT = 1000;    //定距默认汇报  1000m
	public static final int REPORT_DIS_EMPTY = 1000;    //空车汇报  1000m
	public static final int REPORT_DIS_LOAD = 500;    //重车上报  500m
	public static final int REPORT_DIS_ACC_OFF = 2000;    //acc 关闭时  2000m
	public static final int REPORT_DIS_ACC_ON = 500;    //acc 开时  500m
	public static final int REPORT_DIS_NOLOGIN = 1000;    //未登录时  1000m
	public static final int REPORT_DIS_SLEEP = 2000;    //休眠时  2000m
	public static final int REPORT_DIS_ALARM = 100;    //紧急报警时  100m

	//拐角补传角度60度
	public static final int REPORT_ADDITION_ANGLE = 60;

	//电话接听策略
	public static final int CALL_TACTICS_DEFAULT 		= 1;
	public static final int CALL_EVERYTIME_DEFAULT 		= 300;
	public static final int CALL_MONTH_DEFAULT 			= 30000;
	public static final int PHONENUMBER_LENGTH_DEFAULT 	= 3;

	//报警屏蔽字等默认值
	public static final long ALARM_MASK_DEFAULT = 0xffffffffL;
	public static final long ALARM_SWITCH_SMS_DEFAULT = 0xffffffffL;
	public static final long ALARM_SWITCH_CAPTURE_DEFAULT = 0xffffffffL;
	public static final long ALARM_SWITCH_SAVE_DEFAULT = 0xffffffffL;


	//isu 语音播报音量
	public static final int ISU_TTS_VOLUME_DEFAULT 		= 7;

	//默认省市, 没有, 中心设置
	public static final int TAXI_PROVINCE_DEFAULT = -1;
	public static final int TAXI_CITY_DEFAULT = -1;

	//车辆运行时间，速度限制
	public static final int TAXI_SPEED_MAX_DEFAULT = 120;			//最高速度km/h
	public static final int TAXI_SPEED_EXCEED_TIME_DEFAULT = 10;	//超速持续时间s
	public static final int TAXI_DRIVE_TIME_MAX_DEFAULT = 14400;	//连续驾驶时间s(4小时)
	public static final int TAXI_REST_TIME_MIN_DEFAULT = 1200;		//最小休息时间s(20min)
	//最长停车时间s, 暂时不设置
	public static final int TAXI_STOP_TIME_MAX_DEFAULT = 0;
	public static final int TAXI_DRIVE_TIME_DAY_LIMIT_DEFAULT = 28800;//当天累计驾驶时间s(8小时)

	//图像参数默认值
	public static final int PHOTO_QUALITY_DEFAULT = 7;
	public static final int PHOTO_BRIGHT_DEFAULT = 180;
	public static final int PHOTO_CONTRAST_DEFAULT = 70;
	public static final int PHOTO_SATURATION_DEFAULT = 60;
	public static final int PHOTO_CHROMA_DEFAULT = 100;

	//默认录音参数
	public static final int RECORD_MODE_DEFAULT = 2;		 //默认翻空车牌录音
	public static final int RECORD_TIME_LIMIT_DEFAULT = 10;  //默认10分钟


	public static final int LCD_HEART_BEAT_DEFAULT = 30; //默认30s
	public static final int LED_HEART_BEAT_DEFAULT = 30; //默认30s
	//acc off 后 休眠时间
	public static final int ACC_OFF_SLEEP_TIME_DEFAULT = 60 * 1000; //默认60s

	public static final String CHARSET = "GBK";    //和中心通信编码格式


	/**************** sp 相关 开始****************/

	//默认sp名称
	public static final String SP_NAME = "instcarlauncher";
	//默认sp密码
	public static final String DEFAULT_SP_PWD = "INST999";


	//用户级别
	public static final String SP_USER_LEVEL = "user_level";
	//用户级别对应的操作
	public static final String SP_USER_OPERATION = "user_operation";

	//不是第一次运行程序
	public static final String SP_NOT_FIRST_USE = "not_first_use";

	//是否开启行车记录
	public static final String SP_DVR_OPEN = "dvr_open";


	//是否有sd卡及串口摄像头
	public static final String SP_IS_SD_ATTACHED = "is_sd_attached";
	public static final String SP_IS_UVC_ATTACHED = "is_uvc_attached";


	//主页面客次及空驶里程显示
	public static final String SP_MAIN_TRADE_TODAY = "main_trade_today";
	public static final String SP_MAIN_EMPTY_MILES = "main_empty_miles";
	//主页面天气保存
	public static final String SP_MAIN_WEATHER_CITY = "main_weather_city";
	public static final String SP_MAIN_WEATHER_TEMPERATURE = "main_weather_temperature";
	public static final String SP_MAIN_WEATHER_DETAIL = "main_weather_detail";


	//进入重车时间保存
	public static final String SP_LOAD_TIME1 = "load_time1";
	public static final String SP_LOAD_TIME2 = "load_time2";
	public static final String SP_LOAD_TIME3 = "load_time3";
	public static final String SP_LOAD_TIME4 = "load_time4";
	//进入重车位置保存
	public static final String SP_LOAD_LOCATION1 = "load_location1";
	public static final String SP_LOAD_LOCATION2 = "load_location2";
	public static final String SP_LOAD_LOCATION3 = "load_location3";
	public static final String SP_LOAD_LOCATION4 = "load_location4";

	/*****************   电召订单保存   ***************/
	//这四个乘客是否电召订单
	public static final String SP_IS_ORDER_CALL1 = "order_call1";
	public static final String SP_IS_ORDER_CALL2 = "order_call2";
	public static final String SP_IS_ORDER_CALL3 = "order_call3";
	public static final String SP_IS_ORDER_CALL4 = "order_call4";
	//电召订单号
	public static final String SP_ORDER_CALL_ID1 = "id_order_call_car1";
	public static final String SP_ORDER_CALL_ID2 = "id_order_call_car2";
	public static final String SP_ORDER_CALL_ID3 = "id_order_call_car3";
	public static final String SP_ORDER_CALL_ID4 = "id_order_call_car4";

	public static final String SP_ORDER_CALL_PHONE = "id_order_call_phone";
	public static final String SP_ORDER_CALL_STARTLAT = "id_order_call_startlat";
	public static final String SP_ORDER_CALL_STARTLNG = "id_order_call_startlng";
	public static final String SP_ORDER_CALL_ENDLAT = "id_order_call_endlat";
	public static final String SP_ORDER_CALL_ENDLNG = "id_order_call_endlng";
	public static final String SP_ORDER_CALL_START_PLACE = "id_order_call_start_place";


	public static final String SP_ISUID = "isuid";

	//isu参数
	public static final String SP_JTT_HEART_BEAT = "jtt-heart-beat";
	public static final String SP_TCP_CONNECT_TIME_OUT = "tcp-connect-time-out";
	public static final String SP_TCP_CONNECT_FREQUENCY = "tcp-connect-frequency";
	public static final String SP_SNS_TIME_OUT = "sns-time-out";
	public static final String SP_SNS_FREQUENCY = "sns-frequency";


	//服务器地址
	public static final String SP_SOCKET_ADDR = "socket_addr";
	public static final String SP_SOCKET_PORT = "socket_port";
	public static final String SP_BACKUP_ADDR = "backup_addr";
	public static final String SP_BACKUP_PORT = "backup_port";

	//推流服务器地址
	public static final String SP_PUSHER_PUSHING = "pusher_pushing";	//是否推流
	public static final String SP_PUSHER_ADDR = "pusher_addr";			//推流地址
	public static final String SP_PUSHER_PORT = "pusher_port";			//推流端口
	public static final String SP_PUSHER_ID = "pusher_id";				//推流id

	//位置汇报策略
	public static final String SP_REPORT_TACTICS = "report_tactics";
	//位置汇报方案
	public static final String SP_REPORT_PLAN = "report_plan";


	/**    ***  定时汇报   **/
	public static final String SP_REPORT_TIME_NOLOGIN = "report_time_nologin";
	public static final String SP_REPORT_TIME_EMPTY = "report_time_empty";
	public static final String SP_REPORT_TIME_LOAD = "report_time_load";
	//acc状态
	public static final String SP_REPORT_TIME_ACC_OFF = "report_time_acc_off";
	public static final String SP_REPORT_TIME_ACC_ON = "report_time_acc_on";
	public static final String SP_REPORT_TIME_SLEEP = "report_time_sleep";
	public static final String SP_REPORT_TIME_ALARM = "report_time_alarm";


	/**    ***  定距汇报   **/
	public static final String SP_REPORT_DIS_NOLOGIN = "report_time_nologin";
	public static final String SP_REPORT_DIS_EMPTY = "report_dis_empty";
	public static final String SP_REPORT_DIS_LOAD = "report_dis_load";
	//acc状态
	public static final String SP_REPORT_DIS_ACC_OFF = "report_dis_acc_off";
	public static final String SP_REPORT_DIS_ACC_ON = "report_dis_acc_on";
	public static final String SP_REPORT_DIS_SLEEP = "report_dis_sleep";
	public static final String SP_REPORT_DIS_ALARM = "report_dis_alarm";

	//拐角补传
	public static final String SP_REPORT_ADDITION_ANGLE = "report_addition_angle";

	//电话号码
	public static final String SP_PHONENUMBER_CENTER = "phonenumber_center";//监控中心号码
	public static final String SP_PHONENUMBER_REBOOT= "phonenumber_restart";//isu重启
	public static final String SP_PHONENUMBER_RESET = "phonenumber_reset";	//恢复出厂设置
	public static final String SP_PHONENUMBER_CENTER_SMS = "phonenumber_center_sms";  //中心短信号码
	public static final String SP_PHONENUMBER_ALARM_SMS = "phonenumber_alarm_sms";//短信报警号码
	public static final String SP_PHONENUMBER_MONITOR = "phonenumber_monitor";  //监听号码


	//报警参数设置
	public static final String SP_ALARM_MASK = "alarm_mask";
	public static final String SP_ALARM_SWITCH_SMS = "alarm_switch_sms";
	public static final String SP_ALARM_SWITCH_CAPTURE = "alarm_switch_capture";
	public static final String SP_ALARM_SWITCH_SAVE = "alarm_switch_save";

	/**
	 * 报警是否已经发送sms, 和报警标志对应
	 */
	public static final String SP_ALARM_SMS_SEND = "alarm_sms_send";

	//电话接听策略, 自动接听0自动接听1acc on时自动接听, 默认为1
	public static final String SP_CALL_TACTICS = "call_tactics";

	public static final String SP_CALL_EVERYTIME = "call_everytime";            //每次时长限制
	public static final String SP_CALL_MONTH_LIMIT = "call_month_limit";		//每月时长限制

	public static final String SP_DAY_CURRENT = "day_current";		    //自己添加的参数, 当前日期, 判断当天连续驾驶
	public static final String SP_MONTH_CURRENT = "month_current";		//自己添加的参数, 当前月份, 下月清空已用时长
	public static final String SP_CALL_MONTH_USED = "call_month_used";  //自己添加的参数, 统计该月使用时长

	public static final String SP_PHONENUMBER_LENGTH = "phonenumber_length";
	//isu 语音播报音量
	public static final String SP_ISU_TTS_VOLUME = "isu_tts_volume";

	//isu设备维护密码
	public static final String SP_PASSWORD_MAINTAIN = "password_maintain";

	//出租车相关
	//企业营运许可证号
	public static final String SP_TAXI_COMPANY_LICENCE = "taxi_company_licence";
	public static final String SP_TAXI_COMPANY_NAME = "taxi_company_name";

	//驾驶员从业资格证
	public static final String SP_DRIVER_CERTIFICATE = "driver_certificate";

	//出租车营运时间次数限制
	public static final String SP_TAXI_TRADE_COUNT_LIMIT = "taxi_trade_count_limit";
	public static final String SP_TAXI_TRADE_TIME_LIMIT = "taxi_trade_time_limit";

	//车辆运行时间，速度限制
	public static final String SP_TAXI_SPEED_MAX = "taxi_speed_max";
	public static final String SP_TAXI_SPEED_EXCEED_TIME = "taxi_exceed_time";
	public static final String SP_TAXI_DRIVE_TIME_MAX = "taxi_drive_time_max";
	public static final String SP_TAXI_REST_TIME_MIN = "taxi_rest_time_min";
	public static final String SP_TAXI_STOP_TIME_MAX = "taxi_stop_time_max";
	public static final String SP_TAXI_DRIVE_TIME_DAY_LIMIT = "taxi_drive_time_day_limit";

	//自定义的计算连续驾驶等时间
	public static final String SP_TAXI_DRIVE_TIME_START = "taxi_drive_time_start";	//连续驾驶开始时间
	public static final String SP_TAXI_DRIVE_TIME_DAY_USED = "taxi_drive_time_day_used";//当天驾驶时间累计
	public static final String SP_TAXI_REST_TIME_START = "taxi_rest_time_start";	//休息开始时间

	//图像参数
	public static final String SP_PHOTO_QUALITY = "photo_quality";
	public static final String SP_PHOTO_BRIGHT = "photo_bright";
	public static final String SP_PHOTO_CONTRAST = "photo_contrast";
	public static final String SP_PHOTO_SATURATION = "photo_saturation";
	public static final String SP_PHOTO_CHROMA = "photo_chroma";

	//录音参数
	public static final String SP_RECORD_MODE = "record_mode";
	public static final String SP_RECORD_TIME_LIMIT = "record_time_limit";


	public static final String SP_LCD_HEART_BEAT = "lcd_heart_beat";
	public static final String SP_LED_HEART_BEAT = "led_heart_beat";
	//acc off 后 休眠时间
	public static final String SP_ACC_OFF_SLEEP_TIME = "acc_off_sleep_time";

	//车辆里程表及所在省市
	public static final String SP_TAXI_BOARD_MILES = "taxi_board_miles";
	public static final String SP_TAXI_PROVINCE = "taxi_province";
	public static final String SP_TAXI_CITY = "taxi_city";

	//计价器K值
	public static final String SP_TAXI_K_VALUE = "taxi_k_value";

	//签到签退时间保存
	public static final String SP_TIME_SIGN_IN = "time_sign_in";
	public static final String SP_TIME_SIGN_OUT = "time_sign_out";

	//重车时保存乘客信息
	public static final String SP_LOAD_PSNG_INFO = "load_psng_info";
	public static final String SP_LOAD_PSNG_LEFT = "load_psng_left";	//重车时可拼客次保存
	/**
	 * 司机刷卡信息
	 * 第0位, 数据库是否存在, 1:已存在, 0:新用户
	 * 第1位, 签到/签退, 0:签到, 1:签退
	 * 第2位, 正常签到/签退或强制签到/签退, 0:正常, 1:强制
	 */
	public static final String SP_CARD_INFO = "card_info";


	public static final String SP_IS_ID_INVALID = "is_id_invalid";	//司机信息是否无效
	public static final String SP_IS_NO_CARD_WORK = "is_no_card_work";	//是否无卡上班
	public static final String SP_SIGN_IN_FAIL_INFO = "sign_in_fail_info";	//签退失败详情

	/**
	 * 主页显示错误提示值
	 * 第0位, 网络状态, 1-网络有误, 0-网络正常
	 * 第1位, 计价器时间限制, 1-计价器时间限制已到, 0-计价器正常
	 * 第2位, 计价器次数限制, 1-计价器次数限制已到, 0-计价器正常
	 * 第3位, 是否已签退, 1-已签退, 0-签到
	 */
	public static final String SP_ERROR_INFO = "error_info";


	//司机信息相关
	public static final String SP_DRIVER_NAME = "driver_name";            //司机姓名

	public static final String SP_TAXI_PLATE = "driver_plate";       //车牌号
	public static final String SP_DRIVER_RANK = "driver_rank";       //等级评定

	//来电自动接听延时3s
	public static final int ANSWER_CALL_DURATION = 3000;

	//设备关机时间
	public static final String SP_SHUT_DOWN_TIME = "shut_down_time";

	//	当前led灯状态
	public static final String SP_LED_STATUS = "led_status";
	public static final String SP_LED_DOT_MATRIX_STR = "dot_matrix_str";

	//默认进重车是否多乘客模式
	public static final String SP_MULTI_PASSENGER_OPEN = "multi_passenger_open";


	//导航偏好设置
	public static final String SP_NAVI_AVOID_CONGESTION = "navi_avoid_congestion";
	public static final String SP_NAVI_AVOID_COST = "navi_avoid_cost";
	public static final String SP_NAVI_AVOID_HIGHSPEED = "navi_avoid_highspeed";
	public static final String SP_NAVI_PRIORITY_HIGHSPEED = "navi_priority_highspeed";

	/*************** sp 相关 结束****************/

	public static final String WECHAT_CODE = "wxp:";      //识别微信收款二维码的关键字
	public static final String ALIPAY_CODE = "alipay";   //识别支付宝收款二维码的关键字

	//原生定位默认定位时间间隔 1s
	public static final long LOCATION_BEAT = 1000;

	//	高德地图相关
	public static final long AMP_LOCATION_BEAT = 1000; //高德地图默认定位时间间隔 1s
	//高德地图初始比例尺 14=500m, 15=200m, 16=100m, 太小的话实时轨迹会有误
	public static final float AMAP_ZOOM = 15;


	public static final int QR_WIDTH = 300;
	public static final int QR_HEIGHT = 300;

	/**
	 * 日志保存路径
	 */
	public static final String LOG_FOLDER = "uvclog";    //父文件夹

	/**
	 * 参数控制及导出
	 */
	public static final String ISU_CONTROL_PARAM = "CONFIG.CRL";   //参数控制文件
	public static final String ISU_CONTROL_PIC = "RPEXPORT.CRL";   //录音、图片控制文件
	public static final String ISU_CONTROL_TRADE = "YYEXPORT.CRL"; //营运数据采集控制文件
	public static final String ISU_CONTROL_UPDATE = "UPDATE.CRL";  //固件升级控制文件

	public static final String ISU_POSTFIX_CFG = ".CFG";  //参数设置导出后缀
	public static final String ISU_POSTFIX_DAT = ".DAT";  //数据采集后缀
	public static final String ISU_POSTFIX_DAT_QT = "QT.DAT";  //数据采集签退记录后缀
	public static final String ISU_POSTFIX_DAT_YY = "YY.DAT";  //数据采集营运记录后缀
	public static final String ISU_POSTFIX_DAT_JY = "JY.DAT";  //数据采集ic卡刷卡交易后缀


	//自定义配置文件
	public static final String ISU_CONTROL_PARAM_INST = "INSTCONFIG.CRL";  //世新维护人员导出参数, 然后设置到其他车辆

	/**
	 * 字体目录
	 */
	public static final String FONT = "fonts/";

}
