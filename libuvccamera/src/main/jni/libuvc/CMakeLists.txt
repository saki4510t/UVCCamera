cmake_minimum_required(VERSION 3.4)
project(libuvc)

if (NOT CMAKE_BUILD_TYPE)
  message(STATUS "No build type selected, default to Release")
  set(CMAKE_BUILD_TYPE "Release" CACHE STRING "" FORCE)
endif ()

if (NOT CMAKE_BUILD_TARGET)
  message(STATUS "No target type selected, default to shared library")
  set(CMAKE_BUILD_TARGET "Shared" CACHE STRING "" FORCE)
endif()

set(libuvc_VERSION_MAJOR 0)
set(libuvc_VERSION_MINOR 0)
set(libuvc_VERSION_PATCH 4)
set(libuvc_VERSION ${libuvc_VERSION_MAJOR}.${libuvc_VERSION_MINOR}.${libuvc_VERSION_PATCH})

#find_library(LIBUSB_LIBRARY_NAMES usb100
#	PATHS /opt/local/lib)

#find_path(LIBUSB_INCLUDE_DIR libusb/libusb.h
#	PATHS ../libusb/)
set(LIBUSB_INCLUDE_DIR ../libusb/)

#add_library(libusb100
#        SHARED
#        IMPORTED)

# Try to find JPEG using a module or pkg-config. If that doesn't work, search for the header.
find_package(jpeg QUIET)
if(NOT JPEG_FOUND)
  find_path(JPEG_INCLUDE_DIR jpeglib.h)
  if(JPEG_INCLUDE_DIR)
    set(JPEG_FOUND ON)
    set(JPEG_LIBRARIES -ljpeg)
  else()
    set(JPEG_FOUND ON)
    ADD_LIBRARY(jpeg-turbo1500 SHARED IMPORTED)
    SET_TARGET_PROPERTIES(jpeg-turbo1500 PROPERTIES IMPORTED_LOCATION
            ${CMAKE_SOURCE_DIR}/../jniLibs/${ANDROID_ABI}/libjpeg-turbo1500.so)
    set(JPEG_INCLUDE_DIR ../libjpeg-turbo-1.5.0/ ../libjpeg-turbo-1.5.0/include/)
    set(JPEG_LINK_FLAGS jpeg-turbo1500)
  endif()
endif()

#SET(CMAKE_C_FLAGS_DEBUG "-g -DUVC_DEBUGGING")

SET(INSTALL_CMAKE_DIR "${CMAKE_INSTALL_PREFIX}/lib/cmake/libuvc" CACHE PATH
	"Installation directory for CMake files")

SET(SOURCES src/ctrl.c src/device.c src/diag.c
           src/frame.c src/init.c src/stream.c
           src/misc.c)

include_directories(
  ${libuvc_SOURCE_DIR}/include
  ${libuvc_BINARY_DIR}/include
  ${LIBUSB_INCLUDE_DIR}
  ../
)

if(JPEG_FOUND)
  message(STATUS "Building libuvc with JPEG support.")
  include_directories(${JPEG_INCLUDE_DIR})
  SET(HAVE_JPEG TRUE)
  SET(SOURCES ${SOURCES} src/frame-mjpeg.c)
else()
  message(WARNING "JPEG not found. libuvc will not support JPEG decoding.")
endif()

if(${CMAKE_BUILD_TARGET} MATCHES "Shared")
  add_library(uvc SHARED ${SOURCES})
elseif(${CMAKE_BUILD_TARGET} MATCHES "Static")
  add_library(uvc STATIC ${SOURCES})
else()
  message( FATAL_ERROR "Invalid build type ${CMAKE_BUILD_TARGET}" )
endif()

configure_file(include/libuvc/libuvc_config.h.in
  ${PROJECT_BINARY_DIR}/include/libuvc/libuvc_config.h @ONLY)

set_target_properties(uvc PROPERTIES
  PUBLIC_HEADER "include/libuvc/libuvc.h;${libuvc_BINARY_DIR}/include/libuvc/libuvc_config.h" )

if(JPEG_FOUND)
  target_link_libraries (uvc ${JPEG_LINK_FLAGS})
endif(JPEG_FOUND)

target_link_libraries(uvc usb100)

#add_executable(test src/test.c)
#target_link_libraries(test uvc ${LIBUSB_LIBRARY_NAMES} opencv_highgui
#  opencv_core)

#install(TARGETS uvc
#  EXPORT libuvcTargets
#  LIBRARY DESTINATION "${CMAKE_INSTALL_PREFIX}/lib"
#  ARCHIVE DESTINATION "${CMAKE_INSTALL_PREFIX}/lib"
#  PUBLIC_HEADER DESTINATION "${CMAKE_INSTALL_PREFIX}/include/libuvc"
#)
#
#export(TARGETS uvc
#  FILE "${PROJECT_BINARY_DIR}/libuvcTargets.cmake")
#export(PACKAGE libuvc)
#
#set(CONF_INCLUDE_DIR "${CMAKE_INSTALL_PREFIX}/include")
#set(CONF_LIBRARY "${CMAKE_INSTALL_PREFIX}/lib/libuvc.so")
#
#configure_file(libuvcConfig.cmake.in ${PROJECT_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/libuvcConfig.cmake)
#
#configure_file(libuvcConfigVersion.cmake.in ${PROJECT_BINARY_DIR}/libuvcConfigVersion.cmake @ONLY)
#
#install(FILES
#  "${PROJECT_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/libuvcConfig.cmake"
#  "${PROJECT_BINARY_DIR}/libuvcConfigVersion.cmake"
#  DESTINATION "${INSTALL_CMAKE_DIR}")
#
#install(EXPORT libuvcTargets
#  DESTINATION "${INSTALL_CMAKE_DIR}")
