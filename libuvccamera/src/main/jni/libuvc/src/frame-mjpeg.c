/*********************************************************************
 * add and modified some function to avoid crash
 * Copyright (C) 2014-2015 saki@serenegiant All rights reserved.
 *********************************************************************/
/*********************************************************************
 * Software License Agreement (BSD License)
 *
 *  Copyright (C) 2014 Robert Xiao
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
 * @defgroup frame Frame processing
 */
#include "libuvc/libuvc.h"
#include "libuvc/libuvc_internal.h"
#include <jpeglib.h>
#include <setjmp.h>

extern uvc_error_t uvc_ensure_frame_size(uvc_frame_t *frame, size_t need_bytes);

struct error_mgr {
	struct jpeg_error_mgr super;
	jmp_buf jmp;
};

static void _error_exit(j_common_ptr dinfo) {
	struct error_mgr *myerr = (struct error_mgr *) dinfo->err;
#ifndef NDEBUG
#if (defined(ANDROID) || defined(__ANDROID__))
	char err_msg[1024];
	(*dinfo->err->format_message)(dinfo, err_msg);
	err_msg[1023] = 0;
	LOGW("err=%s", err_msg);
#else
	(*dinfo->err->output_message)(dinfo);
#endif
#endif
	longjmp(myerr->jmp, 1);
}

/* ISO/IEC 10918-1:1993(E) K.3.3. Default Huffman tables used by MJPEG UVC devices
 which don't specify a Huffman table in the JPEG stream. */
static const unsigned char dc_lumi_len[] = {
	0, 0, 1, 5, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0 };
static const unsigned char dc_lumi_val[] = {
	0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11 };

static const unsigned char dc_chromi_len[] = {
	0, 0, 3, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0 };
static const unsigned char dc_chromi_val[] = {
	0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11 };

static const unsigned char ac_lumi_len[] = {
	0, 0, 2, 1, 3, 3, 2, 4, 3, 5, 5, 4, 4, 0, 0, 1, 0x7d };
static const unsigned char ac_lumi_val[] = {
	0x01, 0x02, 0x03, 0x00, 0x04, 0x11,	0x05, 0x12,
	0x21, 0x31, 0x41, 0x06, 0x13, 0x51, 0x61, 0x07,
	0x22, 0x71,	0x14, 0x32, 0x81, 0x91, 0xa1, 0x08,
	0x23, 0x42, 0xb1, 0xc1, 0x15, 0x52, 0xd1, 0xf0,
	0x24, 0x33, 0x62, 0x72, 0x82, 0x09, 0x0a, 0x16,
	0x17, 0x18, 0x19, 0x1a, 0x25, 0x26, 0x27, 0x28,
	0x29, 0x2a, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39,
	0x3a, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49,
	0x4a, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59,
	0x5a, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69,
	0x6a, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79,
	0x7a, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89,
	0x8a, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98,
	0x99, 0x9a, 0xa2, 0xa3, 0xa4, 0xa5, 0xa6, 0xa7,
	0xa8, 0xa9, 0xaa, 0xb2, 0xb3, 0xb4, 0xb5, 0xb6,
	0xb7, 0xb8, 0xb9, 0xba, 0xc2, 0xc3, 0xc4, 0xc5,
	0xc6, 0xc7, 0xc8, 0xc9, 0xca, 0xd2, 0xd3, 0xd4,
	0xd5, 0xd6, 0xd7, 0xd8, 0xd9, 0xda, 0xe1, 0xe2,
	0xe3, 0xe4, 0xe5, 0xe6, 0xe7, 0xe8, 0xe9, 0xea,
	0xf1, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7, 0xf8,
	0xf9, 0xfa
};
static const unsigned char ac_chromi_len[] = {
	0, 0, 2, 1, 2, 4, 4, 3, 4, 7, 5, 4, 4, 0, 1, 2, 0x77 };
static const unsigned char ac_chromi_val[] = {
	0x00, 0x01, 0x02, 0x03, 0x11, 0x04, 0x05, 0x21,
	0x31, 0x06, 0x12, 0x41, 0x51, 0x07, 0x61, 0x71,
	0x13, 0x22, 0x32, 0x81, 0x08, 0x14, 0x42, 0x91,
	0xa1, 0xb1, 0xc1, 0x09, 0x23, 0x33, 0x52, 0xf0,
	0x15, 0x62, 0x72, 0xd1, 0x0a, 0x16, 0x24, 0x34,
	0xe1, 0x25, 0xf1, 0x17, 0x18, 0x19, 0x1a, 0x26,
	0x27, 0x28, 0x29, 0x2a, 0x35, 0x36, 0x37, 0x38,
	0x39, 0x3a, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48,
	0x49, 0x4a, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58,
	0x59, 0x5a, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68,
	0x69, 0x6a, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78,
	0x79, 0x7a, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87,
	0x88, 0x89, 0x8a, 0x92, 0x93, 0x94, 0x95, 0x96,
	0x97, 0x98, 0x99, 0x9a, 0xa2, 0xa3, 0xa4, 0xa5,
	0xa6, 0xa7, 0xa8, 0xa9, 0xaa, 0xb2, 0xb3, 0xb4,
	0xb5, 0xb6, 0xb7, 0xb8, 0xb9, 0xba, 0xc2, 0xc3,
	0xc4, 0xc5, 0xc6, 0xc7, 0xc8, 0xc9, 0xca, 0xd2,
	0xd3, 0xd4, 0xd5, 0xd6, 0xd7, 0xd8, 0xd9, 0xda,
	0xe2, 0xe3, 0xe4, 0xe5, 0xe6, 0xe7, 0xe8, 0xe9,
	0xea, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7, 0xf8,
	0xf9, 0xfa
};

#define COPY_HUFF_TABLE(dinfo,tbl,name) do { \
	if (dinfo->tbl == NULL) dinfo->tbl = jpeg_alloc_huff_table((j_common_ptr)dinfo); \
		memcpy(dinfo->tbl->bits, name##_len, sizeof(name##_len)); \
		memset(dinfo->tbl->huffval, 0, sizeof(dinfo->tbl->huffval)); \
		memcpy(dinfo->tbl->huffval, name##_val, sizeof(name##_val)); \
	} while(0)

static inline void insert_huff_tables(j_decompress_ptr dinfo) {
	COPY_HUFF_TABLE(dinfo, dc_huff_tbl_ptrs[0], dc_lumi);
	COPY_HUFF_TABLE(dinfo, dc_huff_tbl_ptrs[1], dc_chromi);
	COPY_HUFF_TABLE(dinfo, ac_huff_tbl_ptrs[0], ac_lumi);
	COPY_HUFF_TABLE(dinfo, ac_huff_tbl_ptrs[1], ac_chromi);
}

// XXX added to improve the performance of decoding
// maximun reading lines for each call of jpeg_read_scanlines
// when defined this macro, it's value should be common factor
// of all available frame height.
// (1, 2, 4, 5, 6, 8, 10, 12, 20, 40...for 720p&1080p)
#define MAX_READLINE 8

#ifndef MAX_READLINE
#define MAX_READLINE 1
#endif
#if MAX_READLINE < 1
#undef MAX_READLINE
#define MAX_READLINE 1
#endif

/** @brief Convert an MJPEG frame to RGB
 * @ingroup frame
 *
 * @param in MJPEG frame
 * @param out RGB frame
 */
uvc_error_t uvc_mjpeg2rgb(uvc_frame_t *in, uvc_frame_t *out) {
	struct jpeg_decompress_struct dinfo;
	struct error_mgr jerr;
	size_t lines_read;
	// local copy
	uint8_t *data = out->data;
	const int out_step = out->step;

	int num_scanlines, i;
	lines_read = 0;
	unsigned char *buffer[MAX_READLINE];

	out->actual_bytes = 0;	// XXX
	if (UNLIKELY(in->frame_format != UVC_FRAME_FORMAT_MJPEG))
		return UVC_ERROR_INVALID_PARAM;

	if (uvc_ensure_frame_size(out, in->width * in->height * 3) < 0)
		return UVC_ERROR_NO_MEM;

	out->width = in->width;
	out->height = in->height;
	out->frame_format = UVC_FRAME_FORMAT_RGB;
	out->step = in->width * 3;
	out->sequence = in->sequence;
	out->capture_time = in->capture_time;
	out->source = in->source;

	dinfo.err = jpeg_std_error(&jerr.super);
	jerr.super.error_exit = _error_exit;

	if (setjmp(jerr.jmp)) {
		goto fail;
	}

	jpeg_create_decompress(&dinfo);
	jpeg_mem_src(&dinfo, in->data, in->actual_bytes);
	jpeg_read_header(&dinfo, TRUE);

	if (dinfo.dc_huff_tbl_ptrs[0] == NULL) {
		/* This frame is missing the Huffman tables: fill in the standard ones */
		insert_huff_tables(&dinfo);
	}

	dinfo.out_color_space = JCS_RGB;
	dinfo.dct_method = JDCT_IFAST;

	jpeg_start_decompress(&dinfo);

	if (LIKELY(dinfo.output_height == out->height)) {
		for (; dinfo.output_scanline < dinfo.output_height ;) {
			buffer[0] = data + (lines_read) * out_step;
			for (i = 1; i < MAX_READLINE; i++)
				buffer[i] = buffer[i-1] + out_step;
			num_scanlines = jpeg_read_scanlines(&dinfo, buffer, MAX_READLINE);
			lines_read += num_scanlines;
		}
		out->actual_bytes = in->width * in->height * 3;	// XXX
	}
	jpeg_finish_decompress(&dinfo);
	jpeg_destroy_decompress(&dinfo);
	return lines_read == out->height ? UVC_SUCCESS : UVC_ERROR_OTHER;	// XXX

fail:
	jpeg_destroy_decompress(&dinfo);
	return UVC_ERROR_OTHER+1;
}

/** @brief Convert an MJPEG frame to BGR
 * @ingroup frame
 *
 * @param in MJPEG frame
 * @param out BGR frame
 */
uvc_error_t uvc_mjpeg2bgr(uvc_frame_t *in, uvc_frame_t *out) {
	struct jpeg_decompress_struct dinfo;
	struct error_mgr jerr;
	size_t lines_read;

	int num_scanlines, i;
	lines_read = 0;
	unsigned char *buffer[MAX_READLINE];

	out->actual_bytes = 0;	// XXX
	if (UNLIKELY(in->frame_format != UVC_FRAME_FORMAT_MJPEG))
		return UVC_ERROR_INVALID_PARAM;

	if (uvc_ensure_frame_size(out, in->width * in->height * 3) < 0)
		return UVC_ERROR_NO_MEM;

	out->width = in->width;
	out->height = in->height;
	out->frame_format = UVC_FRAME_FORMAT_BGR;
	out->step = in->width * 3;
	out->sequence = in->sequence;
	out->capture_time = in->capture_time;
	out->source = in->source;

	dinfo.err = jpeg_std_error(&jerr.super);
	jerr.super.error_exit = _error_exit;

	if (setjmp(jerr.jmp)) {
		goto fail;
	}

	jpeg_create_decompress(&dinfo);
	jpeg_mem_src(&dinfo, in->data, in->actual_bytes);
	jpeg_read_header(&dinfo, TRUE);

	if (dinfo.dc_huff_tbl_ptrs[0] == NULL) {
		/* This frame is missing the Huffman tables: fill in the standard ones */
		insert_huff_tables(&dinfo);
	}

	dinfo.out_color_space = JCS_EXT_BGR;
	dinfo.dct_method = JDCT_IFAST;

	jpeg_start_decompress(&dinfo);

	// local copy
	uint8_t *data = out->data;
	const int out_step = out->step;

	if (LIKELY(dinfo.output_height == out->height)) {
		for (; dinfo.output_scanline < dinfo.output_height ;) {
			buffer[0] = data + (lines_read) * out_step;
			for (i = 1; i < MAX_READLINE; i++)
				buffer[i] = buffer[i-1] + out_step;
			num_scanlines = jpeg_read_scanlines(&dinfo, buffer, MAX_READLINE);
			lines_read += num_scanlines;
		}
		out->actual_bytes = in->width * in->height * 3;	// XXX
	}
	jpeg_finish_decompress(&dinfo);
	jpeg_destroy_decompress(&dinfo);
	return lines_read == out->height ? UVC_SUCCESS : UVC_ERROR_OTHER;	// XXX

fail:
	jpeg_destroy_decompress(&dinfo);
	return UVC_ERROR_OTHER+1;
}

/** @brief Convert an MJPEG frame to RGB565
 * @ingroup frame
 *
 * @param in MJPEG frame
 * @param out RGB frame
 */
uvc_error_t uvc_mjpeg2rgb565(uvc_frame_t *in, uvc_frame_t *out) {
	struct jpeg_decompress_struct dinfo;
	struct error_mgr jerr;
	size_t lines_read;

	int num_scanlines, i;
	lines_read = 0;
	unsigned char *buffer[MAX_READLINE];

	out->actual_bytes = 0;	// XXX
	if (UNLIKELY(in->frame_format != UVC_FRAME_FORMAT_MJPEG))
		return UVC_ERROR_INVALID_PARAM;

	if (uvc_ensure_frame_size(out, in->width * in->height * 2) < 0)
		return UVC_ERROR_NO_MEM;

	out->width = in->width;
	out->height = in->height;
	out->frame_format = UVC_FRAME_FORMAT_RGB565;
	out->step = in->width * 2;
	out->sequence = in->sequence;
	out->capture_time = in->capture_time;
	out->source = in->source;

	dinfo.err = jpeg_std_error(&jerr.super);
	jerr.super.error_exit = _error_exit;

	if (setjmp(jerr.jmp)) {
		goto fail;
	}

	jpeg_create_decompress(&dinfo);
	jpeg_mem_src(&dinfo, in->data, in->actual_bytes);
	jpeg_read_header(&dinfo, TRUE);

	if (dinfo.dc_huff_tbl_ptrs[0] == NULL) {
		/* This frame is missing the Huffman tables: fill in the standard ones */
		insert_huff_tables(&dinfo);
	}

	dinfo.out_color_space = JCS_RGB565;
	dinfo.dct_method = JDCT_IFAST;

	jpeg_start_decompress(&dinfo);

	// local copy
	uint8_t *data = out->data;
	const int out_step = out->step;

	if (LIKELY(dinfo.output_height == out->height)) {
		for (; dinfo.output_scanline < dinfo.output_height ;) {
			buffer[0] = data + (lines_read) * out_step;
			for (i = 1; i < MAX_READLINE; i++)
				buffer[i] = buffer[i-1] + out_step;
			num_scanlines = jpeg_read_scanlines(&dinfo, buffer, MAX_READLINE);
			lines_read += num_scanlines;
		}
		out->actual_bytes = in->width * in->height * 2;	// XXX
	}
	jpeg_finish_decompress(&dinfo);
	jpeg_destroy_decompress(&dinfo);
	return lines_read == out->height ? UVC_SUCCESS : UVC_ERROR_OTHER;	// XXX

fail:
	jpeg_destroy_decompress(&dinfo);
	return UVC_ERROR_OTHER+1;
}

/** @brief Convert an MJPEG frame to RGBX
 * @ingroup frame
 *
 * @param in MJPEG frame
 * @param out RGBX frame
 */
uvc_error_t uvc_mjpeg2rgbx(uvc_frame_t *in, uvc_frame_t *out) {
	struct jpeg_decompress_struct dinfo;
	struct error_mgr jerr;
	size_t lines_read;
	// local copy
	uint8_t *data = out->data;
	const int out_step = out->step;

	int num_scanlines, i;
	lines_read = 0;
	unsigned char *buffer[MAX_READLINE];

	out->actual_bytes = 0;	// XXX
	if (UNLIKELY(in->frame_format != UVC_FRAME_FORMAT_MJPEG))
		return UVC_ERROR_INVALID_PARAM;

	if (uvc_ensure_frame_size(out, in->width * in->height * 4) < 0)
		return UVC_ERROR_NO_MEM;

	out->width = in->width;
	out->height = in->height;
	out->frame_format = UVC_FRAME_FORMAT_RGBX;	// XXX
	out->step = in->width * 4;
	out->sequence = in->sequence;
	out->capture_time = in->capture_time;
	out->source = in->source;

	dinfo.err = jpeg_std_error(&jerr.super);
	jerr.super.error_exit = _error_exit;

	if (setjmp(jerr.jmp)) {
		goto fail;
	}

	jpeg_create_decompress(&dinfo);
	jpeg_mem_src(&dinfo, in->data, in->actual_bytes);	// XXX
	jpeg_read_header(&dinfo, TRUE);

	if (dinfo.dc_huff_tbl_ptrs[0] == NULL) {
		/* This frame is missing the Huffman tables: fill in the standard ones */
		insert_huff_tables(&dinfo);
	}

	dinfo.out_color_space = JCS_EXT_RGBA;
	dinfo.dct_method = JDCT_IFAST;

	jpeg_start_decompress(&dinfo);

	if (LIKELY(dinfo.output_height == out->height)) {
		for (; dinfo.output_scanline < dinfo.output_height ;) {
			buffer[0] = data + (lines_read) * out_step;
			for (i = 1; i < MAX_READLINE; i++)
				buffer[i] = buffer[i-1] + out_step;
			num_scanlines = jpeg_read_scanlines(&dinfo, buffer, MAX_READLINE);
			lines_read += num_scanlines;
		}
		out->actual_bytes = in->width * in->height * 4;	// XXX
	}
	jpeg_finish_decompress(&dinfo);
	jpeg_destroy_decompress(&dinfo);
	return lines_read == out->height ? UVC_SUCCESS : UVC_ERROR_OTHER;	// XXX

fail:
	jpeg_destroy_decompress(&dinfo);
	return UVC_ERROR_OTHER+1;
}

static inline unsigned char sat(int i) {
	return (unsigned char) (i >= 255 ? 255 : (i < 0 ? 0 : i));
}

#define YCbCr_YUYV_2(YCbCr, yuyv) \
	{ \
		*(yuyv++) = *(YCbCr+0); \
		*(yuyv++) = (*(YCbCr+1) + *(YCbCr+4)) >> 1; \
		*(yuyv++) = *(YCbCr+3); \
		*(yuyv++) = (*(YCbCr+2) + *(YCbCr+5)) >> 1; \
	}

/**
 * 将MJPEG转为YUYV
 */
uvc_error_t uvc_mjpeg2yuyv(uvc_frame_t *in, uvc_frame_t *out) {

	out->actual_bytes = 0;	// XXX
	if (UNLIKELY(in->frame_format != UVC_FRAME_FORMAT_MJPEG))
		return UVC_ERROR_INVALID_PARAM;

	if (uvc_ensure_frame_size(out, in->width * in->height * 2) < 0)
		return UVC_ERROR_NO_MEM;

	size_t lines_read = 0;
	int i, j;
	int num_scanlines;
	register uint8_t *yuyv, *ycbcr;

	out->width = in->width;
	out->height = in->height;
	out->frame_format = UVC_FRAME_FORMAT_YUYV;
	out->step = in->width * 2;
	out->sequence = in->sequence;
	out->capture_time = in->capture_time;
	out->source = in->source;

	struct jpeg_decompress_struct dinfo;
	struct error_mgr jerr;
	dinfo.err = jpeg_std_error(&jerr.super);
	jerr.super.error_exit = _error_exit;

	if (setjmp(jerr.jmp)) {
		goto fail;
	}

	jpeg_create_decompress(&dinfo);
	jpeg_mem_src(&dinfo, in->data, in->actual_bytes);	// XXX
	jpeg_read_header(&dinfo, TRUE);

	if (dinfo.dc_huff_tbl_ptrs[0] == NULL) {
		/* This frame is missing the Huffman tables: fill in the standard ones */
		insert_huff_tables(&dinfo);
	}

	dinfo.out_color_space = JCS_YCbCr;
	dinfo.dct_method = JDCT_IFAST;

	// start decompressor
	jpeg_start_decompress(&dinfo);

	// these dinfo.xxx valiables are only valid after jpeg_start_decompress
	const int row_stride = dinfo.output_width * dinfo.output_components;

	// allocate buffer
	register JSAMPARRAY buffer = (*dinfo.mem->alloc_sarray)
		((j_common_ptr) &dinfo, JPOOL_IMAGE, row_stride, MAX_READLINE);

	// local copy
	uint8_t *data = out->data;
	const int out_step = out->step;

	if (LIKELY(dinfo.output_height == out->height)) {
		for (; dinfo.output_scanline < dinfo.output_height ;) {
			// convert lines of mjpeg data to YCbCr
			num_scanlines = jpeg_read_scanlines(&dinfo, buffer, MAX_READLINE);
			// convert YCbCr to yuyv(YUV422)
			for (j = 0; j < num_scanlines; j++) {
				yuyv = data + (lines_read + j) * out_step;
				ycbcr = buffer[j];
				for (i = 0; i < row_stride; i += 24) {	// step by YCbCr x 8 pixels = 3 x 8 bytes
					YCbCr_YUYV_2(ycbcr + i, yuyv);
					YCbCr_YUYV_2(ycbcr + i + 6, yuyv);
					YCbCr_YUYV_2(ycbcr + i + 12, yuyv);
					YCbCr_YUYV_2(ycbcr + i + 18, yuyv);
				}
			}
			lines_read += num_scanlines;
		}
		out->actual_bytes = in->width * in->height * 2;	// XXX
	}

	jpeg_finish_decompress(&dinfo);
	jpeg_destroy_decompress(&dinfo);
	return lines_read == out->height ? UVC_SUCCESS : UVC_ERROR_OTHER;

fail:
	jpeg_destroy_decompress(&dinfo);
	return lines_read == out->height ? UVC_SUCCESS : UVC_ERROR_OTHER+1;
}

