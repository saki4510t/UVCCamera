/** @file libuvc_internal.h
  * @brief Implementation-specific UVC constants and structures.
  * @cond include_hidden
  */
#ifndef LIBUVC_INTERNAL_H
#define LIBUVC_INTERNAL_H

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <signal.h>
#include "utlist.h"

/**
 * Converts an unaligned four-byte little-endian integer into an int32
 * 将未对齐的四字节小端整数转换为int32
 */
#define DW_TO_INT(p) ((p)[0] | ((p)[1] << 8) | ((p)[2] << 16) | ((p)[3] << 24))
/**
 * Converts an unaligned two-byte little-endian integer into an int16
 * 将未对齐的双字节小端整数转换为int16
 */
#define SW_TO_SHORT(p) ((p)[0] | ((p)[1] << 8))
/**
 * Converts an int16 into an unaligned two-byte little-endian integer
 * 将int16转换为未对齐的双字节小端整数
 */
#define SHORT_TO_SW(s, p) \
  (p)[0] = (s); \
  (p)[1] = (s) >> 8;
/**
 * Converts an int32 into an unaligned four-byte little-endian integer
 * 将int32转换为未对齐的四字节小端整数
 */
#define INT_TO_DW(i, p) \
  (p)[0] = (i); \
  (p)[1] = (i) >> 8; \
  (p)[2] = (i) >> 16; \
  (p)[3] = (i) >> 24;

/**
 * Selects the nth item in a doubly linked list. n=-1 selects the last item.
 * 选择双链表中的第n项。n=-1选择最后一项。
 */
#define DL_NTH(head, out, n) \
  do { \
    int dl_nth_i = 0; \
    LDECLTYPE(head) dl_nth_p = (head); \
    if ((n) < 0) { \
      while (dl_nth_p && dl_nth_i > (n)) { \
        dl_nth_p = dl_nth_p->prev; \
        dl_nth_i--; \
      } \
    } else { \
      while (dl_nth_p && dl_nth_i < (n)) { \
        dl_nth_p = dl_nth_p->next; \
        dl_nth_i++; \
      } \
    } \
    (out) = dl_nth_p; \
  } while (0);

#ifdef UVC_DEBUGGING
#include <libgen.h>
#define UVC_DEBUG(format, ...) fprintf(stderr, "[%s:%d/%s] " format "\n", basename(__FILE__), __LINE__, __FUNCTION__, ##__VA_ARGS__)
#define UVC_ENTER() fprintf(stderr, "[%s:%d] begin %s\n", basename(__FILE__), __LINE__, __FUNCTION__)
#define UVC_EXIT(code) fprintf(stderr, "[%s:%d] end %s (%d)\n", basename(__FILE__), __LINE__, __FUNCTION__, code)
#define UVC_EXIT_VOID() fprintf(stderr, "[%s:%d] end %s\n", basename(__FILE__), __LINE__, __FUNCTION__)
#else
#define UVC_DEBUG(format, ...)
#define UVC_ENTER()
#define UVC_EXIT_VOID()
#define UVC_EXIT(code)
#endif

/* http://stackoverflow.com/questions/19452971/array-size-macro-that-rejects-pointers */
#define IS_INDEXABLE(arg) (sizeof(arg[0]))
#define IS_ARRAY(arg) (IS_INDEXABLE(arg) && (((void *) &arg) == ((void *) arg)))
#define ARRAYSIZE(arr) (sizeof(arr) / (IS_ARRAY(arr) ? sizeof(arr[0]) : 0))

/**
 * Video interface subclass code (A.2)
 * 视频接口子类代码(A.2)
 */
enum uvc_int_subclass_code {
  UVC_SC_UNDEFINED = 0x00,
  UVC_SC_VIDEOCONTROL = 0x01,
  UVC_SC_VIDEOSTREAMING = 0x02,
  UVC_SC_VIDEO_INTERFACE_COLLECTION = 0x03
};

/**
 * Video interface protocol code (A.3)
 * 视频接口协议代码(A.3)
 */
enum uvc_int_proto_code {
  UVC_PC_PROTOCOL_UNDEFINED = 0x00
};

/**
 * VideoControl interface descriptor subtype (A.5)
 * 视频控制接口描述符子类型(A.5)
 */
enum uvc_vc_desc_subtype {
  UVC_VC_DESCRIPTOR_UNDEFINED = 0x00,
  UVC_VC_HEADER = 0x01,
  UVC_VC_INPUT_TERMINAL = 0x02,
  UVC_VC_OUTPUT_TERMINAL = 0x03,
  UVC_VC_SELECTOR_UNIT = 0x04,
  UVC_VC_PROCESSING_UNIT = 0x05,
  UVC_VC_EXTENSION_UNIT = 0x06
};

/**
 * VideoStreaming interface descriptor subtype (A.6)
 * 视频流接口描述符子类型(A.6)
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

/**
 * UVC endpoint descriptor subtype (A.7)
 * 端点描述子类型(A.7)
 */
enum uvc_ep_desc_subtype {
  UVC_EP_UNDEFINED = 0x00,
  UVC_EP_GENERAL = 0x01,
  UVC_EP_ENDPOINT = 0x02,
  UVC_EP_INTERRUPT = 0x03
};

/**
 * VideoControl interface control selector (A.9.1)
 * 视频控制界面控制选择器(A.9.1)
 */
enum uvc_vc_ctrl_selector {
  UVC_VC_CONTROL_UNDEFINED = 0x00,
  UVC_VC_VIDEO_POWER_MODE_CONTROL = 0x01,
  UVC_VC_REQUEST_ERROR_CODE_CONTROL = 0x02
};

/**
 * Terminal control selector (A.9.2)
 * 终端控制选择器(A.9.2)
 */
enum uvc_term_ctrl_selector {
  UVC_TE_CONTROL_UNDEFINED = 0x00
};

/**
 * Selector unit control selector (A.9.3)
 * 控制选择器(A.9.3)
 */
enum uvc_su_ctrl_selector {
  UVC_SU_CONTROL_UNDEFINED = 0x00,
  UVC_SU_INPUT_SELECT_CONTROL = 0x01
};

/**
 * Extension unit control selector (A.9.6)
 * 扩展单元控制选择器(A.9.6)
 */
enum uvc_xu_ctrl_selector {
  UVC_XU_CONTROL_UNDEFINED = 0x00
};

/**
 * VideoStreaming interface control selector (A.9.7)
 * 视频流接口控制选择器(A.9.7)
 */
enum uvc_vs_ctrl_selector {
  UVC_VS_CONTROL_UNDEFINED = 0x00,
  UVC_VS_PROBE_CONTROL = 0x01,
  UVC_VS_COMMIT_CONTROL = 0x02,
  UVC_VS_STILL_PROBE_CONTROL = 0x03,
  UVC_VS_STILL_COMMIT_CONTROL = 0x04,
  UVC_VS_STILL_IMAGE_TRIGGER_CONTROL = 0x05,
  UVC_VS_STREAM_ERROR_CODE_CONTROL = 0x06,
  UVC_VS_GENERATE_KEY_FRAME_CONTROL = 0x07,
  UVC_VS_UPDATE_FRAME_SEGMENT_CONTROL = 0x08,
  UVC_VS_SYNC_DELAY_CONTROL = 0x09
};

/**
 * Status packet type (2.4.2.2)
 * 状态包类型(2.4.2.2)
 */
enum uvc_status_type {
  UVC_STATUS_TYPE_CONTROL = 1,
  UVC_STATUS_TYPE_STREAMING = 2
};

/**
 * Payload header flags (2.4.3.3)
 * 有效负载头标志(2.4.3.3)
 */
#define UVC_STREAM_EOH (1 << 7)
#define UVC_STREAM_ERR (1 << 6)
#define UVC_STREAM_STI (1 << 5)
#define UVC_STREAM_RES (1 << 4)
#define UVC_STREAM_SCR (1 << 3)
#define UVC_STREAM_PTS (1 << 2)
#define UVC_STREAM_EOF (1 << 1)
#define UVC_STREAM_FID (1 << 0)

/**
 * Control capabilities (4.1.2)
 * 控制功能(4.1.2)
 */
#define UVC_CONTROL_CAP_GET (1 << 0)
#define UVC_CONTROL_CAP_SET (1 << 1)
#define UVC_CONTROL_CAP_DISABLED (1 << 2)
#define UVC_CONTROL_CAP_AUTOUPDATE (1 << 3)
#define UVC_CONTROL_CAP_ASYNCHRONOUS (1 << 4)

struct uvc_format_desc;
struct uvc_frame_desc;
struct uvc_streaming_interface;
struct uvc_device_info;

/**
 * Frame descriptor
 * A "frame" is a configuration of a streaming format
 * for a particular image size at one of possibly several
 * available frame rates.
 * 帧描述符
 * “帧”是在可能的几个可用帧速率中的一个的特定图像大小的流格式的配置。
 */
typedef struct uvc_frame_desc {
  struct uvc_format_desc *parent;
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
  struct uvc_streaming_interface *parent;
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
  /**
   * Available frame specifications for this format
   * 此格式的可用框架规格
   */
  struct uvc_frame_desc *frame_descs;
} uvc_format_desc_t;


/**
 * VideoStream interface
 * 视频流接口
 */
typedef struct uvc_streaming_interface {
  struct uvc_device_info *parent;
  struct uvc_streaming_interface *prev, *next;
  /**
   * Interface number
   * 接口编号
   */
  uint8_t bInterfaceNumber;
  /**
   * Video formats that this interface provides
   * 此接口提供的视频格式
   */
  struct uvc_format_desc *format_descs;
  /**
   * USB endpoint to use when communicating with this interface
   * 当与此接口通信时使用的USB端点
   */
  uint8_t bEndpointAddress;
  uint8_t bTerminalLink;
} uvc_streaming_interface_t;

/**
 * VideoControl interface
 * 视频控制接口
 */
typedef struct uvc_control_interface {
  struct uvc_device_info *parent;
  struct uvc_input_terminal *input_term_descs;
  // struct uvc_output_terminal *output_term_descs;
  struct uvc_processing_unit *processing_unit_descs;
  struct uvc_extension_unit *extension_unit_descs;
  uint16_t bcdUVC;
  uint8_t bEndpointAddress;
  /**
   * Interface number
   * 接口编号
   */
  uint8_t bInterfaceNumber;
} uvc_control_interface_t;

struct uvc_stream_ctrl;

struct uvc_device {
  struct uvc_context *ctx;
  int ref;
  libusb_device *usb_dev;
};

typedef struct uvc_device_info {
  /**
   * Configuration descriptor for USB device
   * USB设备的配置描述符
   */
  struct libusb_config_descriptor *config;
  /**
   * VideoControl interface provided by device
   * 设备提供的视频控制接口
   */
  uvc_control_interface_t ctrl_if;
  /**
   * VideoStreaming interfaces on the device
   * 设备上的视频流接口
   */
  uvc_streaming_interface_t *stream_ifs;
} uvc_device_info_t;

struct uvc_stream_handle {
  struct uvc_device_handle *devh;
  struct uvc_stream_handle *prev, *next;
  struct uvc_streaming_interface *stream_if;

  /**
   * if true, stream is running (streaming video to host)
   * 如果为真，则流正在运行(流视频到主机)
   */
  uint8_t running;
  /**
   * Current control block
   * 当前控制块
   */
  struct uvc_stream_ctrl cur_ctrl;

  /* listeners may only access hold*, and only when holding a 
   * lock on cb_mutex (probably signaled with cb_cond)
   * 监听器只能在持有 cb_mutex 的锁(可能用 cb_cond 发出信号)时访问 hold*
   */
  uint8_t fid;
  uint32_t seq, hold_seq;
  uint32_t pts, hold_pts;
  uint32_t last_scr, hold_last_scr;
  size_t got_bytes, hold_bytes;
  // outbuf 输出缓存  holdbuf持有缓存
  uint8_t *outbuf, *holdbuf;
  pthread_mutex_t cb_mutex;
  pthread_cond_t cb_cond;
  pthread_t cb_thread;
  uint32_t last_polled_seq;
  uvc_frame_callback_t *user_cb;
  void *user_ptr;
  struct libusb_transfer *transfers[5];
  uint8_t *transfer_bufs[5];
  struct uvc_frame frame;
  enum uvc_frame_format frame_format;
};

/**
 * Handle on an open UVC device
 * 打开UVC设备的句柄
 * @todo move most of this into a uvc_device struct?
 */
struct uvc_device_handle {
  struct uvc_device *dev;
  struct uvc_device_handle *prev, *next;
  /**
   * Underlying USB device handle
   * USB设备底层句柄
   */
  libusb_device_handle *usb_devh;
  struct uvc_device_info *info;
  struct libusb_transfer *status_xfer;
  uint8_t status_buf[32];
  /**
   * Function to call when we receive status updates from the camera
   * 当我们收到来自摄像头的状态更新时调用此函数
   */
  uvc_status_callback_t *status_cb;
  void *status_user_ptr;

  uvc_stream_handle_t *streams;
  /**
   * Whether the camera is an iSight that sends one header per frame
   * 相机是否为每帧发送一个标题的 iSight
   */
  uint8_t is_isight;
};

/**
 * Context within which we communicate with devices
 * 我们与设备通信的上下文
 */
struct uvc_context {
  /**
   * Underlying context for USB communication
   * USB通信的底层上下文
   */
  struct libusb_context *usb_ctx;
  /**
   * True if libuvc initialized the underlying USB context
   * 如果libuvc初始化了底层USB上下文，则为True
   */
  uint8_t own_usb_ctx;
  /**
   * List of open devices in this context
   * 此上下文中打开的设备列表
   */
  uvc_device_handle_t *open_devices;
  pthread_t handler_thread;
  uint8_t kill_handler_thread;
};

uvc_error_t uvc_query_stream_ctrl(
    uvc_device_handle_t *devh,
    uvc_stream_ctrl_t *ctrl,
    uint8_t probe,
    enum uvc_req_code req);

void uvc_start_handler_thread(uvc_context_t *ctx);
uvc_error_t uvc_claim_if(uvc_device_handle_t *devh, int idx);
uvc_error_t uvc_release_if(uvc_device_handle_t *devh, int idx);

#endif // !def(LIBUVC_INTERNAL_H)
/** @endcond */

