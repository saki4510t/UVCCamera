#ifndef __YUV__H__
#define __YUV__H__

#include <jni.h>
#include <stdio.h>

int YUV_TABLE[5][256];

/* Public: Generate and cache the lookup table necessary to convert from YUV to
 * ARGB.
 */
void cache_yuv_lookup_table(int table[5][256]);

/* Private: Convert an Y'UV42 image to an ARGB image.
 *
 * src - the source image buffer.
 * width - the width of the image.
 * height - the height of the image.
 * rgb_buffer - output buffer for RGB data from the conversion.
 * y_buffer - output buffer for alpha (Y) data from the conversion.
 */
void yuyv422_to_argb(unsigned char *src, int width, int height, int* rgb_buffer,
        int* y_buffer);

#endif // __YUV__H__
