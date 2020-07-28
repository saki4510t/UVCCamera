#ifndef LIBUVC_H
#define LIBUVC_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h> // FILE
#include <libusb/libusb.h>
#include <libuvc/libuvc_config.h>

/**
 * UVC error types, based on libusb errors
 * UVC 错误类型，基于libusb错误
 * @ingroup diag
 */
typedef enum uvc_error {
	/** Success (no error) 成功（没有错误） */
	UVC_SUCCESS = 0,
	/** Input/output error 输入输出错误 */
	UVC_ERROR_IO = -1,
	/** Invalid parameter 无效参数 */
	UVC_ERROR_INVALID_PARAM = -2,
	/** Access denied 拒绝访问 */
	UVC_ERROR_ACCESS = -3,
	/** No such device 没有设备 */
	UVC_ERROR_NO_DEVICE = -4,
	/** Entity not found 找不到实体 */
	UVC_ERROR_NOT_FOUND = -5,
	/** Resource busy 资源繁忙 */
	UVC_ERROR_BUSY = -6,
	/** Operation timed out 操作超时 */
	UVC_ERROR_TIMEOUT = -7,
	/** Overflow 溢出 */
	UVC_ERROR_OVERFLOW = -8,
	/** Pipe error 管道错误 */
	UVC_ERROR_PIPE = -9,
	/** System call interrupted 系统调用中断 */
	UVC_ERROR_INTERRUPTED = -10,
	/** Insufficient memory 内存不足 */
	UVC_ERROR_NO_MEM = -11,
	/** Operation not supported 不支持操作 */
	UVC_ERROR_NOT_SUPPORTED = -12,
	/** Device is not UVC-compliant 设备不符合UVC */
	UVC_ERROR_INVALID_DEVICE = -50,
	/** Mode not supported 不支持模式 */
	UVC_ERROR_INVALID_MODE = -51,
	/** Resource has a callback (can't use polling and async) 资源具有回调（不能使用轮询和异步） */
	UVC_ERROR_CALLBACK_EXISTS = -52,
	/** Undefined error 未定义错误 */
	UVC_ERROR_OTHER = -99
} uvc_error_t;

/**
 * Table 4-7 VC Request Error Code Control XXX add saki@serenegiant.com
 * 表 4-7 VC 请求错误代码控制XXX
 */
typedef enum uvc_vc_error_code_control {
	UVC_ERROR_CODECTRL_NO_ERROR = 0x00,
	UVC_ERROR_CODECTRL_NOT_READY = 0x01,
	UVC_ERROR_CODECTRL_WRONG_STATE = 0x02,
	UVC_ERROR_CODECTRL_POWER = 0x03,
	UVC_ERROR_CODECTRL_OUT_OF_RANGE = 0x04,
	UVC_ERROR_CODECTRL_INVALID_UINT = 0x05,
	UVC_ERROR_CODECTRL_INVALID_CONTROL = 0x06,
	UVC_ERROR_CODECTRL_INVALID_REQUEST = 0x07,
	UVC_ERROR_CODECTRL_INVALID_VALUE = 0x08,
	UVC_ERROR_CODECTRL_UNKNOWN = 0xff
} uvc_vc_error_code_control_t;

/**
 * VS Request Error Code Control XXX add saki@serenegiant.com
 * VS 请求错误代码控制XXX
 */
typedef enum uvc_vs_error_code_control {
	UVC_VS_ERROR_CODECTRL_NO_ERROR = 0,
	UVC_VS_ERROR_CODECTRL_PROTECTED = 1,
	UVC_VS_ERROR_CODECTRL_IN_BUFEER_UNDERRUN = 2,
	UVC_VS_ERROR_CODECTRL_DATA_DISCONTINUITY = 3,
	UVC_VS_ERROR_CODECTRL_OUT_BUFEER_UNDERRUN = 4,
	UVC_VS_ERROR_CODECTRL_OUT_BUFEER_OVERRUN = 5,
	UVC_VS_ERROR_CODECTRL_FORMAT_CHANGE = 6,
	UVC_VS_ERROR_CODECTRL_STILL_CAPTURE_ERROR = 7,
	UVC_VS_ERROR_CODECTRL_UNKNOWN = 8,
} uvc_vs_error_code_control_t;

/**
 * Color coding of stream, transport-independent
 * 流的颜色编码，独立于传输
 * @ingroup streaming
 */
enum uvc_frame_format {
	UVC_FRAME_FORMAT_UNKNOWN = 0,
	/**
	 * Any supported format
	 * 任何支持的格式
	 */
	UVC_FRAME_FORMAT_ANY = 0,
	// 未压缩
	UVC_FRAME_FORMAT_UNCOMPRESSED,
	// 压缩
	UVC_FRAME_FORMAT_COMPRESSED,
	/**
	 * YUYV/YUV2/YUV422: YUV encoding with one luminance value per pixel and
	 * one UV (chrominance) pair for every two pixels.
	 * YUYV/YUV2/YUV422: YUV编码，每个像素一个亮度值，每两个像素一对UV（色度）对。
	 */
	UVC_FRAME_FORMAT_YUYV,
	UVC_FRAME_FORMAT_UYVY,
	/** 16-bits RGB */
	UVC_FRAME_FORMAT_RGB565,	// RGB565
	/** 24-bit RGB */
	UVC_FRAME_FORMAT_RGB,		// RGB888
	UVC_FRAME_FORMAT_BGR,		// BGR888
	/* 32-bits RGB */
	UVC_FRAME_FORMAT_RGBX,		// RGBX8888
	/**
	 * Motion-JPEG (or JPEG) encoded images
	 * Motion-JPEG（或JPEG）编码的图像
	 */
	UVC_FRAME_FORMAT_MJPEG,
	UVC_FRAME_FORMAT_GRAY8,
	UVC_FRAME_FORMAT_BY8,
	/**
	 * Number of formats understood
	 * 了解的格式数
	 */
	UVC_FRAME_FORMAT_COUNT,
};

/**
 * UVC_COLOR_FORMAT_* have been replaced with UVC_FRAME_FORMAT_*. Please use
 * UVC_FRAME_FORMAT_* instead of using these.
 * UVC_COLOR_FORMAT_* 已替换为 UVC_FRAME_FORMAT_*。 请使用 UVC_FRAME_FORMAT_* 代替这些。
 */
#define UVC_COLOR_FORMAT_UNKNOWN UVC_FRAME_FORMAT_UNKNOWN
#define UVC_COLOR_FORMAT_UNCOMPRESSED UVC_FRAME_FORMAT_UNCOMPRESSED
#define UVC_COLOR_FORMAT_COMPRESSED UVC_FRAME_FORMAT_COMPRESSED
#define UVC_COLOR_FORMAT_YUYV UVC_FRAME_FORMAT_YUYV
#define UVC_COLOR_FORMAT_UYVY UVC_FRAME_FORMAT_UYVY
#define UVC_COLOR_FORMAT_RGB UVC_FRAME_FORMAT_RGB
#define UVC_COLOR_FORMAT_BGR UVC_FRAME_FORMAT_BGR
#define UVC_COLOR_FORMAT_RGB565 UVC_FRAME_FORMAT_RGB565	// XXX
#define UVC_COLOR_FORMAT_RGBX UVC_FRAME_FORMAT_RGBX		// XXX
#define UVC_COLOR_FORMAT_MJPEG UVC_FRAME_FORMAT_MJPEG
#define UVC_COLOR_FORMAT_GRAY8 UVC_FRAME_FORMAT_GRAY8

/**
 * VideoStreaming interface descriptor subtype (A.6)
 * VideoStreaming接口描述符子类型（A.6）
 */
enum uvc_vs_desc_subtype {
  UVC_VS_UNDEFINED = 0x00,
  UVC_VS_INPUT_HEADER = 0x01,
  UVC_VS_OUTPUT_HEADER = 0x02,
  UVC_VS_STILL_IMAGE_FRAME = 0x03,
  UVC_VS_FORMAT_UNCOMPRESSED = 0x04,
  UVC_VS_FRAME_UNCOMPRESSED = 0x05,
  UVC_VS_FORMAT_MJPEG = 0x06,
  UVC_VS_FRAME_MJPEG = 0x07,
  UVC_VS_FORMAT_MPEG2TS = 0x0a,
  UVC_VS_FORMAT_DV = 0x0c,
  UVC_VS_COLORFORMAT = 0x0d,
  UVC_VS_FORMAT_FRAME_BASED = 0x10,
  UVC_VS_FRAME_FRAME_BASED = 0x11,
  UVC_VS_FORMAT_STREAM_BASED = 0x12
};

struct uvc_format_desc;
struct uvc_frame_desc;

/**
 * Frame descriptor
 * A "frame" is a configuration of a streaming format
 * for a particular image size at one of possibly several
 * available frame rates.
 * 帧描述符
 * “帧”是在可能的几个可用帧速率中的一个的特定图像大小的流格式的配置。
 */
typedef struct uvc_frame_desc {
  // uvc格式描述
  struct uvc_format_desc *parent;
  // uvc帧描述
  struct uvc_frame_desc *prev, *next;
  /**
   * Type of frame, such as JPEG frame or uncompressed frme
   * 帧类型，如JPEG帧或未压缩frme
   */
  enum uvc_vs_desc_subtype bDescriptorSubtype;
  /**
   * Index of the frame within the list of specs available for this format
   * 此格式可用规范列表中的框架索引
   */
  uint8_t bFrameIndex;
  uint8_t bmCapabilities;
  /** Image width */
  uint16_t wWidth;
  /** Image height */
  uint16_t wHeight;
  /**
   * Bitrate of corresponding stream at minimal frame rate
   * 最小帧速率下相应流的比特率
   */
  uint32_t dwMinBitRate;
  /**
   * Bitrate of corresponding stream at maximal frame rate
   * 最大帧速率下相应流的比特率
   */
  uint32_t dwMaxBitRate;
  /**
   * Maximum number of bytes for a video frame
   * 视频帧的最大字节数
   */
  uint32_t dwMaxVideoFrameBufferSize;
  /**
   * Default frame interval (in 100ns units)
   * 默认帧间隔（以100ns为单位）
   */
  uint32_t dwDefaultFrameInterval;
  /**
   * Minimum frame interval for continuous mode (100ns units)
   * 连续模式的最小帧间隔（100ns单位）
   */
  uint32_t dwMinFrameInterval;
  /**
   * Maximum frame interval for continuous mode (100ns units)
   * 连续模式的最大帧间隔（100ns单位）
   */
  uint32_t dwMaxFrameInterval;
  /**
   * Granularity of frame interval range for continuous mode (100ns)
   * 连续模式下帧间隔范围的粒度（100ns）
   */
  uint32_t dwFrameIntervalStep;
  /**
   * Frame intervals
   * 帧间隔
   */
  uint8_t bFrameIntervalType;
  /**
   * number of bytes per line
   * 每行字节数
   */
  uint32_t dwBytesPerLine;
  /**
   * Available frame rates, zero-terminated (in 100ns units)
   * 可用帧速率，零终止（以100ns为单位）
   */
  uint32_t *intervals;
} uvc_frame_desc_t;

/**
 * Format descriptor
 * A "format" determines a stream's image type (e.g., raw YUYV or JPEG)
 * and includes many "frame" configurations.
 * 格式描述符
 * “格式”确定流的图像类型（例如原始YUYV或JPEG），并包括许多“帧”配置。
 */
typedef struct uvc_format_desc {
  //uvc流接口
  struct uvc_streaming_interface *parent;
  //uvc格式描述
  struct uvc_format_desc *prev, *next;
  /**
   * Type of image stream, such as JPEG or uncompressed.
   * 图像流的类型，例如JPEG或未压缩。
   */
  enum uvc_vs_desc_subtype bDescriptorSubtype;
  /**
   * Identifier of this format within the VS interface's format list
   * VS界面格式列表中此格式的标识符
   */
  uint8_t bFormatIndex;
  uint8_t bNumFrameDescriptors;
  /**
   * Format specifier
   * 格式说明符
   */
  union {
    uint8_t guidFormat[16];
    uint8_t fourccFormat[4];
  };
  /**
   * Format-specific data
   * 特定格式的数据
   */
  union {
    /**
     * BPP for uncompressed stream
     * 未压缩流的BPP
     */
    uint8_t bBitsPerPixel;
    /**
     * Flags for JPEG stream
     * JPEG流的标志
     */
    uint8_t bmFlags;
  };
  /**
   * Default {uvc_frame_desc} to choose given this format
   * 默认 {uvc_frame_desc} 选择给定此格式
   */
  uint8_t bDefaultFrameIndex;
  uint8_t bAspectRatioX;
  uint8_t bAspectRatioY;
  uint8_t bmInterlaceFlags;
  uint8_t bCopyProtect;
  uint8_t bVariableSize;
  /**
   * Available frame specifications for this format
   * 此格式的可用框架规格
   */
  struct uvc_frame_desc *frame_descs;
} uvc_format_desc_t;

/**
 * UVC request code (A.8)
 * UVC 请求码
 */
/*
Attribute   Description
GET_CUR     返回流接口的当前状态。所有支持的字段都设置为零将返回一个可接受的协商值。在初始SET_CUR操作之前，GET_CUR状态是未定义的。如果协商失败，该请求将暂停。
GET_MIN     返回协商字段的最小值。
GET_MAX     返回协商字段的最大值。
GET_RES     返回探测/提交数据结构中每个受支持字段的分辨率。
GET_DEF     返回协商字段的默认值。
GET_LEN     返回Probe数据结构的长度。
GET_INFO    查询控件的功能和状态。为此请求返回的值应将D0和D1位分别设置为一（1），其余位设置为零（0）（请参见第4.1.2节“获取请求”）。
SET_CUR     设置流接口探测状态。这是用于流参数协商的属性。如果设备将处于不支持的状态或协商字段的值超出范围，则此请求将协议STALL。有关要注册的确切错误，请参见第4.2.1.2节“请求错误代码控制”。
*/
enum uvc_req_code {
	UVC_RC_UNDEFINED = 0x00,
	UVC_SET_CUR = 0x01,			// bmRequestType=0x21
	UVC_GET_CUR = 0x81,			// bmRequestType=0xa1
	UVC_GET_MIN = 0x82,			// ↑
	UVC_GET_MAX = 0x83,			// ↑
	UVC_GET_RES = 0x84,			// ↑
	UVC_GET_LEN = 0x85,			// ↑
	UVC_GET_INFO = 0x86,		// ↑
	UVC_GET_DEF = 0x87			// ↑
};

enum uvc_device_power_mode {
	UVC_VC_VIDEO_POWER_MODE_FULL = 0x000b,
	UVC_VC_VIDEO_POWER_MODE_DEVICE_DEPENDENT = 0x001b,
};

/**
 * Camera terminal control selector (A.9.4)
 * 相机终端控制选择器（A.9.4）
 */
enum uvc_ct_ctrl_selector {
	UVC_CT_CONTROL_UNDEFINED = 0x00,
	UVC_CT_SCANNING_MODE_CONTROL = 0x01,
	UVC_CT_AE_MODE_CONTROL = 0x02,
	UVC_CT_AE_PRIORITY_CONTROL = 0x03,
	UVC_CT_EXPOSURE_TIME_ABSOLUTE_CONTROL = 0x04,
	UVC_CT_EXPOSURE_TIME_RELATIVE_CONTROL = 0x05,
	UVC_CT_FOCUS_ABSOLUTE_CONTROL = 0x06,
	UVC_CT_FOCUS_RELATIVE_CONTROL = 0x07,
	UVC_CT_FOCUS_AUTO_CONTROL = 0x08,
	UVC_CT_IRIS_ABSOLUTE_CONTROL = 0x09,
	UVC_CT_IRIS_RELATIVE_CONTROL = 0x0a,
	UVC_CT_ZOOM_ABSOLUTE_CONTROL = 0x0b,
	UVC_CT_ZOOM_RELATIVE_CONTROL = 0x0c,
	UVC_CT_PANTILT_ABSOLUTE_CONTROL = 0x0d,
	UVC_CT_PANTILT_RELATIVE_CONTROL = 0x0e,
	UVC_CT_ROLL_ABSOLUTE_CONTROL = 0x0f,
	UVC_CT_ROLL_RELATIVE_CONTROL = 0x10,
	UVC_CT_PRIVACY_CONTROL = 0x11,
	UVC_CT_FOCUS_SIMPLE_CONTROL = 0x12,
	UVC_CT_DIGITAL_WINDOW_CONTROL = 0x13,
	UVC_CT_REGION_OF_INTEREST_CONTROL = 0x14
};

/**
 * Processing unit control selector (A.9.5)
 * 处理单元控制选择器（A.9.5）
 */
enum uvc_pu_ctrl_selector {
	UVC_PU_CONTROL_UNDEFINED = 0x00,
	UVC_PU_BACKLIGHT_COMPENSATION_CONTROL = 0x01,
	UVC_PU_BRIGHTNESS_CONTROL = 0x02,
	UVC_PU_CONTRAST_CONTROL = 0x03,
	UVC_PU_GAIN_CONTROL = 0x04,
	UVC_PU_POWER_LINE_FREQUENCY_CONTROL = 0x05,
	UVC_PU_HUE_CONTROL = 0x06,
	UVC_PU_SATURATION_CONTROL = 0x07,
	UVC_PU_SHARPNESS_CONTROL = 0x08,
	UVC_PU_GAMMA_CONTROL = 0x09,
	UVC_PU_WHITE_BALANCE_TEMPERATURE_CONTROL = 0x0a,
	UVC_PU_WHITE_BALANCE_TEMPERATURE_AUTO_CONTROL = 0x0b,
	UVC_PU_WHITE_BALANCE_COMPONENT_CONTROL = 0x0c,
	UVC_PU_WHITE_BALANCE_COMPONENT_AUTO_CONTROL = 0x0d,
	UVC_PU_DIGITAL_MULTIPLIER_CONTROL = 0x0e,
	UVC_PU_DIGITAL_MULTIPLIER_LIMIT_CONTROL = 0x0f,
	UVC_PU_HUE_AUTO_CONTROL = 0x10,
	UVC_PU_ANALOG_VIDEO_STANDARD_CONTROL = 0x11,
	UVC_PU_ANALOG_LOCK_STATUS_CONTROL = 0x12,
	UVC_PU_CONTRAST_AUTO_CONTROL = 0x13,		// XXX
};

/**
 * USB terminal type (B.1)
 * USB端子类型（B.1）
 */
enum uvc_term_type {
	UVC_TT_VENDOR_SPECIFIC = 0x0100,
	UVC_TT_STREAMING = 0x0101
};

/**
 * Input terminal type (B.2)
 * 输入端子类型（B.2）
 */
enum uvc_it_type {
	UVC_ITT_VENDOR_SPECIFIC = 0x0200,
	UVC_ITT_CAMERA = 0x0201,
	UVC_ITT_MEDIA_TRANSPORT_INPUT = 0x0202
};

/**
 * Output terminal type (B.3)
 * 输出端子类型（B.3）
 */
enum uvc_ot_type {
	UVC_OTT_VENDOR_SPECIFIC = 0x0300,
	UVC_OTT_DISPLAY = 0x0301,
	UVC_OTT_MEDIA_TRANSPORT_OUTPUT = 0x0302
};

/**
 * External terminal type (B.4)
 * 外部端子类型（B.4）
 */
enum uvc_et_type {
	UVC_EXTERNAL_VENDOR_SPECIFIC = 0x0400,
	UVC_COMPOSITE_CONNECTOR = 0x0401,
	UVC_SVIDEO_CONNECTOR = 0x0402,
	UVC_COMPONENT_CONNECTOR = 0x0403
};

/**
 * Context, equivalent to libusb's contexts.
 * May either own a libusb context or use one that's already made.
 * Always create these with uvc_get_context.
 * 上下文，等同于libusb的上下文。
 * 可以拥有libusb上下文，也可以使用已经创建的上下文
 * 始终使用uvc_get_context创建它们。
 */
struct uvc_context;
typedef struct uvc_context uvc_context_t;

/**
 * UVC device.
 * Get this from uvc_get_device_list() or uvc_find_device().
 * UVC设备。
 * 从 uvc_get_device_list()或 uvc_find_device() 获取此文件。
 */
struct uvc_device;
typedef struct uvc_device uvc_device_t;

/**
 * Handle on an open UVC device.
 * Get one of these from uvc_open(). Once you uvc_close()
 * it, it's no longer valid.
 * 处理打开的UVC设备。
 * 从 uvc_open() 中获取其中之一。 一旦您 uvc_close()，它就不再有效。
 */
struct uvc_device_handle;
typedef struct uvc_device_handle uvc_device_handle_t;

/**
 * Handle on an open UVC stream.
 * Get one of these from uvc_stream_open*().
 * Once you uvc_stream_close() it, it will no longer be valid.
 * 处理开放的UVC流。
 * 从 uvc_stream_open*() 中获取其中之一。
 * 一旦您使用 uvc_stream_close()，它将不再有效。
 */
struct uvc_stream_handle;
typedef struct uvc_stream_handle uvc_stream_handle_t;

/**
 * Representation of the interface that brings data into the UVC device
 * 表示将数据导入UVC设备的接口
 */
typedef struct uvc_input_terminal {
	struct uvc_input_terminal *prev, *next;
	/**
	 * Index of the terminal within the device
	 * 设备内终端的索引
	 */
	uint8_t bTerminalID;
	/**
	 * Type of terminal (e.g., camera)
	 * 终端类型（例如摄像机）
	 */
	enum uvc_it_type wTerminalType;
	uint16_t wObjectiveFocalLengthMin;
	uint16_t wObjectiveFocalLengthMax;
	uint16_t wOcularFocalLength;
	/**
	 * Camera controls (meaning of bits given in {uvc_ct_ctrl_selector})
	 * 相机控件（{uvc_ct_ctrl_selector}中给出的位的含义）
	 */
	uint64_t bmControls;
	/**
	 * request code(wIndex)
	 * 请求代码（wIndex）
	 */
	uint16_t request;
} uvc_input_terminal_t;

typedef struct uvc_output_terminal {
	struct uvc_output_terminal *prev, *next;
	/** @todo */
	/**
	 * Index of the terminal within the device
	 * 设备内终端的索引
	 */
	uint8_t bTerminalID;
	/**
	 * Type of terminal (e.g., camera)
	 * 终端类型（例如摄像机）
	 */
	enum uvc_ot_type wTerminalType;
	uint16_t bAssocTerminal;
	uint8_t bSourceID;
	uint8_t iTerminal;
	/**
	 * request code(wIndex)
	 * 请求代码（wIndex）
	 */
	uint16_t request;
} uvc_output_terminal_t;

/**
 * Represents post-capture processing functions
 * 表示捕获后处理功能
 */
typedef struct uvc_processing_unit {
	struct uvc_processing_unit *prev, *next;
	/**
	 * Index of the processing unit within the device
	 * 设备内处理单元的索引
	 */
	uint8_t bUnitID;
	/**
	 * Index of the terminal from which the device accepts images
	 * 设备接受图像的终端的索引
	 */
	uint8_t bSourceID;
	/**
	 * Processing controls (meaning of bits given in {uvc_pu_ctrl_selector})
	 * 处理控件（{uvc_pu_ctrl_selector}中给定的位的含义）
	 */
	uint64_t bmControls;
	/**
	 * request code(wIndex)
	 * 请求代码（wIndex）
	 */
	uint16_t request;
} uvc_processing_unit_t;

/**
 * Custom processing or camera-control functions
 * 定制处理或摄像机控制功能
 */
typedef struct uvc_extension_unit {
	struct uvc_extension_unit *prev, *next;
	/**
	 * Index of the extension unit within the device
	 * 设备内扩展单元的索引
	 */
	uint8_t bUnitID;
	/**
	 * GUID identifying the extension unit
	 * 标识扩展单元的GUID
	 */
	uint8_t guidExtensionCode[16];
	/**
	 * Bitmap of available controls (manufacturer-dependent)
	 * 可用控件的位图（取决于制造商）
	 */
	uint64_t bmControls;
	/**
	 * request code(wIndex)
	 * 请求代码（wIndex）
	 */
	uint16_t request;
} uvc_extension_unit_t;

enum uvc_status_class {
	UVC_STATUS_CLASS_CONTROL = 0x10,
	UVC_STATUS_CLASS_CONTROL_CAMERA = 0x11,
	UVC_STATUS_CLASS_CONTROL_PROCESSING = 0x12,
};

enum uvc_status_attribute {
	UVC_STATUS_ATTRIBUTE_VALUE_CHANGE = 0x00,
	UVC_STATUS_ATTRIBUTE_INFO_CHANGE = 0x01,
	UVC_STATUS_ATTRIBUTE_FAILURE_CHANGE = 0x02,
	UVC_STATUS_ATTRIBUTE_UNKNOWN = 0xff
};

/**
 * A callback function to accept status updates
 * 接受状态更新的回调函数
 * @ingroup device
 */
typedef void (uvc_status_callback_t)(enum uvc_status_class status_class,
		int event, int selector, enum uvc_status_attribute status_attribute,
		void *data, size_t data_len, void *user_ptr);

/**
 * A callback function to accept button events
 * 接受按钮事件的回调函数
 * @ingroup device
 */
typedef void(uvc_button_callback_t)(int button,
                                    int state,
                                    void *user_ptr);

/**
 * Structure representing a UVC device descriptor.
 * (This isn't a standard structure.)
 * 表示UVC设备描述符的结构。
 *（这不是标准结构。）
 */
typedef struct uvc_device_descriptor {
	/** Vendor ID 供应商ID */
	uint16_t idVendor;
	/** Product ID 产品编号 */
	uint16_t idProduct;
	/**
	 * UVC compliance level, e.g. 0x0100 (1.0), 0x0110
	 * 符合UVC标准，例如 0x0100（1.0），0x0110
	 */
	uint16_t bcdUVC;
	/**
	 * Serial number (null if unavailable)
	 * 序列号（如果不可用，则为空）
	 */
	const char *serialNumber;
	/**
	 * Device-reported manufacturer name (or null)
	 * 设备报告的制造商名称（或null）
	 */
	const char *manufacturer;
	/**
	 * Device-reporter product name (or null)
	 * 设备报告者产品名称（或null）
	 */
	const char *product;
} uvc_device_descriptor_t;

/**
 * An image frame received from the UVC device
 * 从UVC设备接收的图像帧
 * @ingroup streaming
 */
typedef struct uvc_frame {
	/**
	 * Image data for this frame
	 * 该帧的图像数据
	 */
	void *data;
	/**
	 * Size of image data buffer
	 * 图像数据缓冲区的大小，大于等于actual_bytes
	 */
	size_t data_bytes;
	/**
	 * XXX Size of actual received data to confirm whether the received bytes is same
	 * as expected on user function when some microframes dropped
	 * XXX实际接收到的数据的大小，以确认当某些微帧丢失时接收到的字节是否与用户功能期望的字节相同
	 * 实际数据长度，大小小于等于data_bytes，数据处理时，使用该值
	 */
	size_t actual_bytes;
	/**
	 * Width of image in pixels
	 * 图片宽度（以像素为单位）
	 */
	uint32_t width;
	/**
	 * Height of image in pixels
	 * 图片高度（以像素为单位）
	 */
	uint32_t height;
	/**
	 * Pixel data format
	 * 像素数据格式
	 */
	enum uvc_frame_format frame_format;
	/**
	 * Number of bytes per horizontal line (undefined for compressed format)
	 * 每行的字节数（对于压缩格式未定义）
	 */
	size_t step;
	/**
	 * Frame number (may skip, but is strictly monotonically increasing)
	 * 帧号（可以跳过，但严格单调递增）
	 */
	uint32_t sequence;
	/**
	 * Estimate of system time when the device started capturing the image
	 * 估计设备开始捕获映像时的系统时间
	 */
	struct timeval capture_time;
	/**
	 * Handle on the device that produced the image.
	 * @warning You must not call any uvc_* functions during a callback.
	 * 生成映像的设备的句柄。
	 * @warning 在回调期间不能调用任何 uvc_* 函数。
	 */
	uvc_device_handle_t *source;
	/**
	 * Is the data buffer owned by the library?
	 * If 1, the data buffer can be arbitrarily reallocated by frame conversion
	 * functions.
	 * If 0, the data buffer will not be reallocated or freed by the library.
	 * Set this field to zero if you are supplying the buffer.
	 * 数据缓冲区是否属于库?
     * 如果是1，数据缓冲区可以被帧转换函数任意重新分配。
     * 如果为0，则库不会重新分配或释放数据缓冲区。
     * 如果要提供缓冲区，则将该字段设置为零。
	 */
	uint8_t library_owns_data;
} uvc_frame_t;

/**
 * A callback function to handle incoming assembled UVC frames
 * 一个回调函数来处理传入的汇编UVC帧
 * @ingroup streaming
 */
typedef void(uvc_frame_callback_t)(struct uvc_frame *frame, void *user_ptr);

/**
 * Streaming mode, includes all information needed to select stream
 * 流模式，包括选择流所需的所有信息
 * @ingroup streaming
 */
typedef struct uvc_stream_ctrl {
	uint16_t bmHint;
	uint8_t bFormatIndex;
	uint8_t bFrameIndex;
	uint32_t dwFrameInterval;
	uint16_t wKeyFrameRate;
	uint16_t wPFrameRate;
	uint16_t wCompQuality;
	uint16_t wCompWindowSize;
	uint16_t wDelay;
	uint32_t dwMaxVideoFrameSize;
	uint32_t dwMaxPayloadTransferSize;
	/** XXX add UVC 1.1 parameters */
	uint32_t dwClockFrequency;
	uint8_t bmFramingInfo; //
	uint8_t bPreferedVersion;
	uint8_t bMinVersion;
	uint8_t bMaxVersion;
	/** XXX add UVC 1.5 parameters */
	uint8_t bUsage;
	uint8_t bBitDepthLuma;
	uint8_t bmSettings;
	uint8_t bMaxNumberOfRefFramesPlus1;
	uint16_t bmRateControlModes;
	uint64_t bmLayoutPerStream;
	//
	uint8_t bInterfaceNumber;
} uvc_stream_ctrl_t;

uvc_error_t uvc_init(uvc_context_t **ctx, struct libusb_context *usb_ctx);
uvc_error_t uvc_init2(uvc_context_t **ctx, struct libusb_context *usb_ctx, const char *usbfs);
void uvc_exit(uvc_context_t *ctx);

uvc_error_t uvc_get_device_list(uvc_context_t *ctx, uvc_device_t ***list);
void uvc_free_device_list(uvc_device_t **list, uint8_t unref_devices);

uvc_error_t uvc_get_device_descriptor(uvc_device_t *dev,
		uvc_device_descriptor_t **desc);
void uvc_free_device_descriptor(uvc_device_descriptor_t *desc);

uint8_t uvc_get_bus_number(uvc_device_t *dev);
uint8_t uvc_get_device_address(uvc_device_t *dev);

uvc_error_t uvc_find_device(uvc_context_t *ctx, uvc_device_t **dev, int vid,
		int pid, const char *sn);

uvc_error_t uvc_find_device2(uvc_context_t *ctx, uvc_device_t **dev, int vid,
		int pid, const char *sn, int fd);
// XXX
uvc_error_t uvc_get_device_with_fd(uvc_context_t *ctx, uvc_device_t **device,
		int vid, int pid, const char *serial, int fd, int busnum, int devaddr);

uvc_error_t uvc_open(uvc_device_t *dev, uvc_device_handle_t **devh);
void uvc_close(uvc_device_handle_t *devh);
// XXX
uvc_error_t uvc_set_reset_altsetting(uvc_device_handle_t *devh, uint8_t reset_on_release_if);

uvc_device_t *uvc_get_device(uvc_device_handle_t *devh);
libusb_device_handle *uvc_get_libusb_handle(uvc_device_handle_t *devh);

void uvc_ref_device(uvc_device_t *dev);
void uvc_unref_device(uvc_device_t *dev);

void uvc_set_status_callback(uvc_device_handle_t *devh,
		uvc_status_callback_t cb, void *user_ptr);

void uvc_set_button_callback(uvc_device_handle_t *devh,
                             uvc_button_callback_t cb,
                             void *user_ptr);

const uvc_input_terminal_t *uvc_get_input_terminals(uvc_device_handle_t *devh);
const uvc_output_terminal_t *uvc_get_output_terminals(uvc_device_handle_t *devh);
const uvc_processing_unit_t *uvc_get_processing_units(uvc_device_handle_t *devh);
const uvc_extension_unit_t *uvc_get_extension_units(uvc_device_handle_t *devh);

uvc_error_t uvc_get_stream_ctrl_format_size(uvc_device_handle_t *devh,
		uvc_stream_ctrl_t *ctrl, enum uvc_frame_format format, int width,
		int height, int fps);
uvc_error_t uvc_get_stream_ctrl_format_size_fps(uvc_device_handle_t *devh,
		uvc_stream_ctrl_t *ctrl, enum uvc_frame_format cf, int width,
		int height, int min_fps, int max_fps);	// XXX added

uvc_error_t uvc_probe_stream_ctrl(uvc_device_handle_t *devh,
		uvc_stream_ctrl_t *ctrl);

uvc_error_t uvc_get_frame_desc(uvc_device_handle_t *devh,
		uvc_stream_ctrl_t *ctrl, uvc_frame_desc_t **desc);

uvc_error_t uvc_start_streaming(uvc_device_handle_t *devh,
		uvc_stream_ctrl_t *ctrl, uvc_frame_callback_t *cb, void *user_ptr,
		uint8_t flags);
uvc_error_t uvc_start_streaming_bandwidth(uvc_device_handle_t *devh,
		uvc_stream_ctrl_t *ctrl, uvc_frame_callback_t *cb, void *user_ptr,
		float bandwidth,
		uint8_t flags);	// XXX added saki

uvc_error_t uvc_start_iso_streaming(uvc_device_handle_t *devh,
		uvc_stream_ctrl_t *ctrl, uvc_frame_callback_t *cb, void *user_ptr);

void uvc_stop_streaming(uvc_device_handle_t *devh);

uvc_error_t uvc_stream_open_ctrl(uvc_device_handle_t *devh,
		uvc_stream_handle_t **strmh, uvc_stream_ctrl_t *ctrl);
uvc_error_t uvc_stream_ctrl(uvc_stream_handle_t *strmh,
		uvc_stream_ctrl_t *ctrl);
uvc_error_t uvc_stream_start(uvc_stream_handle_t *strmh,
		uvc_frame_callback_t *cb, void *user_ptr, uint8_t flags);
uvc_error_t uvc_stream_start_bandwidth(uvc_stream_handle_t *strmh,
		uvc_frame_callback_t *cb, void *user_ptr, float bandwidth, uint8_t flags);	// XXX added saki
uvc_error_t uvc_stream_start_iso(uvc_stream_handle_t *strmh,
		uvc_frame_callback_t *cb, void *user_ptr);
uvc_error_t uvc_stream_get_frame(uvc_stream_handle_t *strmh,
		uvc_frame_t **frame, int32_t timeout_us);
uvc_error_t uvc_stream_stop(uvc_stream_handle_t *strmh);
void uvc_stream_close(uvc_stream_handle_t *strmh);

// Generic Controls 通用控制
int uvc_get_ctrl_len(uvc_device_handle_t *devh, uint8_t unit, uint8_t ctrl);
int uvc_get_ctrl(uvc_device_handle_t *devh, uint8_t unit, uint8_t ctrl,
		void *data, int len, enum uvc_req_code req_code);
int uvc_set_ctrl(uvc_device_handle_t *devh, uint8_t unit, uint8_t ctrl,
		void *data, int len);

// Camera Controls 相机控制
uvc_error_t uvc_vc_get_error_code(uvc_device_handle_t *devh,
		uvc_vc_error_code_control_t *error_code, enum uvc_req_code req_code);	// XXX added saki
uvc_error_t uvc_vs_get_error_code(uvc_device_handle_t *devh,
		uvc_vs_error_code_control_t *error_code, enum uvc_req_code req_code);	// XXX added saki
uvc_error_t uvc_get_power_mode(uvc_device_handle_t *devh,
		enum uvc_device_power_mode *mode, enum uvc_req_code req_code);
uvc_error_t uvc_set_power_mode(uvc_device_handle_t *devh,
		enum uvc_device_power_mode mode);
//----------------------------------------------------------------------
uvc_error_t uvc_get_scanning_mode(uvc_device_handle_t *devh, uint8_t *mode,
		enum uvc_req_code req_code);
uvc_error_t uvc_set_scanning_mode(uvc_device_handle_t *devh, uint8_t mode);
//----------------------------------------------------------------------
uvc_error_t uvc_get_ae_mode(uvc_device_handle_t *devh, uint8_t *mode,
		enum uvc_req_code req_code);
uvc_error_t uvc_set_ae_mode(uvc_device_handle_t *devh, uint8_t mode);
//----------------------------------------------------------------------
uvc_error_t uvc_get_ae_priority(uvc_device_handle_t *devh, uint8_t *priority,
		enum uvc_req_code req_code);
uvc_error_t uvc_set_ae_priority(uvc_device_handle_t *devh, uint8_t priority);
//----------------------------------------------------------------------
uvc_error_t uvc_get_exposure_abs(uvc_device_handle_t *devh, int *time,
		enum uvc_req_code req_code);
uvc_error_t uvc_set_exposure_abs(uvc_device_handle_t *devh, int time);
//----------------------------------------------------------------------
uvc_error_t uvc_get_exposure_rel(uvc_device_handle_t *devh, int *step,
		enum uvc_req_code req_code);
uvc_error_t uvc_set_exposure_rel(uvc_device_handle_t *devh, int step);
//----------------------------------------------------------------------
uvc_error_t uvc_get_focus_abs(uvc_device_handle_t *devh, short *focus,
		enum uvc_req_code req_code);
uvc_error_t uvc_set_focus_abs(uvc_device_handle_t *devh, short focus);
//----------------------------------------------------------------------
uvc_error_t uvc_get_focus_rel(uvc_device_handle_t *devh, int8_t *focus, uint8_t *speed,
		enum uvc_req_code req_code);
uvc_error_t uvc_set_focus_rel(uvc_device_handle_t *devh, int8_t focus, uint8_t speed);
//----------------------------------------------------------------------
uvc_error_t uvc_get_focus_simple_range(uvc_device_handle_t *devh, uint8_t* focus,
		enum uvc_req_code req_code);
uvc_error_t uvc_set_focus_simple_range(uvc_device_handle_t *devh, uint8_t focus);
//----------------------------------------------------------------------
uvc_error_t uvc_get_focus_auto(uvc_device_handle_t *devh, uint8_t *autofocus,
		enum uvc_req_code req_code);
uvc_error_t uvc_set_focus_auto(uvc_device_handle_t *devh, uint8_t autofocus);
//----------------------------------------------------------------------
uvc_error_t uvc_get_iris_abs(uvc_device_handle_t *devh, uint16_t *iris,
		enum uvc_req_code req_code);
uvc_error_t uvc_set_iris_abs(uvc_device_handle_t *devh, uint16_t iris);
//----------------------------------------------------------------------
uvc_error_t uvc_get_iris_rel(uvc_device_handle_t *devh, uint8_t *iris,
		enum uvc_req_code req_code);
uvc_error_t uvc_set_iris_rel(uvc_device_handle_t *devh, uint8_t iris);
//----------------------------------------------------------------------
uvc_error_t uvc_get_zoom_abs(uvc_device_handle_t *devh, uint16_t *zoom,
		enum uvc_req_code req_code);
uvc_error_t uvc_set_zoom_abs(uvc_device_handle_t *devh, uint16_t zoom);
//----------------------------------------------------------------------
uvc_error_t uvc_get_zoom_rel(uvc_device_handle_t *devh, int8_t *zoom, uint8_t *isdigital, uint8_t *speed,
		enum uvc_req_code req_code);
uvc_error_t uvc_set_zoom_rel(uvc_device_handle_t *devh, int8_t zoom, uint8_t isdigital, uint8_t speed);
//----------------------------------------------------------------------
uvc_error_t uvc_get_pantilt_abs(uvc_device_handle_t *devh, int32_t* pan, int32_t* tilt, enum uvc_req_code req_code);
uvc_error_t uvc_set_pantilt_abs(uvc_device_handle_t *devh, int32_t pan, int32_t tilt);
uvc_error_t uvc_get_pantilt_rel(uvc_device_handle_t *devh, int8_t* pan_rel, uint8_t* pan_speed, int8_t* tilt_rel, uint8_t* tilt_speed, enum uvc_req_code req_code);
uvc_error_t uvc_set_pantilt_rel(uvc_device_handle_t *devh, int8_t pan_rel, uint8_t pan_speed, int8_t tilt_rel, uint8_t tilt_speed);
//----------------------------------------------------------------------
uvc_error_t uvc_get_roll_abs(uvc_device_handle_t *devh, int16_t* roll, enum uvc_req_code req_code);
uvc_error_t uvc_set_roll_abs(uvc_device_handle_t *devh, int16_t roll);
uvc_error_t uvc_get_roll_rel(uvc_device_handle_t *devh, int8_t* roll_rel, uint8_t* speed, enum uvc_req_code req_code);
uvc_error_t uvc_set_roll_rel(uvc_device_handle_t *devh, int8_t roll_rel, uint8_t speed);
//----------------------------------------------------------------------
uvc_error_t uvc_get_privacy(uvc_device_handle_t *devh, uint8_t* privacy, enum uvc_req_code req_code);
uvc_error_t uvc_set_privacy(uvc_device_handle_t *devh, uint8_t privacy);
//----------------------------------------------------------------------
uvc_error_t uvc_get_digital_window(uvc_device_handle_t *devh, uint16_t* window_top, uint16_t* window_left, uint16_t* window_bottom, uint16_t* window_right, uint16_t* num_steps, uint16_t* num_steps_units, enum uvc_req_code req_code);
uvc_error_t uvc_set_digital_window(uvc_device_handle_t *devh, uint16_t window_top, uint16_t window_left, uint16_t window_bottom, uint16_t window_right, uint16_t num_steps, uint16_t num_steps_units);
//----------------------------------------------------------------------
uvc_error_t uvc_get_digital_roi(uvc_device_handle_t *devh, uint16_t* roi_top, uint16_t* roi_left, uint16_t* roi_bottom, uint16_t* roi_right, uint16_t* auto_controls, enum uvc_req_code req_code);
uvc_error_t uvc_set_digital_roi(uvc_device_handle_t *devh, uint16_t roi_top, uint16_t roi_left, uint16_t roi_bottom, uint16_t roi_right, uint16_t auto_controls);

//----------------------------------------------------------------------
// Processing Unit Controls 处理单元控制
uvc_error_t uvc_get_backlight_compensation(uvc_device_handle_t *devh, int16_t *comp,
		enum uvc_req_code req_code);
uvc_error_t uvc_set_backlight_compensation(uvc_device_handle_t *devh, int16_t comp);
//----------------------------------------------------------------------
uvc_error_t uvc_get_brightness(uvc_device_handle_t *devh, int16_t *brightness,
		enum uvc_req_code req_code);
uvc_error_t uvc_set_brightness(uvc_device_handle_t *devh, int16_t brightness);
//----------------------------------------------------------------------
uvc_error_t uvc_get_contrast(uvc_device_handle_t *devh, uint16_t *contrast,
		enum uvc_req_code req_code);
uvc_error_t uvc_set_contrast(uvc_device_handle_t *devh, uint16_t contrast);
//----------------------------------------------------------------------
uvc_error_t uvc_get_contrast_auto(uvc_device_handle_t *devh, uint8_t *autoContrast,
		enum uvc_req_code req_code);
uvc_error_t uvc_set_contrast_auto(uvc_device_handle_t *devh, uint8_t autoContrast);
//----------------------------------------------------------------------
uvc_error_t uvc_get_gain(uvc_device_handle_t *devh, uint16_t *gain,
		enum uvc_req_code req_code);
uvc_error_t uvc_set_gain(uvc_device_handle_t *devh, uint16_t gain);
//----------------------------------------------------------------------
uvc_error_t uvc_get_powerline_freqency(uvc_device_handle_t *devh, uint8_t *power_line_frequency,
		enum uvc_req_code req_code);
uvc_error_t uvc_set_powerline_freqency(uvc_device_handle_t *devh, uint8_t power_line_frequency);
//----------------------------------------------------------------------
uvc_error_t uvc_get_hue(uvc_device_handle_t *devh, int16_t *hue,
		enum uvc_req_code req_code);
uvc_error_t uvc_set_hue(uvc_device_handle_t *devh, int16_t hue);
//----------------------------------------------------------------------
uvc_error_t uvc_get_hue_auto(uvc_device_handle_t *devh, uint8_t *autoHue,
		enum uvc_req_code req_code);
uvc_error_t uvc_set_hue_auto(uvc_device_handle_t *devh, uint8_t autoHue);
//----------------------------------------------------------------------
uvc_error_t uvc_get_saturation(uvc_device_handle_t *devh, uint16_t *saturation,
		enum uvc_req_code req_code);
uvc_error_t uvc_set_saturation(uvc_device_handle_t *devh, uint16_t saturation);
//----------------------------------------------------------------------
uvc_error_t uvc_get_sharpness(uvc_device_handle_t *devh, uint16_t *sharpness,
		enum uvc_req_code req_code);
uvc_error_t uvc_set_sharpness(uvc_device_handle_t *devh, uint16_t sharpness);
//----------------------------------------------------------------------
uvc_error_t uvc_get_gamma(uvc_device_handle_t *devh, uint16_t *gamma,
		enum uvc_req_code req_code);
uvc_error_t uvc_set_gamma(uvc_device_handle_t *devh, uint16_t gamma);
//----------------------------------------------------------------------
uvc_error_t uvc_get_white_balance_temperature(uvc_device_handle_t *devh, uint16_t *wb_temperature,
		enum uvc_req_code req_code);
uvc_error_t uvc_set_white_balance_temperature(uvc_device_handle_t *devh, uint16_t wb_temperature);
//----------------------------------------------------------------------
uvc_error_t uvc_get_white_balance_temperature_auto(uvc_device_handle_t *devh, uint8_t *autoWbTemp,
		enum uvc_req_code req_code);
uvc_error_t uvc_set_white_balance_temperature_auto(uvc_device_handle_t *devh, uint8_t autoWbTemp);
//----------------------------------------------------------------------
uvc_error_t uvc_get_white_balance_component(uvc_device_handle_t *devh, uint32_t *wb_compo,
		enum uvc_req_code req_code);
uvc_error_t uvc_set_white_balance_component(uvc_device_handle_t *devh, uint32_t wb_compo);
//----------------------------------------------------------------------
uvc_error_t uvc_get_white_balance_component_auto(uvc_device_handle_t *devh, uint8_t *autoWbCompo,
		enum uvc_req_code req_code);
uvc_error_t uvc_set_white_balance_component_auto(uvc_device_handle_t *devh, uint8_t autoWbCompo);
//----------------------------------------------------------------------
uvc_error_t uvc_get_digital_multiplier(uvc_device_handle_t *devh, uint16_t *multiplier,
		enum uvc_req_code req_code);
uvc_error_t uvc_set_digital_multiplier(uvc_device_handle_t *devh, uint16_t multiplier);
//----------------------------------------------------------------------
uvc_error_t uvc_get_digital_multiplier_limit(uvc_device_handle_t *devh, uint16_t *limit,
		enum uvc_req_code req_code);
uvc_error_t uvc_set_digital_multiplier_limit(uvc_device_handle_t *devh, uint16_t limit);
//----------------------------------------------------------------------
uvc_error_t uvc_get_analog_video_standard(uvc_device_handle_t *devh, uint8_t *standard,
		enum uvc_req_code req_code);
uvc_error_t uvc_set_analog_video_standard(uvc_device_handle_t *devh, uint8_t standard);
//----------------------------------------------------------------------
uvc_error_t uvc_get_analog_video_lockstate(uvc_device_handle_t *devh, uint8_t* state, enum uvc_req_code req_code);
uvc_error_t uvc_set_analog_video_lockstate(uvc_device_handle_t *devh, uint8_t status);
//----------------------------------------------------------------------
uvc_error_t uvc_get_input_select(uvc_device_handle_t *devh, uint8_t* selector, enum uvc_req_code req_code);
uvc_error_t uvc_set_input_select(uvc_device_handle_t *devh, uint8_t selector);
//----------------------------------------------------------------------

void uvc_perror(uvc_error_t err, const char *msg);
const char* uvc_strerror(uvc_error_t err);
void uvc_print_diag(uvc_device_handle_t *devh, FILE *stream);
void uvc_print_stream_ctrl(uvc_stream_ctrl_t *ctrl, FILE *stream);

uvc_frame_t *uvc_allocate_frame(size_t data_bytes);
void uvc_free_frame(uvc_frame_t *frame);

uvc_error_t uvc_duplicate_frame(uvc_frame_t *in, uvc_frame_t *out);
//----------------------------------------------------------------------
uvc_error_t uvc_yuyv2rgb(uvc_frame_t *in, uvc_frame_t *out);
uvc_error_t uvc_uyvy2rgb(uvc_frame_t *in, uvc_frame_t *out);
uvc_error_t uvc_any2rgb(uvc_frame_t *in, uvc_frame_t *out);
//----------------------------------------------------------------------
uvc_error_t uvc_yuyv2bgr(uvc_frame_t *in, uvc_frame_t *out);
uvc_error_t uvc_uyvy2bgr(uvc_frame_t *in, uvc_frame_t *out);
uvc_error_t uvc_any2bgr(uvc_frame_t *in, uvc_frame_t *out);

#ifdef LIBUVC_HAS_JPEG
// 将MJPEG转为rgb
uvc_error_t uvc_mjpeg2rgb(uvc_frame_t *in, uvc_frame_t *out);
// 将MJPEG转为bgr
uvc_error_t uvc_mjpeg2bgr(uvc_frame_t *in, uvc_frame_t *out);		// XXX
// 将MJPEG转为bgr565
uvc_error_t uvc_mjpeg2rgb565(uvc_frame_t *in, uvc_frame_t *out);	// XXX
// 将MJPEG转为rgbx8888
uvc_error_t uvc_mjpeg2rgbx(uvc_frame_t *in, uvc_frame_t *out);		// XXX
// 将MJPEG转为YUYV
uvc_error_t uvc_mjpeg2yuyv(uvc_frame_t *in, uvc_frame_t *out);		// XXX
#endif

uvc_error_t uvc_yuyv2rgb565(uvc_frame_t *in, uvc_frame_t *out);		// XXX
uvc_error_t uvc_uyvy2rgb565(uvc_frame_t *in, uvc_frame_t *out);		// XXX
uvc_error_t uvc_rgb2rgb565(uvc_frame_t *in, uvc_frame_t *out);		// XXX
uvc_error_t uvc_any2rgb565(uvc_frame_t *in, uvc_frame_t *out);		// XXX

uvc_error_t uvc_yuyv2rgbx(uvc_frame_t *in, uvc_frame_t *out);		// XXX
uvc_error_t uvc_uyvy2rgbx(uvc_frame_t *in, uvc_frame_t *out);		// XXX
uvc_error_t uvc_rgb2rgbx(uvc_frame_t *in, uvc_frame_t *out);		// XXX
uvc_error_t uvc_any2rgbx(uvc_frame_t *in, uvc_frame_t *out);		// XXX

uvc_error_t uvc_yuyv2yuv420P(uvc_frame_t *in, uvc_frame_t *out);	// XXX
uvc_error_t uvc_yuyv2yuv420SP(uvc_frame_t *in, uvc_frame_t *out);	// XXX
uvc_error_t uvc_any2yuv420SP(uvc_frame_t *in, uvc_frame_t *out);	// XXX

uvc_error_t uvc_yuyv2iyuv420SP(uvc_frame_t *in, uvc_frame_t *out);	// XXX
uvc_error_t uvc_yuyv2iyuv420SP(uvc_frame_t *in, uvc_frame_t *out);	// XXX
uvc_error_t uvc_any2iyuv420SP(uvc_frame_t *in, uvc_frame_t *out);	// XXX

uvc_error_t uvc_any2yuyv(uvc_frame_t *in, uvc_frame_t *out);		// XXX

uvc_error_t uvc_ensure_frame_size(uvc_frame_t *frame, size_t need_bytes); // XXX

//**********************************************************************
// added for diagnostic 添加的诊断
// t_saki@serenegiant.com
void uvc_print_format_desc_one(uvc_format_desc_t *format_descriptors, FILE *stream);
void uvc_print_format_desc(uvc_format_desc_t *format_descriptors, FILE *stream);
void uvc_print_device_desc(uvc_device_handle_t *devh, FILE *stream);
void uvc_print_configuration_desc(uvc_device_handle_t *devh, FILE *stream);
void uvc_print_interface_desc(
	const struct libusb_interface *interface, const int num_interface,
	const char *prefix, FILE *stream);
void uvc_print_endpoint_desc(
	const struct libusb_endpoint_descriptor *endpoint, const int num_endpoint,
	const char *prefix, FILE *stream);

#define uvc_print_format_descriptor_one uvc_print_format_desc_one
#define uvc_print_format_descriptor uvc_print_format_desc
#define uvc_print_device_descriptor uvc_print_device_desc
#define uvc_print_configuration_descriptor uvc_print_configuration_desc
#define uvc_print_interface_descriptor uvc_print_interface_desc
#define uvc_print_endpoint_descriptor uvc_print_endpoint_desc;

#ifdef __cplusplus
}

#endif

#endif // !def(LIBUVC_H)

