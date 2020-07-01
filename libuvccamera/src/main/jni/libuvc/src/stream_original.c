/*********************************************************************
* Software License Agreement (BSD License)
*
*  Copyright (C) 2010-2012 Ken Tossell
*  All rights reserved.
*
*  Redistribution and use in source and binary forms, with or without
*  modification, are permitted provided that the following conditions
*  are met:
*
*   * Redistributions of source code must retain the above copyright
*     notice, this list of conditions and the following disclaimer.
*   * Redistributions in binary form must reproduce the above
*     copyright notice, this list of conditions and the following
*     disclaimer in the documentation and/or other materials provided
*     with the distribution.
*   * Neither the name of the author nor other contributors may be
*     used to endorse or promote products derived from this software
*     without specific prior written permission.
*
*  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
*  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
*  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
*  FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
*  COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
*  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
*  BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
*  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
*  CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
*  LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
*  ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
*  POSSIBILITY OF SUCH DAMAGE.
*********************************************************************/
/**
 * @defgroup streaming Streaming control functions
 * 流式流控制功能
 */

#include "libuvc/libuvc.h"
#include "libuvc/libuvc_internal.h"

uvc_frame_desc_t *uvc_find_frame_desc_stream(uvc_stream_handle_t *strmh,
    uint16_t format_id, uint16_t frame_id);
uvc_frame_desc_t *uvc_find_frame_desc(uvc_device_handle_t *devh,
    uint16_t format_id, uint16_t frame_id);
void *_uvc_user_caller(void *arg);
void _uvc_populate_frame(uvc_stream_handle_t *strmh);

struct format_table_entry {
  enum uvc_frame_format format;
  uint8_t abstract_fmt;
  uint8_t guid[16];
  int children_count;
  enum uvc_frame_format *children;
};

struct format_table_entry *_get_format_entry(enum uvc_frame_format format) {
  #define ABS_FMT(_fmt, ...) \
    case _fmt: { \
    static enum uvc_frame_format _fmt##_children[] = __VA_ARGS__; \
    static struct format_table_entry _fmt##_entry = { \
      _fmt, 0, {}, ARRAYSIZE(_fmt##_children), _fmt##_children }; \
    return &_fmt##_entry; }

  #define FMT(_fmt, ...) \
    case _fmt: { \
    static struct format_table_entry _fmt##_entry = { \
      _fmt, 0, __VA_ARGS__, 0, NULL }; \
    return &_fmt##_entry; }

  switch(format) {
    /* Define new formats here
     * 在这里定义新格式
     */
    ABS_FMT(UVC_FRAME_FORMAT_ANY, {UVC_FRAME_FORMAT_UNCOMPRESSED, UVC_FRAME_FORMAT_COMPRESSED})

    ABS_FMT(UVC_FRAME_FORMAT_UNCOMPRESSED, {UVC_FRAME_FORMAT_YUYV, UVC_FRAME_FORMAT_UYVY, UVC_FRAME_FORMAT_GRAY8})
    FMT(UVC_FRAME_FORMAT_YUYV, {'Y',  'U',  'Y',  '2', 0x00, 0x00, 0x10, 0x00, 0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71})
    FMT(UVC_FRAME_FORMAT_UYVY, {'U',  'Y',  'V',  'Y', 0x00, 0x00, 0x10, 0x00, 0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71})
    FMT(UVC_FRAME_FORMAT_GRAY8, {'Y',  '8',  '0',  '0', 0x00, 0x00, 0x10, 0x00, 0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71})

    ABS_FMT(UVC_FRAME_FORMAT_COMPRESSED, {UVC_FRAME_FORMAT_MJPEG})
    FMT(UVC_FRAME_FORMAT_MJPEG, {'M',  'J',  'P',  'G'})

    default:
      return NULL;
  }

  #undef ABS_FMT
  #undef FMT
}

static uint8_t _uvc_frame_format_matches_guid(enum uvc_frame_format fmt, uint8_t guid[16]) {
  struct format_table_entry *format;
  int child_idx;

  format = _get_format_entry(fmt);
  if (!format)
    return 0;

  if (!format->abstract_fmt && !memcmp(guid, format->guid, 16))
    return 1;

  for (child_idx = 0; child_idx < format->children_count; child_idx++) {
    if (_uvc_frame_format_matches_guid(format->children[child_idx], guid))
      return 1;
  }

  return 0;
}

static enum uvc_frame_format uvc_frame_format_for_guid(uint8_t guid[16]) {
  struct format_table_entry *format;
  enum uvc_frame_format fmt;

  for (fmt = 0; fmt < UVC_FRAME_FORMAT_COUNT; ++fmt) {
    format = _get_format_entry(fmt);
    if (!format || format->abstract_fmt)
      continue;
    if (!memcmp(format->guid, guid, 16))
      return format->format;
  }

  return UVC_FRAME_FORMAT_UNKNOWN;
}

/** @internal
 * Run a streaming control query
 * @param[in] devh UVC device
 * @param[in,out] ctrl Control block
 * @param[in] probe Whether this is a probe query or a commit query
 * @param[in] req Query type
 * 运行流控制查询
 */
uvc_error_t uvc_query_stream_ctrl(
    uvc_device_handle_t *devh,
    uvc_stream_ctrl_t *ctrl,
    uint8_t probe,
    enum uvc_req_code req) {
  uint8_t buf[34];
  size_t len;
  uvc_error_t err;

  bzero(buf, sizeof(buf));

  if (devh->info->ctrl_if.bcdUVC >= 0x0110)
    len = 34;
  else
    len = 26;

  /* prepare for a SET transfer
   * 准备SET转移
   */
  if (req == UVC_SET_CUR) {
    SHORT_TO_SW(ctrl->bmHint, buf);
    buf[2] = ctrl->bFormatIndex;
    buf[3] = ctrl->bFrameIndex;
    INT_TO_DW(ctrl->dwFrameInterval, buf + 4);
    SHORT_TO_SW(ctrl->wKeyFrameRate, buf + 8);
    SHORT_TO_SW(ctrl->wPFrameRate, buf + 10);
    SHORT_TO_SW(ctrl->wCompQuality, buf + 12);
    SHORT_TO_SW(ctrl->wCompWindowSize, buf + 14);
    SHORT_TO_SW(ctrl->wDelay, buf + 16);
    INT_TO_DW(ctrl->dwMaxVideoFrameSize, buf + 18);
    INT_TO_DW(ctrl->dwMaxPayloadTransferSize, buf + 22);

    if (len == 34) {
      /** @todo support UVC 1.1 */
      return UVC_ERROR_NOT_SUPPORTED;
    }
  }

  /* do the transfer
   * 做转移
   */
  err = libusb_control_transfer(
      devh->usb_devh,
      req == UVC_SET_CUR ? 0x21 : 0xA1,
      req,
      probe ? (UVC_VS_PROBE_CONTROL << 8) : (UVC_VS_COMMIT_CONTROL << 8),
      ctrl->bInterfaceNumber,
      buf, len, 0
  );

  if (err <= 0) {
    return err;
  }

  /* now decode following a GET transfer
   * 在GET传输后解码
   */
  if (req != UVC_SET_CUR) {
    ctrl->bmHint = SW_TO_SHORT(buf);
    ctrl->bFormatIndex = buf[2];
    ctrl->bFrameIndex = buf[3];
    ctrl->dwFrameInterval = DW_TO_INT(buf + 4);
    ctrl->wKeyFrameRate = SW_TO_SHORT(buf + 8);
    ctrl->wPFrameRate = SW_TO_SHORT(buf + 10);
    ctrl->wCompQuality = SW_TO_SHORT(buf + 12);
    ctrl->wCompWindowSize = SW_TO_SHORT(buf + 14);
    ctrl->wDelay = SW_TO_SHORT(buf + 16);
    ctrl->dwMaxVideoFrameSize = DW_TO_INT(buf + 18);
    ctrl->dwMaxPayloadTransferSize = DW_TO_INT(buf + 22);

    if (len == 34) {
      /** @todo support UVC 1.1 */
      return UVC_ERROR_NOT_SUPPORTED;
    }

    /* fix up block for cameras that fail to set dwMax
     * 修复无法设置 dwMax 的摄像机的模块
     */
    if (ctrl->dwMaxVideoFrameSize == 0) {
      uvc_frame_desc_t *frame = uvc_find_frame_desc(devh, ctrl->bFormatIndex, ctrl->bFrameIndex);

      if (frame) {
        ctrl->dwMaxVideoFrameSize = frame->dwMaxVideoFrameBufferSize;
      }
    }
  }

  return UVC_SUCCESS;
}

/**
 * @brief Reconfigure stream with a new stream format.
 * @ingroup streaming
 * 使用新的流格式重新配置流。
 *
 * This may be executed whether or not the stream is running.
 * 无论流是否正在运行，都可以执行此操作。
 *
 * @param[in] strmh Stream handle
 * @param[in] ctrl Control block, processed using {uvc_probe_stream_ctrl} or
 *             {uvc_get_stream_ctrl_format_size}
 */
uvc_error_t uvc_stream_ctrl(uvc_stream_handle_t *strmh, uvc_stream_ctrl_t *ctrl) {
  uvc_error_t ret;

  if (strmh->stream_if->bInterfaceNumber != ctrl->bInterfaceNumber)
    return UVC_ERROR_INVALID_PARAM;

  /* @todo Allow the stream to be modified without restarting the stream */
  if (strmh->running)
    return UVC_ERROR_BUSY;

  ret = uvc_query_stream_ctrl(strmh->devh, ctrl, 0, UVC_SET_CUR);
  if (ret != UVC_SUCCESS)
    return ret;

  strmh->cur_ctrl = *ctrl;
  return UVC_SUCCESS;
}

/** @internal
 * @brief Find the descriptor for a specific frame configuration
 * @param stream_if Stream interface
 * @param format_id Index of format class descriptor
 * @param frame_id Index of frame descriptor
 * 查找特定帧配置的描述符
 */
static uvc_frame_desc_t *_uvc_find_frame_desc_stream_if(uvc_streaming_interface_t *stream_if,
    uint16_t format_id, uint16_t frame_id) {
 
  uvc_format_desc_t *format = NULL;
  uvc_frame_desc_t *frame = NULL;

  DL_FOREACH(stream_if->format_descs, format) {
    if (format->bFormatIndex == format_id) {
      DL_FOREACH(format->frame_descs, frame) {
        if (frame->bFrameIndex == frame_id)
          return frame;
      }
    }
  }

  return NULL;
}

uvc_frame_desc_t *uvc_find_frame_desc_stream(uvc_stream_handle_t *strmh,
    uint16_t format_id, uint16_t frame_id) {
  return _uvc_find_frame_desc_stream_if(strmh->stream_if, format_id, frame_id);
}

/** @internal
 * @brief Find the descriptor for a specific frame configuration
 * @param devh UVC device
 * @param format_id Index of format class descriptor
 * @param frame_id Index of frame descriptor
 * 查找特定帧配置的描述符
 */
uvc_frame_desc_t *uvc_find_frame_desc(uvc_device_handle_t *devh,
    uint16_t format_id, uint16_t frame_id) {
 
  uvc_streaming_interface_t *stream_if;
  uvc_frame_desc_t *frame;

  DL_FOREACH(devh->info->stream_ifs, stream_if) {
    frame = _uvc_find_frame_desc_stream_if(stream_if, format_id, frame_id);
    if (frame)
      return frame;
  }

  return NULL;
}

/**
 * Get a negotiated streaming control block for some common parameters.
 * @ingroup streaming
 * 获取一些常用参数的协商流控制块。
 *
 * @param[in] devh Device handle
 * @param[in,out] ctrl Control block
 * @param[in] format_class Type of streaming format
 * @param[in] width Desired frame width
 * @param[in] height Desired frame height
 * @param[in] fps Frame rate, frames per second
 */
uvc_error_t uvc_get_stream_ctrl_format_size(
    uvc_device_handle_t *devh,
    uvc_stream_ctrl_t *ctrl,
    enum uvc_frame_format cf,
    int width, int height,
    int fps) {
  uvc_streaming_interface_t *stream_if;
  enum uvc_vs_desc_subtype format_class;

  /* get the max values
   * 获得最大值
   */
  uvc_query_stream_ctrl(
      devh, ctrl, 1, UVC_GET_MAX
  );

  /* find a matching frame descriptor and interval
   * 查找匹配的帧描述符和间隔
   */
  DL_FOREACH(devh->info->stream_ifs, stream_if) {
    uvc_format_desc_t *format;

    DL_FOREACH(stream_if->format_descs, format) {
      uvc_frame_desc_t *frame;

      if (!_uvc_frame_format_matches_guid(cf, format->guidFormat))
        continue;

      DL_FOREACH(format->frame_descs, frame) {
        if (frame->wWidth != width || frame->wHeight != height)
          continue;

        uint32_t *interval;

        if (frame->intervals) {
          for (interval = frame->intervals; *interval; ++interval) {
            if (10000000 / *interval == (unsigned int) fps) {
              /* don't negotiate interval
               * 不协商间隔
               */
              ctrl->bmHint = (1 << 0);
              ctrl->bFormatIndex = format->bFormatIndex;
              ctrl->bFrameIndex = frame->bFrameIndex;
              ctrl->bInterfaceNumber = stream_if->bInterfaceNumber;
              ctrl->dwFrameInterval = *interval;

              goto found;
            }
          }
        } else {
          uint32_t interval_100ns = 10000000 / fps;
          uint32_t interval_offset = interval_100ns - frame->dwMinFrameInterval;

          if (interval_100ns >= frame->dwMinFrameInterval
              && interval_100ns <= frame->dwMaxFrameInterval
              && !(interval_offset
                   && (interval_offset % frame->dwFrameIntervalStep))) {
            ctrl->bmHint = (1 << 0);
            ctrl->bFormatIndex = format->bFormatIndex;
            ctrl->bFrameIndex = frame->bFrameIndex;
            ctrl->bInterfaceNumber = stream_if->bInterfaceNumber;
            ctrl->dwFrameInterval = interval_100ns;

            goto found;
          }
        }
      }
    }
  }

  return UVC_ERROR_INVALID_MODE;

found:
  return uvc_probe_stream_ctrl(devh, ctrl);
}

/** @internal
 * Negotiate streaming parameters with the device
 * 与设备协商流参数
 *
 * @param[in] devh UVC device
 * @param[in,out] ctrl Control block
 */
uvc_error_t uvc_probe_stream_ctrl(
    uvc_device_handle_t *devh,
    uvc_stream_ctrl_t *ctrl) {
 
  uvc_claim_if(devh, ctrl->bInterfaceNumber);

  uvc_query_stream_ctrl(
      devh, ctrl, 1, UVC_SET_CUR
  );

  uvc_query_stream_ctrl(
      devh, ctrl, 1, UVC_GET_CUR
  );

  /** @todo make sure that worked */
  return UVC_SUCCESS;
}

/** @internal
 * @brief Isochronous transfer callback
 * 同步传输回调
 * 
 * Processes stream, places frames into buffer, signals listeners
 * (such as user callback thread and any polling thread) on new frame
 * 处理流，将帧放入缓冲区，在新帧上向侦听器（例如用户回调线程和任何轮询线程）发出信号
 *
 * @param transfer Active transfer
 */ 
void _uvc_iso_callback(struct libusb_transfer *transfer) {
  uvc_stream_handle_t *strmh;
  int packet_id;

  /* per packet
   * 包
   */
  uint8_t *pktbuf;
  uint8_t check_header;
  size_t header_len;
  struct libusb_iso_packet_descriptor *pkt;
  uint8_t *tmp_buf;

  static uint8_t isight_tag[] = {
    0x11, 0x22, 0x33, 0x44,
    0xde, 0xad, 0xbe, 0xef, 0xde, 0xad, 0xfa, 0xce
  };

  strmh = transfer->user_data;

  switch (transfer->status) {
  case LIBUSB_TRANSFER_COMPLETED:
    for (packet_id = 0; packet_id < transfer->num_iso_packets; ++packet_id) {
      check_header = 1;

      pkt = transfer->iso_packet_desc + packet_id;

      if (pkt->status != 0) {
        printf("bad packet (transfer): %d\n", pkt->status);
        continue;
      }

      if (pkt->actual_length == 0) {
        continue;
      }
      // 获取数据包
      pktbuf = libusb_get_iso_packet_buffer_simple(transfer, packet_id);

      if (strmh->devh->is_isight) {
        if (pkt->actual_length < 30 ||
            (memcmp(isight_tag, pktbuf + 2, sizeof(isight_tag))
             && memcmp(isight_tag, pktbuf + 3, sizeof(isight_tag)))) {
          check_header = 0;
          header_len = 0;
        } else {
          header_len = pktbuf[0];
        }
      } else {
        header_len = pktbuf[0];
      }

      if (check_header && pktbuf[1] & 0x40) {
        printf("bad packet\n");
        continue;
      }

      /** @todo support sending the frame on EOF instead of on flip(FID) */
      if (check_header && strmh->fid != (pktbuf[1] & 1)) {
        pthread_mutex_lock(&strmh->cb_mutex);

        /* swap the buffers
         * 交换缓冲区
         */
        tmp_buf = strmh->holdbuf;
        strmh->hold_bytes = strmh->got_bytes;
        strmh->holdbuf = strmh->outbuf;
        strmh->outbuf = tmp_buf;
        strmh->hold_last_scr = strmh->last_scr;
        strmh->hold_pts = strmh->pts;
        strmh->hold_seq = strmh->seq;

        pthread_cond_broadcast(&strmh->cb_cond);
        pthread_mutex_unlock(&strmh->cb_mutex);

        strmh->seq++;
        strmh->got_bytes = 0;
        strmh->last_scr = 0;
        strmh->pts = 0;
        strmh->fid = pktbuf[1] & 1;
      }

      if (check_header) {
        if (pktbuf[1] & (1 << 2))
          strmh->pts = DW_TO_INT(pktbuf + 2);

        if (pktbuf[1] & (1 << 3))
          strmh->last_scr = DW_TO_INT(pktbuf + 6);

        if (strmh->devh->is_isight)
          // don't look for data after an iSight header
          // 不要在iSight标头后查找数据
          continue;
      }

      if (pkt->actual_length < header_len) {
        /* Bogus packet received
         * 收到假包
         */
        printf("bogus packet: actual_len=%d, header_len=%zd\n", pkt->actual_length, header_len);
        continue;
      }
      if (pkt->actual_length - header_len > 0)
        memcpy(strmh->outbuf + strmh->got_bytes, pktbuf + header_len, pkt->actual_length - header_len);

      strmh->got_bytes += pkt->actual_length - header_len;
    }
    break;
  case LIBUSB_TRANSFER_CANCELLED: 
  case LIBUSB_TRANSFER_ERROR:
  case LIBUSB_TRANSFER_NO_DEVICE: {
    int i;
    UVC_DEBUG("not retrying transfer, status = %d", transfer->status);
    pthread_mutex_lock(&strmh->cb_mutex);

    /* Mark transfer as deleted.
     * 将转移标记为已删除。
     */
    for(i=0; i<ARRAYSIZE(strmh->transfers); i++) {
      if(strmh->transfers[i] == transfer) {
        UVC_DEBUG("Freeing transfer %d (%p)", i, transfer);
        free(transfer->buffer);
        libusb_free_transfer(transfer);
        strmh->transfers[i] = NULL;
        break;
      }
    }
    if(i == ARRAYSIZE(strmh->transfers)) {
      UVC_DEBUG("transfer %p not found; not freeing!", transfer);
    }

    pthread_cond_broadcast(&strmh->cb_cond);
    pthread_mutex_unlock(&strmh->cb_mutex);

    break;
  }
  case LIBUSB_TRANSFER_TIMED_OUT:
  case LIBUSB_TRANSFER_STALL:
  case LIBUSB_TRANSFER_OVERFLOW:
    UVC_DEBUG("retrying transfer, status = %d", transfer->status);
    break;
  }
  
  if (strmh->running)
    libusb_submit_transfer(transfer);
}

/**
 * Begin streaming video from the camera into the callback function.
 * @ingroup streaming
 * 开始将视频从摄像机传输到回调函数中。
 *
 * @param devh UVC device
 * @param ctrl Control block, processed using {uvc_probe_stream_ctrl} or
 *             {uvc_get_stream_ctrl_format_size}
 * @param cb   User callback function. See {uvc_frame_callback_t} for restrictions.
 * @param isochronous Whether to use isochronous transfers rather than bulk ones
 */
uvc_error_t uvc_start_streaming(
    uvc_device_handle_t *devh,
    uvc_stream_ctrl_t *ctrl,
    uvc_frame_callback_t *cb,
    void *user_ptr,
    uint8_t isochronous
) {
  uvc_error_t ret;
  uvc_stream_handle_t *strmh;

  ret = uvc_stream_open_ctrl(devh, &strmh, ctrl);
  if (ret != UVC_SUCCESS)
    return ret;

  ret = uvc_stream_start(strmh, cb, user_ptr, isochronous);
  if (ret != UVC_SUCCESS) {
    uvc_stream_close(strmh);
    return ret;
  }

  return UVC_SUCCESS;
}

/**
 * Begin streaming video from the camera into the callback function.
 * @ingroup streaming
 * 开始将视频从摄像机传输到回调函数中。
 *
 * @param devh UVC device
 * @param ctrl Control block, processed using {uvc_probe_stream_ctrl} or
 *             {uvc_get_stream_ctrl_format_size}
 * @param cb   User callback function. See {uvc_frame_callback_t} for restrictions.
 */
uvc_error_t uvc_start_iso_streaming(
    uvc_device_handle_t *devh,
    uvc_stream_ctrl_t *ctrl,
    uvc_frame_callback_t *cb,
    void *user_ptr
) {
  return uvc_start_streaming(devh, ctrl, cb, user_ptr, 1);
}

static uvc_stream_handle_t *_uvc_get_stream_by_interface(uvc_device_handle_t *devh, int interface_idx) {
  uvc_stream_handle_t *strmh;

  DL_FOREACH(devh->streams, strmh) {
    if (strmh->stream_if->bInterfaceNumber == interface_idx)
      return strmh;
  }

  return NULL;
}

static uvc_streaming_interface_t *_uvc_get_stream_if(uvc_device_handle_t *devh, int interface_idx) {
  uvc_streaming_interface_t *stream_if;

  DL_FOREACH(devh->info->stream_ifs, stream_if) {
    if (stream_if->bInterfaceNumber == interface_idx)
      return stream_if;
  }
  
  return NULL;
}

/**
 * Open a new video stream.
 * @ingroup streaming
 * 打开新视频流
 *
 * @param devh UVC device
 * @param ctrl Control block, processed using {uvc_probe_stream_ctrl} or
 *             {uvc_get_stream_ctrl_format_size}
 */
uvc_error_t uvc_stream_open_ctrl(uvc_device_handle_t *devh, uvc_stream_handle_t **strmhp, uvc_stream_ctrl_t *ctrl) {
  /* Chosen frame and format descriptors
   * 选择帧和格式描述符
   */
  uvc_stream_handle_t *strmh = NULL;
  uvc_streaming_interface_t *stream_if;
  uvc_error_t ret;

  UVC_ENTER();

  if (_uvc_get_stream_by_interface(devh, ctrl->bInterfaceNumber) != NULL) {
    /* Stream is already opened
     * 流已打开
     */
    ret = UVC_ERROR_BUSY;
    goto fail;
  }

  stream_if = _uvc_get_stream_if(devh, ctrl->bInterfaceNumber);
  if (!stream_if) {
    ret = UVC_ERROR_INVALID_PARAM;
    goto fail;
  }

  strmh = calloc(1, sizeof(*strmh));
  if (!strmh) {
    ret = UVC_ERROR_NO_MEM;
    goto fail;
  }
  strmh->devh = devh;
  strmh->stream_if = stream_if;
  strmh->frame.library_owns_data = 1;

  ret = uvc_claim_if(strmh->devh, strmh->stream_if->bInterfaceNumber);
  if (ret != UVC_SUCCESS)
    goto fail;

  ret = uvc_stream_ctrl(strmh, ctrl);
  if (ret != UVC_SUCCESS)
    goto fail;

  // Set up the streaming status and data space
  // 设置流状态和数据空间
  int bufSize = 8 * 1024 * 1024;
  strmh->running = 0;
  strmh->outbuf = malloc(bufSize); // 8MB /** @todo take only what we need */
  strmh->holdbuf = malloc(bufSize); // 8MB

  UVC_DEBUG("LIBUVC_XFER_BUF_SIZE:"+bufSize);
   
  pthread_mutex_init(&strmh->cb_mutex, NULL);
  pthread_cond_init(&strmh->cb_cond, NULL);

  DL_APPEND(devh->streams, strmh);

  *strmhp = strmh;

  UVC_EXIT(0);
  return UVC_SUCCESS;

fail:
  if(strmh)
    free(strmh);
  UVC_EXIT(ret);
  return ret;
}

/**
 * Begin streaming video from the stream into the callback function.
 * @ingroup streaming
 *
 * @param strmh UVC stream
 * @param cb   User callback function. See {uvc_frame_callback_t} for restrictions.
 * @param isochronous Whether to use isochronous transfers rather than bulk ones
 *
 * 开始将视频从流传输到回调函数中。
 * @param strmh UVC流
 * @param cb 用户回调函数。 有关限制，请参见{uvc_frame_callback_t}。
 * @param flags 流设置标志，当前未定义。 将此设置为零。 保留低位用于向后兼容。
 */
uvc_error_t uvc_stream_start(
    uvc_stream_handle_t *strmh,
    uvc_frame_callback_t *cb,
    void *user_ptr,
    uint8_t isochronous
) {
  /* USB interface we'll be using
   * 我们将使用的USB接口
   */
  const struct libusb_interface *interface;
  int interface_id;
  uvc_frame_desc_t *frame_desc;
  uvc_format_desc_t *format_desc;
  uvc_stream_ctrl_t *ctrl;
  uvc_error_t ret;

  ctrl = &strmh->cur_ctrl;

  UVC_ENTER();

  if (strmh->running) {
    UVC_EXIT(UVC_ERROR_BUSY);
    return UVC_ERROR_BUSY;
  }

  strmh->running = 1;
  strmh->seq = 0;
  strmh->fid = 0;
  strmh->pts = 0;
  strmh->last_scr = 0;

  frame_desc = uvc_find_frame_desc_stream(strmh, ctrl->bFormatIndex, ctrl->bFrameIndex);
  if (!frame_desc) {
    ret = UVC_ERROR_INVALID_PARAM;
    goto fail;
  }
  format_desc = frame_desc->parent;

  strmh->frame_format = uvc_frame_format_for_guid(format_desc->guidFormat);
  if (strmh->frame_format == UVC_FRAME_FORMAT_UNKNOWN) {
    ret = UVC_ERROR_NOT_SUPPORTED;
    goto fail;
  }

  // Get the interface that provides the chosen format and frame configuration
  // 获取提供所选格式和帧配置的界面
  interface_id = strmh->stream_if->bInterfaceNumber;
  interface = &strmh->devh->info->config->interface[interface_id];

  if (isochronous) {
    /* For isochronous streaming, we choose an appropriate altsetting for the endpoint
     * and set up several transfers
	 * 为同步流，我们为端点选择一个适当的altsetting，并设置几个传输
     */
    const struct libusb_interface_descriptor *altsetting;
    const struct libusb_endpoint_descriptor *endpoint;
    /* The greatest number of bytes that the device might provide, per packet, in this
     * configuration
     * 在此配置中，每个数据包设备可能提供的最大字节数
     */
    size_t config_bytes_per_packet;
    /* Number of packets per transfer
     * 每次传输的数据包数量
     */
    size_t packets_per_transfer;
    /* Total amount of data per transfer
     * 每次传输的数据总量
     */
    size_t total_transfer_size;
    /* Size of packet transferable from the chosen endpoint
     * 可从所选端点传输的数据包大小
     */
    size_t endpoint_bytes_per_packet;
    /* Index of the altsetting
     * altsetting的索引
     */
    int alt_idx, ep_idx;
    
    struct libusb_transfer *transfer;
    int transfer_id;

    /* If the interface doesn't support isochronous mode, give up
     * 如果接口不支持同步模式，请放弃
     */
    if (interface->num_altsetting == 0) {
      ret = UVC_ERROR_INVALID_DEVICE;
      goto fail;
    }

    config_bytes_per_packet = strmh->cur_ctrl.dwMaxPayloadTransferSize;

    /* Go through the altsettings and find one whose packets are at least
     * as big as our format's maximum per-packet usage. Assume that the
     * packet sizes are increasing.
     * 通过altsettings，找到一个其数据包至少与我们格式最大的每数据包使用量一样大的数据包。 假设数据包大小正在增加。
     */
    for (alt_idx = 0; alt_idx < interface->num_altsetting; alt_idx++) {
      altsetting = interface->altsetting + alt_idx;
      endpoint_bytes_per_packet = 0;

      /* Find the endpoint with the number specified in the VS header
       * 使用VS标头中指定的编号查找端点
       */
      for (ep_idx = 0; ep_idx < altsetting->bNumEndpoints; ep_idx++) {
        endpoint = altsetting->endpoint + ep_idx;

        if (endpoint->bEndpointAddress == format_desc->parent->bEndpointAddress) {
          endpoint_bytes_per_packet = endpoint->wMaxPacketSize;
          // wMaxPacketSize: [unused:2 (multiplier-1):3 size:11]
          endpoint_bytes_per_packet = (endpoint_bytes_per_packet & 0x07ff) *
                                      (((endpoint_bytes_per_packet >> 11) & 3) + 1);
          break;
        }
      }

      if (endpoint_bytes_per_packet >= config_bytes_per_packet) {
        /* Transfers will be at most one frame long: Divide the maximum frame size
         * by the size of the endpoint and round up
         * 传输最多为一帧：将最大帧大小除以端点大小并四舍五入
         */
        packets_per_transfer = (ctrl->dwMaxVideoFrameSize + endpoint_bytes_per_packet - 1) / endpoint_bytes_per_packet;

        /* But keep a reasonable limit: Otherwise we start dropping data
         * 但请保持合理的限制：否则我们将开始删除数据
         */
        if (packets_per_transfer > 32)
          packets_per_transfer = 32;
        
        total_transfer_size = packets_per_transfer * endpoint_bytes_per_packet;
        break;
      }
    }

    /* If we searched through all the altsettings and found nothing usable
     * 如果我们搜索了所有的altsettings，却发现没有可用的东西
     */
    if (alt_idx == interface->num_altsetting) {
      ret = UVC_ERROR_INVALID_MODE;
      goto fail;
    }

    /* Select the altsetting
     * 选择altsetting
     */
    ret = libusb_set_interface_alt_setting(strmh->devh->usb_devh,
                                           altsetting->bInterfaceNumber,
                                           altsetting->bAlternateSetting);
    if (ret != UVC_SUCCESS) {
      UVC_DEBUG("libusb_set_interface_alt_setting failed");
      goto fail;
    }

    /* Set up the transfers
     * 设置转移
     */
    for (transfer_id = 0; transfer_id < ARRAYSIZE(strmh->transfers); ++transfer_id) {
      // 为libusb传输分配指定数量的同步数据包描述符
      transfer = libusb_alloc_transfer(packets_per_transfer);
      strmh->transfers[transfer_id] = transfer;      
      strmh->transfer_bufs[transfer_id] = malloc(total_transfer_size);

      // Helper函数填充同步传输所需的 libusb_transfer 字段。
      libusb_fill_iso_transfer(
        transfer, strmh->devh->usb_devh, format_desc->parent->bEndpointAddress,
        strmh->transfer_bufs[transfer_id],
        total_transfer_size, packets_per_transfer, _uvc_iso_callback, (void*) strmh, 5000);

      // 便利功能可根据传输结构中的num_iso_packets字段设置同步传输中所有数据包的长度。
      libusb_set_iso_packet_lengths(transfer, endpoint_bytes_per_packet);
    }
  } else {
    /** @todo prepare for bulk transfer */
  }

  strmh->user_cb = cb;
  strmh->user_ptr = user_ptr;

  /* If the user wants it, set up a thread that calls the user's function
   * with the contents of each frame.
   * 如果用户需要，请设置一个线程，该线程使用每帧的内容来调用用户的函数。
   */
  if (cb) {
    pthread_create(&strmh->cb_thread, NULL, _uvc_user_caller, (void*) strmh);
  }

  if (isochronous) {
    int transfer_id;

    for (transfer_id = 0; transfer_id < ARRAYSIZE(strmh->transfers); transfer_id++) {
      ret = libusb_submit_transfer(strmh->transfers[transfer_id]);
      if (ret != UVC_SUCCESS) {
        UVC_DEBUG("libusb_submit_transfer failed");
        break;
      }
    }
  } else {
    /** @todo submit bulk transfer */
  }

  if (ret != UVC_SUCCESS) {
    /** @todo clean up transfers and memory */
    goto fail;
  }

  UVC_EXIT(ret);
  return ret;
fail:
  strmh->running = 0;
  UVC_EXIT(ret);
  return ret;
}

/**
 * Begin streaming video from the stream into the callback function.
 * @ingroup streaming
 * 开始将视频从流传输到回调函数中。
 *
 * @param strmh UVC stream
 * @param cb   User callback function. See {uvc_frame_callback_t} for restrictions.
 * @param isochronous Whether to use isochronous transfers rather than bulk ones
 */
uvc_error_t uvc_stream_start_iso(
    uvc_stream_handle_t *strmh,
    uvc_frame_callback_t *cb,
    void *user_ptr
) {
  return uvc_stream_start(strmh, cb, user_ptr, 1);
}

/** @internal
 * @brief User callback runner thread
 * @note There should be at most one of these per currently streaming device
 * @param arg Device handle
 * 用户回调运行器线程
 * 每个当前流式传输设备最多应有一个
 */
void *_uvc_user_caller(void *arg) {
  uvc_stream_handle_t *strmh = (uvc_stream_handle_t *) arg;

  uint32_t last_seq = 0;

  do {
    pthread_mutex_lock(&strmh->cb_mutex);

    while (strmh->running && last_seq == strmh->hold_seq) {
      pthread_cond_wait(&strmh->cb_cond, &strmh->cb_mutex);
    }

    if (!strmh->running) {
      pthread_mutex_unlock(&strmh->cb_mutex);
      break;
    }
    
    last_seq = strmh->hold_seq;
    _uvc_populate_frame(strmh);
    
    pthread_mutex_unlock(&strmh->cb_mutex);
    
    strmh->user_cb(&strmh->frame, strmh->user_ptr);
  } while(1);

  return NULL; // return value ignored
}

/** @internal
 * @brief Populate the fields of a frame to be handed to user code
 * must be called with stream cb lock held!
 * 填充要提交给用户代码的帧的字段时，必须使用流cb锁保持调用！
 */
void _uvc_populate_frame(uvc_stream_handle_t *strmh) {
  size_t alloc_size = strmh->cur_ctrl.dwMaxVideoFrameSize;
  uvc_frame_t *frame = &strmh->frame;
  uvc_frame_desc_t *frame_desc;

  /** @todo this stuff that hits the main config cache should really happen 碰到主配置缓存的东西应该真的发生
   * in start() so that only one thread hits these data. all of this stuff
   * is going to be reopen_on_change anyway
   * 在start（）中，以便只有一个线程可以命中这些数据。所有这些东西无论如何都将是reopen_on_change
   */

  frame_desc = uvc_find_frame_desc(strmh->devh, strmh->cur_ctrl.bFormatIndex,
				   strmh->cur_ctrl.bFrameIndex);

  frame->frame_format = strmh->frame_format;
  
  frame->width = frame_desc->wWidth;
  frame->height = frame_desc->wHeight;
  
  switch (frame->frame_format) {
  case UVC_FRAME_FORMAT_YUYV:
    frame->step = frame->width * 2;
    break;
  case UVC_FRAME_FORMAT_MJPEG:
    frame->step = 0;
    break;
  default:
    frame->step = 0;
    break;
  }
  
  /* copy the image data from the hold buffer to the frame (unnecessary extra buf?)
   * 将图像数据从保持缓冲区复制到帧（不必要的额外缓冲区？）
   */
  if (frame->data_bytes < strmh->hold_bytes) {
    frame->data = realloc(frame->data, strmh->hold_bytes);
    frame->data_bytes = strmh->hold_bytes;
  }
  memcpy(frame->data, strmh->holdbuf, frame->data_bytes);
  
  /** @todo set the frame time */
}

/**
 * Poll for a frame
 * @ingroup streaming
 * 获取一帧
 *
 * @param devh UVC device
 * @param[out] frame Location to store pointer to captured frame (NULL on error)
 * @param timeout_us >0: Wait at most N microseconds; 0: Wait indefinitely; -1: return immediately
 */
uvc_error_t uvc_stream_get_frame(uvc_stream_handle_t *strmh,
			  uvc_frame_t **frame,
			  int32_t timeout_us) {
  time_t add_secs;
  time_t add_nsecs;
  struct timespec ts;
  struct timeval tv;

  if (!strmh->running)
    return UVC_ERROR_INVALID_PARAM;

  if (strmh->user_cb)
    return UVC_ERROR_CALLBACK_EXISTS;

  pthread_mutex_lock(&strmh->cb_mutex);

  if (strmh->last_polled_seq < strmh->hold_seq) {
    _uvc_populate_frame(strmh);
    *frame = &strmh->frame;
    strmh->last_polled_seq = strmh->hold_seq;
  } else if (timeout_us != -1) {
    if (timeout_us == 0) {
      pthread_cond_wait(&strmh->cb_cond, &strmh->cb_mutex);
    } else {
      add_secs = timeout_us / 1000000;
      add_nsecs = (timeout_us % 1000000) * 1000;
      ts.tv_sec = 0;
      ts.tv_nsec = 0;

#if _POSIX_TIMERS > 0
      clock_gettime(CLOCK_REALTIME, &ts);
#else
      gettimeofday(&tv, NULL);
      ts.tv_sec = tv.tv_sec;
      ts.tv_nsec = tv.tv_usec * 1000;
#endif

      ts.tv_sec += add_secs;
      ts.tv_nsec += add_nsecs;

      pthread_cond_timedwait(&strmh->cb_cond, &strmh->cb_mutex, &ts);
    }
    
    if (strmh->last_polled_seq < strmh->hold_seq) {
      _uvc_populate_frame(strmh);
      *frame = &strmh->frame;
      strmh->last_polled_seq = strmh->hold_seq;
    } else {
      *frame = NULL;
    }
  } else {
    *frame = NULL;
  }

  pthread_mutex_unlock(&strmh->cb_mutex);

  return UVC_SUCCESS;
}

/** @brief Stop streaming video
 * @ingroup streaming
 * 停止视频流
 *
 * Closes all streams, ends threads and cancels pollers
 * 关闭所有流，结束线程并取消轮询
 *
 * @param devh UVC device
 */
void uvc_stop_streaming(uvc_device_handle_t *devh) {
  uvc_stream_handle_t *strmh, *strmh_tmp;

  DL_FOREACH_SAFE(devh->streams, strmh, strmh_tmp) {
    uvc_stream_close(strmh);
  }
}

/**
 * @brief Stop stream.
 * @ingroup streaming
 * 停止流
 *
 * Stops stream, ends threads and cancels pollers
 * 关闭流，结束线程并取消轮询
 *
 * @param devh UVC device
 */
uvc_error_t uvc_stream_stop(uvc_stream_handle_t *strmh) {
  int i;

  if (!strmh->running)
    return UVC_ERROR_INVALID_PARAM;

  strmh->running = 0;

  pthread_mutex_lock(&strmh->cb_mutex);

  for(i=0; i<ARRAYSIZE(strmh->transfers); i++) {
    if(strmh->transfers[i] != NULL) {
      int res = libusb_cancel_transfer(strmh->transfers[i]);
      if(res < 0) {
        free(strmh->transfers[i]->buffer);
        libusb_free_transfer(strmh->transfers[i]);
        strmh->transfers[i] = NULL;
      }
    }
  }

  /* Wait for transfers to complete/cancel
   * 等待传输完成/取消
   */
  do {
    for(i=0; i<ARRAYSIZE(strmh->transfers); i++) {
      if(strmh->transfers[i] != NULL)
        break;
    }
    if(i == ARRAYSIZE(strmh->transfers))
      break;
    pthread_cond_wait(&strmh->cb_cond, &strmh->cb_mutex);
  } while(1);
  // Kick the user thread awake
  // 唤醒用户线程
  pthread_cond_broadcast(&strmh->cb_cond);
  pthread_mutex_unlock(&strmh->cb_mutex);

  /** @todo stop the actual stream, camera side? */

  if (strmh->user_cb) {
    /* wait for the thread to stop (triggered by
     * LIBUSB_TRANSFER_CANCELLED transfer)
     * 等待线程停止（由LIBUSB_TRANSFER_CANCELLED传输触发）
     */
    pthread_join(strmh->cb_thread, NULL);
  }

  return UVC_SUCCESS;
}

/**
 * @brief Close stream.
 * @ingroup streaming
 * 关闭流
 *
 * Closes stream, frees handle and all streaming resources.
 * 关闭流，释放句柄和所有流资源。
 *
 * @param strmh UVC stream handle
 */
void uvc_stream_close(uvc_stream_handle_t *strmh) {
  if (strmh->running)
    uvc_stream_stop(strmh);

  uvc_release_if(strmh->devh, strmh->stream_if->bInterfaceNumber);

  if (strmh->frame.data)
    free(strmh->frame.data);

  free(strmh->outbuf);
  free(strmh->holdbuf);

  pthread_cond_destroy(&strmh->cb_cond);
  pthread_mutex_destroy(&strmh->cb_mutex);

  DL_DELETE(strmh->devh->streams, strmh);
  free(strmh);
}
