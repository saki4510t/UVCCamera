#ifndef LIBUVC_H
#define LIBUVC_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h> // FILE
#include <libusb-1.0/libusb.h>
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
  UVC_FRAME_FORMAT_UNCOMPRESSED,
  UVC_FRAME_FORMAT_COMPRESSED,
  /**
   * YUYV/YUV2/YUV422: YUV encoding with one luminance value per pixel and
   * one UV (chrominance) pair for every two pixels.
   * YUYV/YUV2/YUV422: YUV编码，每个像素一个亮度值，每两个像素一对UV（色度）对。
   */
  UVC_FRAME_FORMAT_YUYV,
  UVC_FRAME_FORMAT_UYVY,
  /** 24-bit RGB */
  UVC_FRAME_FORMAT_RGB,
  UVC_FRAME_FORMAT_BGR,
  /**
   * Motion-JPEG (or JPEG) encoded images
   * Motion-JPEG（或JPEG）编码的图像
   */
  UVC_FRAME_FORMAT_MJPEG,
  UVC_FRAME_FORMAT_GRAY8,
  /**
   * Number of formats understood
   * 了解的格式数
   */
  UVC_FRAME_FORMAT_COUNT,
};

/* UVC_COLOR_FORMAT_* have been replaced with UVC_FRAME_FORMAT_*. Please use
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
#define UVC_COLOR_FORMAT_MJPEG UVC_FRAME_FORMAT_MJPEG
#define UVC_COLOR_FORMAT_GRAY8 UVC_FRAME_FORMAT_GRAY8

/**
 * UVC request code (A.8)
 * UVC 请求码
 */
enum uvc_req_code {
  UVC_RC_UNDEFINED = 0x00,
  UVC_SET_CUR = 0x01,
  UVC_GET_CUR = 0x81,
  UVC_GET_MIN = 0x82,
  UVC_GET_MAX = 0x83,
  UVC_GET_RES = 0x84,
  UVC_GET_LEN = 0x85,
  UVC_GET_INFO = 0x86,
  UVC_GET_DEF = 0x87
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
  UVC_CT_PRIVACY_CONTROL = 0x11
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
  UVC_PU_ANALOG_LOCK_STATUS_CONTROL = 0x12
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
} uvc_input_terminal_t;

typedef struct uvc_output_terminal {
  struct uvc_output_terminal *prev, *next;
  /** @todo */
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
typedef void(uvc_status_callback_t)(enum uvc_status_class status_class,
                                    int event,
                                    int selector,
                                    enum uvc_status_attribute status_attribute,
                                    void *data, size_t data_len,
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
   * 图像数据缓冲区的大小
   */
  size_t data_bytes;
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
   * 每水平线的字节数（对于压缩格式未定义）
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
  /** @todo add UVC 1.1 parameters */
  uint8_t bInterfaceNumber;
} uvc_stream_ctrl_t;

uvc_error_t uvc_init(uvc_context_t **ctx, struct libusb_context *usb_ctx);
void uvc_exit(uvc_context_t *ctx);

uvc_error_t uvc_get_device_list(
    uvc_context_t *ctx,
    uvc_device_t ***list);
void uvc_free_device_list(uvc_device_t **list, uint8_t unref_devices);

uvc_error_t uvc_get_device_descriptor(
    uvc_device_t *dev,
    uvc_device_descriptor_t **desc);
void uvc_free_device_descriptor(
    uvc_device_descriptor_t *desc);

uint8_t uvc_get_bus_number(uvc_device_t *dev);
uint8_t uvc_get_device_address(uvc_device_t *dev);

uvc_error_t uvc_find_device(
    uvc_context_t *ctx,
    uvc_device_t **dev,
    int vid, int pid, const char *sn);

uvc_error_t uvc_open(
    uvc_device_t *dev,
    uvc_device_handle_t **devh);
void uvc_close(uvc_device_handle_t *devh);

uvc_device_t *uvc_get_device(uvc_device_handle_t *devh);
libusb_device_handle *uvc_get_libusb_handle(uvc_device_handle_t *devh);

void uvc_ref_device(uvc_device_t *dev);
void uvc_unref_device(uvc_device_t *dev);

void uvc_set_status_callback(uvc_device_handle_t *devh,
                             uvc_status_callback_t cb,
                             void *user_ptr);

const uvc_input_terminal_t *uvc_get_input_terminals(uvc_device_handle_t *devh);
const uvc_output_terminal_t *uvc_get_output_terminals(uvc_device_handle_t *devh);
const uvc_processing_unit_t *uvc_get_processing_units(uvc_device_handle_t *devh);
const uvc_extension_unit_t *uvc_get_extension_units(uvc_device_handle_t *devh);

uvc_error_t uvc_get_stream_ctrl_format_size(
    uvc_device_handle_t *devh,
    uvc_stream_ctrl_t *ctrl,
    enum uvc_frame_format format,
    int width, int height,
    int fps
    );

uvc_error_t uvc_probe_stream_ctrl(
    uvc_device_handle_t *devh,
    uvc_stream_ctrl_t *ctrl);

uvc_error_t uvc_start_streaming(
    uvc_device_handle_t *devh,
    uvc_stream_ctrl_t *ctrl,
    uvc_frame_callback_t *cb,
    void *user_ptr,
    uint8_t isochronous);

uvc_error_t uvc_start_iso_streaming(
    uvc_device_handle_t *devh,
    uvc_stream_ctrl_t *ctrl,
    uvc_frame_callback_t *cb,
    void *user_ptr);

void uvc_stop_streaming(uvc_device_handle_t *devh);

uvc_error_t uvc_stream_open_ctrl(uvc_device_handle_t *devh, uvc_stream_handle_t **strmh, uvc_stream_ctrl_t *ctrl);
uvc_error_t uvc_stream_ctrl(uvc_stream_handle_t *strmh, uvc_stream_ctrl_t *ctrl);
uvc_error_t uvc_stream_start(uvc_stream_handle_t *strmh,
    uvc_frame_callback_t *cb,
    void *user_ptr,
    uint8_t isochronous);
uvc_error_t uvc_stream_start_iso(uvc_stream_handle_t *strmh,
    uvc_frame_callback_t *cb,
    void *user_ptr);
uvc_error_t uvc_stream_get_frame(
    uvc_stream_handle_t *strmh,
    uvc_frame_t **frame,
    int32_t timeout_us
);
uvc_error_t uvc_stream_stop(uvc_stream_handle_t *strmh);
void uvc_stream_close(uvc_stream_handle_t *strmh);

uvc_error_t uvc_get_power_mode(uvc_device_handle_t *devh, enum uvc_device_power_mode *mode, enum uvc_req_code req_code);
uvc_error_t uvc_set_power_mode(uvc_device_handle_t *devh, enum uvc_device_power_mode mode);
uvc_error_t uvc_get_ae_mode(uvc_device_handle_t *devh, int *mode, enum uvc_req_code req_code);
uvc_error_t uvc_set_ae_mode(uvc_device_handle_t *devh, int mode);
uvc_error_t uvc_get_ae_priority(uvc_device_handle_t *devh, uint8_t *priority, enum uvc_req_code req_code);
uvc_error_t uvc_set_ae_priority(uvc_device_handle_t *devh, uint8_t priority);
uvc_error_t uvc_get_exposure_abs(uvc_device_handle_t *devh, int *time, enum uvc_req_code req_code);
uvc_error_t uvc_set_exposure_abs(uvc_device_handle_t *devh, int time);
uvc_error_t uvc_get_exposure_rel(uvc_device_handle_t *devh, int *step, enum uvc_req_code req_code);
uvc_error_t uvc_set_exposure_rel(uvc_device_handle_t *devh, int step);
uvc_error_t uvc_get_scanning_mode(uvc_device_handle_t *devh, int *step, enum uvc_req_code req_code);
uvc_error_t uvc_set_scanning_mode(uvc_device_handle_t *devh, int mode);
uvc_error_t uvc_get_focus_abs(uvc_device_handle_t *devh, short *focus, enum uvc_req_code req_code);
uvc_error_t uvc_set_focus_abs(uvc_device_handle_t *devh, short focus);
uvc_error_t uvc_get_pantilt_abs(uvc_device_handle_t *devh, int *pan, int *tilt, enum uvc_req_code req_code);
uvc_error_t uvc_set_pantilt_abs(uvc_device_handle_t *devh, int pan, int tilt);

int uvc_get_ctrl_len(uvc_device_handle_t *devh, uint8_t unit, uint8_t ctrl);
int uvc_get_ctrl(uvc_device_handle_t *devh, uint8_t unit, uint8_t ctrl, void *data, int len, enum uvc_req_code req_code);
int uvc_set_ctrl(uvc_device_handle_t *devh, uint8_t unit, uint8_t ctrl, void *data, int len);

void uvc_perror(uvc_error_t err, const char *msg);
const char* uvc_strerror(uvc_error_t err);
void uvc_print_diag(uvc_device_handle_t *devh, FILE *stream);
void uvc_print_stream_ctrl(uvc_stream_ctrl_t *ctrl, FILE *stream);

uvc_frame_t *uvc_allocate_frame(size_t data_bytes);
void uvc_free_frame(uvc_frame_t *frame);

uvc_error_t uvc_duplicate_frame(uvc_frame_t *in, uvc_frame_t *out);

uvc_error_t uvc_yuyv2rgb(uvc_frame_t *in, uvc_frame_t *out);
uvc_error_t uvc_uyvy2rgb(uvc_frame_t *in, uvc_frame_t *out);
uvc_error_t uvc_any2rgb(uvc_frame_t *in, uvc_frame_t *out);

uvc_error_t uvc_yuyv2bgr(uvc_frame_t *in, uvc_frame_t *out);
uvc_error_t uvc_uyvy2bgr(uvc_frame_t *in, uvc_frame_t *out);
uvc_error_t uvc_any2bgr(uvc_frame_t *in, uvc_frame_t *out);

#ifdef LIBUVC_HAS_JPEG
uvc_error_t uvc_mjpeg2rgb(uvc_frame_t *in, uvc_frame_t *out);
#endif

#ifdef __cplusplus
}
#endif

#endif // !def(LIBUVC_H)

