/*********************************************************************
 *********************************************************************/
/*********************************************************************
 * modified some function to avoid crash, support Android
 * Copyright (C) 2014-2016 saki@serenegiant All rights reserved.
 *********************************************************************/
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
 * @brief Tools for creating, managing and consuming video streams
 * 流式流控制功能
 * 用于创建，管理和使用视频流的工具
 */

#define LOCAL_DEBUG 1

#define DLY_DEBUG 0

#define LOG_TAG "libuvc/stream"
#if 1	// デバッグ情報を出さない時1  不输出调试信息时1
	#ifndef LOG_NDEBUG
		#define	LOG_NDEBUG		// LOGV/LOGD/MARKを出力しない時
		#endif
	#undef USE_LOGALL			// 指定したLOGxだけを出力
#else
	#define USE_LOGALL
	#undef LOG_NDEBUG
	#undef NDEBUG
	#define GET_RAW_DESCRIPTOR
#endif

#include <assert.h>		// XXX add assert for debugging

#include "libuvc/libuvc.h"
#include "libuvc/libuvc_internal.h"


char * uint8_to_hex(uint8_t aa, char *buffer, int *index)
{
    char ddl,ddh;
    ddh = 48 + aa / 16;
    ddl = 48 + aa % 16;
    if (ddh > 57) ddh = ddh + 7;
    if (ddl > 57) ddl = ddl + 7;
    buffer[(*index)++] = ddh;
    buffer[(*index)++] = ddl;
    buffer[(*index)++] = ' ';
    return buffer;
}

uvc_frame_desc_t *uvc_find_frame_desc_stream(uvc_stream_handle_t *strmh,
		uint16_t format_id, uint16_t frame_id);
uvc_frame_desc_t *uvc_find_frame_desc(uvc_device_handle_t *devh,
		uint16_t format_id, uint16_t frame_id);
static void *_uvc_user_caller(void *arg);
// 获取视频帧
static void _uvc_populate_frame(uvc_stream_handle_t *strmh);

struct format_table_entry {
	enum uvc_frame_format format;
	uint8_t abstract_fmt;
	uint8_t guid[16];
	int children_count;
	enum uvc_frame_format *children;
};

// 获取格式实体
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

	switch (format) {
        /* Define new formats here
         * 在这里定义新格式
         */
        ABS_FMT(UVC_FRAME_FORMAT_ANY, {UVC_FRAME_FORMAT_UNCOMPRESSED, UVC_FRAME_FORMAT_COMPRESSED})

        // 未压缩格式
        ABS_FMT(UVC_FRAME_FORMAT_UNCOMPRESSED, {UVC_FRAME_FORMAT_YUYV, UVC_FRAME_FORMAT_UYVY, UVC_FRAME_FORMAT_GRAY8})
        FMT(UVC_FRAME_FORMAT_YUYV, {'Y', 'U', 'Y', '2', 0x00, 0x00, 0x10, 0x00, 0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71})
        FMT(UVC_FRAME_FORMAT_UYVY, {'U', 'Y', 'V', 'Y', 0x00, 0x00, 0x10, 0x00, 0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71})
        FMT(UVC_FRAME_FORMAT_GRAY8, {'Y', '8', '0', '0', 0x00, 0x00, 0x10, 0x00, 0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71})
        FMT(UVC_FRAME_FORMAT_BY8, {'B', 'Y', '8', ' ', 0x00, 0x00, 0x10, 0x00, 0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71})

        // 压缩格式
        ABS_FMT(UVC_FRAME_FORMAT_COMPRESSED, {UVC_FRAME_FORMAT_MJPEG})
        FMT(UVC_FRAME_FORMAT_MJPEG, {'M', 'J', 'P', 'G'})

        default:
            return NULL;
	}

#undef ABS_FMT
#undef FMT
}

// 获取匹配的帧格式
static uint8_t _uvc_frame_format_matches_guid(enum uvc_frame_format fmt,
		uint8_t guid[16]) {
	struct format_table_entry *format;
	int child_idx;

    // 获取格式实体
	format = _get_format_entry(fmt);
	if (UNLIKELY(!format))
		return 0;

	if (!format->abstract_fmt && !memcmp(guid, format->guid, 16))
		return 1;

	for (child_idx = 0; child_idx < format->children_count; child_idx++) {
		if (_uvc_frame_format_matches_guid(format->children[child_idx], guid)) {
		    // 找到匹配的帧格式
		    return 1;
		}
	}

	return 0;
}

// 获取帧格式
static enum uvc_frame_format uvc_frame_format_for_guid(uint8_t guid[16]) {
	struct format_table_entry *format;
	enum uvc_frame_format fmt;

	for (fmt = 0; fmt < UVC_FRAME_FORMAT_COUNT; ++fmt) {
	     // 获取格式实体
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
 * 向设备发送特定请求获取流相关参数
 */
uvc_error_t uvc_query_stream_ctrl(uvc_device_handle_t *devh,
		uvc_stream_ctrl_t *ctrl, uint8_t probe, enum uvc_req_code req) {
	uint8_t buf[48];	// XXX support UVC 1.1 & 1.5
	size_t len;
	uvc_error_t err;

	memset(buf, 0, sizeof(buf));	// bzero(buf, sizeof(buf));	// bzero is deprecated

    // UVC 标准版本
	const uint16_t bcdUVC = devh->info->ctrl_if.bcdUVC;
	if (bcdUVC >= 0x0150) {
	    // 1.5
	    len = 48;
	}else if (bcdUVC >= 0x0110){
	    // 1.1
	    len = 34;
	}else{
	    // 1.0
	    len = 26;
	}
//	LOGI("bcdUVC:%x,req:0x%02x,probe:%d", bcdUVC, req, probe);
	/* prepare for a SET transfer
	 * 准备SET传输 按照UVC协议生成请求数据
	 */
	 /* 协议
	   UVC 1.5  表4-75视频探测和提交控件
	   Offset   Field           Size     Value      Description
       0        bmHint          2       Bitmap      位域控制，向功能指示哪些字段应保持固定（仅指示性）：
                                                       D0: dwFrameInterval
                                                       D1: wKeyFrameRate
                                                       D2: wPFrameRate
                                                       D3: wCompQuality
                                                       D4: wCompWindowSize
                                                       D15..5: Reserved (0)
                                                    提示位图向视频流接口指示在流参数协商期间哪些字段应保持恒定。 例如，如果选择要使帧速率胜于质量，则dwFrameInterval设为1。
                                                    该字段由主机设置，并且仅对于视频流接口是只读的。
       2        bFormatIndex    1      Number       来自此视频接口的格式描述符的视频格式索引。 通过将此字段设置为关联的Format描述符的基于一的索引来选择特定的视频流格式。 要选择设备定义的第一种格式，将一个（1）值写入此字段。 即使设备仅支持一种视频格式，也必须支持此字段。
                                                    该字段由主机设置。
       3        bFrameIndex     1      Number       来自帧描述符的视频帧索引。
                                                    该字段从所选流支持的分辨率阵列中选择视频帧分辨率。 索引值的范围是1到特定格式描述符之后的帧描述符数。 对于基于帧的格式，即使设备仅支持一个视频帧索引，也必须支持此字段。
                                                    对于没有定义帧描述符的视频有效载荷，该字段应设置为零（0）。
                                                    来自帧描述符的视频帧索引。
       4        dwFrameInterval 4      Number       帧间隔，以100ns为单位。
                                                    该字段为选定的视频流和帧索引设置所需的视频帧间隔。 帧间隔值以100ns为单位指定。设备应支持设置在帧描述符中报告的与所选视频帧索引相对应的所有帧间隔。 对于基于帧的格式，即使设备仅支持一个视频帧间隔，也必须实现此字段。
                                                    当与IN端点结合使用时，主机应在探测阶段指示其偏好。 该值必须在设备支持的值范围内。当与OUT端点结合使用时，主机应接受设备指示的值。
       8        wKeyFrameRate   2      Number       每个视频帧单位的关键帧中的关键帧速率。
                                                    该字段仅适用于能够以可调压缩参数流式传输视频的源（和格式）。该控件的使用由设备决定，并在VS输入或输出标头描述符中指示。
                                                    “关键帧速率”字段用于指定压缩器的关键帧速率。例如，如果视频流序列中每十个编码帧中的一个是关键帧，则此控件将报告值10。值0表示仅第一帧是关键帧。
                                                    当与IN端点结合使用时，主机应在探测阶段指示其偏好。 该值必须在设备支持的值范围内。当与OUT端点结合使用时，主机应接受设备指示的值。
       10       wPFrameRate     2      Number       PFrame速率，以PFrame/关键帧为单位。
                                                    该字段仅适用于能够以可调压缩参数流式传输视频的源（和格式）。该控件的使用由设备决定，并在VS输入或输出标头描述符中指示。
                                                    P帧速率控制用于指定每个关键帧的P帧数。 作为编码帧的类型之间的关系的示例，假设关键帧每10帧出现一次，并且每个关键帧有3个P帧。 P帧将在关键帧之间均匀间隔。
                                                    出现在关键帧和P帧之间的其他6帧将是双向（B）帧。
                                                    当与IN端点结合使用时，主机应在探测阶段指示其偏好。 该值必须在设备支持的值范围内。当与OUT端点结合使用时，主机应接受设备指示的值。
       12       wCompQuality    2     Number        压缩质量控制以抽象单位1（最低）到10000（最高）为单位。该字段仅适用于能够以可调压缩参数流式传输视频的源（和格式）。
                                                    该字段的使用由设备决定，并在VS输入或输出标头描述符中指示。此字段用于指定视频压缩的质量。 此属性的值的范围是1到10000（1表示最低质量，10000表示最高质量）。 此控件报告的分辨率将确定它可以支持的离散质量设置的数量。
                                                    当与IN端点结合使用时，主机应在探测阶段指示其偏好。 该值必须在设备支持的值范围内。当与OUT端点结合使用时，主机应接受设备指示的值。
       14       wCompWindowSize 2     Number        用于平均比特率控制的窗口大小。
                                                    该字段仅适用于能够以可调压缩参数流式传输视频的源（和格式）。该控件的使用由设备决定，并在VS输入或输出标头描述符中指示。
                                                    压缩窗口大小控件用于指定平均大小不能超过指定数据速率的编码视频帧数。对于大小为n的窗口，任何连续n个帧的平均帧大小将不超过流的指定数据速率。
                                                    各个框架可以更大或更小。例如，如果已将压缩窗口大小为10的每秒10帧（fps）的电影的数据速率设置为每秒100 KB（kbps），则各个帧可以是任何大小，只要平均大小任何10帧序列中的一帧的最大字节数小于或等于10 KB。
                                                    当与IN端点结合使用时，主机应在探测阶段指示其偏好。该值必须在设备支持的值范围内。当与OUT端点结合使用时，主机应接受设备指示的值。
       16       wDelay          2     Number        从视频数据捕获到USB演示，内部视频流接口延迟（毫秒）。
                                                    与IN端点结合使用时，此字段由设备设置，并且仅从主机读取。与OUT端点结合使用时，此字段由主机设置，并且仅从设备读取。
       18       dwMaxVideoFrameSize 4     Number    视频帧或编解码器特定的最大段大小（以字节为单位）。
                                                    对于基于帧的格式，此字段指示单个视频帧的最大大小。 在进行流式联播时，此数字反映协商的帧描述符的最大视频帧大小。 对于基于帧的格式，必须支持此字段。
                                                    对于基于流的格式，当通过bmFramingInfo字段（如下）启用此行为时，此字段指示单个编解码器特定段的最大大小。 发送方需要通过有效载荷报头中的FID位指示段边界。如果未启用bmFramingInfo位，则将忽略此字段（对于基于流的格式）。
                                                    与IN端点结合使用时，此字段由设备设置，并且仅从主机读取。与OUT端点结合使用时，此字段由主机设置，并且仅从设备读取。
       22       dwMaxPayloadTransferSize 4 Number   指定设备在一次有效负载传输中可以发送或接收的最大字节数。 必须支持该字段。
                                                    该字段由设备设置，并且只能从主机读取。 某些主机实现会限制此字段允许的最大值。 主机应通过重新配置设备来避免单个有效载荷传输大小的过冲。（例如，通过更新比特率，分辨率等）
       26       dwClockFrequency 4  Number          指定格式的设备时钟频率（以Hz为单位）。 这将在数据流的视频有效负载标题中指定用于时间信息字段的单位。
                                                    该参数由设备设置，并且只能从主机读取。
       30       bmFramingInfo   1     Bitmap        位域控件支持以下值：
                                                    D0：如果设置为1，则在有效载荷标题中需要帧ID（FID）字段（请参见第2.4.3.3节“视频和静止图像有效载荷标题”中对D0的描述）。需要发送者至少每隔dwMaxVideoFrameSize个字节切换帧ID（请参见上文）。
                                                    D1：如果设置为1，则表示有效载荷标题中可能存在帧末尾（EOF）字段（请参见第2.4.3.3节“视频和静止图像有效载荷标题”中对D1的描述）。在不指定D0的情况下指定该位是错误的。
                                                    D2：如果设置为1，则表示净荷报头中可能存在分片结尾（EOS）字段。在不指定D0的情况下指定该位是错误的。
                                                    D7..3：保留（0）
                                                    该控件向功能指示有效载荷传输是否将在视频有效载荷报头中包含带外成帧信息（请参见第2.4.3.3节“视频和静止图像有效载荷报头”）。对于已知的基于帧的格式（例如MJPEG，未压缩，DV），此字段将被忽略。
                                                    对于已知的基于流的格式，此字段允许发送方指示它将标识流中的段边界，从而使接收方可以进行低延迟缓冲区处理，而无需解析流本身。
                                                    与IN端点结合使用时，此控件由设备设置，并且对主机是只读的。与OUT端点结合使用时，此参数由主机设置，并且对设备是只读的。
       31       bPreferedVersion 1  Number          主机或设备为指定的bFormatIndex值支持的首选有效负载格式版本。
                                                    此参数允许主机和设备协商与bFormatIndex字段关联的有效载荷格式的相互认可的版本。主机对此进行初始化，并在第一个探针集上将随后的bMinVersion和bMaxVersion字段初始化为零。获取探测信息后，设备应返回其首选版本，以及设备支持的最低和最高版本（请参见下面的bMinVersion和bMaxVersion）。
                                                    主机可以发出后续的探针设置/获取序列以指定其首选版本（在初始探针设置/获取序列的bMinVersion和bMaxVersion返回的范围内）。不允许主机更改bMinVersion和bMaxVersion值。
                                                    该字段将支持单个有效负载格式的多达256（1-255）个版本。版本号来自有效载荷格式规范的次要版本。例如，有效载荷格式规范的1.2版将导致此参数的值为2。
       32       bMinVersion     1   Number          设备支持的指定bFormatIndex值的最小有效负载格式版本。
                                                    该值由主机初始化为零，并由设备重置为1到255之间的值。 不允许主机修改该值（通过将bPreferredVersion，bMinVersion和bMaxVersion设置为零来重新开始协商）。
       33       bMaxVersion     1   Number          设备支持的指定bFormatIndex值的最大有效负载格式版本。
                                                    该值由主机初始化为零，并由设备重置为1到255之间的值。 不允许主机修改该值（通过将bPreferredVersion，bMinVersion和bMaxVersion设置为零来重新开始协商）。
       34       bUsage          1   Number          当前使用情况：
                                                    1-8：实时模式
                                                    9-16：广播模式
                                                    17-24：文件存储模式
                                                    25 – 31：多视图模式
                                                    32-255：保留
                                                    此位图启用由视频帧描述符的bmUsages字段报告的功能。对于时间编码的视频格式，即使设备仅支持bUsage的单个值，也必须支持此字段。
       35       bBitDepthLuma   1   Number          表示bit_depth_luma_minus8 + 8，必须与bit_depth_chroma_minus8 + 8相同。
       36       bmSettings      1   Bitmap          标志的位图，用于发现和控制时间编码视频流的特定功能。 如果支持，则在关联的有效负载规范中定义。 此位图启用由视频帧描述符的bmCapabilities字段报告的功能。
                                                    对于时间编码的视频格式，必须支持此字段。
       37   bMaxNumberOfRefFramesPlus1 1 Number     主机指示存储的最大帧数，以用作参考。
       38   bmRateControlModes  2   Number          该字段包含4个子字段，每个子字段都是4位数字。它启用了由视频格式描述符的bmSupportedRateControlModes字段报告的功能。
                                                    每个4位数字表示编码视频流的速率控制模式。如果视频有效负载不支持速率控制，则应将整个字段设置为0。bmRateControlModes最多支持四个同时广播流。对于联播传输，从bmLayoutPerStream字段推断出流的数量。否则，流数为1。
                                                    D3-D0：第一个联播流的速率控制模式（stream_id = 0。）
                                                    D7-D4：第二个联播流的码率控制模式（stream_id = 1）。
                                                    D11-D8：第三联播流的速率控制模式（stream_id = 2）。
                                                    D15-D12：第四联播流的速率控制模式（stream_id = 3。）
                                                    当bmRateControlModes不为零时，每个4位子字段可以采用以下值之一：
                                                    0：不适用，因为此流不存在。
                                                    1：允许下溢的VBR
                                                    2：CBR
                                                    3：恒定QP
                                                    4：全局VBR，允许下溢
                                                    5：VBR无下溢
                                                    6：全局VBR，无下溢
                                                    7-15：保留
                                                    对于时间编码的视频格式，即使设备仅支持bmRateControlModes的单个值，也必须支持此字段。
       40   bmLayoutPerStream   8   Number          该字段包含4个子字段，每个子字段均为2字节数字。
                                                    对于联播传输，此字段指示每个流（最多四个联播流）的特定分层结构。 对于单个多层流，仅使用前两个字节。 对于没有增强层的单个流，该字段应设置为0。有关如何解释每个2字节子字段的信息，请参见各个有效负载规范。
                                                    对于时间编码的视频格式，必须支持此字段。

      设备将不支持的字段设置为零。 主机将流参数协商剩余的字段设置为零。 例如，在SET_CUR请求初始化FormatIndex和FrameIndex之后，设备将在检索Probe控件GET_CUR属性时返回受支持字段的新协商字段值。
      为了避免协商循环，设备应始终以降低的数据速率要求返回流参数。 流协商接口应根据协商循环避免规则将不支持的流参数重置为支持的值。 此约定允许主机在字段的受支持值之间循环。
	 */
	if (req == UVC_SET_CUR) {
		SHORT_TO_SW(ctrl->bmHint, buf);// wLength
		buf[2] = ctrl->bFormatIndex; // bFormatIndex
		buf[3] = ctrl->bFrameIndex;
		INT_TO_DW(ctrl->dwFrameInterval, buf + 4);
		SHORT_TO_SW(ctrl->wKeyFrameRate, buf + 8);
		SHORT_TO_SW(ctrl->wPFrameRate, buf + 10);
		SHORT_TO_SW(ctrl->wCompQuality, buf + 12);
		SHORT_TO_SW(ctrl->wCompWindowSize, buf + 14);
		SHORT_TO_SW(ctrl->wDelay, buf + 16);
		INT_TO_DW(ctrl->dwMaxVideoFrameSize, buf + 18);
		INT_TO_DW(ctrl->dwMaxPayloadTransferSize, buf + 22);

		if (len > 26) {	// len == 34
			// XXX add to support UVC 1.1
			INT_TO_DW(ctrl->dwClockFrequency, buf + 26);
			buf[30] = ctrl->bmFramingInfo;
			buf[31] = ctrl->bPreferedVersion;
			buf[32] = ctrl->bMinVersion;
			buf[33] = ctrl->bMaxVersion;
			if (len == 48) {
				// XXX add to support UVC1.5
				buf[34] = ctrl->bUsage;
				buf[35] = ctrl->bBitDepthLuma;
				buf[36] = ctrl->bmSettings;
				buf[37] = ctrl->bMaxNumberOfRefFramesPlus1;
				SHORT_TO_SW(ctrl->bmRateControlModes, buf + 38);
				LONG_TO_QW(ctrl->bmLayoutPerStream, buf + 40);
			}
		}
	}

	/* do the transfer
	 * 做传输 发送请求数据
	 */
	err = libusb_control_transfer(devh->usb_devh,
			req == UVC_SET_CUR ? 0x21 : 0xA1, req,
			probe ? (UVC_VS_PROBE_CONTROL << 8) : (UVC_VS_COMMIT_CONTROL << 8),
			ctrl->bInterfaceNumber, buf, len, 0);

	if (UNLIKELY(err <= 0)) {
		// when libusb_control_transfer returned error or transfer bytes was zero.
		// 当libusb_control_transfer返回错误或传输字节为零时。
		if (!err) {
			UVC_DEBUG("libusb_control_transfer transfered zero length data");
			err = UVC_ERROR_OTHER;
		}
		return err;
	}
	if (err < len) {
#if !defined(__LP64__)
		LOGE("transfered bytes is smaller than data bytes:%d expected %d", err, len);
#else
		LOGE("transfered bytes is smaller than data bytes:%d expected %ld", err, len);
#endif
		return UVC_ERROR_OTHER;
	}
	/* now decode following a GET transfer
	 * 在GET传输后解码
	 * 设置设备返回的结果
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

		if (len > 26) {	// len == 34
			// XXX add to support UVC 1.1
			ctrl->dwClockFrequency = DW_TO_INT(buf + 26);
			ctrl->bmFramingInfo = buf[30];
			ctrl->bPreferedVersion = buf[31];
			ctrl->bMinVersion = buf[32];
			ctrl->bMaxVersion = buf[33];
			if (len >= 48) {
				// XXX add to support UVC1.5
				ctrl->bUsage = buf[34];
				ctrl->bBitDepthLuma = buf[35];
				ctrl->bmSettings = buf[36];
				ctrl->bMaxNumberOfRefFramesPlus1 = buf[37];
				ctrl->bmRateControlModes = SW_TO_SHORT(buf + 38);
				ctrl->bmLayoutPerStream = QW_TO_LONG(buf + 40);
			}
		}

		/* fix up block for cameras that fail to set dwMax
		 * 修复无法设置 dwMax 的摄像机的模块
		 * 设备没有返回 dwMaxVideoFrameSize 值或返回值为0，则通过帧描述获取最大视频帧缓存大小
		 */
		if (!ctrl->dwMaxVideoFrameSize) {
			LOGW("fix up block for cameras that fail to set dwMax");
			uvc_frame_desc_t *frame_desc = uvc_find_frame_desc(devh, ctrl->bFormatIndex, ctrl->bFrameIndex);

			if (frame_desc) {
				ctrl->dwMaxVideoFrameSize = frame_desc->dwMaxVideoFrameBufferSize;
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

	if (UNLIKELY(strmh->stream_if->bInterfaceNumber != ctrl->bInterfaceNumber)) {
	    return UVC_ERROR_INVALID_PARAM;
	}

	/* @todo Allow the stream to be modified without restarting the stream
	 * 允许修改流而不重新启动流
	 */
	if (UNLIKELY(strmh->running)) {
	    return UVC_ERROR_BUSY;
	}

    // 获取设备的流接口探测状态参数
	ret = uvc_query_stream_ctrl(strmh->devh, ctrl, 0, UVC_SET_CUR);
	if (UNLIKELY(ret != UVC_SUCCESS)) {
	    return ret;
	}

	strmh->cur_ctrl = *ctrl;
	return UVC_SUCCESS;
}

/** @internal
 * @brief Find the descriptor for a specific frame configuration
 * 查找特定帧配置的描述符
 * @param stream_if Stream interface
 * @param format_id Index of format class descriptor
 * @param frame_id Index of frame descriptor
 */
static uvc_frame_desc_t *_uvc_find_frame_desc_stream_if(
		uvc_streaming_interface_t *stream_if, uint16_t format_id,
		uint16_t frame_id) {

	uvc_format_desc_t *format = NULL;
	uvc_frame_desc_t *frame = NULL;

	DL_FOREACH(stream_if->format_descs, format)
	{
		if (format->bFormatIndex == format_id) {
			DL_FOREACH(format->frame_descs, frame)
			{
				if (frame->bFrameIndex == frame_id) {
				    return frame;
				}
			}
		}
	}

	return NULL ;
}

uvc_error_t uvc_get_frame_desc(uvc_device_handle_t *devh,
		uvc_stream_ctrl_t *ctrl, uvc_frame_desc_t **desc) {

	*desc = uvc_find_frame_desc(devh, ctrl->bFormatIndex, ctrl->bFrameIndex);
	return *desc ? UVC_SUCCESS : UVC_ERROR_INVALID_PARAM;
}

uvc_frame_desc_t *uvc_find_frame_desc_stream(uvc_stream_handle_t *strmh,
		uint16_t format_id, uint16_t frame_id) {
	//查找特定帧配置的描述符
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

	DL_FOREACH(devh->info->stream_ifs, stream_if)
	{
	    //查找特定帧配置的描述符
		frame = _uvc_find_frame_desc_stream_if(stream_if, format_id, frame_id);
		if (frame) {
		    return frame;
		}
	}

	return NULL;
}

static void _uvc_print_streaming_interface_one(uvc_streaming_interface_t *stream_if) {
//	struct uvc_device_info *parent;
//	struct uvc_streaming_interface *prev, *next;
	MARK("bInterfaceNumber:%d", stream_if->bInterfaceNumber);
	uvc_print_format_desc_one(stream_if->format_descs, NULL);
	MARK("bEndpointAddress:%d", stream_if->bEndpointAddress);
	MARK("bTerminalLink:%d", stream_if->bTerminalLink);
}

static uvc_error_t _prepare_stream_ctrl(uvc_device_handle_t *devh, uvc_stream_ctrl_t *ctrl) {
	// XXX some camera may need to call uvc_query_stream_ctrl with UVC_GET_CUR/UVC_GET_MAX/UVC_GET_MIN
	// before negotiation otherwise stream stall. added by saki
	// 一些相机可能需要在协商之前用 UVC_GET_CUR/UVC_GET_MAX/UVC_GET_MIN 调用 uvc_query_stream_ctrl，否则流停顿。
	// 获取设备的流接口的当前状态
	uvc_error_t result = uvc_query_stream_ctrl(devh, ctrl, 1, UVC_GET_CUR);	// probe query
	if (LIKELY(!result)) {
	    // 获取设备的协商字段的最小值
		result = uvc_query_stream_ctrl(devh, ctrl, 1, UVC_GET_MIN);			// probe query
		if (LIKELY(!result)) {
		    // 获取设备的协商字段的最大值
			result = uvc_query_stream_ctrl(devh, ctrl, 1, UVC_GET_MAX);		// probe query
			if (UNLIKELY(result))
				LOGE("uvc_query_stream_ctrl:UVC_GET_MAX:err=%d", result);	// XXX 最大値の方を後で取得しないとだめ 您稍后必须获得最大值
		} else {
			LOGE("uvc_query_stream_ctrl:UVC_GET_MIN:err=%d", result);
		}
	} else {
		LOGE("uvc_query_stream_ctrl:UVC_GET_CUR:err=%d", result);
	}
	return result;
}

// 获取流控制格式
static uvc_error_t _uvc_get_stream_ctrl_format(uvc_device_handle_t *devh,
	uvc_streaming_interface_t *stream_if, uvc_stream_ctrl_t *ctrl, uvc_format_desc_t *format,
	const int width, const int height,
	const int min_fps, const int max_fps) {

	ENTER();

	int i;
	uvc_frame_desc_t *frame;

	ctrl->bInterfaceNumber = stream_if->bInterfaceNumber;
	uvc_error_t result = uvc_claim_if(devh, ctrl->bInterfaceNumber);
	if (UNLIKELY(result)) {
		LOGE("uvc_claim_if:err=%d", result);
		goto fail;
	}
	for (i = 0; i < 2; i++) {
		result = _prepare_stream_ctrl(devh, ctrl);
	}
	if (UNLIKELY(result)) {
		LOGE("_prepare_stream_ctrl:err=%d", result);
		goto fail;
	}
	DL_FOREACH(format->frame_descs, frame)
	{
		if (frame->wWidth != width || frame->wHeight != height)
			continue;

		uint32_t *interval;

		if (frame->intervals) {
			for (interval = frame->intervals; *interval; ++interval) {
				if (UNLIKELY(!(*interval))) continue;
				uint32_t it = 10000000 / *interval;
				LOGV("it:%d", it);
				if ((it >= (uint32_t) min_fps) && (it <= (uint32_t) max_fps)) {
					ctrl->bmHint = (1 << 0); /* don't negotiate interval 不协商间隔 */
					ctrl->bFormatIndex = format->bFormatIndex;
					ctrl->bFrameIndex = frame->bFrameIndex;
					ctrl->dwFrameInterval = *interval;

					goto found;
				}
			}
		} else {
			int32_t fps;
			for (fps = max_fps; fps >= min_fps; fps--) {
				if (UNLIKELY(!fps)) continue;
				uint32_t interval_100ns = 10000000 / fps;
				uint32_t interval_offset = interval_100ns - frame->dwMinFrameInterval;
				LOGV("fps:%d", fps);
				if (interval_100ns >= frame->dwMinFrameInterval
					&& interval_100ns <= frame->dwMaxFrameInterval
					&& !(interval_offset
						&& (interval_offset % frame->dwFrameIntervalStep) ) ) {
					/* don't negotiate interval
					 * 不协商间隔
					 */
					ctrl->bmHint = (1 << 0);
					ctrl->bFormatIndex = format->bFormatIndex;
					ctrl->bFrameIndex = frame->bFrameIndex;
					ctrl->dwFrameInterval = interval_100ns;

					goto found;
				}
			}
		}
	}
	result = UVC_ERROR_INVALID_MODE;
fail:
	uvc_release_if(devh, ctrl->bInterfaceNumber);
	RETURN(result, uvc_error_t);

found:
	RETURN(UVC_SUCCESS, uvc_error_t);
}

/**
 * Get a negotiated streaming control block for some common parameters.
 * @ingroup streaming
 * 获取一些常用参数的协商流控制块。
 *
 * @param[in] devh Device handle
 * @param[in,out] ctrl Control block
 * @param[in] cf Type of streaming format
 * @param[in] width Desired frame width
 * @param[in] height Desired frame height
 * @param[in] fps Frame rate, frames per second
 */
uvc_error_t uvc_get_stream_ctrl_format_size(uvc_device_handle_t *devh,
		uvc_stream_ctrl_t *ctrl, enum uvc_frame_format cf, int width, int height, int fps) {

	return uvc_get_stream_ctrl_format_size_fps(devh, ctrl, cf, width, height, fps, fps);
}

/**
 * Get a negotiated streaming control block for some common parameters.
 * @ingroup streaming
 * 获取一些常用参数的协商流控制块。
 *
 * @param[in] devh Device handle
 * @param[in,out] ctrl Control block
 * @param[in] cf Type of streaming format
 * @param[in] width Desired frame width
 * @param[in] height Desired frame height
 * @param[in] min_fps Frame rate, minimum frames per second, this value is included
 * @param[in] max_fps Frame rate, maximum frames per second, this value is included
 */
uvc_error_t uvc_get_stream_ctrl_format_size_fps(uvc_device_handle_t *devh,
		uvc_stream_ctrl_t *ctrl, enum uvc_frame_format cf, int width,
		int height, int min_fps, int max_fps) {

	ENTER();

	uvc_streaming_interface_t *stream_if;
	uvc_error_t result;

	memset(ctrl, 0, sizeof(*ctrl));	// XXX add
	/* find a matching frame descriptor and interval
	 * 查找匹配的帧描述符和间隔
	 */
	uvc_format_desc_t *format;
	DL_FOREACH(devh->info->stream_ifs, stream_if)
	{
		DL_FOREACH(stream_if->format_descs, format)
		{
			if (!_uvc_frame_format_matches_guid(cf, format->guidFormat)){
			    // 没有找到匹配的帧格式
			    continue;
			}
            // 获取流控制格式
			result = _uvc_get_stream_ctrl_format(devh, stream_if, ctrl, format, width, height, min_fps, max_fps);
			if (!result) {	// UVC_SUCCESS
				goto found;
			}
		}
	}

	RETURN(UVC_ERROR_INVALID_MODE, uvc_error_t);

found:
    // 与设备协商流参数
	RETURN(uvc_probe_stream_ctrl(devh, ctrl), uvc_error_t);
}

/** @internal
 * Negotiate streaming parameters with the device
 * 与设备协商流参数
 *
 * @param[in] devh UVC device
 * @param[in,out] ctrl Control block
 */
uvc_error_t uvc_probe_stream_ctrl(uvc_device_handle_t *devh,
		uvc_stream_ctrl_t *ctrl) {
	uvc_error_t err;
	// 声明UVC接口，必要时分离内核驱动程序。
	err = uvc_claim_if(devh, ctrl->bInterfaceNumber);
	if (UNLIKELY(err)) {
		LOGE("uvc_claim_if:err=%d", err);
		return err;
	}

    // 获取设备的流接口探测状态参数
	err = uvc_query_stream_ctrl(devh, ctrl, 1, UVC_SET_CUR);	// probe query
	if (UNLIKELY(err)) {
		LOGE("uvc_query_stream_ctrl(UVC_SET_CUR):err=%d", err);
		return err;
	}

    // 获取设备的流接口的当前状态
	err = uvc_query_stream_ctrl(devh, ctrl, 1, UVC_GET_CUR);	// probe query ここでエラーが返ってくる 错误在这里返回
	if (UNLIKELY(err)) {
		LOGE("uvc_query_stream_ctrl(UVC_GET_CUR):err=%d", err);
		return err;
	}

	return UVC_SUCCESS;
}

/** @internal
 * @brief Swap the working buffer with the presented buffer and notify consumers
 * 用提供的缓冲区交换工作缓冲区并通知使用者
 */
static void _uvc_swap_buffers(uvc_stream_handle_t *strmh, int broadcast) {
	uint8_t *tmp_buf;

	pthread_mutex_lock(&strmh->cb_mutex);
	{
		/* swap the buffers
		 * 交换缓冲区
		 */
		tmp_buf = strmh->holdbuf;
		strmh->hold_bfh_err = strmh->bfh_err;	// XXX
		strmh->hold_bytes = strmh->got_bytes;
		strmh->holdbuf = strmh->outbuf;
		strmh->outbuf = tmp_buf;
		strmh->hold_last_stc = strmh->last_stc;
		strmh->hold_pts = strmh->pts;
		strmh->hold_seq = strmh->seq;

        if(broadcast == 1){
            // 唤醒所有等待视频帧线程
            pthread_cond_broadcast(&strmh->cb_cond);
        }
	}
	pthread_mutex_unlock(&strmh->cb_mutex);

	strmh->seq++;
	strmh->got_bytes = 0;
	strmh->last_stc = 0;
	strmh->bfh_err = 0;	// XXX
}

static void _uvc_delete_transfer(struct libusb_transfer *transfer) {
	ENTER();

//	MARK("");
	uvc_stream_handle_t *strmh = transfer->user_data;
	if (UNLIKELY(!strmh)) EXIT();		// XXX
	int i;

	pthread_mutex_lock(&strmh->cb_mutex);	// XXX crash while calling uvc_stop_streaming 调用uvc_stop_streaming时崩溃
	{
		// 将传输标记为已删除。
		for (i = 0; i < LIBUVC_NUM_TRANSFER_BUFS; i++) {
			if (strmh->transfers[i] == transfer) {
			    // usb取消传输数据
				libusb_cancel_transfer(strmh->transfers[i]);	// XXX 20141112追加
				UVC_DEBUG("Freeing transfer %d (%p)", i, transfer);
				free(transfer->buffer);
				libusb_free_transfer(transfer);
				strmh->transfers[i] = NULL;
				break;
			}
		}
		if (UNLIKELY(i == LIBUVC_NUM_TRANSFER_BUFS)) {
			UVC_DEBUG("transfer %p not found; not freeing!", transfer);
		}
        // 唤醒所有等待视频帧线程
		pthread_cond_broadcast(&strmh->cb_cond);
	}
	pthread_mutex_unlock(&strmh->cb_mutex);
	EXIT();
}

/** @internal
 * @brief Process a payload transfer
 * 处理有效载荷传输。块模式传输，因此只有一个有效负载传输
 * 从数据中按照UVC协议解析视频帧数据
 * 
 * Processes stream, places frames into buffer, signals listeners
 * (such as user callback thread and any polling thread) on new frame
 * 处理流，将帧放入缓冲区，在新帧上向侦听器（例如用户回调线程和任何轮询线程）发出信号
 *
 * @param payload Contents of the payload transfer, either a packet (isochronous) or a full
 * transfer (bulk mode)
 * @param payload_len Length of the payload transfer
 */
static void _uvc_process_payload(uvc_stream_handle_t *strmh, const uint8_t *payload, size_t const payload_len) {
	size_t header_len;
	uint8_t header_info;
	size_t data_len;
	struct libusb_iso_packet_descriptor *pkt;
	uvc_vc_error_code_control_t vc_error_code;
	uvc_vs_error_code_control_t vs_error_code;

	// magic numbers for identifying header packets from some iSight cameras
	// 魔术数字，用于识别某些iSight摄像机的标头数据包
	static const uint8_t isight_tag[] = {
		0x11, 0x22, 0x33, 0x44,
		0xde, 0xad, 0xbe, 0xef, 0xde, 0xad, 0xfa, 0xce
	};

	// ignore empty payload transfers
	// 忽略空的有效载荷传输
	if (UNLIKELY(!payload || !payload_len || !strmh->outbuf)) {
	    return;
	}

	/* 添加数据打印，用于调试，测完后应删除 start */
#if DLY_DEBUG == 1
	char *printStr = (char *) malloc(200);
	memset(printStr, '\0', 200);
	const uint8_t * printPtr = payload;
	int index = 0;
	for(int pi = 0; pi <payload_len; pi++, printPtr++){
	    printStr = uint8_to_hex(*printPtr, printStr, &index);
	    if(pi >= 50){
	        break;
	    }
	}
	*(printStr + index) = '\0';
	LOGE("_uvc_process_payload 打印每个数据包数据 %s\n", printStr);
	free(printStr);
#endif
	/* 添加数据打印，用于调试，测完后应删除 end */

	/* Certain iSight cameras have strange behavior: They send header
	 * information in a packet with no image data, and then the following
  	 * packets have only image data, with no more headers until the next frame.
	 *
	 * The iSight header: len(1), flags(1 or 2), 0x11223344(4),
	 * 0xdeadbeefdeadface(8), ??(16)
	 * 某些iSight摄像机的行为很奇怪：它们在没有图像数据的数据包中发送标头信息，然后以下数据包仅包含图像数据，直到下一帧才有标头。
	 * iSight标头：len(1), flags(1 or 2), 0x11223344(4), 0xdeadbeefdeadface(8), ??(16)
	*/

    /*
        UVC 1.1 增加 有效载荷标题的格式 Format of the Payload Header
        Offset      Field           Size    Value   Description
        0       bHeaderLength       1       Number  有效负载头的长度（以字节为单位），包括此字段。
        1       bmHeaderInfo        1       Bitmap  提供有关标题后面的示例数据的信息，以及此标题中可选标题字段的可用性。
                                                    D0：帧ID —— 对于基于帧的格式，每次新的视频帧开始时，此位在0和1之间切换。对于基于流的格式，在每个新的特定于编解码器的段的开头，该位在0和1之间切换。
                                                        此行为对于基于帧的有效载荷格式（例如DV）是必需的，对于基于流的有效载荷格式（例如MPEG-2 TS）是可选的。对于基于流的格式，必须通过“视频探测和提交”控件的bmFramingInfo字段指示对此位的支持（请参见第4.3.1.1节“视频探测和提交控件”）。
                                                    D1：帧结束 —— 如果以下有效载荷数据标记了当前视频或静止图像帧的结束（对于基于帧的格式），或者指示编解码器特定段的结束（对于基于流的帧），则该位置1格式）。对于所有有效负载格式，此行为都是可选的。对于基于流的格式，必须通过“视频探测和提交控件”的bmFramingInfo字段指示对此位的支持（请参见第4.3.1.1节“视频探测和提交控件”）。
                                                    D2：演示时间 —— 如果dwPresentationTime字段作为标题的一部分发送，则此位置1。
                                                    D3：源时钟参考 —— 如果dwSourceClock字段作为标题的一部分发送，则此位置1。
                                                    D4：有效负载特定位。 请参阅各个有效负载规格以进行使用。
                                                    D5：静止图像 —— 如果随后的数据是静止图像帧的一部分，则此位置1，仅用于静止图像捕获的方法2和3。 对于时间编码格式，该位指示随后的数据是帧内编码帧的一部分。
                                                    D6：错误 —— 如果此有效负载的视频或静止图像传输出现错误，则此位置1。 流错误代码控件将反映错误原因。
                                                    D7：标头结尾 —— 如果这是数据包中的最后一个标头组，则设置此位，其中标头组引用此字段以及由该字段中的位标识的任何可选字段（定义为将来扩展）。

        注意 标头中可能包含以下字段，也可能不包含以下字段，具体取决于上面bmHeaderInfo字段中指定的位。 这些字段的顺序是在上面的位图标题字段中指定的顺序，即最低有效位在先。 由于标头本身可能会在将来扩展，因此dwPresentationTime的偏移量也是可变的。 设备将在特定于类的VideoStreaming描述符内的有效负载格式描述符中指示是否支持这些字段。 请参见第3.9.2.3节“有效载荷格式描述符”。

        Variable dwPresentationTime 4       Number  演示时间戳（PTS）。 开始原始帧捕获时，以本机设备时钟为单位的源时钟时间。 对于包括单个视频帧的多个有效载荷传输，可以重复此字段，但要限制该值在整个视频帧中保持相同。 PTS是以与视频探针控制响应的dwClockFrequency字段中指定的单位相同。
        Variable scrSourceClock     6       Number  两部分的源时钟参考（SCR）值
                                                    D31..D0：源时间时钟，以本机设备时钟为单位
                                                    D42..D32：1KHz SOF令牌计数器
                                                    D47..D43：保留，设置为零。
                                                    最低有效的32位（D31..D0）包含从源处的系统时间时钟（STC）采样的时钟值。时钟分辨率应根据本规范表4-75中定义的设备的探测和提交响应的dwClockFrequency字段。
                                                    该值应符合相关的流有效载荷规范。采样STC的时间必须与USB总线时钟相关联。为此，SCR的下一个最高11位（D42..D32）包含一个1 KHz SOF计数器，该计数器表示对STC进行采样时的帧号。
                                                    * 当视频帧的第一个视频数据为放在USB总线上。
                                                    * 对于单个负载内的所有有效负载传输，SCR必须保持恒定视频帧。
                                                    保留最高有效的5位（D47..D43），并且必须将其设置为零。包含SCR值的有效负载报头之间的最大间隔为100ms或视频帧间隔，以较大者为准。允许间隔更短。
    */

	if (UNLIKELY(strmh->devh->is_isight &&
		((payload_len < 14) || memcmp(isight_tag, payload + 2, sizeof(isight_tag)) ) &&
		((payload_len < 15) || memcmp(isight_tag, payload + 3, sizeof(isight_tag)) ) )) {
	    // 没有载体头(Payload Header)
		// The payload transfer doesn't have any iSight magic, so it's all image data
		// 有效负载传输没有任何iSight魔法，因此全部为图像数据
		header_len = 0;
		data_len = payload_len;
	} else {
	    // 有载体头(Payload Header)
		header_len = payload[0];

		if (UNLIKELY(header_len > payload_len)) {
			strmh->bfh_err |= UVC_STREAM_ERR;
			UVC_DEBUG("bogus packet: actual_len=%zd, header_len=%zd\n", payload_len, header_len);
			return;
		}

		if (UNLIKELY(strmh->devh->is_isight)) {
		    data_len = 0;
		} else {
		    data_len = payload_len - header_len;
		}
	}
	if (data_len == 0) {
		if(header_len<2 || !(payload[1] & UVC_STREAM_EOF) ){
		    // 数据包只有头，没有实体数据 且没有EOF标记
			return;
		}
	}

	if (UNLIKELY(header_len < 2)) {
	    // 没有载体头(Payload Header)
		header_info = 0;
	} else {
		//  @todo we should be checking the end-of-header bit 我们应该检查标题结束位=
		header_info = payload[1];

		if (UNLIKELY(header_info & UVC_STREAM_ERR)) {
		    // 错误 —— 如果此有效负载的视频或静止图像传输出现错误，则此位置1。 流错误代码控件将反映错误原因。
//			strmh->bfh_err |= UVC_STREAM_ERR;
			UVC_DEBUG("bad packet: error bit set");
			libusb_clear_halt(strmh->devh->usb_devh, strmh->stream_if->bEndpointAddress);
//			uvc_vc_get_error_code(strmh->devh, &vc_error_code, UVC_GET_CUR);
			uvc_vs_get_error_code(strmh->devh, &vs_error_code, UVC_GET_CUR);
//			return;
		}

        // 图像时间戳，同个视频帧多个数据包中保持相同
        uint32_t frame_pts = 0;
        // 系统时间时钟，采样的时钟值
        uint32_t frame_stc = 0;
        // 帧计数器，同个视频帧会有不相同
        uint32_t frame_sof = 0;

		if (header_info & UVC_STREAM_PTS) {
			// XXX saki some camera may send broken packet or failed to receive all data
			// saki某些相机可能发送了损坏的数据包或无法接收所有数据
			if (LIKELY(header_len >= 6)) {
				frame_pts = DW_TO_INT(payload + 2);
			} else {
				MARK("bogus packet: header info has UVC_STREAM_PTS, but no data");
				frame_pts = 0;
			}
		}
		if (header_info & UVC_STREAM_SCR) {
			// XXX saki some camera may send broken packet or failed to receive all data
			// saki某些相机可能发送了损坏的数据包或无法接收所有数据
			if (LIKELY(header_len >= 10)) {
				frame_stc = DW_TO_INT(payload + 6);
                if (LIKELY(header_len >= 12)) {
                    frame_sof = _11bits_TO_INT(payload + 10);
                }
			}
		}
		uint8_t frame_fid = header_info & UVC_STREAM_FID;
		if (strmh->got_bytes > 0 && (strmh->fid != frame_fid || strmh->pts != frame_pts)) {
			/* The frame ID bit was flipped, but we have image data sitting
				around from prior transfers. This means the camera didn't send
				an EOF for the last transfer of the previous frame.
				帧ID位被翻转，但是我们有来自先前传输的图像数据，这意味着相机在上一帧的最后传输中没有发送EOF。
				PTS不一样，这意味着相机在上一帧的最后传输中没有发送EOF。

				可能出现花帧了
			*/
			LOGE("_uvc_process_payload some frames losted 可能出现花帧了 \n");
			_uvc_swap_buffers(strmh, drop_incomplete_frame);
		}

		strmh->fid = frame_fid;
		strmh->pts = frame_pts;
		strmh->last_stc = frame_stc;
		strmh->sof = frame_sof;
	}

	if (LIKELY(data_len > 0)) {
		if (LIKELY(strmh->got_bytes + data_len < strmh->size_buf)) {
			memcpy(strmh->outbuf + strmh->got_bytes, payload + header_len, data_len);
			strmh->got_bytes += data_len;
		} else {
			strmh->bfh_err |= UVC_STREAM_ERR;
		}

		if (header_info & UVC_STREAM_EOF) {
			// The EOF bit is set, so publish the complete frame
			// EOF位置1，因此发布完整帧
			_uvc_swap_buffers(strmh, 1);
		}
	}
}

/**
 * 同步模式传输，每个数据包都有一个有效负载传输
 * 从数据中按照UVC协议解析视频帧数据
 */
static inline void _uvc_process_payload_iso(uvc_stream_handle_t *strmh, struct libusb_transfer *transfer) {
	/* per packet
	 * 包
	 */
	uint8_t *pktbuf;
	uint8_t check_header;
	size_t header_len;
	uint8_t header_info;
	struct libusb_iso_packet_descriptor *pkt;

    /*
        UVC 1.1 增加 有效载荷标题的格式 Format of the Payload Header
        Offset      Field           Size    Value   Description
        0       bHeaderLength       1       Number  有效负载头的长度（以字节为单位），包括此字段。
        1       bmHeaderInfo        1       Bitmap  提供有关标题后面的示例数据的信息，以及此标题中可选标题字段的可用性。
                                                    D0：帧ID —— 对于基于帧的格式，每次新的视频帧开始时，此位在0和1之间切换。对于基于流的格式，在每个新的特定于编解码器的段的开头，该位在0和1之间切换。
                                                        此行为对于基于帧的有效载荷格式（例如DV）是必需的，对于基于流的有效载荷格式（例如MPEG-2 TS）是可选的。对于基于流的格式，必须通过“视频探测和提交”控件的bmFramingInfo字段指示对此位的支持（请参见第4.3.1.1节“视频探测和提交控件”）。
                                                    D1：帧结束 —— 如果以下有效载荷数据标记了当前视频或静止图像帧的结束（对于基于帧的格式），或者指示编解码器特定段的结束（对于基于流的帧），则该位置1格式）。对于所有有效负载格式，此行为都是可选的。对于基于流的格式，必须通过“视频探测和提交控件”的bmFramingInfo字段指示对此位的支持（请参见第4.3.1.1节“视频探测和提交控件”）。
                                                    D2：演示时间 —— 如果dwPresentationTime字段作为标题的一部分发送，则此位置1。
                                                    D3：源时钟参考 —— 如果dwSourceClock字段作为标题的一部分发送，则此位置1。
                                                    D4：有效负载特定位。 请参阅各个有效负载规格以进行使用。
                                                    D5：静止图像 —— 如果随后的数据是静止图像帧的一部分，则此位置1，仅用于静止图像捕获的方法2和3。 对于时间编码格式，该位指示随后的数据是帧内编码帧的一部分。
                                                    D6：错误 —— 如果此有效负载的视频或静止图像传输出现错误，则此位置1。 流错误代码控件将反映错误原因。
                                                    D7：标头结尾 —— 如果这是数据包中的最后一个标头组，则设置此位，其中标头组引用此字段以及由该字段中的位标识的任何可选字段（定义为将来扩展）。

        注意 标头中可能包含以下字段，也可能不包含以下字段，具体取决于上面bmHeaderInfo字段中指定的位。 这些字段的顺序是在上面的位图标题字段中指定的顺序，即最低有效位在先。 由于标头本身可能会在将来扩展，因此dwPresentationTime的偏移量也是可变的。 设备将在特定于类的VideoStreaming描述符内的有效负载格式描述符中指示是否支持这些字段。 请参见第3.9.2.3节“有效载荷格式描述符”。

        Variable dwPresentationTime 4       Number  演示时间戳（PTS）。 开始原始帧捕获时，以本机设备时钟为单位的源时钟时间。 对于包括单个视频帧的多个有效载荷传输，可以重复此字段，但要限制该值在整个视频帧中保持相同。 PTS是以与视频探针控制响应的dwClockFrequency字段中指定的单位相同。
        Variable scrSourceClock     6       Number  两部分的源时钟参考（SCR）值
                                                    D31..D0：源时间时钟，以本机设备时钟为单位
                                                    D42..D32：1KHz SOF令牌计数器
                                                    D47..D43：保留，设置为零。
                                                    最低有效的32位（D31..D0）包含从源处的系统时间时钟（STC）采样的时钟值。时钟分辨率应根据本规范表4-75中定义的设备的探测和提交响应的dwClockFrequency字段。
                                                    该值应符合相关的流有效载荷规范。采样STC的时间必须与USB总线时钟相关联。为此，SCR的下一个最高11位（D42..D32）包含一个1 KHz SOF计数器，该计数器表示对STC进行采样时的帧号。
                                                    * 当视频帧的第一个视频数据为放在USB总线上。
                                                    * 对于单个负载内的所有有效负载传输，SCR必须保持恒定视频帧。
                                                    保留最高有效的5位（D47..D43），并且必须将其设置为零。包含SCR值的有效负载报头之间的最大间隔为100ms或视频帧间隔，以较大者为准。允许间隔更短。
    */

	/* magic numbers for identifying header packets from some iSight cameras
	 * 魔术数字，用于识别某些iSight摄像机的标头数据包
	 */
	static const uint8_t isight_tag[] = {
		0x11, 0x22, 0x33, 0x44, 0xde, 0xad,
		0xbe, 0xef, 0xde, 0xad, 0xfa, 0xce };
	int packet_id;
	uvc_vc_error_code_control_t vc_error_code;
	uvc_vs_error_code_control_t vs_error_code;

	for (packet_id = 0; packet_id < transfer->num_iso_packets; ++packet_id) {
		check_header = 1;

		pkt = transfer->iso_packet_desc + packet_id;

		if (UNLIKELY(pkt->status != 0)) {
			MARK("bad packet:status=%d,actual_length=%d", pkt->status, pkt->actual_length);
			strmh->bfh_err |= UVC_STREAM_ERR;
			libusb_clear_halt(strmh->devh->usb_devh, strmh->stream_if->bEndpointAddress);
//			uvc_vc_get_error_code(strmh->devh, &vc_error_code, UVC_GET_CUR);
//			uvc_vs_get_error_code(strmh->devh, &vs_error_code, UVC_GET_CUR);
			continue;
		}

		if (UNLIKELY(!pkt->actual_length)) {	// why transfered byte is zero...  传输字节为0
//			MARK("zero packet (transfer):");
//			strmh->bfh_err |= UVC_STREAM_ERR;	// don't set this flag here 不要在这里设置此标志
			continue;
		}
		// XXX accessing to pktbuf could lead to crash on the original implementation
		// because the substances of pktbuf will be deleted in uvc_stream_stop.
		// 访问pktbuf可能会导致原始实现崩溃，因为pktbuf的内容将在uvc_stream_stop中删除。
		pktbuf = libusb_get_iso_packet_buffer_simple(transfer, packet_id);
		if (LIKELY(pktbuf)) {	// XXX add null check because libusb_get_iso_packet_buffer_simple could return null  添加空检查，因为libusb_get_iso_packet_buffer_simple可能返回空

	/* 添加数据打印，用于调试，测完后应删除 start */
#if DLY_DEBUG == 1
            char *printStr = (char *) malloc(200);
            memset(printStr, '\0', 200);
            uint8_t * printPtr = pktbuf;
            int index = 0;
            for(int pi = 0; pi < pkt->actual_length; pi++, printPtr++){
                printStr = uint8_to_hex(*printPtr, printStr, &index);
                if(pi >= 50){
                    break;
                }
            }
            *(printStr + index) = '\0';
            LOGE("_uvc_process_payload_iso 打印每个数据包数据 %s\n", printStr);
            free(printStr);
#endif
	/* 添加数据打印，用于调试，测完后应删除 end */

//			assert(pktbuf < transfer->buffer + transfer->length - 1);	// XXX
			if (strmh->devh->is_isight)
			{
				if (pkt->actual_length < 30
					|| (memcmp(isight_tag, pktbuf + 2, sizeof(isight_tag))
						&& memcmp(isight_tag, pktbuf + 3, sizeof(isight_tag)))) {
					check_header = 0;
					header_len = 0;
				} else {
					header_len = pktbuf[0];
				}
			} else {
				header_len = pktbuf[0];	// Header length field of Stream Header  流头的头长度字段
			}
			if (pkt->actual_length == header_len) {
			    if(header_len<2 || !(pktbuf[1] & UVC_STREAM_EOF) ){
			        // 数据包只有头，没有实体数据 且没有EOF标记
			        continue;
			    }
			}

			if (LIKELY(check_header)) {
				header_info = pktbuf[1];
				if (UNLIKELY(header_info & UVC_STREAM_ERR)) {
//					strmh->bfh_err |= UVC_STREAM_ERR;
					MARK("bad packet:status=0x%2x", header_info);
					libusb_clear_halt(strmh->devh->usb_devh, strmh->stream_if->bEndpointAddress);
//					uvc_vc_get_error_code(strmh->devh, &vc_error_code, UVC_GET_CUR);
					uvc_vs_get_error_code(strmh->devh, &vs_error_code, UVC_GET_CUR);
					continue;
				}

                // 图像时间戳，同个视频帧多个数据包中保持相同
                uint32_t frame_pts = 0;
                // 系统时间时钟，采样的时钟值
                uint32_t frame_stc = 0;
                // 帧计数器，同个视频帧会有不相同
                uint32_t frame_sof = 0;

                if (header_info & UVC_STREAM_PTS) {
                    // XXX saki some camera may send broken packet or failed to receive all data
                    // saki某些相机可能发送了损坏的数据包或无法接收所有数据
                    if (LIKELY(header_len >= 6)) {
                        frame_pts = DW_TO_INT(pktbuf + 2);
                    } else {
                        MARK("bogus packet: header info has UVC_STREAM_PTS, but no data");
                        frame_pts = 0;
                    }
                }
                if (header_info & UVC_STREAM_SCR) {
                    // XXX saki some camera may send broken packet or failed to receive all data
                    // saki某些相机可能发送了损坏的数据包或无法接收所有数据
                    if (LIKELY(header_len >= 10)) {
                        frame_stc = DW_TO_INT(pktbuf + 6);
                        if (LIKELY(header_len >= 12)) {
                            frame_sof = _11bits_TO_INT(pktbuf + 10);
                        }
                    }
                }
		        uint8_t frame_fid = header_info & UVC_STREAM_FID;
				if (strmh->got_bytes > 0 && (strmh->fid != frame_fid || strmh->pts != frame_pts)) {
                    /* The frame ID bit was flipped, but we have image data sitting
                     around from prior transfers. This means the camera didn't send
                     an EOF for the last transfer of the previous frame or some frames losted.
                     帧ID位被翻转了，但是我们有来自先前传输的图像数据，这意味着相机没有为前一帧的最后一次传输发送EOF或丢失了一些帧。
				     PTS不一样，这意味着相机在上一帧的最后传输中没有发送EOF。

				     可能出现花帧了
                     */
                    LOGE("_uvc_process_payload_iso some frames losted 可能出现花帧了 \n");
					_uvc_swap_buffers(strmh, drop_incomplete_frame);
				}

				strmh->fid = frame_fid;
				strmh->pts = frame_pts;
				strmh->last_stc = frame_stc;
				strmh->sof = frame_sof;

				if (strmh->devh->is_isight) {
					MARK("is_isight");
					continue; // don't look for data after an iSight header  不要在iSight标头后查找数据
				}
			} // if LIKELY(check_header)

			if (UNLIKELY(pkt->actual_length < header_len)) {
				/* Bogus packet received
				 * 收到假包
				 */
				strmh->bfh_err |= UVC_STREAM_ERR;
				MARK("bogus packet: actual_len=%d, header_len=%zd", pkt->actual_length, header_len);
				continue;
			}

			// XXX original implementation could lead to trouble because unsigned values
			// and there calculated value never become minus.
			// therefor changed to "if (pkt->actual_length > header_len)"
			// from "if (pkt->actual_length - header_len > 0)"
			// 原始实现可能会导致麻烦，因为未签名的值和在那里计算出的值永远不会变为负值。
			// 因此从"if (pkt->actual_length - header_len > 0)"更改为"if (pkt->actual_length > header_len)"
			if (LIKELY(pkt->actual_length > header_len)) {
				const size_t odd_bytes = pkt->actual_length - header_len;
				assert(strmh->got_bytes + odd_bytes < strmh->size_buf);
				assert(strmh->outbuf);
				assert(pktbuf);
				memcpy(strmh->outbuf + strmh->got_bytes, pktbuf + header_len, odd_bytes);
				strmh->got_bytes += odd_bytes;
			}

			if ((pktbuf[1] & UVC_STREAM_EOF) && strmh->got_bytes != 0) {
				/* The EOF bit is set, so publish the complete frame
				 * E0F帧结束，如果有，指示视频帧的结束，并在属于帧的最后一个视频样本中设置
				 * EOF位置1，因此发布完整帧
				 */
				_uvc_swap_buffers(strmh, 1);
			}

		} else {	// if (LIKELY(pktbuf))
			strmh->bfh_err |= UVC_STREAM_ERR;
			MARK("libusb_get_iso_packet_buffer_simple returned null");
			continue;
		}
	}	// for
}

/** @internal
 * @brief Isochronous transfer callback
 * 同步传输回调 有数据回调
 * 
 * Processes stream, places frames into buffer, signals listeners
 * (such as user callback thread and any polling thread) on new frame
 * 处理流，将帧放入缓冲区，在新帧上向侦听器（例如用户回调线程和任何轮询线程）发出信号
 *
 * @param transfer Active transfer
 */
static void _uvc_stream_callback(struct libusb_transfer *transfer) {
	if UNLIKELY(!transfer) return;

	uvc_stream_handle_t *strmh = transfer->user_data;
	if UNLIKELY(!strmh) return;

	int resubmit = 1;

#ifndef NDEBUG
	static int cnt = 0;
	if UNLIKELY((++cnt % 1000) == 0) {
	    MARK("cnt=%d", cnt);
	}
#endif
	switch (transfer->status) {
        case LIBUSB_TRANSFER_COMPLETED: // 传输已完成，没有错误。 请注意，这并不表示已传输了全部所需数据。
            {
                    int isOk;
                    for (int i = 0; i < transfer->num_iso_packets; i++) {
                        isOk = transfer->iso_packet_desc[i].status != LIBUSB_TRANSFER_COMPLETED ? 0 : 1;
                        if(0 == isOk){
                            break;
                        }
                    }
                    if(1 == isOk){
                        if (!transfer->num_iso_packets) {
                                /* This is a bulk mode transfer, so it just has one payload transfer
                                 * 这是块模式传输，因此只有一个有效负载传输
                                 */
                                _uvc_process_payload(strmh, transfer->buffer, transfer->actual_length);
                        } else {
                                /* This is an isochronous mode transfer, so each packet has a payload transfer
                                 * 这是同步模式传输，因此每个数据包都有一个有效负载传输
                                 */
                                _uvc_process_payload_iso(strmh, transfer);
                        }
                    }
            }
            break;
        case LIBUSB_TRANSFER_NO_DEVICE: // 传输没有设备
            strmh->running = 0;	// this needs for unexpected disconnect of cable otherwise hangup  这需要意外断开电缆连接，否则会挂断
            // pass through to following lines 通过以下几行
        case LIBUSB_TRANSFER_CANCELLED: // 传输被取消
        case LIBUSB_TRANSFER_ERROR: // 传输失败
            UVC_DEBUG("not retrying transfer, status = %d", transfer->status);
    //		MARK("not retrying transfer, status = %d", transfer->status);
    //		_uvc_delete_transfer(transfer);
            resubmit = 0;
            break;
        case LIBUSB_TRANSFER_TIMED_OUT: // 传输超时
        case LIBUSB_TRANSFER_STALL: // 对于批量/中断端点：检测到停止条件（端点已停止）。 对于控制端点：不支持控制请求。
        case LIBUSB_TRANSFER_OVERFLOW: // 设备发送的数据超出要求
            UVC_DEBUG("retrying transfer, status = %d", transfer->status);
    //		MARK("retrying transfer, status = %d", transfer->status);
            break;
	}

	if (LIKELY(strmh->running && resubmit)) {
	    // 提交传输。 此功能将触发USB传输，然后立即返回。
	    // 继续请求
		libusb_submit_transfer(transfer);
	} else {
		// XXX delete non-reusing transfer
		// real implementation of deleting transfer moves to _uvc_delete_transfer
		// 删除不重复使用的转移
		// 删除传输的实际实现移至_uvc_delete_transfer
		_uvc_delete_transfer(transfer);
	}
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
 * @param flags Stream setup flags, currently undefined. Set this to zero. The lower bit
 * is reserved for backward compatibility.
 */
uvc_error_t uvc_start_streaming(uvc_device_handle_t *devh,
		uvc_stream_ctrl_t *ctrl, uvc_frame_callback_t *cb, void *user_ptr,
		uint8_t flags) {
	return uvc_start_streaming_bandwidth(devh, ctrl, cb, user_ptr, 0, flags);
}

/**
 * Begin streaming video from the camera into the callback function.
 * @ingroup streaming
 *
 * @param devh UVC device
 * @param ctrl Control block, processed using {uvc_probe_stream_ctrl} or
 *             {uvc_get_stream_ctrl_format_size}
 * @param cb   User callback function. See {uvc_frame_callback_t} for restrictions.
 * @param bandwidth_factor [0.0f, 1.0f]
 * @param flags Stream setup flags, currently undefined. Set this to zero. The lower bit
 * is reserved for backward compatibility.
 *
 * 开始将视频从摄像机传输到回调函数中。
 *
 * @param devh UVC设备
 * @param ctrl 控制块，使用{uvc_probe_stream_ctrl}或{uvc_get_stream_ctrl_format_size}处理
 * @param cb  回调方法
 * @param bandwidth_factor [0.0f, 1.0f]
 * @param flags 流设置标志，当前未定义。 将此设置为零。 保留低位用于向后兼容。
 */
uvc_error_t uvc_start_streaming_bandwidth(uvc_device_handle_t *devh,
		uvc_stream_ctrl_t *ctrl, uvc_frame_callback_t *cb, void *user_ptr,
		float bandwidth_factor,
		uint8_t flags) {
	uvc_error_t ret;
	uvc_stream_handle_t *strmh;
	// 打开新视频流
	ret = uvc_stream_open_ctrl(devh, &strmh, ctrl);
	if (UNLIKELY(ret != UVC_SUCCESS)) {
	    return ret;
	}

    // 开始将视频从流传输到回调函数中。
	ret = uvc_stream_start_bandwidth(strmh, cb, user_ptr, bandwidth_factor, flags);
	if (UNLIKELY(ret != UVC_SUCCESS)) {
	    // 关闭流
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
 * @deprecated The stream type (bulk vs. isochronous) will be determined by the
 * type of interface associated with the uvc_stream_ctrl_t parameter, regardless
 * of whether the caller requests isochronous streaming. Please switch to
 * uvc_start_streaming().
 * @deprecated 流类型（批量vs.等时）将由与uvc_stream_ctrl_t参数关联的接口的类型确定，无论调用方是否请求等时流。 请切换到uvc_start_streaming（）。
 *
 * @param devh UVC device
 * @param ctrl Control block, processed using {uvc_probe_stream_ctrl} or
 *             {uvc_get_stream_ctrl_format_size}
 * @param cb   User callback function. See {uvc_frame_callback_t} for restrictions.
 */
uvc_error_t uvc_start_iso_streaming(uvc_device_handle_t *devh,
		uvc_stream_ctrl_t *ctrl, uvc_frame_callback_t *cb, void *user_ptr) {
	return uvc_start_streaming_bandwidth(devh, ctrl, cb, user_ptr, 0.0f, 0);
}

// 通过接口获取流
static uvc_stream_handle_t *_uvc_get_stream_by_interface(
		uvc_device_handle_t *devh, int interface_idx) {
	uvc_stream_handle_t *strmh;

    // 循环查找流
	DL_FOREACH(devh->streams, strmh)
	{
		if (strmh->stream_if->bInterfaceNumber == interface_idx) {
		    // 接口编码一致，找到接口并返回
		    return strmh;
		}
	}

	return NULL;
}

// 获取流
static uvc_streaming_interface_t *_uvc_get_stream_if(uvc_device_handle_t *devh,
		int interface_idx) {
	uvc_streaming_interface_t *stream_if;

    // 循环查找流
	DL_FOREACH(devh->info->stream_ifs, stream_if)
	{
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
uvc_error_t uvc_stream_open_ctrl(uvc_device_handle_t *devh,
		uvc_stream_handle_t **strmhp, uvc_stream_ctrl_t *ctrl) {
	/* Chosen frame and format descriptors
	 * 选择帧和格式描述符
	 */
	uvc_stream_handle_t *strmh = NULL;
	uvc_streaming_interface_t *stream_if;
	uvc_error_t ret;

	UVC_ENTER();

    // 循通过接口循环查找流
	if (UNLIKELY(_uvc_get_stream_by_interface(devh, ctrl->bInterfaceNumber) != NULL)) {
		ret = UVC_ERROR_BUSY; /* Stream is already opened */
		goto fail;
	}

    // 获取流
	stream_if = _uvc_get_stream_if(devh, ctrl->bInterfaceNumber);
	if (UNLIKELY(!stream_if)) {
		ret = UVC_ERROR_INVALID_PARAM;
		goto fail;
	}

	strmh = calloc(1, sizeof(*strmh));
	if (UNLIKELY(!strmh)) {
		ret = UVC_ERROR_NO_MEM;
		goto fail;
	}
	strmh->devh = devh;
	strmh->stream_if = stream_if;
	strmh->frame.library_owns_data = 1;
	// 声明UVC接口，必要时分离内核驱动程序。
	ret = uvc_claim_if(strmh->devh, strmh->stream_if->bInterfaceNumber);
	if (UNLIKELY(ret != UVC_SUCCESS))
		goto fail;

    // 使用新的流格式重新配置流。
	ret = uvc_stream_ctrl(strmh, ctrl);
	if (UNLIKELY(ret != UVC_SUCCESS))
		goto fail;

	// Set up the streaming status and data space
	// 设置流状态和数据空间
	strmh->running = 0;
	/** @todo take only what we need  只拿我们需要的东西 */
	// 创建接收数据缓存空间，要确保空间足够，至少能存一帧数据
	// @todo 可以优化，根据协商的信息来创建缓存空间大小
	uint32_t sizeBuf = frame_buffer_size;
	strmh->outbuf = malloc(sizeBuf); // 原值是 LIBUVC_XFER_BUF_SIZE
	strmh->holdbuf = malloc(sizeBuf); // 原值是 LIBUVC_XFER_BUF_SIZE
	strmh->size_buf = sizeBuf;	// xxx for boundary check  用于边界检查

	UVC_DEBUG("LIBUVC_XFER_BUF_SIZE:"+sizeBuf);

	pthread_mutex_init(&strmh->cb_mutex, NULL);
	pthread_cond_init(&strmh->cb_cond, NULL);

    // 添加到双向队列中
	DL_APPEND(devh->streams, strmh);

	*strmhp = strmh;

	UVC_EXIT(0);
	return UVC_SUCCESS;

fail:
	if (strmh)
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
 * @param flags Stream setup flags, currently undefined. Set this to zero. The lower bit
 * is reserved for backward compatibility.
 *
 * 开始将视频从流传输到回调函数中。
 * @param strmh UVC流
 * @param cb 用户回调函数。 有关限制，请参见{uvc_frame_callback_t}。
 * @param flags 流设置标志，当前未定义。 将此设置为零。 保留低位用于向后兼容。
 */
uvc_error_t uvc_stream_start(uvc_stream_handle_t *strmh,
		uvc_frame_callback_t *cb, void *user_ptr, uint8_t flags) {
	return uvc_stream_start_bandwidth(strmh, cb, user_ptr, 0, flags);
}

/**
 * Begin streaming video from the stream into the callback function.
 * @ingroup streaming
 *
 * @param strmh UVC stream
 * @param cb   User callback function. See {uvc_frame_callback_t} for restrictions.
 * @param bandwidth_factor [0.0f, 1.0f]
 * @param flags Stream setup flags, currently undefined. Set this to zero. The lower bit
 * is reserved for backward compatibility.
 *
 * 开始将视频从流传输到回调函数中。
 * @param strmh UVC流
 * @param cb   用户回调函数。 有关限制，请参见{uvc_frame_callback_t}。
 * @param bandwidth_factor [0.0f, 1.0f]
 * @param flags 流设置标志，当前未定义。 将此设置为零。 保留低位用于向后兼容。
 */
uvc_error_t uvc_stream_start_bandwidth(uvc_stream_handle_t *strmh,
		uvc_frame_callback_t *cb, void *user_ptr, float bandwidth_factor, uint8_t flags) {
	/* USB interface we'll be using
	 * 我们将使用的USB接口
	 */
	const struct libusb_interface *interface;
	int interface_id;
	char isochronous;
	uvc_frame_desc_t *frame_desc;
	uvc_format_desc_t *format_desc;
	uvc_stream_ctrl_t *ctrl;
	uvc_error_t ret;
	/* Total amount of data per transfer
	 * 每次传输的数据总量
	 */
	size_t total_transfer_size;
	struct libusb_transfer *transfer;
	int transfer_id;

	ctrl = &strmh->cur_ctrl;

	UVC_ENTER();

	if (UNLIKELY(strmh->running)) {
		UVC_EXIT(UVC_ERROR_BUSY);
		return UVC_ERROR_BUSY;
	}

	strmh->running = 1;
	strmh->seq = 0;
	strmh->fid = 0;
	strmh->pts = 0;
	strmh->last_stc = 0;
	strmh->sof = 0;
	strmh->bfh_err = 0;	// XXX

    // 查找特定帧配置的描述符
	frame_desc = uvc_find_frame_desc_stream(strmh, ctrl->bFormatIndex, ctrl->bFrameIndex);
	if (UNLIKELY(!frame_desc)) {
		ret = UVC_ERROR_INVALID_PARAM;
		LOGE("UVC_ERROR_INVALID_PARAM");
		goto fail;
	}
	format_desc = frame_desc->parent;

    // 获取帧格式
	strmh->frame_format = uvc_frame_format_for_guid(format_desc->guidFormat);
	if (UNLIKELY(strmh->frame_format == UVC_FRAME_FORMAT_UNKNOWN)) {
		ret = UVC_ERROR_NOT_SUPPORTED;
		LOGE("unlnown frame format");
		goto fail;
	}
	// 最大视频帧大小
	const uint32_t dwMaxVideoFrameSize = ctrl->dwMaxVideoFrameSize <= frame_desc->dwMaxVideoFrameBufferSize
		? ctrl->dwMaxVideoFrameSize : frame_desc->dwMaxVideoFrameBufferSize;

	// Get the interface that provides the chosen format and frame configuration
	// 获取提供所选格式和帧配置的界面
	interface_id = strmh->stream_if->bInterfaceNumber;
	interface = &strmh->devh->info->config->interface[interface_id];

	/* A VS interface uses isochronous transfers if it has multiple altsettings.
	 * (UVC 1.5: 2.4.3. VideoStreaming Interface, on page 19)
	 * 如果VS接口具有多个altsettings，则使用等时传输。
	 *（UVC 1.5：2.4.3.VideoStreaming接口，第19页）
	 */
	isochronous = interface->num_altsetting > 1;

	if (isochronous) {
		MARK("isochronous transfer mode:num_altsetting=%d", interface->num_altsetting);
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
		
		if ((bandwidth_factor > 0) && (bandwidth_factor < 1.0f)) {
			config_bytes_per_packet = (size_t)(strmh->cur_ctrl.dwMaxPayloadTransferSize * bandwidth_factor);
			if (!config_bytes_per_packet) {
				config_bytes_per_packet = strmh->cur_ctrl.dwMaxPayloadTransferSize;
			}
		} else {
			config_bytes_per_packet = strmh->cur_ctrl.dwMaxPayloadTransferSize;
		}
//#if !defined(__LP64__)
//		LOGI("config_bytes_per_packet=%d", config_bytes_per_packet);
//#else
//		LOGI("config_bytes_per_packet=%ld", config_bytes_per_packet);
//#endif
		if (UNLIKELY(!config_bytes_per_packet)) {	// XXX added to privent zero divided exception at the following code 在以下代码中添加了以防止零除异常
			ret = UVC_ERROR_IO;
			LOGE("config_bytes_per_packet is zero");
			goto fail;
		}

		/* Go through the altsettings and find one whose packets are at least
		 * as big as our format's maximum per-packet usage. Assume that the
		 * packet sizes are increasing.
		 * 通过altsettings，找到一个其数据包至少与我们格式最大的每数据包使用量一样大的数据包。 假设数据包大小正在增加。
		 */
		const int num_alt = interface->num_altsetting - 1;
		for (alt_idx = 0; alt_idx <= num_alt ; alt_idx++) {
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
					// bit10…0:		maximum packet size
					// bit12…11:	the number of additional transaction opportunities per microframe for high-speed
					//				00 = None (1 transaction per microframe)
					//				01 = 1 additional (2 per microframe)
					//				10 = 2 additional (3 per microframe)
					//				11 = Reserved
					endpoint_bytes_per_packet = (endpoint_bytes_per_packet & 0x07ff) * (((endpoint_bytes_per_packet >> 11) & 3) + 1);
					break;
				}
			}
			// XXX config_bytes_per_packet should not be zero otherwise zero divided exception occur
			// config_bytes_per_packet不应为零，否则会发生零除异常
			if (LIKELY(endpoint_bytes_per_packet)) {
				if ( (endpoint_bytes_per_packet >= config_bytes_per_packet)
					|| (alt_idx == num_alt) ) {	// XXX always match to last altsetting for buggy device  始终与设备的最后一个高度设置匹配
					/* Transfers will be at most one frame long: Divide the maximum frame size
					 * by the size of the endpoint and round up
					 * 传输最多为一帧：将最大帧大小除以端点大小并四舍五入
					 */
					packets_per_transfer = (dwMaxVideoFrameSize + endpoint_bytes_per_packet - 1) / endpoint_bytes_per_packet;		// XXX cashed by zero divided exception occured  发生除0异常

					/* But keep a reasonable limit: Otherwise we start dropping data
					 * 但请保持合理的限制：否则我们将开始删除数据
					 */
					if (packets_per_transfer > 32){
					    packets_per_transfer = 32;
					}

					total_transfer_size = packets_per_transfer * endpoint_bytes_per_packet;
					break;
				}
			}
		}
		if (UNLIKELY(!endpoint_bytes_per_packet)) {
			LOGE("endpoint_bytes_per_packet is zero");
			ret = UVC_ERROR_INVALID_MODE;
			goto fail;
		}
		if (UNLIKELY(!total_transfer_size)) {
			LOGE("total_transfer_size is zero");
			ret = UVC_ERROR_INVALID_MODE;
			goto fail;
		}

		/* If we searched through all the altsettings and found nothing usable
		 * 如果我们搜索了所有的altsettings，却发现没有可用的东西
		 */
/*		if (UNLIKELY(alt_idx == interface->num_altsetting)) {	// XXX never hit this condition  从未遇到过这种情况
			UVC_DEBUG("libusb_set_interface_alt_setting failed");
			ret = UVC_ERROR_INVALID_MODE;
			goto fail;
		} */

		/* Select the altsetting
		 * 选择altsetting
		 */
		MARK("Select the altsetting");
		ret = libusb_set_interface_alt_setting(strmh->devh->usb_devh,
				altsetting->bInterfaceNumber, altsetting->bAlternateSetting);
		if (UNLIKELY(ret != UVC_SUCCESS)) {
			UVC_DEBUG("libusb_set_interface_alt_setting failed");
			goto fail;
		}

		/* Set up the transfers
		 * 设置传输
		 */
		MARK("Set up the transfers");
		for (transfer_id = 0; transfer_id < LIBUVC_NUM_TRANSFER_BUFS; ++transfer_id) {
		    // 创建传输句柄 为libusb传输分配指定数量的同步数据包描述符 不再使用需要libusb_free_transfer(transfer)
			transfer = libusb_alloc_transfer(packets_per_transfer);
			strmh->transfers[transfer_id] = transfer;
			strmh->transfer_bufs[transfer_id] = malloc(total_transfer_size);

            // Helper函数填充同步传输所需的 libusb_transfer 字段。
            // 当获得数据后回调 _uvc_stream_callback
			libusb_fill_iso_transfer(transfer, strmh->devh->usb_devh,
				format_desc->parent->bEndpointAddress,
				strmh->transfer_bufs[transfer_id], total_transfer_size,
				packets_per_transfer, _uvc_stream_callback,
				(void*) strmh, 5000);

            // 便利功能可根据传输结构中的num_iso_packets字段设置同步传输中所有数据包的长度。
			libusb_set_iso_packet_lengths(transfer, endpoint_bytes_per_packet);
		}
	} else {
		MARK("bulk transfer mode");
		/* prepare for bulk transfer
		 * 准备批量传输
		 */
		for (transfer_id = 0; transfer_id < LIBUVC_NUM_TRANSFER_BUFS; ++transfer_id) {
		    // 为libusb传输分配指定数量的同步数据包描述符 不再使用需要libusb_free_transfer(transfer)
			transfer = libusb_alloc_transfer(0);
			strmh->transfers[transfer_id] = transfer;
			strmh->transfer_bufs[transfer_id] = malloc(strmh->cur_ctrl.dwMaxPayloadTransferSize);

			// Helper函数可填充批量传输所需的libusb_transfer字段。
            // 当获得数据后回调 _uvc_stream_callback
			libusb_fill_bulk_transfer(transfer, strmh->devh->usb_devh,
				format_desc->parent->bEndpointAddress,
				strmh->transfer_bufs[transfer_id],
				strmh->cur_ctrl.dwMaxPayloadTransferSize, _uvc_stream_callback,
				(void *)strmh, 5000);
		}
	}

	strmh->user_cb = cb;
	strmh->user_ptr = user_ptr;

	/* If the user wants it, set up a thread that calls the user's function
	 * with the contents of each frame.
	 * 如果用户需要，请设置一个线程，该线程使用每帧的内容来调用用户的函数。
	 */
	MARK("create callback thread");
	if LIKELY(cb) {
		pthread_create(&strmh->cb_thread, NULL, _uvc_user_caller, (void*) strmh);
	}
	MARK("submit transfers");
	for (transfer_id = 0; transfer_id < LIBUVC_NUM_TRANSFER_BUFS; transfer_id++) {
	    // 提交传输。 此功能将触发USB传输，然后立即返回。
		ret = libusb_submit_transfer(strmh->transfers[transfer_id]);
		if (UNLIKELY(ret != UVC_SUCCESS)) {
			UVC_DEBUG("libusb_submit_transfer failed");
			break;
		}
	}

	if (UNLIKELY(ret != UVC_SUCCESS)) {
		/** @todo clean up transfers and memory 清理传输和内存 */
		goto fail;
	}

	UVC_EXIT(ret);
	return ret;
fail:
	LOGE("fail");
	strmh->running = 0;
	UVC_EXIT(ret);
	return ret;
}

/**
 * Begin streaming video from the stream into the callback function.
 * @ingroup streaming
 * 开始将视频从流传输到回调函数中。
 *
 * @deprecated The stream type (bulk vs. isochronous) will be determined by the
 * type of interface associated with the uvc_stream_ctrl_t parameter, regardless
 * of whether the caller requests isochronous streaming. Please switch to
 * uvc_stream_start().
 *
 * @param strmh UVC stream
 * @param cb   User callback function. See {uvc_frame_callback_t} for restrictions.
 */
uvc_error_t uvc_stream_start_iso(uvc_stream_handle_t *strmh,
		uvc_frame_callback_t *cb, void *user_ptr) {
	return uvc_stream_start(strmh, cb, user_ptr, 0);
}

/** @internal
 * @brief User callback runner thread
 * @note There should be at most one of these per currently streaming device
 * @param arg Device handle
 * 用户回调运行器线程
 * 每个当前流式传输设备最多应有一个
 */
static void *_uvc_user_caller(void *arg) {
	uvc_stream_handle_t *strmh = (uvc_stream_handle_t *) arg;

	uint32_t last_seq = 0;

	for (; 1 ;) {
		pthread_mutex_lock(&strmh->cb_mutex);
		{
			for (; strmh->running && (last_seq == strmh->hold_seq) ;) {
			    // 等待
				pthread_cond_wait(&strmh->cb_cond, &strmh->cb_mutex);
			}

			if (UNLIKELY(!strmh->running)) {
			    // 没有运行退出
				pthread_mutex_unlock(&strmh->cb_mutex);
				break;
			}

			last_seq = strmh->hold_seq;
			if (LIKELY(!strmh->hold_bfh_err)){	// XXX
			    // 获取视频帧
				_uvc_populate_frame(strmh);
			}
		}
		pthread_mutex_unlock(&strmh->cb_mutex);

		if (LIKELY(!strmh->hold_bfh_err)){	// XXX
		    // 回调接口
			strmh->user_cb(&strmh->frame, strmh->user_ptr);	// call user callback function
		}
	}

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

    // 查找特定帧配置的描述符
	frame_desc = uvc_find_frame_desc(strmh->devh, strmh->cur_ctrl.bFormatIndex, strmh->cur_ctrl.bFrameIndex);

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
	if (UNLIKELY(frame->data_bytes < strmh->hold_bytes)) {
	    // 帧空间不够重新创建
		frame->data = realloc(frame->data, strmh->hold_bytes);	// TODO add error handling when failed realloc  重新分配失败时添加错误处理
		frame->data_bytes = strmh->hold_bytes;
	}
	// XXX set actual_bytes to zero when erro bits is on
	// 当错误位打开时，将 actual_bytes 设置为零
	frame->actual_bytes = LIKELY(!strmh->hold_bfh_err) ? strmh->hold_bytes : 0;
	memcpy(frame->data, strmh->holdbuf, strmh->hold_bytes);	// XXX

	/** @todo set the frame time 设置帧时间 */
}

/** Poll for a frame
 * @ingroup streaming
 * 获取一帧
 *
 * @param devh UVC device
 * @param[out] frame Location to store pointer to captured frame (NULL on error)
 * @param timeout_us >0: Wait at most N microseconds; 0: Wait indefinitely; -1: return immediately
 */
uvc_error_t uvc_stream_get_frame(uvc_stream_handle_t *strmh,
		uvc_frame_t **frame, int32_t timeout_us) {
	time_t add_secs;
	time_t add_nsecs;
	struct timespec ts;
	struct timeval tv;

	if (UNLIKELY(!strmh->running)) {
	    // 没有运行
	    return UVC_ERROR_INVALID_PARAM;
	}

	if (UNLIKELY(strmh->user_cb)) {
	    // 没有回调接口
	    return UVC_ERROR_CALLBACK_EXISTS;
	}

	pthread_mutex_lock(&strmh->cb_mutex);
	{
		if (strmh->last_polled_seq < strmh->hold_seq) {
		    // 获取视频帧
			_uvc_populate_frame(strmh);
			*frame = &strmh->frame;
			strmh->last_polled_seq = strmh->hold_seq;
		} else if (timeout_us != -1) {
			if (!timeout_us) {
			    // 等待
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
                // 等待到指定时间，如果在指定时间前被唤醒返回0，否则返回1
				pthread_cond_timedwait(&strmh->cb_cond, &strmh->cb_mutex, &ts);
			}

			if (LIKELY(strmh->last_polled_seq < strmh->hold_seq)) {
			    // 获取视频帧
				_uvc_populate_frame(strmh);
				*frame = &strmh->frame;
				strmh->last_polled_seq = strmh->hold_seq;
			} else {
				*frame = NULL;
			}
		} else {
			*frame = NULL;
		}
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

	UVC_ENTER();
	DL_FOREACH_SAFE(devh->streams, strmh, strmh_tmp)
	{
		uvc_stream_close(strmh);
	}
	UVC_EXIT_VOID();
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
	ENTER();

	if (!strmh) RETURN(UVC_SUCCESS, uvc_error_t);

	if (UNLIKELY(!strmh->running)) {
		UVC_EXIT(UVC_ERROR_INVALID_PARAM);
		RETURN(UVC_ERROR_INVALID_PARAM, uvc_error_t);
	}

	strmh->running = 0;

	pthread_mutex_lock(&strmh->cb_mutex);
	{
		for (i = 0; i < LIBUVC_NUM_TRANSFER_BUFS; i++) {
			if (strmh->transfers[i]) {
			    // usb取消传输数据
				int res = libusb_cancel_transfer(strmh->transfers[i]);
				if ((res < 0) && (res != LIBUSB_ERROR_NOT_FOUND)) {
					UVC_DEBUG("libusb_cancel_transfer failed");
					// XXX originally freed buffers and transfer here
					// but this could lead to crash in _uvc_callback
					// therefore we comment out these lines
					// and free these objects in _uvc_iso_callback when strmh->running is false
					// 最初释放缓冲区并在此处传输，但这可能导致_uvc_callback崩溃，因此当strmh-> running为false时，我们注释掉这些行并在_uvc_iso_callback中释放这些对象
/*					free(strmh->transfers[i]->buffer);
					libusb_free_transfer(strmh->transfers[i]);
					strmh->transfers[i] = NULL; */
				}
			}
		}

		/* Wait for transfers to complete/cancel
		 * 等待传输完成/取消
		 */
		for (; 1 ;) {
			for (i = 0; i < LIBUVC_NUM_TRANSFER_BUFS; i++) {
				if (strmh->transfers[i] != NULL) {
				    break;
				}
			}
			if (i == LIBUVC_NUM_TRANSFER_BUFS) {
			    break;
			}
			pthread_cond_wait(&strmh->cb_cond, &strmh->cb_mutex);
		}
		// Kick the user thread awake
		// 唤醒用户线程
		pthread_cond_broadcast(&strmh->cb_cond);
	}
	pthread_mutex_unlock(&strmh->cb_mutex);

	/** @todo stop the actual stream, camera side? */

	if (strmh->user_cb) {
		/* wait for the thread to stop (triggered by LIBUSB_TRANSFER_CANCELLED transfer)
		 * 等待线程停止（由LIBUSB_TRANSFER_CANCELLED传输触发）
		 */
		pthread_join(strmh->cb_thread, NULL);
	}

	RETURN(UVC_SUCCESS, uvc_error_t);
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
	UVC_ENTER();

	if (!strmh) { UVC_EXIT_VOID() };

	if (strmh->running)
		uvc_stream_stop(strmh);

	uvc_release_if(strmh->devh, strmh->stream_if->bInterfaceNumber);

	if (strmh->frame.data) {
		free(strmh->frame.data);
		strmh->frame.data = NULL;
	}

	if (strmh->outbuf) {
		free(strmh->outbuf);
		strmh->outbuf = NULL;
	}
	if (strmh->holdbuf) {
		free(strmh->holdbuf);
		strmh->holdbuf = NULL;
	}

	pthread_cond_destroy(&strmh->cb_cond);
	pthread_mutex_destroy(&strmh->cb_mutex);

	DL_DELETE(strmh->devh->streams, strmh);
	free(strmh);

	UVC_EXIT_VOID();
}
