#include "capture.h"

#include "util.h"
#include "yuv.h"

#include <assert.h>
#include <fcntl.h>
#include <errno.h>
#include <linux/videodev2.h>

int start_capture(int fd) {
    unsigned int i;
    enum v4l2_buf_type type;

    for(i = 0; i < BUFFER_COUNT; ++i) {
        struct v4l2_buffer buf;
        CLEAR(buf);
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.index = i;

        if(-1 == xioctl(fd, VIDIOC_QBUF, &buf)) {
            return errnoexit("VIDIOC_QBUF");
        }
    }

    type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if(-1 == xioctl(fd, VIDIOC_STREAMON, &type)) {
        return errnoexit("VIDIOC_STREAMON");
    }

    return SUCCESS_LOCAL;
}

int read_frame(int fd, buffer* frame_buffers, int width, int height,
        int* rgb_buffer, int* y_buffer) {
    struct v4l2_buffer buf;
    CLEAR(buf);
    buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buf.memory = V4L2_MEMORY_MMAP;

    if(-1 == xioctl(fd, VIDIOC_DQBUF, &buf)) {
        switch(errno) {
            case EAGAIN:
                return 0;
            case EIO:
            default:
                return errnoexit("VIDIOC_DQBUF");
        }
    }

    assert(buf.index < BUFFER_COUNT);
    yuyv422_to_argb(frame_buffers[buf.index].start, width, height, rgb_buffer,
            y_buffer);

    if(-1 == xioctl(fd, VIDIOC_QBUF, &buf)) {
        return errnoexit("VIDIOC_QBUF");
    }

    return 1;
}

int stop_capturing(int fd) {
    enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if(-1 != fd && -1 == xioctl(fd, VIDIOC_STREAMOFF, &type)) {
        return errnoexit("VIDIOC_STREAMOFF");
    }

    return SUCCESS_LOCAL;
}

void process_camera(int fd, buffer* frame_buffers, int width,
        int height, int* rgb_buffer, int* ybuf) {
    if(fd == -1) {
        return;
    }

    for(;;) {
        fd_set fds;
        FD_ZERO(&fds);
        FD_SET(fd, &fds);

        struct timeval tv;
        tv.tv_sec = 2;
        tv.tv_usec = 0;

        int result = select(fd + 1, &fds, NULL, NULL, &tv);
        if(-1 == result) {
            if(EINTR == errno) {
                continue;
            }
            errnoexit("select");
        } else if(0 == result) {
            LOGE("select timeout");
        }

        if(read_frame(fd, frame_buffers, width, height, rgb_buffer, ybuf) == 1) {
            break;
        }
    }
}

void stop_camera(int* fd, int* rgb_buffer, int* y_buffer) {
    stop_capturing(*fd);
    uninit_device();
    close_device(fd);

    if(rgb_buffer) {
        free(rgb_buffer);
    }

    if(y_buffer) {
        free(y_buffer);
    }
}
