CMAKE_MINIMUM_REQUIRED(VERSION 3.4.1)

INCLUDE_DIRECTORIES(../)
INCLUDE_DIRECTORIES(libusb)
INCLUDE_DIRECTORIES(libusb/os)
INCLUDE_DIRECTORIES(android)

#ADD_DEFINITIONS(-DENABLE_DEBUG_LOGGING)
ADD_DEFINITIONS(-DACCESS_RAW_DESCRIPTORS)

#SET(CMAKE_CXX_FLAGS "-isystem ${NDK_HOME}/sysroot/usr/include/arm-linux-androideabi")
#include_directories(${NDK_HOME}/sysroot/usr/include/arm-linux-androideabi)
SET(CMAKE_CXX_FLAGS "-isystem ${ANDROID_SYSROOT}/usr/include/arm-linux-androideabi")
include_directories(${ANDROID_SYSROOT}/usr/include/arm-linux-androideabi)
SET(CMAKE_C_FLAGS "-O3 -fstrict-aliasing -fprefetch-loop-arrays")

SET(DIR_LIB_USB_SRCS
        libusb/core.c
        libusb/descriptor.c
        libusb/hotplug.c
        libusb/io.c
        libusb/sync.c
        libusb/strerror.c
        )

SET(DIR_LIB_USB_OS_SRCS
        libusb/os/android_netlink.c
        libusb/os/android_usbfs.c
        libusb/os/poll_posix.c
        libusb/os/threads_posix.c
        )

# 指定生成 logger 链接库
ADD_LIBRARY(usb100 SHARED ${DIR_LIB_USB_SRCS} ${DIR_LIB_USB_OS_SRCS})

FIND_LIBRARY(log-lib log)

TARGET_LINK_LIBRARIES(usb100 ${log-lib})