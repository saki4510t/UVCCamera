#ifndef __CAPTURE_H__
#define __CAPTURE_H__

#include <jni.h>
#include <stdio.h>

#include "video_device.h"

/* Private: Begins capturing video frames from a previously initialized device.
 *
 * The buffers in FRAME_BUFFERS are handed off to the device.
 *
 * fd - a valid file descriptor to the device.
 *
 * Returns SUCCESS_LOCAL if no errors, otherwise ERROR_LOCAL.
 */
int start_capture(int fd);

/* Private: Read a single frame of video from the device into a buffer.
 *
 * The resulting image is stored in RGBA format across two buffers, rgb_buffer
 * and y_buffer.
 *
 * fd - a valid file descriptor pointing to the camera device.
 * frame_buffers - memory mapped buffers that contain the image from the device.
 * width - the width of the image.
 * height - the height of the image.
 * rgb_buffer - output buffer for RGB data.
 * y_buffer - output buffer for alpha (Y) data.
 *
 * Returns SUCCESS_LOCAL if no errors, otherwise ERROR_LOCAL.
 */
int read_frame(int fd, buffer* frame_buffers, int width, int height,
        int* rgb_buffer, int* y_buffer);

/* Private: Unconfigure the video device for capturing.
 *
 * Returns SUCCESS_LOCAL if no errors, otherwise ERROR_LOCAL.
 */
int stop_capturing(int fd);

/* Private: Request a frame of video from the device to be output into the rgb
 * and y buffers.
 *
 * If the descriptor is not valid, no frame will be read.
 *
 * fd - a valid file descriptor pointing to the camera device.
 * frame_buffers - memory mapped buffers that contain the image from the device.
 * width - the width of the image.
 * height - the height of the image.
 * rgb_buffer - output buffer for RGB data.
 * y_buffer - output buffer for alpha (Y) data.
 */
void process_camera(int fd, buffer* frame_buffers, int width,
        int height, int* rgb_buffer, int* ybuf);

/* Private: Stop capturing, uninitialize the device and free all memory. */
void stop_camera();

#endif // __CAPTURE_H__
