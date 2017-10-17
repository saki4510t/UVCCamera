#include "yuv.h"
#include <stdio.h>

void cache_yuv_lookup_table(int table[5][256]) {
    for(int i = 0; i < 256; i++) {
        table[0][i] = 1192 * (i - 16);
        if(table[0][i] < 0) {
            table[0][i] = 0;
        }

        table[1][i] = 1634 * (i - 128);
        table[2][i] = 833 * (i - 128);
        table[3][i] = 400 * (i - 128);
        table[4][i] = 2066 * (i - 128);
    }
}

void yuyv422_to_argb(unsigned char *src, int width, int height, int* rgb_buffer,
        int* y_buffer) {
    if((!rgb_buffer || !y_buffer)) {
        return;
    }

    int frameSize = width * height * 2;
    int* lrgb = &rgb_buffer[0];
    int* lybuf = &y_buffer[0];
    for(int i = 0; i < frameSize; i += 4) {
        unsigned char y1, y2, u, v;
        y1 = src[i];
        u = src[i + 1];
        y2 = src[i + 2];
        v = src[i + 3];

        int y1192_1 = YUV_TABLE[0][y1];
        int r1 = (y1192_1 + YUV_TABLE[1][v]) >> 10;
        int g1 = (y1192_1 - YUV_TABLE[2][v] - YUV_TABLE[3][u]) >> 10;
        int b1 = (y1192_1 + YUV_TABLE[4][u]) >> 10;

        int y1192_2 = YUV_TABLE[0][y2];
        int r2 = (y1192_2 + YUV_TABLE[1][v]) >> 10;
        int g2 = (y1192_2 - YUV_TABLE[2][v] - YUV_TABLE[3][u]) >> 10;
        int b2 = (y1192_2 + YUV_TABLE[4][u]) >> 10;

        r1 = r1 > 255 ? 255 : r1 < 0 ? 0 : r1;
        g1 = g1 > 255 ? 255 : g1 < 0 ? 0 : g1;
        b1 = b1 > 255 ? 255 : b1 < 0 ? 0 : b1;
        r2 = r2 > 255 ? 255 : r2 < 0 ? 0 : r2;
        g2 = g2 > 255 ? 255 : g2 < 0 ? 0 : g2;
        b2 = b2 > 255 ? 255 : b2 < 0 ? 0 : b2;

        *lrgb++ = 0xff000000 | b1 << 16 | g1 << 8 | r1;
        *lrgb++ = 0xff000000 | b2 << 16 | g2 << 8 | r2;

        if(lybuf != NULL) {
            *lybuf++ = y1;
            *lybuf++ = y2;
        }
    }
}
