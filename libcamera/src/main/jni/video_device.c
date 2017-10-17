#include "video_device.h"
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <linux/videodev2.h>
#include "util.h"
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <malloc.h>

int open_device(const char* dev_name, int* fd) {
    struct stat st;
    if(-1 == stat(dev_name, &st)) {
        LOGE("Cannot identify '%s': %d, %s", dev_name, errno, strerror(errno));
        return ERROR_LOCAL;
    }

    if(!S_ISCHR(st.st_mode)) {
        LOGE("%s is not a valid device", dev_name);
        return ERROR_LOCAL;
    }

    *fd = open(dev_name, O_RDWR | O_NONBLOCK, 0);
    if(-1 == *fd) {
        LOGE("Cannot open '%s': %d, %s", dev_name, errno, strerror(errno));
        if(EACCES == errno) {
            LOGE("Insufficient permissions on '%s': %d, %s", dev_name, errno,
                    strerror(errno));
        }
        return ERROR_LOCAL;
    }

    return SUCCESS_LOCAL;
}

int init_mmap(int fd) {
    struct v4l2_requestbuffers req;
    CLEAR(req);
    req.count = 4;
    req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    req.memory = V4L2_MEMORY_MMAP;

    if(-1 == xioctl(fd, VIDIOC_REQBUFS, &req)) {
        if(EINVAL == errno) {
            LOGE("device does not support memory mapping");
            return ERROR_LOCAL;
        } else {
            return errnoexit("VIDIOC_REQBUFS");
        }
    }

    if(req.count < 2) {
        LOGE("Insufficient buffer memory");
        return ERROR_LOCAL;
    }

    FRAME_BUFFERS = calloc(req.count, sizeof(*FRAME_BUFFERS));
    if(!FRAME_BUFFERS) {
        LOGE("Out of memory");
        return ERROR_LOCAL;
    }

    for(BUFFER_COUNT = 0; BUFFER_COUNT < req.count; ++BUFFER_COUNT) {
        struct v4l2_buffer buf;
        CLEAR(buf);

        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.index = BUFFER_COUNT;

        if(-1 == xioctl(fd, VIDIOC_QUERYBUF, &buf)) {
            return errnoexit("VIDIOC_QUERYBUF");
        }

        FRAME_BUFFERS[BUFFER_COUNT].length = buf.length;
        FRAME_BUFFERS[BUFFER_COUNT].start = mmap(NULL, buf.length,
                PROT_READ | PROT_WRITE, MAP_SHARED, fd, buf.m.offset);

        if(MAP_FAILED == FRAME_BUFFERS[BUFFER_COUNT].start) {
            return errnoexit("mmap");
        }
    }

    return SUCCESS_LOCAL;
}

int init_device(int fd, int width, int height) {
    struct v4l2_capability cap;
    struct v4l2_cropcap cropcap;
    struct v4l2_crop crop;
    struct v4l2_format fmt;
    unsigned int min;

    if(-1 == xioctl(fd, VIDIOC_QUERYCAP, &cap)) {
        if(EINVAL == errno) {
            LOGE("not a valid V4L2 device");
            return ERROR_LOCAL;
        } else {
            return errnoexit("VIDIOC_QUERYCAP");
        }
    }

    if(!(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE)) {
        LOGE("device is not a video capture device");
        return ERROR_LOCAL;
    }

    if(!(cap.capabilities & V4L2_CAP_STREAMING)) {
        LOGE("device does not support streaming i/o");
        return ERROR_LOCAL;
    }

    CLEAR(cropcap);
    cropcap.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    if(0 == xioctl(fd, VIDIOC_CROPCAP, &cropcap)) {
        crop.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        crop.c = cropcap.defrect;

        if(-1 == xioctl(fd, VIDIOC_S_CROP, &crop)) {
            switch(errno) {
                case EINVAL:
                    break;
                default:
                    break;
            }
        }
    }

    CLEAR(fmt);
    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    fmt.fmt.pix.width = width;
    fmt.fmt.pix.height = height;

    fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV;
    fmt.fmt.pix.field = V4L2_FIELD_INTERLACED;

    if(-1 == xioctl(fd, VIDIOC_S_FMT, &fmt)) {
        return errnoexit("VIDIOC_S_FMT");
    }

    min = fmt.fmt.pix.width * 2;
    if(fmt.fmt.pix.bytesperline < min) {
        fmt.fmt.pix.bytesperline = min;
    }

    min = fmt.fmt.pix.bytesperline * fmt.fmt.pix.height;
    if(fmt.fmt.pix.sizeimage < min) {
        fmt.fmt.pix.sizeimage = min;
    }

    return init_mmap(fd);
}

int uninit_device() {
    for(unsigned int i = 0; i < BUFFER_COUNT; ++i) {
        if(-1 == munmap(FRAME_BUFFERS[i].start, FRAME_BUFFERS[i].length)) {
            return errnoexit("munmap");
        }
    }

    free(FRAME_BUFFERS);
    return SUCCESS_LOCAL;
}

int close_device(int* fd) {
    int result = SUCCESS_LOCAL;
    if(-1 != *fd && -1 == close(*fd)) {
        result = errnoexit("close");
    }
    *fd = -1;
    return result;
}

