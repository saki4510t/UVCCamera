
#include "libUVCCamera.h"

#pragma interface

#ifndef ROTATEIMAGE_H_
#define ROTATEIMAGE_H_

class RotateImage {
private:
    // 图像数据
    void * rotate_data;
    // des_data 长度
    size_t rotate_data_bytes;

    void SpaceSizeProcessing(uvc_frame_t *src_frame);

public:
	RotateImage();
	~RotateImage();

    // 顺时针旋转 90 度
    void rotate_yuyv_90(uvc_frame_t *src_frame);
    void rotateYuyvDegree90(void *rotatedYuyv, void *yuyv, uint32_t width, uint32_t height);

    // 顺时针旋转 180 度
    void rotate_yuyv_180(uvc_frame_t *src_frame);
    void rotateYuyvDegree180(void *rotatedYuyv, void *yuyv, uint32_t width, uint32_t height);

    // 顺时针旋转 270 度
    void rotate_yuyv_270(uvc_frame_t *src_frame);
    void rotateYuyvDegree270(void *rotatedYuyv, void *yuyv, uint32_t width, uint32_t height);

    // 水平镜像
    void horizontal_mirror_yuyv(uvc_frame_t *src_frame);
    void horizontalMirrorYuyv(void *_mirrorYuyv, void *_yuyv, uint32_t width, uint32_t height);


    // 垂直镜像
    void vertical_mirror_yuyv(uvc_frame_t *src_frame);
    void verticalMirrorYuyv(void *_mirrorYuyv, void *_yuyv, uint32_t width, uint32_t height);
};

#endif