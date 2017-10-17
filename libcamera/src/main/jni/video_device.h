#ifndef __VIDEO_DEVICE_H__
#define __VIDEO_DEVICE_H__

#include <jni.h>
#include <stdio.h>

typedef struct {
    void* start;
    size_t length;
} buffer;

unsigned int BUFFER_COUNT;
buffer* FRAME_BUFFERS;

/* Private: Open the video device at the named device node.
 *
 * dev_name - the path to a device, e.g. /dev/video0
 * fd - an output parameter to store the file descriptor once opened.
 *
 * Returns SUCCESS_LOCAL if the device was found and opened and ERROR_LOCAL if
 * an error occurred.
 */
int open_device(const char* dev_name, int* fd);

/* Private: Initialize memory mapped buffers for video frames.
 *
 * fd - a valid file descriptor pointing to the camera device.
 *
 * Returns SUCCESS_LOCAL if no errors, otherwise ERROR_LOCAL.
 */
int init_mmap(int fd);

/* Private: Initialize video device with the given frame size.
 *
 * Initializes the device as a video capture device (must support V4L2) and
 * checks to make sure it has the streaming I/O interface. Configures the device
 * to crop the image to the given dimensions and initailizes a memory mapped
 * frame buffer.
 *
 * fd - a valid file descriptor to the device.
 * width - the desired width for the output images.
 * height - the desired height for the output images.
 *
 * Returns SUCCESS_LOCAL if no errors, otherwise ERROR_LOCAL.
 */
int init_device(int fd, int width, int height);

/* Private: Unmap and free memory-mapped frame buffers from the device.
 *
 * Returns SUCCESS_LOCAL if no errors, otherwise ERROR_LOCAL.
 */
int uninit_device();

/* Private: Close a file descriptor.
 *
 * fd - a pointer to the descriptor to close, which will be set to -1 on success
 *      or fail.
 *
 * Returns SUCCESS_LOCAL if no errors, otherwise ERROR_LOCAL.
 */
int close_device(int* fd);

#endif // __VIDEO_DEVICE_H__
