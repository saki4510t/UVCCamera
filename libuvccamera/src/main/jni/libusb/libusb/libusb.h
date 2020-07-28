/*
 * Public libusb header file
 * Copyright © 2001 Johannes Erdfelt <johannes@erdfelt.com>
 * Copyright © 2007-2008 Daniel Drake <dsd@gentoo.org>
 * Copyright © 2012 Pete Batard <pete@akeo.ie>
 * Copyright © 2012 Nathan Hjelm <hjelmn@cs.unm.edu>
 * For more information, please visit: http://libusb.info
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#ifndef LIBUSB_H
#define LIBUSB_H

#ifdef _MSC_VER
/* on MS environments, the inline keyword is available in C++ only
 * 在MS环境中，inline关键字仅在C ++中可用
 */
#if !defined(__cplusplus)
#define inline __inline
#endif
/* ssize_t is also not available (copy/paste from MinGW)
 * ssize_t也不可用（从MinGW复制/粘贴）
 */
#ifndef _SSIZE_T_DEFINED
#define _SSIZE_T_DEFINED
#undef ssize_t
#ifdef _WIN64
  typedef __int64 ssize_t;
#else
  typedef int ssize_t;
#endif /* _WIN64 */
#endif /* _SSIZE_T_DEFINED */
#endif /* _MSC_VER */

/* stdint.h is not available on older MSVC
 * stdint.h在较旧的MSVC上不可用
 */
#if defined(_MSC_VER) && (_MSC_VER < 1600) && (!defined(_STDINT)) && (!defined(_STDINT_H))
typedef unsigned __int8   uint8_t;
typedef unsigned __int16  uint16_t;
typedef unsigned __int32  uint32_t;
#else
#include <stdint.h>
#endif

#if !defined(_WIN32_WCE)
#include <sys/types.h>
#endif

#if defined(__linux) || defined(__APPLE__) || defined(__CYGWIN__)
#include <sys/time.h>
#endif

#include <time.h>
#include <limits.h>

/* 'interface' might be defined as a macro on Windows, so we need to
 * undefine it so as not to break the current libusb API, because
 * libusb_config_descriptor has an 'interface' member
 * As this can be problematic if you include windows.h after libusb.h
 * in your sources, we force windows.h to be included first.
 *
 * 在Windows上可能将'interface'定义为宏，因此我们需要取消定义它，以免破坏当前的libusb API，
 * 因为libusb_config_descriptor具有'interface'成员。如果在libusb之后包含windows.h，可能会出现问题。
 * 在您的资源中，我们强制首先包含windows.h。
 */
#if defined(_WIN32) || defined(__CYGWIN__) || defined(_WIN32_WCE)
#include <windows.h>
#if defined(interface)
#undef interface
#endif
#if !defined(__CYGWIN__)
#include <winsock.h>
#endif
#endif

#if __GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 5)
#define LIBUSB_DEPRECATED_FOR(f) \
  __attribute__((deprecated("Use " #f " instead")))
#else
#define LIBUSB_DEPRECATED_FOR(f)
#endif /* __GNUC__ */

/** \def LIBUSB_CALL
 * \ingroup misc
 * libusb's Windows calling convention.
 *
 * Under Windows, the selection of available compilers and configurations
 * means that, unlike other platforms, there is not <em>one true calling
 * convention</em> (calling convention: the manner in which parameters are
 * passed to funcions in the generated assembly code).
 *
 * Matching the Windows API itself, libusb uses the WINAPI convention (which
 * translates to the <tt>stdcall</tt> convention) and guarantees that the
 * library is compiled in this way. The public header file also includes
 * appropriate annotations so that your own software will use the right
 * convention, even if another convention is being used by default within
 * your codebase.
 *
 * The one consideration that you must apply in your software is to mark
 * all functions which you use as libusb callbacks with this LIBUSB_CALL
 * annotation, so that they too get compiled for the correct calling
 * convention.
 *
 * On non-Windows operating systems, this macro is defined as nothing. This
 * means that you can apply it to your code without worrying about
 * cross-platform compatibility.
 *
 * libusb的Windows调用约定。
 * 在Windows下，选择可用的编译器和配置意味着与其他平台不同，没有一个真正的调用约定（调用约定：将参数传递给生成的汇编代码中的函数的方式）。
 * libusb与Windows API本身匹配，使用WINAPI约定（转换为stdcall约定）并保证以这种方式编译该库。 公共头文件还包含适当的注释，即使您的代码库默认情况下使用其他约定，您自己的软件也将使用正确的约定。
 * 您必须在软件中应用的一个注意事项是，使用此LIBUSB_CALL批注标记所有用作libusb回调的函数，以便也针对正确的调用约定对它们进行编译。
 * 在非Windows操作系统上，此宏定义为空。 这意味着您可以将其应用于代码，而不必担心跨平台兼容性。
 */
/* LIBUSB_CALL must be defined on both definition and declaration of libusb
 * functions. You'd think that declaration would be enough, but cygwin will
 * complain about conflicting types unless both are marked this way.
 * The placement of this macro is important too; it must appear after the
 * return type, before the function name. See internal documentation for
 * API_EXPORTED.
 *
 * LIBUSB_CALL必须在libusb函数的定义和声明中都定义。
 * 您可能认为声明就足够了，但是cygwin会抱怨类型冲突，除非这两种方式都被标记了。
 * 这个宏的位置也很重要。 它必须出现在返回类型之后，函数名称之前。
 * 请参阅API_EXPORTED的内部文档。
 */
#if defined(_WIN32) || defined(__CYGWIN__) || defined(_WIN32_WCE)
#define LIBUSB_CALL WINAPI
#else
#define LIBUSB_CALL
#endif

/** \def LIBUSB_API_VERSION
 * \ingroup misc
 * libusb's API version.
 *
 * Since version 1.0.13, to help with feature detection, libusb defines
 * a LIBUSB_API_VERSION macro that gets increased every time there is a
 * significant change to the API, such as the introduction of a new call,
 * the definition of a new macro/enum member, or any other element that
 * libusb applications may want to detect at compilation time.
 *
 * The macro is typically used in an application as follows:
 * \code
 * #if defined(LIBUSB_API_VERSION) && (LIBUSB_API_VERSION >= 0x01001234)
 * // Use one of the newer features from the libusb API
 * #endif
 * \endcode
 *
 * Another feature of LIBUSB_API_VERSION is that it can be used to detect
 * whether you are compiling against the libusb or the libusb library.
 *
 * Internally, LIBUSB_API_VERSION is defined as follows:
 * (libusb major << 24) | (libusb minor << 16) | (16 bit incremental)
 *
 * libusb的API版本。
 * 从1.0.13版本开始，libusb定义了一个LIBUSB_API_VERSION宏，该宏会在API发生重大更改时增加，
 * 例如引入新的调用，定义新的宏/枚举成员， 或libusb应用程序可能希望在编译时检测到的任何其他元素。
 * 宏通常在应用程序中使用，如下所示：
 * #if defined（LIBUSB_API_VERSION）&&（LIBUSB_API_VERSION> = 0x01001234）
 * //使用libusb API的较新功能之一
 * #endif
 * LIBUSB_API_VERSION的另一个功能是可以用来检测无论是针对libusb还是针对libusb库进行编译。
 * 在内部，LIBUSB_API_VERSION定义如下：
 * (libusb major << 24) | (libusb minor << 16) | (16 bit incremental)
 */
#define LIBUSB_API_VERSION 0x01000103

/* The following is kept for compatibility, but will be deprecated in the future
 * 出于兼容性考虑，保留了以下内容，但将来会不推荐使用
 */
#define LIBUSBX_API_VERSION LIBUSB_API_VERSION

#ifdef __cplusplus
extern "C" {
#endif

/**
 * \ingroup misc
 * Convert a 16-bit value from host-endian to little-endian format. On
 * little endian systems, this function does nothing. On big endian systems,
 * the bytes are swapped.
 * \param x the host-endian value to convert
 * \returns the value in little-endian byte order
 *
 * 将16位值从host-endian转换为little-endian格式。 在小端系统上，此功能不执行任何操作。 在大字节序系统上，字节被交换。
 */
static inline uint16_t libusb_cpu_to_le16(const uint16_t x)
{
	union {
		uint8_t  b8[2];
		uint16_t b16;
	} _tmp;
	_tmp.b8[1] = (uint8_t) (x >> 8);
	_tmp.b8[0] = (uint8_t) (x & 0xff);
	return _tmp.b16;
}

/** \def libusb_le16_to_cpu
 * \ingroup misc
 * Convert a 16-bit value from little-endian to host-endian format. On
 * little endian systems, this function does nothing. On big endian systems,
 * the bytes are swapped.
 * \param x the little-endian value to convert
 * \returns the value in host-endian byte order
 *
 * 将16位值从little-endian转换为host-endian格式。 在小端系统上，此功能不执行任何操作。 在大字节序系统上，字节被交换。
 */
#define libusb_le16_to_cpu libusb_cpu_to_le16

/* standard USB stuff
 * 标准USB东西
 */

/** \ingroup desc
 * Device and/or Interface Class codes
 * 设备和/或接口类代码
 */
enum libusb_class_code {
	/** In the context of a \ref libusb_device_descriptor "device descriptor",
	 * this bDeviceClass value indicates that each interface specifies its
	 * own class information and all interfaces operate independently.
	 *
	 * 在libusb_device_descriptor“设备描述符”的上下文中，此bDeviceClass值指示每个接口指定其自己的类信息，并且所有接口都独立运行。
	 */
	LIBUSB_CLASS_PER_INTERFACE = 0,

	/** Audio class
	 * 音频类
	 */
	LIBUSB_CLASS_AUDIO = 1,

	/** Communications class
	 * 通讯类
	 */
	LIBUSB_CLASS_COMM = 2,

	/** Human Interface Device class
	 * 人机界面设备类
	 */
	LIBUSB_CLASS_HID = 3,

	/** Physical
	 * 物理 */
	LIBUSB_CLASS_PHYSICAL = 5,

	/** Image class
	 * 图片类
	 */
	LIBUSB_CLASS_PTP = 6, /* legacy name from libusb-0.1 usb.h  libusb-0.1 usb.h中的旧名称 */
	LIBUSB_CLASS_IMAGE = 6,

	/** Printer class
	 * 打印类 */
	LIBUSB_CLASS_PRINTER = 7,

	/** Mass storage class
	 * 大容量存储类
	 */
	LIBUSB_CLASS_MASS_STORAGE = 8,

	/** Hub class
	 * 集线器类
	 */
	LIBUSB_CLASS_HUB = 9,

	/** Data class
	 * 资料类
	 */
	LIBUSB_CLASS_DATA = 10,

	/** Smart Card
	 * 智能卡
	 */
	LIBUSB_CLASS_SMART_CARD = 0x0b,

	/** Content Security
	 * 内容安全 */
	LIBUSB_CLASS_CONTENT_SECURITY = 0x0d,

	/** Video
	 * 视频
	 */
	LIBUSB_CLASS_VIDEO = 0x0e,

	/** Personal Healthcare
	 * 个人保健
	 */
	LIBUSB_CLASS_PERSONAL_HEALTHCARE = 0x0f,

	/** Diagnostic Device
	 * 诊断装置
	 */
	LIBUSB_CLASS_DIAGNOSTIC_DEVICE = 0xdc,

	/** Wireless class
	 * 无线类
	 */
	LIBUSB_CLASS_WIRELESS = 0xe0,

	/** Application class
	 * 应用类
	 */
	LIBUSB_CLASS_APPLICATION = 0xfe,

	/** Class is vendor-specific
	 * 类是特定于供应商的
	 */
	LIBUSB_CLASS_VENDOR_SPEC = 0xff
};

/** \ingroup desc
 * Descriptor types as defined by the USB specification.
 * USB规范定义的描述符类型。
 */
enum libusb_descriptor_type {
	/** Device descriptor. See libusb_device_descriptor.
	 * 设备描述符。 请参阅libusb_device_descriptor。
	 */
	LIBUSB_DT_DEVICE = 0x01,

	/** Configuration descriptor. See libusb_config_descriptor.
	 * 配置描述符。 请参阅libusb_config_descriptor。
	 */
	LIBUSB_DT_CONFIG = 0x02,

	/** String descriptor
	 * 字符串描述符
	 */
	LIBUSB_DT_STRING = 0x03,

	/** Interface descriptor. See libusb_interface_descriptor.
	 * 接口描述符。 参见libusb_interface_descriptor。
	 */
	LIBUSB_DT_INTERFACE = 0x04,

	/** Endpoint descriptor. See libusb_endpoint_descriptor.
	 * 端点描述符。 参见libusb_endpoint_descriptor。
	 */
	LIBUSB_DT_ENDPOINT = 0x05,

	/** XXX */
	LIBUSB_DT_DEVICE_QUALIFIER = 0x06,				// deprecated on USB3.0  在USB3.0上已弃用

	/** XXX */
	LIBUSB_DT_OTHER_SPEED_CONFIGURATION = 0x07,		// deprecated on USB3.0  在USB3.0上已弃用

	/** XXX */
	LIBUSB_DT_INTERFACE_POWER = 0x08,

	/** XXX */
	LIBUSB_DT_OTG = 0x09,

	/** XXX */
	LIBUSB_DT_DEBUG = 0x0a,

	/** XXX Interface Association descriptor(IAD) See libusb_association_descriptor
	 * 接口关联描述符（IAD）参见libusb_association_descriptor
	 */
	LIBUSB_DT_ASSOCIATION = 0x0b,

	/** BOS descriptor
	 * BOS描述符
	 */
	LIBUSB_DT_BOS = 0x0f,

	/** Device Capability descriptor
	 * 设备功能描述符
	 */
	LIBUSB_DT_DEVICE_CAPABILITY = 0x10,
// Class specified descriptors  类指定的描述符
	/** HID descriptor
	 * HID描述符
	 */
	LIBUSB_DT_HID = 0x21,

	/** HID report descriptor
	 * HID报告描述符
	 */
	LIBUSB_DT_HID_REPORT = 0x22,

	/** Physical descriptor
	 * 物理描述符
	 */
	LIBUSB_DT_HID_PHYSICAL = 0x23,

	/* Class specific interface descriptor
	 * 类特定的接口描述符
	 */
	LIBUSB_DT_CS_INTERFACE = 0x24,

	/* Class specific endpoint descriptor
	 * 类特定的端点描述符
	 */
	LIBUSB_DT_CS_ENDPOINT = 0x25,

	/** Hub descriptor
	 * 集线器描述符
	 */
	LIBUSB_DT_HUB = 0x29,

	/** SuperSpeed Hub descriptor
	 * SuperSpeed Hub描述符
	 */
	LIBUSB_DT_SUPERSPEED_HUB = 0x2a,

	/** SuperSpeed Endpoint Companion descriptor
	 * SuperSpeed Endpoint Companion描述符
	 */
	LIBUSB_DT_SS_ENDPOINT_COMPANION = 0x30		// defined on USB 3.0   在USB 3.0上定义
};

/* Descriptor sizes per descriptor type
 * 每种描述符类型的描述符大小
 */
#define LIBUSB_DT_HEADER_SIZE					2	// XXX
#define LIBUSB_DT_DEVICE_SIZE					18
#define LIBUSB_DT_CONFIG_SIZE					9
#define LIBUSB_DT_INTERFACE_SIZE				9
#define LIBUSB_DT_ENDPOINT_SIZE					7
#define LIBUSB_DT_ENDPOINT_AUDIO_SIZE			9	/* Audio extension  音频扩展 */
#define LIBUSB_DT_HUB_NONVAR_SIZE				7
#define LIBUSB_DT_SS_ENDPOINT_COMPANION_SIZE	6
#define LIBUSB_DT_BOS_SIZE						5
#define LIBUSB_DT_DEVICE_CAPABILITY_SIZE		3
#define LIBUSB_DT_QUALIFER_SIZE					10	// XXX
#define LIBUSB_DT_OTHER_SPEED_SIZE				9	// XXX
#define LIBUSB_DT_ASSOCIATION_SIZE				8	// XXX add to support IAD  添加以支持IAD

/* BOS descriptor sizes
 * BOS描述符大小
 */
#define LIBUSB_BT_USB_2_0_EXTENSION_SIZE		7
#define LIBUSB_BT_SS_USB_DEVICE_CAPABILITY_SIZE	10
#define LIBUSB_BT_CONTAINER_ID_SIZE				20

/* We unwrap the BOS => define its max size
 * 我们解开BOS => 定义其最大大小
 */
#define LIBUSB_DT_BOS_MAX_SIZE		((LIBUSB_DT_BOS_SIZE)     +\
					(LIBUSB_BT_USB_2_0_EXTENSION_SIZE)       +\
					(LIBUSB_BT_SS_USB_DEVICE_CAPABILITY_SIZE) +\
					(LIBUSB_BT_CONTAINER_ID_SIZE))

#define LIBUSB_ENDPOINT_ADDRESS_MASK	0x0f    /* in bEndpointAddress 在bEndpointAddress中 */
#define LIBUSB_ENDPOINT_DIR_MASK		0x80

/** \ingroup desc
 * Endpoint direction. Values for bit 7 of the
 * \ref libusb_endpoint_descriptor::bEndpointAddress "endpoint address" scheme.
 * 端点方向。 libusb_endpoint_descriptor::bEndpointAddress "endpoint address" 方案的位7的值。
 */
enum libusb_endpoint_direction {
	/**
	 * In: device-to-host
	 * 在：设备到主机
	 */
	LIBUSB_ENDPOINT_IN = 0x80,

	/**
	 * Out: host-to-device
	 * 输出：主机到设备
	 */
	LIBUSB_ENDPOINT_OUT = 0x00
};

#define LIBUSB_TRANSFER_TYPE_MASK			0x03    /* in bmAttributes  在bmAttributes中 */

/** \ingroup desc
 * Endpoint transfer type. Values for bits 0:1 of the
 * \ref libusb_endpoint_descriptor::bmAttributes "endpoint attributes" field.
 * 端点传输类型。 libusb_endpoint_descriptor::bmAttributes "endpoint attributes" 字段的位 0:1 的值。
 */
enum libusb_transfer_type {
	/**
	 * Control endpoint
	 * 控制端点
	 */
	LIBUSB_TRANSFER_TYPE_CONTROL = 0,

	/**
	 * Isochronous endpoint
	 * 同步端点
	 */
	LIBUSB_TRANSFER_TYPE_ISOCHRONOUS = 1,

	/**
	 * Bulk endpoint
	 * 批量端点
	 */
	LIBUSB_TRANSFER_TYPE_BULK = 2,

	/**
	 * Interrupt endpoint
	 * 中断端点
	 */
	LIBUSB_TRANSFER_TYPE_INTERRUPT = 3,

	/**
	 * Stream endpoint
	 * 流端点
	 */
	LIBUSB_TRANSFER_TYPE_BULK_STREAM = 4,
};

/** \ingroup misc
 * Standard requests, as defined in table 9-5 of the USB 3.0 specifications
 * 标准要求，如USB 3.0规范的表9-5中所定义
 */
enum libusb_standard_request {
	/** Request status of the specific recipient
	 * 特定收件人的请求状态
	 */
	LIBUSB_REQUEST_GET_STATUS = 0x00,

	/** Clear or disable a specific feature
	 * 清除或禁用特定功能 */
	LIBUSB_REQUEST_CLEAR_FEATURE = 0x01,

	/* 0x02 is reserved  被预留 */

	/** Set or enable a specific feature
	 * 设置或启用特定功能 */
	LIBUSB_REQUEST_SET_FEATURE = 0x03,

	/* 0x04 is reserved  被预留 */

	/** Set device address for all future accesses
	 * 设置所有将来访问的设备地址 */
	LIBUSB_REQUEST_SET_ADDRESS = 0x05,

	/** Get the specified descriptor
	 * 获取指定的描述符
	 */
	LIBUSB_REQUEST_GET_DESCRIPTOR = 0x06,

	/** Used to update existing descriptors or add new descriptors
	 * 用于更新现有描述符或添加新描述符
	 */
	LIBUSB_REQUEST_SET_DESCRIPTOR = 0x07,

	/** Get the current device configuration value
	 * 获取当前设备配置值
	 */
	LIBUSB_REQUEST_GET_CONFIGURATION = 0x08,

	/** Set device configuration
	 * 设置设备配置
	 */
	LIBUSB_REQUEST_SET_CONFIGURATION = 0x09,

	/** Return the selected alternate setting for the specified interface
	 * 返回指定接口的选定替代设置 */
	LIBUSB_REQUEST_GET_INTERFACE = 0x0A,

	/** Select an alternate interface for the specified interface
	 * 为指定接口选择备用接口
	 */
	LIBUSB_REQUEST_SET_INTERFACE = 0x0B,

	/** Set then report an endpoint's synchronization frame
	 * 设置然后报告端点的同步帧
	 */
	LIBUSB_REQUEST_SYNCH_FRAME = 0x0C,

	/** Sets both the U1 and U2 Exit Latency
	 * 设置U1和U2退出延迟
	 */
	LIBUSB_REQUEST_SET_SEL = 0x30,

	/** Delay from the time a host transmits a packet to the time it is received by the device.
	  * 从主机发送数据包到设备接收数据包的时间延迟。 */
	LIBUSB_SET_ISOCH_DELAY = 0x31,
};

/** \ingroup misc
 * Request type bits of the
 * \ref libusb_control_setup::bmRequestType "bmRequestType" field in control
 * transfers.
 * 控制传输中libusb_control_setup::bmRequestType "bmRequestType" 字段的请求类型位。
 */
enum libusb_request_type {
	/** Standard
	 * 标准
	 */
	LIBUSB_REQUEST_TYPE_STANDARD = (0x00 << 5),

	/** Class
	 * 类
	 */
	LIBUSB_REQUEST_TYPE_CLASS = (0x01 << 5),

	/** Vendor
	 * 供应商
	 */
	LIBUSB_REQUEST_TYPE_VENDOR = (0x02 << 5),

	/** Reserved
	 * 预留
	 */
	LIBUSB_REQUEST_TYPE_RESERVED = (0x03 << 5)
};

/** \ingroup misc
 * Recipient bits of the
 * \ref libusb_control_setup::bmRequestType "bmRequestType" field in control
 * transfers. Values 4 through 31 are reserved.
 * 控制传输中 libusb_control_setup::bmRequestType "bmRequestType" 字段的收件人位。 值4到31保留。
 */
enum libusb_request_recipient {
	/** Device
	 * 设备
	 */
	LIBUSB_RECIPIENT_DEVICE = 0x00,

	/** Interface
	 * 接口
	 */
	LIBUSB_RECIPIENT_INTERFACE = 0x01,

	/** Endpoint
	 * 终点
	 */
	LIBUSB_RECIPIENT_ENDPOINT = 0x02,

	/** Other
	 * 其他
	 */
	LIBUSB_RECIPIENT_OTHER = 0x03,
};

#define LIBUSB_ISO_SYNC_TYPE_MASK		0x0C

/** \ingroup desc
 * Synchronization type for isochronous endpoints. Values for bits 2:3 of the
 * \ref libusb_endpoint_descriptor::bmAttributes "bmAttributes" field in
 * libusb_endpoint_descriptor.
 * 等时端点的同步类型。  libusb_endpoint_descriptor中libusb_endpoint_descriptor::bmAttributes "bmAttributes"字段的位2:3的值。
 */
enum libusb_iso_sync_type {
	/** No synchronization
	 * 没有同步
	 */
	LIBUSB_ISO_SYNC_TYPE_NONE = 0,

	/** Asynchronous
	 * 异步
	 */
	LIBUSB_ISO_SYNC_TYPE_ASYNC = 1,

	/** Adaptive
	 * 自适应
	 */
	LIBUSB_ISO_SYNC_TYPE_ADAPTIVE = 2,

	/** Synchronous
	 * 同步
	 */
	LIBUSB_ISO_SYNC_TYPE_SYNC = 3
};

#define LIBUSB_ISO_USAGE_TYPE_MASK 0x30

/** \ingroup desc
 * Usage type for isochronous endpoints. Values for bits 4:5 of the
 * \ref libusb_endpoint_descriptor::bmAttributes "bmAttributes" field in
 * libusb_endpoint_descriptor.
 * 同步端点的用法类型。  libusb_endpoint_descriptor中libusb_endpoint_descriptor::bmAttributes "bmAttributes"字段的4:5位的值。
 */
enum libusb_iso_usage_type {
	/** Data endpoint
	 * 数据端点
	 */
	LIBUSB_ISO_USAGE_TYPE_DATA = 0,

	/** Feedback endpoint
	 * 反馈端点
	 */
	LIBUSB_ISO_USAGE_TYPE_FEEDBACK = 1,

	/** Implicit feedback Data endpoint
	 * 隐式反馈数据端点
	 */
	LIBUSB_ISO_USAGE_TYPE_IMPLICIT = 2,
};

/** \ingroup desc
 * A structure representing the standard USB device descriptor. This
 * descriptor is documented in section 9.6.1 of the USB 3.0 specification.
 * All multiple-byte fields are represented in host-endian format.
 *
 * 表示标准USB设备描述符的结构。USB 3.0规范的第9.6.1节记录了该描述符。所有多字节字段均以主机字节序格式表示。
 */
struct libusb_device_descriptor {
	/** Size of this descriptor (in bytes)
	 * 该描述符的大小（以字节为单位）
	 */
	uint8_t  bLength;

	/** Descriptor type. Will have value
	 * \ref libusb_descriptor_type::LIBUSB_DT_DEVICE LIBUSB_DT_DEVICE in this
	 * context.
	 * 描述符类型。 在这种情况下，将具有值 libusb_descriptor_type::LIBUSB_DT_DEVICE LIBUSB_DT_DEVICE。
	 */
	uint8_t  bDescriptorType;

	/** USB specification release number in binary-coded decimal. A value of
	 * 0x0200 indicates USB 2.0, 0x0110 indicates USB 1.1, etc.
	 * USB规范版本号，以二进制编码的十进制数表示。 值0x0200表示USB 2.0,0x0110表示USB 1.1，依此类推。
	 */
	uint16_t bcdUSB;

	/** USB-IF class code for the device. See \ref libusb_class_code.
	 * 设备的USB-IF类代码。 参见libusb_class_code。
	 */
	uint8_t  bDeviceClass;

	/** USB-IF subclass code for the device, qualified by the bDeviceClass value
	 * 设备的USB-IF子类代码，由bDeviceClass值限定 */
	uint8_t  bDeviceSubClass;

	/** USB-IF protocol code for the device, qualified by the bDeviceClass and bDeviceSubClass values
	 * 设备的USB-IF协议代码，由bDeviceClass和bDeviceSubClass值限定 */
	uint8_t  bDeviceProtocol;

	/** Maximum packet size for endpoint 0
	 * 端点0的最大封包大小
	 */
	uint8_t  bMaxPacketSize0;

	/** USB-IF vendor ID
	 * USB-IF供应商ID
	 */
	uint16_t idVendor;

	/** USB-IF product ID
	 * USB-IF产品编号
	 */
	uint16_t idProduct;

	/** Device release number in binary-coded decimal
	 * 设备发布号（二进制编码的十进制）
	 */
	uint16_t bcdDevice;

	/** Index of string descriptor describing manufacturer
	 * 描述制造商的字符串描述符索引
	 */
	uint8_t  iManufacturer;

	/** Index of string descriptor describing product
	 * 描述产品的字符串描述符索引
	 */
	uint8_t  iProduct;

	/** Index of string descriptor containing device serial number
	 * 包含设备序列号的字符串描述符的索引
	 */
	uint8_t  iSerialNumber;

	/** Number of possible configurations
	 * 可能的配置数量
	 */
	uint8_t  bNumConfigurations;
};

/** \ingroup desc
 * A structure representing the standard USB endpoint descriptor. This
 * descriptor is documented in section 9.6.6 of the USB 3.0 specification.
 * All multiple-byte fields are represented in host-endian format.
 *
 * 表示标准USB端点描述符的结构。USB 3.0规范的第9.6.6节记录了该描述符。 所有多字节字段均以主机字节序格式表示。
 */
struct libusb_endpoint_descriptor {
	/** Size of this descriptor (in bytes)
	 * 该描述符的大小（以字节为单位）
	 */
	uint8_t  bLength;

	/** Descriptor type. Will have value
	 * \ref libusb_descriptor_type::LIBUSB_DT_ENDPOINT LIBUSB_DT_ENDPOINT in
	 * this context.
	 * 描述符类型。 在这种情况下，将具有值libusb_descriptor_type::LIBUSB_DT_ENDPOINT LIBUSB_DT_ENDPOINT。
	 */
	uint8_t  bDescriptorType;

	/** The address of the endpoint described by this descriptor. Bits 0:3 are
	 * the endpoint number. Bits 4:6 are reserved. Bit 7 indicates direction,
	 * see \ref libusb_endpoint_direction.
	 * 此描述符描述的端点地址。 位0:3是端点号。 位4:6被保留。 位7指示方向，请参见libusb_endpoint_direction。
	 */
	uint8_t  bEndpointAddress;

	/** Attributes which apply to the endpoint when it is configured using
	 * the bConfigurationValue. Bits 0:1 determine the transfer type and
	 * correspond to \ref libusb_transfer_type. Bits 2:3 are only used for
	 * isochronous endpoints and correspond to \ref libusb_iso_sync_type.
	 * Bits 4:5 are also only used for isochronous endpoints and correspond to
	 * \ref libusb_iso_usage_type. Bits 6:7 are reserved.
	 * 使用bConfigurationValue配置端点时适用于端点的属性。
	 * 位0:1确定传输类型，并对应于libusb_transfer_type。
	 * 位2:3仅用于同步端点，并且对应于libusb_iso_sync_type。
	 * 位4:5也仅用于同步端点，并且对应于libusb_iso_usage_type。
	 * 位6:7被保留。
	 */
	uint8_t  bmAttributes;

	/** Maximum packet size this endpoint is capable of sending/receiving.
	 * 该端点能够发送/接收的最大数据包大小。
	 */
	uint16_t wMaxPacketSize;

	/** Interval for polling endpoint for data transfers.
	 * 轮询端点以进行数据传输的时间间隔。
	 */
	uint8_t  bInterval;

	/** For audio devices only: the rate at which synchronization feedback is provided.
	 * 仅对于音频设备: 提供同步反馈的速率。 */
	uint8_t  bRefresh;

	/** For audio devices only: the address if the synch endpoint
	 * 仅对于音频设备: 同步端点的地址 */
	uint8_t  bSynchAddress;

	/** Extra descriptors. If libusb encounters unknown endpoint descriptors,
	 * it will store them here, should you wish to parse them.
	 * 额外的描述符。 如果libusb遇到未知的终结点描述符，则将其存储在此处（如果您希望解析它们）。
	 */
	const unsigned char *extra;

	/** Length of the extra descriptors, in bytes.
	 * 额外描述符的长度，以字节为单位。
	 */
	int extra_length;
};

/** \ingroup desc
 * A structure representing the standard USB interface descriptor. This
 * descriptor is documented in section 9.6.5 of the USB 3.0 specification.
 * All multiple-byte fields are represented in host-endian format.
 *
 * 表示标准USB接口描述符的结构。 USB 3.0规范的第9.6.5节记录了该描述符。 所有多字节字段均以主机字节序格式表示。
 */
struct libusb_interface_descriptor {
	/** Size of this descriptor (in bytes)
	 * 该描述符的大小（以字节为单位）
	 */
	uint8_t  bLength;

	/** Descriptor type. Will have value
	 * \ref libusb_descriptor_type::LIBUSB_DT_INTERFACE LIBUSB_DT_INTERFACE
	 * in this context.
	 * 描述符类型。 在这种情况下，将具有值libusb_descriptor_type::LIBUSB_DT_INTERFACE LIBUSB_DT_INTERFACE。
	 */
	uint8_t  bDescriptorType;

	/** Number of this interface
	 * 该接口号
	 */
	uint8_t  bInterfaceNumber;

	/** Value used to select this alternate setting for this interface
	 * 用于为此接口选择此替代设置的值
	 */
	uint8_t  bAlternateSetting;

	/** Number of endpoints used by this interface (excluding the control endpoint).
	 * 该接口使用的端点数（不包括控制端点） */
	uint8_t  bNumEndpoints;

	/** USB-IF class code for this interface. See \ref libusb_class_code.
	 * 该接口的USB-IF类代码。 请参见libusb_class_code。
	 */
	uint8_t  bInterfaceClass;

	/** USB-IF subclass code for this interface, qualified by the bInterfaceClass value
	 * 此接口的USB-IF子类代码，由bInterfaceClass值限定 */
	uint8_t  bInterfaceSubClass;

	/** USB-IF protocol code for this interface, qualified by the bInterfaceClass and bInterfaceSubClass values
	 * 此接口的USB-IF协议代码，由bInterfaceClass和bInterfaceSubClass值限定
	 */
	uint8_t  bInterfaceProtocol;

	/** Index of string descriptor describing this interface
	 * 描述此接口的字符串描述符的索引
	 */
	uint8_t  iInterface;

	/** Array of endpoint descriptors. This length of this array is determined by the bNumEndpoints field.
	 * 端点描述符数组。 该数组的长度由bNumEndpoints字段确定。 */
	const struct libusb_endpoint_descriptor *endpoint;

	/** Extra descriptors. If libusb encounters unknown interface descriptors, it will store them here, should you wish to parse them.
	 * 额外的描述符。 如果libusb遇到未知的接口描述符，则在需要解析它们的情况下会将其存储在此处。 */
	const unsigned char *extra;

	/** Length of the extra descriptors, in bytes.
	 * 额外描述符的长度，以字节为单位。
	 */
	int extra_length;
};

/** \ingroup desc
 * A collection of alternate settings for a particular USB interface.
 * 特定USB接口的替代设置的集合。
 */
struct libusb_interface {
	/** Array of interface descriptors. The length of this array is determined by the num_altsetting field.
	 * 接口描述符的数组。 该数组的长度由num_altsetting字段确定。 */
	const struct libusb_interface_descriptor *altsetting;

	/** The number of alternate settings that belong to this interface
	 * 属于此接口的替代设置的数量
	 */
	int num_altsetting;
};

/** \ingroup desc
 * A structure representing the Interface Association descriptor(IAD).
 * 表示接口关联描述符（IAD）的结构。
 */
struct libusb_association_descriptor {	// XXX added to support composit device   添加以支持复合设备
	uint8_t 	bLength;			// Size of this descriptor (in bytes)   该描述符的大小（以字节为单位）
	uint8_t 	bDescriptorType;	// Descriptor type(LIBUSB_DT_ASSOCIATION)  描述符类型（LIBUSB_DT_ASSOCIATION）
	uint8_t 	bFirstInterface;	// First interface number of the set of interfaces that follow this descriptor.  跟随此描述符的一组接口的第一个接口号。
	uint8_t 	bInterfaceCount;	// The Number of interfaces follow this descriptor that are considered "associated".  遵循此描述符的接口数量被认为是“关联的”。
	uint8_t 	bFunctionClass;		// bInterfaceClass used for this associated interfaces   bInterfaceClass用于此关联接口
	uint8_t 	bFunctionSubClass;	// bInterfaceSubClass used for the associated interfaces  bInterfaceSubClass用于关联的接口
	uint8_t 	bFunctionProtocol;	// bInterfaceProtocol used for the associated interfaces  bInterfaceProtocol用于关联的接口
	uint8_t 	iFunction;			// Index of string descriptor describing the associated interfaces.  描述相关接口的字符串描述符的索引。

	/** Extra descriptors. If libusb encounters unknown configuration
	 * descriptors, it will store them here, should you wish to parse them.
	 * 额外的描述符。 如果libusb遇到未知的配置描述符，则将其存储在此处（如果您希望解析它们）。
	 */
	const unsigned char *extra;

	/** Length of the extra descriptors, in bytes.
	 * 额外描述符的长度，以字节为单位。
	 */
	int extra_length;
};

/** \ingroup desc
 * A structure representing the standard USB configuration descriptor. This
 * descriptor is documented in section 9.6.3 of the USB 3.0 specification.
 * All multiple-byte fields are represented in host-endian format.
 * 表示标准USB配置描述符的结构。 USB 3.0规范的第9.6.3节记录了该描述符。
 * 所有多字节字段均以主机字节序格式表示。
 */
struct libusb_config_descriptor {
	/**
	 * Size of this descriptor (in bytes)
	 * 该描述符的大小（以字节为单位）
	 */
	uint8_t  bLength;

	/**
	 * Descriptor type. Will have value
	 * \ref libusb_descriptor_type::LIBUSB_DT_CONFIG LIBUSB_DT_CONFIG
	 * in this context.
	 * 描述符类型。 在这种情况下，将具有 libusb_descriptor_type::LIBUSB_DT_CONFIG LIBUSB_DT_CONFIG 值。
	 */
	uint8_t  bDescriptorType;

	/**
	 * Total length of data returned for this configuration
	 * 此配置返回的数据总长度
	 */
	uint16_t wTotalLength;

	/**
	 * Number of interfaces supported by this configuration
	 * 此配置支持的接口数
	 */
	uint8_t  bNumInterfaces;

	/**
	 * Identifier value for this configuration
	 * 此配置的标识符值
	 */
	uint8_t  bConfigurationValue;

	/**
	 * Index of string descriptor describing this configuration
	 * 描述此配置的字符串描述符的索引
	 */
	uint8_t  iConfiguration;

	/**
	 * Configuration characteristics
	 * 配置特征
	 */
	uint8_t  bmAttributes;

	/**
	 * Maximum power consumption of the USB device from this bus in this
	 * configuration when the device is fully opreation. Expressed in units
	 * of 2 mA.
	 * 当设备完全使用时，在这种配置下，USB设备从该总线获得的最大功耗。 以2 mA为单位表示。
	 */
	uint8_t  MaxPower;

	/**
	 * Array of interfaces supported by this configuration. The length of
	 * this array is determined by the bNumInterfaces field.
	 * 此配置支持的接口数组。 该数组的长度由bNumInterfaces字段确定。
	 */
	const struct libusb_interface *interface;

	/**
	 * Single link list of interface association descriptors related to this configuration.
	 * The length of this list is determined by the num_associations field.
	 * 与该配置相关的接口关联描述符的单链接列表。
     * 该列表的长度由num_associations字段确定。
	 */
	struct libusb_association_descriptor *association_descriptor;
	uint8_t num_associations;
	uint8_t selected_iad;

	/**
	 * Extra descriptors. If libusb encounters unknown configuration
	 * descriptors, it will store them here, should you wish to parse them.
	 * 额外的描述符。 如果libusb遇到未知的配置描述符，则将在您希望解析它们的情况下将其存储在此处。
	 */
	const unsigned char *extra;

	/**
	 * Length of the extra descriptors, in bytes.
	 * 额外描述符的长度，以字节为单位。
	 */
	int extra_length;
};

/** \ingroup desc
 * A structure representing the superspeed endpoint companion
 * descriptor. This descriptor is documented in section 9.6.7 of
 * the USB 3.0 specification. All multiple-byte fields are represented in
 * host-endian format.
 * 表示超速端点伴随描述符的结构。 USB 3.0规范的第9.6.7节记录了该描述符。 所有多字节字段均以主机字节序格式表示。
 */
struct libusb_ss_endpoint_companion_descriptor {

	/**
	 * Size of this descriptor (in bytes)
	 * 该描述符的大小（以字节为单位）
	 */
	uint8_t  bLength;

	/**
	 * Descriptor type. Will have value
	 * \ref libusb_descriptor_type::LIBUSB_DT_SS_ENDPOINT_COMPANION in
	 * this context.
	 * 描述符类型。 在这种情况下，将具有 \ref libusb_descriptor_type::LIBUSB_DT_SS_ENDPOINT_COMPANION 值。
	 */
	uint8_t  bDescriptorType;


	/**
	 * The maximum number of packets the endpoint can send or
	 *  recieve as part of a burst.
	 * 端点可以作为突发的一部分发送或接收的最大数据包数。
	 */
	uint8_t  bMaxBurst;

	/**
	 *  In bulk EP:	bits 4:0 represents the	maximum	number of
	 *  streams the	EP supports. In	isochronous EP:	bits 1:0
	 *  represents the Mult	- a zero based value that determines
	 *  the	maximum	number of packets within a service interval
	 *  批量EP：位4：0表示EP支持的最大流数。 在同步EP中：位1：0表示Mult-从零开始的值，该值确定服务间隔内的最大数据包数
	 */
	uint8_t  bmAttributes;

	/**
	 *  The	total number of bytes this EP will transfer every
	 *  service interval. valid only for periodic EPs.
	 * 该EP将在每个服务间隔传输的字节总数。 仅对定期EP有效。
	 */
	uint16_t wBytesPerInterval;
};

/** \ingroup desc
 * A generic representation of a BOS Device Capability descriptor. It is
 * advised to check bDevCapabilityType and call the matching
 * libusb_get_*_descriptor function to get a structure fully matching the type.
 * BOS设备功能描述符的一般表示。 建议检查bDevCapabilityType并调用匹配的libusb_get _ * _ descriptor函数以获取与该类型完全匹配的结构。
 */
struct libusb_bos_dev_capability_descriptor {
	/**
	 * Size of this descriptor (in bytes)
	 * 该描述符的大小（以字节为单位）
	 */
	uint8_t bLength;
	/**
	 * Descriptor type. Will have value
	 * \ref libusb_descriptor_type::LIBUSB_DT_DEVICE_CAPABILITY
	 * LIBUSB_DT_DEVICE_CAPABILITY in this context.
	 * 描述符类型。 在这种情况下，将具有 \ref libusb_descriptor_type::LIBUSB_DT_DEVICE_CAPABILITY 值。
	 */
	uint8_t bDescriptorType;
	/**
	 * Device Capability type
	 * 设备功能类型
	 */
	uint8_t bDevCapabilityType;
	/**
	 * Device Capability data (bLength - 3 bytes)
	 * 设备功能数据（bLength-3个字节）
	 */
	uint8_t dev_capability_data
#if defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 199901L)
	[] /* valid C99 code */
#else
	[0] /* non-standard, but usually working code */
#endif
	;
};

/** \ingroup desc
 * A structure representing the Binary Device Object Store (BOS) descriptor.
 * This descriptor is documented in section 9.6.2 of the USB 3.0 specification.
 * All multiple-byte fields are represented in host-endian format.
 *
 * 表示二进制设备对象存储（BOS）描述符的结构。USB 3.0规范的第9.6.2节记录了该描述符。所有多字节字段均以主机字节序格式表示。
 */
struct libusb_bos_descriptor {
	/** Size of this descriptor (in bytes)
	 * 该描述符的大小（以字节为单位）
	 */
	uint8_t  bLength;

	/** Descriptor type. Will have value
	 * \ref libusb_descriptor_type::LIBUSB_DT_BOS LIBUSB_DT_BOS
	 * in this context.
	 * 描述符类型。在这种情况下，将具有值libusb_descriptor_type::LIBUSB_DT_BOS LIBUSB_DT_BOS。
	 */
	uint8_t  bDescriptorType;

	/** Length of this descriptor and all of its sub descriptors
	 * 该描述符及其所有子描述符的长度
	 */
	uint16_t wTotalLength;

	/** The number of separate device capability descriptors in the BOS
	 * BOS中单独的设备功能描述符的数量 */
	uint8_t  bNumDeviceCaps;

	/** bNumDeviceCap Device Capability Descriptors
	 * bNumDeviceCap设备功能描述符
	 */
	struct libusb_bos_dev_capability_descriptor *dev_capability
#if defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 199901L)
	[] /* valid C99 code */
#else
	[0] /* non-standard, but usually working code */
#endif
	;
};

/** \ingroup desc
 * A structure representing the USB 2.0 Extension descriptor
 * This descriptor is documented in section 9.6.2.1 of the USB 3.0 specification.
 * All multiple-byte fields are represented in host-endian format.
 *
 * 表示USB 2.0扩展描述符的结构该描述符记录在USB 3.0规范的9.6.2.1节中。 所有多字节字段均以主机字节序格式表示。
 */
struct libusb_usb_2_0_extension_descriptor {
	/** Size of this descriptor (in bytes)
	 * 该描述符的大小（以字节为单位）
	 */
	uint8_t  bLength;

	/** Descriptor type. Will have value
	 * \ref libusb_descriptor_type::LIBUSB_DT_DEVICE_CAPABILITY
	 * LIBUSB_DT_DEVICE_CAPABILITY in this context.
	 * 描述符类型。 在这种情况下，将具有值libusb_descriptor_type::LIBUSB_DT_DEVICE_CAPABILITY LIBUSB_DT_DEVICE_CAPABILITY。
	 */
	uint8_t  bDescriptorType;

	/** Capability type. Will have value
	 * \ref libusb_capability_type::LIBUSB_BT_USB_2_0_EXTENSION
	 * LIBUSB_BT_USB_2_0_EXTENSION in this context.
	 * 能力类型。 在这种情况下，将具有值libusb_capability_type::LIBUSB_BT_USB_2_0_EXTENSION LIBUSB_BT_USB_2_0_EXTENSION。
	 */
	uint8_t  bDevCapabilityType;

	/** Bitmap encoding of supported device level features.
	 * A value of one in a bit location indicates a feature is
	 * supported; a value of zero indicates it is not supported.
	 * See \ref libusb_usb_2_0_extension_attributes.
	 * 支持的设备级别功能的位图编码。位位置的值1表示支持功能。零值表示不支持。参见libusb_usb_2_0_extension_attributes。
	 */
	uint32_t  bmAttributes;
};

/** \ingroup desc
 * A structure representing the SuperSpeed USB Device Capability descriptor
 * This descriptor is documented in section 9.6.2.2 of the USB 3.0 specification.
 * All multiple-byte fields are represented in host-endian format.
 * 表示SuperSpeed USB设备功能描述符的结构该描述符记录在USB 3.0规范的9.6.2.2节中。所有多字节字段均以主机字节序格式表示。
 */
struct libusb_ss_usb_device_capability_descriptor {
	/** Size of this descriptor (in bytes)
	 * 该描述符的大小（以字节为单位）
	 */
	uint8_t  bLength;

	/** Descriptor type. Will have value
	 * \ref libusb_descriptor_type::LIBUSB_DT_DEVICE_CAPABILITY
	 * LIBUSB_DT_DEVICE_CAPABILITY in this context.
	 * 描述符类型。 在这种情况下，将具有值libusb_descriptor_type::LIBUSB_DT_DEVICE_CAPABILITY LIBUSB_DT_DEVICE_CAPABILITY。
	 */
	uint8_t  bDescriptorType;

	/** Capability type. Will have value
	 * \ref libusb_capability_type::LIBUSB_BT_SS_USB_DEVICE_CAPABILITY
	 * LIBUSB_BT_SS_USB_DEVICE_CAPABILITY in this context.
	 * 能力类型。 在这种情况下，将具有值libusb_capability_type::LIBUSB_BT_SS_USB_DEVICE_CAPABILITY LIBUSB_BT_SS_USB_DEVICE_CAPABILITY。
	 */
	uint8_t  bDevCapabilityType;

	/** Bitmap encoding of supported device level features.
	 * A value of one in a bit location indicates a feature is
	 * supported; a value of zero indicates it is not supported.
	 * See \ref libusb_ss_usb_device_capability_attributes.
	 * 支持的设备级别功能的位图编码。位位置的值1表示支持该功能；零值表示不支持。请参见libusb_ss_usb_device_capability_attributes。
	 */
	uint8_t  bmAttributes;

	/** Bitmap encoding of the speed supported by this device when
	 * operating in SuperSpeed mode. See \ref libusb_supported_speed.
	 * 在SuperSpeed模式下操作时，此设备支持的速度的位图编码。请参阅libusb_supported_speed。
	 */
	uint16_t wSpeedSupported;

	/** The lowest speed at which all the functionality supported
	 * by the device is available to the user. For example if the
	 * device supports all its functionality when connected at
	 * full speed and above then it sets this value to 1.
	 * 用户可以使用设备支持的所有功能的最低速度。 例如，如果设备以全速或更高速度连接时支持所有功能，则将其设置为1。
	 */
	uint8_t  bFunctionalitySupport;

	/** U1 Device Exit Latency.
	 * U1设备退出延迟。
	 */
	uint8_t  bU1DevExitLat;

	/** U2 Device Exit Latency.
	 * U2设备退出延迟。
	 */
	uint16_t bU2DevExitLat;
};

/** \ingroup desc
 * A structure representing the Container ID descriptor.
 * This descriptor is documented in section 9.6.2.3 of the USB 3.0 specification.
 * All multiple-byte fields, except UUIDs, are represented in host-endian format.
 * 表示容器ID描述符的结构，该描述符记录在USB 3.0规范的9.6.2.3节中。除UUID之外，所有多字节字段均以主机字节序格式表示。
 */
struct libusb_container_id_descriptor {
	/** Size of this descriptor (in bytes)
	 * 该描述符的大小（以字节为单位）
	 */
	uint8_t  bLength;

	/** Descriptor type. Will have value
	 * \ref libusb_descriptor_type::LIBUSB_DT_DEVICE_CAPABILITY
	 * LIBUSB_DT_DEVICE_CAPABILITY in this context.
	 * 描述符类型。 在这种情况下，将具有值libusb_descriptor_type::LIBUSB_DT_DEVICE_CAPABILITY LIBUSB_DT_DEVICE_CAPABILITY。
	 */
	uint8_t  bDescriptorType;

	/** Capability type. Will have value
	 * \ref libusb_capability_type::LIBUSB_BT_CONTAINER_ID
	 * LIBUSB_BT_CONTAINER_ID in this context.
	 * 能力类型。在这种情况下，将具有值libusb_capability_type::LIBUSB_BT_CONTAINER_ID LIBUSB_BT_CONTAINER_ID。
	 */
	uint8_t  bDevCapabilityType;

	/** Reserved field  保留 */
	uint8_t bReserved;

	/** 128 bit UUID  128位UUID */
	uint8_t  ContainerID[16];
};

/** \ingroup asyncio
 * Setup packet for control transfers.
 * 用于控制传输的设置数据包。
 */
struct libusb_control_setup {
	/** Request type. Bits 0:4 determine recipient, see
	 * \ref libusb_request_recipient. Bits 5:6 determine type, see
	 * \ref libusb_request_type. Bit 7 determines data transfer direction, see
	 * \ref libusb_endpoint_direction.
	 * 请求类型。
	 * 位0:4确定接收者，请参见libusb_request_recipient。
	 * 位5:6确定类型，请参见libusb_request_type。
	 * 位7确定数据传输方向，请参见libusb_endpoint_direction。
	 */
	uint8_t  bmRequestType;

	/** Request. If the type bits of bmRequestType are equal to
	 * \ref libusb_request_type::LIBUSB_REQUEST_TYPE_STANDARD
	 * "LIBUSB_REQUEST_TYPE_STANDARD" then this field refers to
	 * \ref libusb_standard_request. For other cases, use of this field is
	 * application-specific.
	 * 请求。 如果bmRequestType的类型位等于libusb_request_type::LIBUSB_REQUEST_TYPE_STANDARD "LIBUSB_REQUEST_TYPE_STANDARD"，
	 * 则此字段引用libusb_standard_request。 对于其他情况，此字段的使用是特定于应用程序的。
	 */
	uint8_t  bRequest;

	/** Value. Varies according to request
	 * 值。 根据要求而定
	 */
	uint16_t wValue;

	/** Index. Varies according to request, typically used to pass an index or offset
	 * 索引。 根据要求而变化，通常用于传递索引或偏移量 */
	uint16_t wIndex;

	/** Number of bytes to transfer
	 * 传输字节数
	 */
	uint16_t wLength;
};

#define LIBUSB_CONTROL_SETUP_SIZE (sizeof(struct libusb_control_setup))

/* libusb */

struct libusb_context;
struct libusb_device;
struct libusb_device_handle;
struct libusb_hotplug_callback;

/** \ingroup lib
 * Structure providing the version of the libusb runtime
 * 提供libusb运行时版本的结构
 */
struct libusb_version {
	/** Library major version. */
	const uint16_t major;

	/** Library minor version. */
	const uint16_t minor;

	/** Library micro version. */
	const uint16_t micro;

	/** Library nano version. */
	const uint16_t nano;

	/** Library release candidate suffix string, e.g. "-rc4". */
	const char *rc;

	/** For ABI compatibility only. */
	const char* describe;
};

/** \ingroup lib
 * Structure representing a libusb session. The concept of individual libusb
 * sessions allows for your program to use two libraries (or dynamically
 * load two modules) which both independently use libusb. This will prevent
 * interference between the individual libusb users - for example
 * libusb_set_debug() will not affect the other user of the library, and
 * libusb_exit() will not destroy resources that the other user is still
 * using.
 *
 * Sessions are created by libusb_init() and destroyed through libusb_exit().
 * If your application is guaranteed to only ever include a single libusb
 * user (i.e. you), you do not have to worry about contexts: pass NULL in
 * every function call where a context is required. The default context
 * will be used.
 *
 * For more information, see \ref contexts.
 *
 * 表示libusb会话的结构。
 * 单独的libusb会话的概念允许您的程序使用两个都独立使用libusb的库（或动态加载两个模块）。
 * 这将防止单个libusb用户之间的干扰-例如libusb_set_debug()不会影响该库的另一个用户，而libusb_exit()不会破坏该另一个用户仍在使用的资源。
 * 会话由libusb_init()创建，并通过libusb_exit()销毁。 如果保证您的应用程序仅包含一个libusb用户（即您），则您不必担心上下文：在需要上下文的每个函数调用中传递NULL。 将使用默认上下文。
 * 有关更多信息，请参见上下文。
 */
typedef struct libusb_context libusb_context;

/** \ingroup dev
 * Structure representing a USB device detected on the system. This is an
 * opaque type for which you are only ever provided with a pointer, usually
 * originating from libusb_get_device_list().
 *
 * Certain operations can be performed on a device, but in order to do any
 * I/O you will have to first obtain a device handle using libusb_open().
 *
 * Devices are reference counted with libusb_ref_device() and
 * libusb_unref_device(), and are freed when the reference count reaches 0.
 * New devices presented by libusb_get_device_list() have a reference count of
 * 1, and libusb_free_device_list() can optionally decrease the reference count
 * on all devices in the list. libusb_open() adds another reference which is
 * later destroyed by libusb_close().
 *
 * 表示系统上检测到的USB设备的结构。
 * 这是一种不透明类型，通常只为libusb_get_device_list()提供指针。
 * 可以在设备上执行某些操作，但是要执行任何I/O，必须首先使用libusb_open()获得设备句柄。
 * 设备使用libusb_ref_device()和libusb_unref_device()进行引用计数，并在引用计数达到0时释放。
 * libusb_get_device_list()表示的新设备的引用计数为1，并且libusb_free_device_list()可以选择减少所有设备上的引用计数在列表中。
 * libusb_open()添加了另一个引用，该引用后来被libusb_close()销毁。
 */
typedef struct libusb_device libusb_device;


/** \ingroup dev
 * Structure representing a handle on a USB device. This is an opaque type for
 * which you are only ever provided with a pointer, usually originating from
 * libusb_open().
 *
 * A device handle is used to perform I/O and other operations. When finished
 * with a device handle, you should call libusb_close().
 *
 * 表示USB设备上的句柄的结构。 这是一种不透明类型，通常只为libusb_open()提供一个指针。
 * 设备句柄用于执行I/O和其他操作。 完成设备句柄后，应调用libusb_close()。
 */
typedef struct libusb_device_handle libusb_device_handle;

/** \ingroup dev
 * Speed codes. Indicates the speed at which the device is operating.
 * 速度代码。 指示设备运行的速度。
 */
enum libusb_speed {
	/** The OS doesn't report or know the device speed.
	 * 操作系统不报告或不知道设备速度。
	 */
	LIBUSB_SPEED_UNKNOWN = 0,

	/** The device is operating at low speed (1.5MBit/s).
	 * 设备正在低速（1.5MBit/s）下运行。
	 */
	LIBUSB_SPEED_LOW = 1,

	/** The device is operating at full speed (12MBit/s).
	 * 设备以全速（12MBit/s）运行。 */
	LIBUSB_SPEED_FULL = 2,

	/** The device is operating at high speed (480MBit/s).
	 * 设备正在高速运行（480MBit/s）。
	 */
	LIBUSB_SPEED_HIGH = 3,

	/** The device is operating at super speed (5000MBit/s).
	 * 该设备以超高速（5000MBit/s）运行。
	 */
	LIBUSB_SPEED_SUPER = 4,
};

/** \ingroup dev
 * Supported speeds (wSpeedSupported) bitfield. Indicates what
 * speeds the device supports.
 * 支持的速度（wSpeedSupported）位字段。 指示设备支持的速度。
 */
enum libusb_supported_speed {
	/** Low speed operation supported (1.5MBit/s).
	 * 支持低速运行（1.5MBit/s）。
	 */
	LIBUSB_LOW_SPEED_OPERATION   = 1,

	/** Full speed operation supported (12MBit/s).
	 * 支持全速运行（12MBit/s）。
	 */
	LIBUSB_FULL_SPEED_OPERATION  = 2,

	/** High speed operation supported (480MBit/s).
	 * 支持高速运行（480MBit/s）。
	 */
	LIBUSB_HIGH_SPEED_OPERATION  = 4,

	/** Superspeed operation supported (5000MBit/s).
	 * 支持超高速操作（5000MBit/s）。
	 */
	LIBUSB_SUPER_SPEED_OPERATION = 8,
};

/** \ingroup dev
 * Masks for the bits of the
 * \ref libusb_usb_2_0_extension_descriptor::bmAttributes "bmAttributes" field
 * of the USB 2.0 Extension descriptor.
 * USB 2.0扩展描述符的libusb_usb_2_0_extension_descriptor::bmAttributes "bmAttributes"字段的位的掩码。
 */
enum libusb_usb_2_0_extension_attributes {
	/** Supports Link Power Management (LPM)
	 * 支持链路电源管理（LPM）
	 */
	LIBUSB_BM_LPM_SUPPORT = 2,
};

/** \ingroup dev
 * Masks for the bits of the
 * \ref libusb_ss_usb_device_capability_descriptor::bmAttributes "bmAttributes" field
 * field of the SuperSpeed USB Device Capability descriptor.
 * SuperSpeed USB设备功能描述符的libusb_ss_usb_device_capability_descriptor::bmAttributes "bmAttributes"字段字段的位的掩码。
 */
enum libusb_ss_usb_device_capability_attributes {
	/** Supports Latency Tolerance Messages (LTM)
	 * 支持延迟容忍消息（LTM）
	 */
	LIBUSB_BM_LTM_SUPPORT = 2,
};

/** \ingroup dev
 * USB capability types
 * USB功能类型
 */
enum libusb_bos_type {
	/** Wireless USB device capability
	 * 无线USB设备功能
	 */
	LIBUSB_BT_WIRELESS_USB_DEVICE_CAPABILITY	= 1,

	/** USB 2.0 extensions
	 * USB 2.0扩展
	 */
	LIBUSB_BT_USB_2_0_EXTENSION			= 2,

	/** SuperSpeed USB device capability
	 * 超高速USB设备功能
	 */
	LIBUSB_BT_SS_USB_DEVICE_CAPABILITY		= 3,

	/** Container ID type
	 * 容器ID类型
	 */
	LIBUSB_BT_CONTAINER_ID				= 4,
};

/** \ingroup misc
 * Error codes. Most libusb functions return 0 on success or one of these
 * codes on failure.
 * You can call libusb_error_name() to retrieve a string representation of an
 * error code or libusb_strerror() to get an end-user suitable description of
 * an error code.
 *
 * 错误代码。大多数libusb函数成功返回0或失败返回这些代码之一。
 * 您可以调用libusb_error_name()来检索错误代码的字符串表示形式，或者可以调用libusb_strerror()来获取最终用户适合的错误代码描述。
 */
enum libusb_error {
	/** Success (no error)
	 * 成功（无错误）
	 */
	LIBUSB_SUCCESS = 0,

	/** Input/output error
	 * 输入/输出错误
	 */
	LIBUSB_ERROR_IO = -1,

	/** Invalid parameter
	 * 无效的参数
	 */
	LIBUSB_ERROR_INVALID_PARAM = -2,

	/** Access denied (insufficient permissions)
	 * 拒绝访问（权限不足）
	 */
	LIBUSB_ERROR_ACCESS = -3,

	/** No such device (it may have been disconnected)
	 * 没有此类设备（可能已断开连接）
	 */
	LIBUSB_ERROR_NO_DEVICE = -4,

	/** Entity not found
	 * 找不到实体
	 */
	LIBUSB_ERROR_NOT_FOUND = -5,

	/** Resource busy
	 * 资源繁忙
	 */
	LIBUSB_ERROR_BUSY = -6,

	/** Operation timed out
	 * 操作超时
	 */
	LIBUSB_ERROR_TIMEOUT = -7,

	/** Overflow
	 * 溢出
	 */
	LIBUSB_ERROR_OVERFLOW = -8,

	/** Pipe error
	 * 管道错误
	 */
	LIBUSB_ERROR_PIPE = -9,

	/** System call interrupted (perhaps due to signal)
	 * 系统调用中断（可能是由于信号）
	 */
	LIBUSB_ERROR_INTERRUPTED = -10,

	/** Insufficient memory
	 * 内存不足
	 */
	LIBUSB_ERROR_NO_MEM = -11,

	/** Operation not supported or unimplemented on this platform
	 * 该平台不支持或未实现该操作
	 */
	LIBUSB_ERROR_NOT_SUPPORTED = -12,

	/* NB: Remember to update LIBUSB_ERROR_COUNT below as well as the
	   message strings in strerror.c when adding new error codes here.
	   注意：在此处添加新的错误代码时，请记住更新下面的LIBUSB_ERROR_COUNT以及strerror.c中的消息字符串。
	 */

	/** Other error 其他错误 */
	LIBUSB_ERROR_OTHER = -99,
};

/* Total number of error codes in enum libusb_error
 * 枚举libusb_error中的错误代码总数
 */
#define LIBUSB_ERROR_COUNT 14

/** \ingroup asyncio
 * Transfer status codes
 * 参数状态码
 */
enum libusb_transfer_status {
	/**
	 * Transfer completed without error. Note that this does not indicate
	 * that the entire amount of requested data was transferred.
	 * 传输已完成，没有错误。 请注意，这并不表示已传输了全部所需数据。
	 */
	LIBUSB_TRANSFER_COMPLETED,

	/**
	 * Transfer failed
	 * 传输失败
	 */
	LIBUSB_TRANSFER_ERROR,

	/**
	 * Transfer timed out
	 * 传输超时
	 */
	LIBUSB_TRANSFER_TIMED_OUT,

	/**
	 * Transfer was cancelled
	 * 传输被取消
	 */
	LIBUSB_TRANSFER_CANCELLED,

	/**
	 * For bulk/interrupt endpoints: halt condition detected (endpoint
	 * stalled). For control endpoints: control request not supported.
	 * 对于批量/中断端点：检测到停止条件（端点已停止）。 对于控制端点：不支持控制请求。
	 */
	LIBUSB_TRANSFER_STALL,

	/**
	 * Device was disconnected
	 * 设备断开连接
	 */
	LIBUSB_TRANSFER_NO_DEVICE,

	/**
	 * Device sent more data than requested
	 * 设备发送的数据超出要求
	 */
	LIBUSB_TRANSFER_OVERFLOW,

	/* NB! Remember to update libusb_error_name()
	   when adding new status codes here.
	 注意！ 在此处添加新的状态代码时，请记住要更新libusb_error_name()。
	 */
};

/** \ingroup asyncio
 * libusb_transfer.flags values
 * libusb_transfer.flags值
 */
enum libusb_transfer_flags {
	/**
	 * Report short frames as errors
	 * 将短帧报告为错误
	 */
	LIBUSB_TRANSFER_SHORT_NOT_OK = 1<<0,

	/**
	 * Automatically free() transfer buffer during libusb_free_transfer()
	 * libusb_free_transfer()期间自动释放传输缓冲区
	 */
	LIBUSB_TRANSFER_FREE_BUFFER = 1<<1,

	/**
	 * Automatically call libusb_free_transfer() after callback returns.
	 * If this flag is set, it is illegal to call libusb_free_transfer()
	 * from your transfer callback, as this will result in a double-free
	 * when this flag is acted upon.
	 * 回调返回后自动调用libusb_free_transfer()。
	 * 如果设置了此标志，则从您的传输回调中调用libusb_free_transfer()是非法的，因为对此标志进行操作将导致两次释放。
	 */
	LIBUSB_TRANSFER_FREE_TRANSFER = 1<<2,

	/**
	 * Terminate transfers that are a multiple of the endpoint's
	 * wMaxPacketSize with an extra zero length packet. This is useful
	 * when a device protocol mandates that each logical request is
	 * terminated by an incomplete packet (i.e. the logical requests are
	 * not separated by other means).
	 *
	 * This flag only affects host-to-device transfers to bulk and interrupt
	 * endpoints. In other situations, it is ignored.
	 *
	 * This flag only affects transfers with a length that is a multiple of
	 * the endpoint's wMaxPacketSize. On transfers of other lengths, this
	 * flag has no effect. Therefore, if you are working with a device that
	 * needs a ZLP whenever the end of the logical request falls on a packet
	 * boundary, then it is sensible to set this flag on <em>every</em>
	 * transfer (you do not have to worry about only setting it on transfers
	 * that end on the boundary).
	 *
	 * This flag is currently only supported on Linux.
	 * On other systems, libusb_submit_transfer() will return
	 * LIBUSB_ERROR_NOT_SUPPORTED for every transfer where this flag is set.
	 *
	 * Available since libusb-1.0.9.
	 *
	 * 用额外的零长度数据包终止端点的wMaxPacketSize的倍数的传输。当设备协议要求每个逻辑请求由不完整的数据包终止时（即，逻辑请求不通过其他方式分开），这很有用。
	 * 该标志仅影响主机到设备到批量端点和中断端点的传输。在其他情况下，它将被忽略。
	 * 此标志仅影响长度为端点wMaxPacketSize倍数的传输。在其他长度的传输上，此标志无效。因此，如果您使用的设备在逻辑请求的末尾落在数据包边界上时都需要ZLP，则明智的做法是在每次传输时都设置该标志（您不必担心仅在传输时进行设置）在边界上结束）。
	 * 当前仅在Linux上支持此标志。
	 * 在其他系统上，libusb_submit_transfer()将针对设置了该标志的每次传输返回LIBUSB_ERROR_NOT_SUPPORTED。
	 * 自libusb-1.0.9起可用。
	 */
	LIBUSB_TRANSFER_ADD_ZERO_PACKET = 1 << 3,
};

/** \ingroup asyncio
 * Isochronous packet descriptor.
 * 同步数据包描述符。
 */
struct libusb_iso_packet_descriptor {
	/**
	 * Length of data to request in this packet
	 * 此数据包中要请求的数据长度
	 */
	unsigned int length;

	/**
	 * Amount of data that was actually transferred
	 * 实际传输的数据量
	 */
	unsigned int actual_length;

	/**
	 * Status code for this packet
	 * 该数据包的状态码
	 */
	enum libusb_transfer_status status;
};

struct libusb_transfer;

/** \ingroup asyncio
 * Asynchronous transfer callback function type. When submitting asynchronous
 * transfers, you pass a pointer to a callback function of this type via the
 * \ref libusb_transfer::callback "callback" member of the libusb_transfer
 * structure. libusb will call this function later, when the transfer has
 * completed or failed. See \ref asyncio for more information.
 * \param transfer The libusb_transfer struct the callback function is being
 * notified about.
 *
 * 异步传输回调函数类型。
 * 提交异步传输时，可以通过libusb_transfer结构的libusb_transfer::callback "callback"成员将指针传递给该类型的回调函数。
 * 传输完成或失败后，libusb将稍后调用此函数。有关更多信息，请参见asyncio。
 */
typedef void (LIBUSB_CALL *libusb_transfer_cb_fn)(struct libusb_transfer *transfer);

/** \ingroup asyncio
 * The generic USB transfer structure. The user populates this structure and
 * then submits it in order to request a transfer. After the transfer has
 * completed, the library populates the transfer with the results and passes
 * it back to the user.
 * 通用USB传输结构。 用户填充此结构，然后提交它以请求传输。 传输完成后，库将使用结果填充传输，并将其传递回用户。
 */
struct libusb_transfer {
	/**
	 * Handle of the device that this transfer will be submitted to
	 * 传输将提交到的设备的句柄
	 */
	libusb_device_handle *dev_handle;

	/**
	 * A bitwise OR combination of \ref libusb_transfer_flags.
	 * libusb_transfer_flags 的按位或组合。
	 */
	uint8_t flags;

	/**
	 * Address of the endpoint where this transfer will be sent.
	 * 传输将发送到的端点的地址。
	 */
	unsigned char endpoint;

	/**
	 * Type of the endpoint from \ref libusb_transfer_type
	 * 来自libusb_transfer_type的端点类型
	 */
	unsigned char type;

	/**
	 * Timeout for this transfer in millseconds. A value of 0 indicates no timeout.
	 * 传输超时（以毫秒为单位）。 值为0表示没有超时。
	 */
	unsigned int timeout;

	/**
	 * The status of the transfer. Read-only, and only for use within
	 * transfer callback function.
	 *
	 * If this is an isochronous transfer, this field may read COMPLETED even
	 * if there were errors in the frames. Use the
	 * \ref libusb_iso_packet_descriptor::status "status" field in each packet
	 * to determine if errors occurred.
	 * 传输状态。 只读，仅在传输回调函数中使用。
     * 如果这是同步传输，则即使帧中有错误，此字段也可能显示为COMPLETED。 在每个数据包中使用 libusb_iso_packet_descriptor::status "status" 字段来确定是否发生错误。
	 */
	enum libusb_transfer_status status;

	/**
	 * Length of the data buffer
	 * 数据缓冲区的长度
	 */
	int length;

	/**
	 * Actual length of data that was transferred. Read-only, and only for
	 * use within transfer callback function. Not valid for isochronous
	 * endpoint transfers.
	 * 实际传输的数据长度。 只读，仅在传输回调函数中使用。 对于同步端点传输无效。
	 */
	int actual_length;

	/**
	 * Callback function. This will be invoked when the transfer completes,
	 * fails, or is cancelled.
	 * 回调函数。 传输完成，失败或取消时将调用此方法。
	 */
	libusb_transfer_cb_fn callback;

	/**
	 * User context data to pass to the callback function.
	 * 用户上下文数据传递给回调函数。
	 */
	void *user_data;

	/** Data buffer */
	unsigned char *buffer;

	/**
	 * Number of isochronous packets. Only used for I/O with isochronous endpoints.
	 * 同步包数。 仅用于具有同步端点的I/O。
	 */
	int num_iso_packets;

	/**
	 * Isochronous packet descriptors, for isochronous transfers only.
	 * 同步数据包描述符，仅用于同步传输。
	 */
	struct libusb_iso_packet_descriptor iso_packet_desc
#if defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 199901L)
	[] /* valid C99 code */
#else
	[0] /* non-standard, but usually working code */
#endif
	;
};

/** \ingroup misc
 * Capabilities supported by an instance of libusb on the current running
 * platform. Test if the loaded library supports a given capability by calling
 * \ref libusb_has_capability().
 * 当前运行平台上的libusb实例支持的功能。通过调用libusb_has_capability()测试加载的库是否支持给定的功能。
 */
enum libusb_capability {
	/** The libusb_has_capability() API is available.
	 * libusb_has_capability()API可用。
	 */
	LIBUSB_CAP_HAS_CAPABILITY = 0x0000,
	/** Hotplug support is available on this platform.
	 * 此平台上提供热插拔支持。
	 */
	LIBUSB_CAP_HAS_HOTPLUG = 0x0001,
	/** The library can access HID devices without requiring user intervention.
	 * Note that before being able to actually access an HID device, you may
	 * still have to call additional libusb functions such as
	 * \ref libusb_detach_kernel_driver().
	 * 该库可以访问HID设备，而无需用户干预。请注意，在实际访问HID设备之前，您可能仍必须调用其他libusb函数，例如libusb_detach_kernel_driver()。
	 */
	LIBUSB_CAP_HAS_HID_ACCESS = 0x0100,
	/** The library supports detaching of the default USB driver, using 
	 * \ref libusb_detach_kernel_driver(), if one is set by the OS kernel
	 * 如果操作系统内核设置了该库，则该库支持使用libusb_detach_kernel_driver()分离默认的USB驱动程序。
	 */
	LIBUSB_CAP_SUPPORTS_DETACH_KERNEL_DRIVER = 0x0101
};

/** \ingroup lib
 *  Log message levels.
 *  - LIBUSB_LOG_LEVEL_NONE (0)    : no messages ever printed by the library (default)
 *  - LIBUSB_LOG_LEVEL_ERROR (1)   : error messages are printed to stderr
 *  - LIBUSB_LOG_LEVEL_WARNING (2) : warning and error messages are printed to stderr
 *  - LIBUSB_LOG_LEVEL_INFO (3)    : informational messages are printed to stdout, warning
 *    and error messages are printed to stderr
 *  - LIBUSB_LOG_LEVEL_DEBUG (4)   : debug and informational messages are printed to stdout,
 *    warnings and errors to stderr
 *
 * 记录消息级别。
 * -LIBUSB_LOG_LEVEL_NONE（0）：库未打印任何消息（默认）
 * -LIBUSB_LOG_LEVEL_ERROR（1）：错误消息已打印到stderr
 * -LIBUSB_LOG_LEVEL_WARNING（2）：警告和错误消息已打印到stderr
 * -LIBUSB_LOG_LEVEL_INFO（3）：信息 打印到stdout，将警告和错误消息打印到stderr
 * -LIBUSB_LOG_LEVEL_DEBUG（4）：将调试和参考消息打印到stdout，将警告和错误打印到stderr
 */
enum libusb_log_level {
	LIBUSB_LOG_LEVEL_NONE = 0,
	LIBUSB_LOG_LEVEL_ERROR,
	LIBUSB_LOG_LEVEL_WARNING,
	LIBUSB_LOG_LEVEL_INFO,
	LIBUSB_LOG_LEVEL_DEBUG,
};

int LIBUSB_CALL libusb_init(libusb_context **ctx);
int LIBUSB_CALL libusb_init2(libusb_context **ctx, const char *usbfs);
void LIBUSB_CALL libusb_exit(libusb_context *ctx);
void LIBUSB_CALL libusb_set_debug(libusb_context *ctx, int level);
const struct libusb_version * LIBUSB_CALL libusb_get_version(void);
int LIBUSB_CALL libusb_has_capability(uint32_t capability);
const char * LIBUSB_CALL libusb_error_name(int errcode);
int LIBUSB_CALL libusb_setlocale(const char *locale);
const char * LIBUSB_CALL libusb_strerror(enum libusb_error errcode);

ssize_t LIBUSB_CALL libusb_get_device_list(libusb_context *ctx,
	libusb_device ***list);
void LIBUSB_CALL libusb_free_device_list(libusb_device **list,
	int unref_devices);
libusb_device * LIBUSB_CALL libusb_ref_device(libusb_device *dev);
void LIBUSB_CALL libusb_unref_device(libusb_device *dev);
libusb_device *libusb_find_device(libusb_context *ctx,
	const int vid, const int pid, const char *sn, int fd);	// XXX add for mainly non-rooted Android  为主要是非root用户的Android添加

int LIBUSB_CALL libusb_get_raw_descriptor(libusb_device *dev,
		unsigned char **buffer, int *descriptors_len, int *host_endian);
int LIBUSB_CALL libusb_get_configuration(libusb_device_handle *dev,
	int *config);
int LIBUSB_CALL libusb_get_device_descriptor(libusb_device *dev,
	struct libusb_device_descriptor *desc);
int LIBUSB_CALL libusb_get_active_config_descriptor(libusb_device *dev,
	struct libusb_config_descriptor **config);
int LIBUSB_CALL libusb_get_config_descriptor(libusb_device *dev,
	uint8_t config_index, struct libusb_config_descriptor **config);
int LIBUSB_CALL libusb_get_config_descriptor_by_value(libusb_device *dev,
	uint8_t bConfigurationValue, struct libusb_config_descriptor **config);
void LIBUSB_CALL libusb_free_config_descriptor(
	struct libusb_config_descriptor *config);
int LIBUSB_CALL libusb_get_ss_endpoint_companion_descriptor(
	struct libusb_context *ctx,
	const struct libusb_endpoint_descriptor *endpoint,
	struct libusb_ss_endpoint_companion_descriptor **ep_comp);
void LIBUSB_CALL libusb_free_ss_endpoint_companion_descriptor(
	struct libusb_ss_endpoint_companion_descriptor *ep_comp);
int LIBUSB_CALL libusb_get_bos_descriptor(libusb_device_handle *handle,
	struct libusb_bos_descriptor **bos);
void LIBUSB_CALL libusb_free_bos_descriptor(struct libusb_bos_descriptor *bos);
int LIBUSB_CALL libusb_get_usb_2_0_extension_descriptor(
	struct libusb_context *ctx,
	struct libusb_bos_dev_capability_descriptor *dev_cap,
	struct libusb_usb_2_0_extension_descriptor **usb_2_0_extension);
void LIBUSB_CALL libusb_free_usb_2_0_extension_descriptor(
	struct libusb_usb_2_0_extension_descriptor *usb_2_0_extension);
int LIBUSB_CALL libusb_get_ss_usb_device_capability_descriptor(
	struct libusb_context *ctx,
	struct libusb_bos_dev_capability_descriptor *dev_cap,
	struct libusb_ss_usb_device_capability_descriptor **ss_usb_device_cap);
void LIBUSB_CALL libusb_free_ss_usb_device_capability_descriptor(
	struct libusb_ss_usb_device_capability_descriptor *ss_usb_device_cap);
int LIBUSB_CALL libusb_get_container_id_descriptor(struct libusb_context *ctx,
	struct libusb_bos_dev_capability_descriptor *dev_cap,
	struct libusb_container_id_descriptor **container_id);
void LIBUSB_CALL libusb_free_container_id_descriptor(
	struct libusb_container_id_descriptor *container_id);
uint8_t LIBUSB_CALL libusb_get_bus_number(libusb_device *dev);
uint8_t LIBUSB_CALL libusb_get_port_number(libusb_device *dev);
int LIBUSB_CALL libusb_get_port_numbers(libusb_device *dev, uint8_t* port_numbers, int port_numbers_len);
LIBUSB_DEPRECATED_FOR(libusb_get_port_numbers)
int LIBUSB_CALL libusb_get_port_path(libusb_context *ctx, libusb_device *dev, uint8_t* path, uint8_t path_length);
libusb_device * LIBUSB_CALL libusb_get_parent(libusb_device *dev);
uint8_t LIBUSB_CALL libusb_get_device_address(libusb_device *dev);
int LIBUSB_CALL libusb_get_device_speed(libusb_device *dev);
int LIBUSB_CALL libusb_get_max_packet_size(libusb_device *dev,
	unsigned char endpoint);
int LIBUSB_CALL libusb_get_max_iso_packet_size(libusb_device *dev,
	unsigned char endpoint);

int LIBUSB_CALL libusb_set_device_fd(libusb_device *dev, int fd);	// XXX add for mainly non-rooted Android  为主要是非root用户的Android添加
libusb_device * LIBUSB_CALL libusb_get_device_with_fd(libusb_context *ctx,
	int vid, int pid, const char *serial, int fd, int busnum, int devaddr);
int LIBUSB_CALL libusb_open(libusb_device *dev, libusb_device_handle **handle);
void LIBUSB_CALL libusb_close(libusb_device_handle *dev_handle);
libusb_device * LIBUSB_CALL libusb_get_device(libusb_device_handle *dev_handle);

int LIBUSB_CALL libusb_set_configuration(libusb_device_handle *dev,
	int configuration);
int LIBUSB_CALL libusb_claim_interface(libusb_device_handle *dev,
	int interface_number);
int LIBUSB_CALL libusb_release_interface(libusb_device_handle *dev,
	int interface_number);

libusb_device_handle * LIBUSB_CALL libusb_open_device_with_vid_pid(
	libusb_context *ctx, uint16_t vendor_id, uint16_t product_id);

int LIBUSB_CALL libusb_set_interface_alt_setting(libusb_device_handle *dev,
	int interface_number, int alternate_setting);
int LIBUSB_CALL libusb_clear_halt(libusb_device_handle *dev,
	unsigned char endpoint);
int LIBUSB_CALL libusb_reset_device(libusb_device_handle *dev);

int LIBUSB_CALL libusb_alloc_streams(libusb_device_handle *dev,
	uint32_t num_streams, unsigned char *endpoints, int num_endpoints);
int LIBUSB_CALL libusb_free_streams(libusb_device_handle *dev,
	unsigned char *endpoints, int num_endpoints);

int LIBUSB_CALL libusb_kernel_driver_active(libusb_device_handle *dev,
	int interface_number);
int LIBUSB_CALL libusb_detach_kernel_driver(libusb_device_handle *dev,
	int interface_number);
int LIBUSB_CALL libusb_attach_kernel_driver(libusb_device_handle *dev,
	int interface_number);
int LIBUSB_CALL libusb_set_auto_detach_kernel_driver(
	libusb_device_handle *dev, int enable);

/* async I/O */

/** \ingroup asyncio
 * Get the data section of a control transfer. This convenience function is here
 * to remind you that the data does not start until 8 bytes into the actual
 * buffer, as the setup packet comes first.
 *
 * Calling this function only makes sense from a transfer callback function,
 * or situations where you have already allocated a suitably sized buffer at
 * transfer->buffer.
 *
 * \param transfer a transfer
 * \returns pointer to the first byte of the data section
 *
 * 获取控件传输的数据部分。此便利功能在这里提醒您，直到设置缓冲区中的第一个字节进入实际缓冲区后，数据才开始到8个字节。
 * 调用此函数仅在传输回调函数中有意义，或者已经在transfer->buffer中分配了适当大小的缓冲区的情况下才有意义。
 */
static inline unsigned char *libusb_control_transfer_get_data(
	struct libusb_transfer *transfer)
{
	return transfer->buffer + LIBUSB_CONTROL_SETUP_SIZE;
}

/** \ingroup asyncio
 * Get the control setup packet of a control transfer. This convenience
 * function is here to remind you that the control setup occupies the first
 * 8 bytes of the transfer data buffer.
 *
 * Calling this function only makes sense from a transfer callback function,
 * or situations where you have already allocated a suitably sized buffer at
 * transfer->buffer.
 *
 * \param transfer a transfer
 * \returns a casted pointer to the start of the transfer data buffer
 *
 * 获取控制传输的控制设置包。此便利功能在这里提醒您控制设置占用传输数据缓冲区的前8个字节。
 * 调用此函数仅在传输回调函数中有意义，或者已经在transfer->buffer中分配了适当大小的缓冲区的情况下才有意义。
 */
static inline struct libusb_control_setup *libusb_control_transfer_get_setup(
	struct libusb_transfer *transfer)
{
	return (struct libusb_control_setup *)(void *) transfer->buffer;
}

/** \ingroup asyncio
 * Helper function to populate the setup packet (first 8 bytes of the data
 * buffer) for a control transfer. The wIndex, wValue and wLength values should
 * be given in host-endian byte order.
 *
 * \param buffer buffer to output the setup packet into
 * This pointer must be aligned to at least 2 bytes boundary.
 * \param bmRequestType see the
 * \ref libusb_control_setup::bmRequestType "bmRequestType" field of
 * \ref libusb_control_setup
 * \param bRequest see the
 * \ref libusb_control_setup::bRequest "bRequest" field of
 * \ref libusb_control_setup
 * \param wValue see the
 * \ref libusb_control_setup::wValue "wValue" field of
 * \ref libusb_control_setup
 * \param wIndex see the
 * \ref libusb_control_setup::wIndex "wIndex" field of
 * \ref libusb_control_setup
 * \param wLength see the
 * \ref libusb_control_setup::wLength "wLength" field of
 * \ref libusb_control_setup
 *
 * 帮助程序功能填充设置数据包（数据缓冲区的前8个字节）以进行控制传输。  wIndex，wValue和wLength值应以主机字节序排列。
 */
static inline void libusb_fill_control_setup(unsigned char *buffer,
	uint8_t bmRequestType, uint8_t bRequest, uint16_t wValue, uint16_t wIndex,
	uint16_t wLength)
{
	struct libusb_control_setup *setup = (struct libusb_control_setup *)(void *) buffer;
	setup->bmRequestType = bmRequestType;
	setup->bRequest = bRequest;
	setup->wValue = libusb_cpu_to_le16(wValue);
	setup->wIndex = libusb_cpu_to_le16(wIndex);
	setup->wLength = libusb_cpu_to_le16(wLength);
}

struct libusb_transfer * LIBUSB_CALL libusb_alloc_transfer(int iso_packets);
int LIBUSB_CALL libusb_submit_transfer(struct libusb_transfer *transfer);
int LIBUSB_CALL libusb_cancel_transfer(struct libusb_transfer *transfer);
void LIBUSB_CALL libusb_free_transfer(struct libusb_transfer *transfer);
void LIBUSB_CALL libusb_transfer_set_stream_id(
	struct libusb_transfer *transfer, uint32_t stream_id);
uint32_t LIBUSB_CALL libusb_transfer_get_stream_id(
	struct libusb_transfer *transfer);

/** \ingroup asyncio
 * Helper function to populate the required \ref libusb_transfer fields
 * for a control transfer.
 *
 * If you pass a transfer buffer to this function, the first 8 bytes will
 * be interpreted as a control setup packet, and the wLength field will be
 * used to automatically populate the \ref libusb_transfer::length "length"
 * field of the transfer. Therefore the recommended approach is:
 * -# Allocate a suitably sized data buffer (including space for control setup)
 * -# Call libusb_fill_control_setup()
 * -# If this is a host-to-device transfer with a data stage, put the data
 *    in place after the setup packet
 * -# Call this function
 * -# Call libusb_submit_transfer()
 *
 * It is also legal to pass a NULL buffer to this function, in which case this
 * function will not attempt to populate the length field. Remember that you
 * must then populate the buffer and length fields later.
 *
 * \param transfer the transfer to populate
 * \param dev_handle handle of the device that will handle the transfer
 * \param buffer data buffer. If provided, this function will interpret the
 * first 8 bytes as a setup packet and infer the transfer length from that.
 * This pointer must be aligned to at least 2 bytes boundary.
 * \param callback callback function to be invoked on transfer completion
 * \param user_data user data to pass to callback function
 * \param timeout timeout for the transfer in milliseconds
 *
 * 帮助程序函数填充控件传输所需的libusb_transfer字段。
 * 如果将传输缓冲区传递给此函数，则前8个字节将被解释为控制设置包，并且wLength字段将用于自动填充传输的libusb_transfer::length "length"字段。
 * 因此，推荐的方法是：
 * -＃分配适当大小的数据缓冲区（包括用于控制设置的空间）
 * -＃调用libusb_fill_control_setup()
 * -＃如果这是具有数据阶段的主机到设备的传输，则将数据放置在设置数据包
 * -＃调用此函数
 * -＃调用libusb_submit_transfer()
 * 向该函数传递NULL缓冲区也是合法的，在这种情况下，该函数将不会尝试填充length字段。 请记住，然后必须稍后填充缓冲区和长度字段。
 */
static inline void libusb_fill_control_transfer(
	struct libusb_transfer *transfer, libusb_device_handle *dev_handle,
	unsigned char *buffer, libusb_transfer_cb_fn callback, void *user_data,
	unsigned int timeout)
{
	struct libusb_control_setup *setup = (struct libusb_control_setup *)(void *) buffer;
	transfer->dev_handle = dev_handle;
	transfer->endpoint = 0;
	transfer->type = LIBUSB_TRANSFER_TYPE_CONTROL;
	transfer->timeout = timeout;
	transfer->buffer = buffer;
	if (setup)
		transfer->length = (int) (LIBUSB_CONTROL_SETUP_SIZE
			+ libusb_le16_to_cpu(setup->wLength));
	transfer->user_data = user_data;
	transfer->callback = callback;
}

/** \ingroup asyncio
 * Helper function to populate the required \ref libusb_transfer fields
 * for a bulk transfer.
 *
 * \param transfer the transfer to populate
 * \param dev_handle handle of the device that will handle the transfer
 * \param endpoint address of the endpoint where this transfer will be sent
 * \param buffer data buffer
 * \param length length of data buffer
 * \param callback callback function to be invoked on transfer completion
 * \param user_data user data to pass to callback function
 * \param timeout timeout for the transfer in milliseconds
 *
 * Helper函数可填充块传输所需的libusb_transfer字段。
 */
static inline void libusb_fill_bulk_transfer(struct libusb_transfer *transfer,
	libusb_device_handle *dev_handle, unsigned char endpoint,
	unsigned char *buffer, int length, libusb_transfer_cb_fn callback,
	void *user_data, unsigned int timeout)
{
	transfer->dev_handle = dev_handle;
	transfer->endpoint = endpoint;
	transfer->type = LIBUSB_TRANSFER_TYPE_BULK;
	transfer->timeout = timeout;
	transfer->buffer = buffer;
	transfer->length = length;
	transfer->user_data = user_data;
	transfer->callback = callback;
}

/** \ingroup asyncio
 * Helper function to populate the required \ref libusb_transfer fields
 * for a bulk transfer using bulk streams.
 *
 * Since version 1.0.19, \ref LIBUSB_API_VERSION >= 0x01000103
 *
 * \param transfer the transfer to populate
 * \param dev_handle handle of the device that will handle the transfer
 * \param endpoint address of the endpoint where this transfer will be sent
 * \param stream_id bulk stream id for this transfer
 * \param buffer data buffer
 * \param length length of data buffer
 * \param callback callback function to be invoked on transfer completion
 * \param user_data user data to pass to callback function
 * \param timeout timeout for the transfer in milliseconds
 *
 * Helper函数使用批量流填充批量传输所需的libusb_transfer字段。
 * 从1.0.19版本开始，LIBUSB_API_VERSION >= 0x01000103
 */
static inline void libusb_fill_bulk_stream_transfer(
	struct libusb_transfer *transfer, libusb_device_handle *dev_handle,
	unsigned char endpoint, uint32_t stream_id,
	unsigned char *buffer, int length, libusb_transfer_cb_fn callback,
	void *user_data, unsigned int timeout)
{
    // Helper函数可填充批量传输所需的libusb_transfer字段。
	libusb_fill_bulk_transfer(transfer, dev_handle, endpoint, buffer,
				  length, callback, user_data, timeout);
	transfer->type = LIBUSB_TRANSFER_TYPE_BULK_STREAM;
	libusb_transfer_set_stream_id(transfer, stream_id);
}

/** \ingroup asyncio
 * Helper function to populate the required \ref libusb_transfer fields
 * for an interrupt transfer.
 *
 * \param transfer the transfer to populate
 * \param dev_handle handle of the device that will handle the transfer
 * \param endpoint address of the endpoint where this transfer will be sent
 * \param buffer data buffer
 * \param length length of data buffer
 * \param callback callback function to be invoked on transfer completion
 * \param user_data user data to pass to callback function
 * \param timeout timeout for the transfer in milliseconds
 *
 * Helper函数填充中断传输所需的libusb_transfer字段。
 */
static inline void libusb_fill_interrupt_transfer(
	struct libusb_transfer *transfer, libusb_device_handle *dev_handle,
	unsigned char endpoint, unsigned char *buffer, int length,
	libusb_transfer_cb_fn callback, void *user_data, unsigned int timeout)
{
	transfer->dev_handle = dev_handle;
	transfer->endpoint = endpoint;
	transfer->type = LIBUSB_TRANSFER_TYPE_INTERRUPT;
	transfer->timeout = timeout;
	transfer->buffer = buffer;
	transfer->length = length;
	transfer->user_data = user_data;
	transfer->callback = callback;
}

/** \ingroup asyncio
 * Helper function to populate the required \ref libusb_transfer fields
 * for an isochronous transfer.
 *
 * \param transfer the transfer to populate
 * \param dev_handle handle of the device that will handle the transfer
 * \param endpoint address of the endpoint where this transfer will be sent
 * \param buffer data buffer
 * \param length length of data buffer
 * \param num_iso_packets the number of isochronous packets
 * \param callback callback function to be invoked on transfer completion
 * \param user_data user data to pass to callback function
 * \param timeout timeout for the transfer in milliseconds
 *
 * Helper函数填充同步传输所需的 libusb_transfer 字段。
 */
static inline void libusb_fill_iso_transfer(struct libusb_transfer *transfer,
	libusb_device_handle *dev_handle, unsigned char endpoint,
	unsigned char *buffer, int length, int num_iso_packets,
	libusb_transfer_cb_fn callback, void *user_data, unsigned int timeout)
{
	transfer->dev_handle = dev_handle;
	transfer->endpoint = endpoint;
	transfer->type = LIBUSB_TRANSFER_TYPE_ISOCHRONOUS;
	transfer->timeout = timeout;
	transfer->buffer = buffer;
	transfer->length = length;
	transfer->num_iso_packets = num_iso_packets;
	transfer->user_data = user_data;
	transfer->callback = callback;
}

/** \ingroup asyncio
 * Convenience function to set the length of all packets in an isochronous
 * transfer, based on the num_iso_packets field in the transfer structure.
 *
 * \param transfer a transfer
 * \param length the length to set in each isochronous packet descriptor
 * \see libusb_get_max_packet_size()
 *
 * 便利功能可根据传输结构中的num_iso_packets字段设置同步传输中所有数据包的长度。
 */
static inline void libusb_set_iso_packet_lengths(
	struct libusb_transfer *transfer, unsigned int length)
{
	int i;
	for (i = 0; i < transfer->num_iso_packets; i++) {
	    transfer->iso_packet_desc[i].length = length;
	}
}

/** \ingroup asyncio
 * Convenience function to locate the position of an isochronous packet
 * within the buffer of an isochronous transfer.
 *
 * This is a thorough function which loops through all preceding packets,
 * accumulating their lengths to find the position of the specified packet.
 * Typically you will assign equal lengths to each packet in the transfer,
 * and hence the above method is sub-optimal. You may wish to use
 * libusb_get_iso_packet_buffer_simple() instead.
 *
 * \param transfer a transfer
 * \param packet the packet to return the address of
 * \returns the base address of the packet buffer inside the transfer buffer,
 * or NULL if the packet does not exist.
 * \see libusb_get_iso_packet_buffer_simple()
 *
 * 便利功能，用于在同步传输的缓冲区内定位同步数据包的位置。
 * 这是一项彻底的功能，它遍历所有先前的数据包，累积它们的长度以找到指定数据包的位置。
 * 通常，您将为传输中的每个数据包分配相等的长度，因此上述方法不是最优的。
 * 您可能希望使用libusb_get_iso_packet_buffer_simple()。
 */
static inline unsigned char *libusb_get_iso_packet_buffer(
	struct libusb_transfer *transfer, unsigned int packet)
{
	int i;
	size_t offset = 0;
	int _packet;

	/* oops..slight bug in the API. packet is an unsigned int, but we use
	 * signed integers almost everywhere else. range-check and convert to
	 * signed to avoid compiler warnings. FIXME for libusb-2.
	 *
	 * API中的oops..slight错误。
	 * packet是一个无符号整数，但是我们几乎在其他所有地方都使用有符号整数。
	 * 进行范围检查并转换为带符号以避免编译器警告。
	 */
	if (packet > INT_MAX)
		return NULL;
	_packet = (int) packet;

	if (_packet >= transfer->num_iso_packets)
		return NULL;

	for (i = 0; i < _packet; i++)
		offset += transfer->iso_packet_desc[i].length;

	return transfer->buffer + offset;
}

/** \ingroup asyncio
 * Convenience function to locate the position of an isochronous packet
 * within the buffer of an isochronous transfer, for transfers where each
 * packet is of identical size.
 *
 * This function relies on the assumption that every packet within the transfer
 * is of identical size to the first packet. Calculating the location of
 * the packet buffer is then just a simple calculation:
 * <tt>buffer + (packet_size * packet)</tt>
 *
 * Do not use this function on transfers other than those that have identical
 * packet lengths for each packet.
 *
 * \param transfer a transfer
 * \param packet the packet to return the address of
 * \returns the base address of the packet buffer inside the transfer buffer,
 * or NULL if the packet does not exist.
 * \see libusb_get_iso_packet_buffer()
 *
 * 方便的功能，用于将同步数据包的位置定位在同步传输的缓冲区内，用于每个数据包大小相同的传输。
 * 此功能基于以下假设：传输中的每个数据包都具有与第一个数据包相同的大小。 那么，计算数据包缓冲区的位置只是一个简单的计算：缓冲区+（packet_size *数据包）
 * 除了每个数据包具有相同数据包长度的传输之外，请勿在传输中使用此功能。
 */
static inline unsigned char *libusb_get_iso_packet_buffer_simple(
	struct libusb_transfer *transfer, unsigned int packet)
{
	int _packet;

	/* oops..slight bug in the API. packet is an unsigned int, but we use
	 * signed integers almost everywhere else. range-check and convert to
	 * signed to avoid compiler warnings. FIXME for libusb-2.
	 *
	 * API中的oops..slight错误。
	 * packet是一个无符号整数，但是我们几乎在其他所有地方都使用有符号整数。
	 * 进行范围检查并转换为带符号以避免编译器警告。
	 */
	if (packet > INT_MAX)
		return NULL;
	_packet = (int) packet;

	if (_packet >= transfer->num_iso_packets)
		return NULL;

	return transfer->buffer + ((int) transfer->iso_packet_desc[0].length * _packet);
}

/* sync I/O
 * 同步输入/输出
 */

int LIBUSB_CALL libusb_control_transfer(libusb_device_handle *dev_handle,
	uint8_t request_type, uint8_t bRequest, uint16_t wValue, uint16_t wIndex,
	unsigned char *data, uint16_t wLength, unsigned int timeout);

int LIBUSB_CALL libusb_bulk_transfer(libusb_device_handle *dev_handle,
	unsigned char endpoint, unsigned char *data, int length,
	int *actual_length, unsigned int timeout);

int LIBUSB_CALL libusb_interrupt_transfer(libusb_device_handle *dev_handle,
	unsigned char endpoint, unsigned char *data, int length,
	int *actual_length, unsigned int timeout);

/** \ingroup desc
 * Retrieve a descriptor from the default control pipe.
 * This is a convenience function which formulates the appropriate control
 * message to retrieve the descriptor.
 *
 * \param dev a device handle
 * \param desc_type the descriptor type, see \ref libusb_descriptor_type
 * \param desc_index the index of the descriptor to retrieve
 * \param data output buffer for descriptor
 * \param length size of data buffer
 * \returns number of bytes returned in data, or LIBUSB_ERROR code on failure
 *
 * 从默认控制管道检索描述符。 这是一种便利功能，可制定适当的控制消息以检索描述符。
 */
static inline int libusb_get_descriptor(libusb_device_handle *dev,
	uint8_t desc_type, uint8_t desc_index, unsigned char *data, int length)
{
	return libusb_control_transfer(dev, LIBUSB_ENDPOINT_IN,
		LIBUSB_REQUEST_GET_DESCRIPTOR, (uint16_t) ((desc_type << 8) | desc_index),
		0, data, (uint16_t) length, 1000);
}

/** \ingroup desc
 * Retrieve a descriptor from a device.
 * This is a convenience function which formulates the appropriate control
 * message to retrieve the descriptor. The string returned is Unicode, as
 * detailed in the USB specifications.
 *
 * \param dev a device handle
 * \param desc_index the index of the descriptor to retrieve
 * \param langid the language ID for the string descriptor
 * \param data output buffer for descriptor
 * \param length size of data buffer
 * \returns number of bytes returned in data, or LIBUSB_ERROR code on failure
 * \see libusb_get_string_descriptor_ascii()
 *
 * 从设备检索描述符。 这是一种便利功能，可制定适当的控制消息以检索描述符。 返回的字符串是Unicode，如USB规范中所述。
 */
static inline int libusb_get_string_descriptor(libusb_device_handle *dev,
	uint8_t desc_index, uint16_t langid, unsigned char *data, int length)
{
	return libusb_control_transfer(dev, LIBUSB_ENDPOINT_IN,
		LIBUSB_REQUEST_GET_DESCRIPTOR, (uint16_t)((LIBUSB_DT_STRING << 8) | desc_index),
		langid, data, (uint16_t) length, 1000);
}

int LIBUSB_CALL libusb_get_string_descriptor_ascii(libusb_device_handle *dev,
	uint8_t desc_index, unsigned char *data, int length);

/* polling and timeouts
 * 轮询和超时
 */

int LIBUSB_CALL libusb_try_lock_events(libusb_context *ctx);
void LIBUSB_CALL libusb_lock_events(libusb_context *ctx);
void LIBUSB_CALL libusb_unlock_events(libusb_context *ctx);
int LIBUSB_CALL libusb_event_handling_ok(libusb_context *ctx);
int LIBUSB_CALL libusb_event_handler_active(libusb_context *ctx);
void LIBUSB_CALL libusb_lock_event_waiters(libusb_context *ctx);
void LIBUSB_CALL libusb_unlock_event_waiters(libusb_context *ctx);
int LIBUSB_CALL libusb_wait_for_event(libusb_context *ctx, struct timeval *tv);

int LIBUSB_CALL libusb_handle_events_timeout(libusb_context *ctx,
	struct timeval *tv);
int LIBUSB_CALL libusb_handle_events_timeout_completed(libusb_context *ctx,
	struct timeval *tv, int *completed);
int LIBUSB_CALL libusb_handle_events(libusb_context *ctx);
int LIBUSB_CALL libusb_handle_events_completed(libusb_context *ctx, int *completed);
int LIBUSB_CALL libusb_handle_events_locked(libusb_context *ctx,
	struct timeval *tv);
int LIBUSB_CALL libusb_pollfds_handle_timeouts(libusb_context *ctx);
int LIBUSB_CALL libusb_get_next_timeout(libusb_context *ctx,
	struct timeval *tv);

/** \ingroup poll
 * File descriptor for polling
 * 用于轮询的文件描述符
 */
struct libusb_pollfd {
	/** Numeric file descriptor
	 * 数值文件描述符
	 */
	int fd;

	/** Event flags to poll for from <poll.h>. POLLIN indicates that you
	 * should monitor this file descriptor for becoming ready to read from,
	 * and POLLOUT indicates that you should monitor this file descriptor for
	 * nonblocking write readiness.
	 * 从<poll.h>进行轮询的事件标志。POLLIN指示您应监视此文件描述符以使其准备就绪以供读取，而POLLOUT指示应监视此文件描述符以确保无阻塞的写入准备状态。
	 */
	short events;
};

/** \ingroup poll
 * Callback function, invoked when a new file descriptor should be added
 * to the set of file descriptors monitored for events.
 * \param fd the new file descriptor
 * \param events events to monitor for, see \ref libusb_pollfd for a
 * description
 * \param user_data User data pointer specified in
 * libusb_set_pollfd_notifiers() call
 * \see libusb_set_pollfd_notifiers()
 *
 * 回调函数，在将新文件描述符添加到受事件监视的文件描述符集中时调用。
 */
typedef void (LIBUSB_CALL *libusb_pollfd_added_cb)(int fd, short events,
	void *user_data);

/** \ingroup poll
 * Callback function, invoked when a file descriptor should be removed from
 * the set of file descriptors being monitored for events. After returning
 * from this callback, do not use that file descriptor again.
 * \param fd the file descriptor to stop monitoring
 * \param user_data User data pointer specified in
 * libusb_set_pollfd_notifiers() call
 * \see libusb_set_pollfd_notifiers()
 *
 * 回调函数，当应从正在监视事件的文件描述符集中删除文件描述符时调用。 从此回调返回后，请勿再次使用该文件描述符。
 */
typedef void (LIBUSB_CALL *libusb_pollfd_removed_cb)(int fd, void *user_data);

const struct libusb_pollfd ** LIBUSB_CALL libusb_get_pollfds(
	libusb_context *ctx);
void LIBUSB_CALL libusb_set_pollfd_notifiers(libusb_context *ctx,
	libusb_pollfd_added_cb added_cb, libusb_pollfd_removed_cb removed_cb,
	void *user_data);

/** \ingroup hotplug
 * Callback handle.
 *
 * Callbacks handles are generated by libusb_hotplug_register_callback()
 * and can be used to deregister callbacks. Callback handles are unique
 * per libusb_context and it is safe to call libusb_hotplug_deregister_callback()
 * on an already deregisted callback.
 *
 * Since version 1.0.16, \ref LIBUSB_API_VERSION >= 0x01000102
 *
 * For more information, see \ref hotplug.
 *
 * 回调句柄。
 * 回调句柄由libusb_hotplug_register_callback()生成，可用于取消注册回调。
 * 回调句柄在每个libusb_context中都是唯一的，可以在已经注销的回调上调用libusb_hotplug_deregister_callback()是安全的。
 * 从1.0.16版本开始，LIBUSB_API_VERSION >= 0x01000102有关更多信息，请参见hotplug。
 */
typedef int libusb_hotplug_callback_handle;

/** \ingroup hotplug
 *
 * Since version 1.0.16, \ref LIBUSB_API_VERSION >= 0x01000102
 *
 * Flags for hotplug events
 * 从1.0.16版本开始，LIBUSB_API_VERSION >= 0x01000102热插拔事件的标志
 */
typedef enum {
	/** Arm the callback and fire it for all matching currently attached devices.
	 * 武装回调并为所有匹配的当前连接的设备触发回调。
	 */
	LIBUSB_HOTPLUG_ENUMERATE = 1,
} libusb_hotplug_flag;

/** \ingroup hotplug
 *
 * Since version 1.0.16, \ref LIBUSB_API_VERSION >= 0x01000102
 *
 * Hotplug events
 * 从1.0.16版本开始，LIBUSB_API_VERSION >= 0x01000102热插拔事件
 */
typedef enum {
	/** A device has been plugged in and is ready to use
	 * 设备已插入并可以使用
	 */
	LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED = 0x01,

	/** A device has left and is no longer available.
	 * It is the user's responsibility to call libusb_close on any handle associated with a disconnected device.
	 * It is safe to call libusb_get_device_descriptor on a device that has left
	 * 设备已离开，不再可用。
     * 在与断开连接的设备关联的任何句柄上调用libusb_close是用户的责任。
     * 在离开的设备上调用libusb_get_device_descriptor是安全的
	 */
	LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT    = 0x02,
} libusb_hotplug_event;

/** \ingroup hotplug
 * Wildcard matching for hotplug events
 * 热插拔事件的通配符匹配
 */
#define LIBUSB_HOTPLUG_MATCH_ANY -1

/** \ingroup hotplug
 * Hotplug callback function type. When requesting hotplug event notifications,
 * you pass a pointer to a callback function of this type.
 *
 * This callback may be called by an internal event thread and as such it is
 * recommended the callback do minimal processing before returning.
 *
 * libusb will call this function later, when a matching event had happened on
 * a matching device. See \ref hotplug for more information.
 *
 * It is safe to call either libusb_hotplug_register_callback() or
 * libusb_hotplug_deregister_callback() from within a callback function.
 *
 * Since version 1.0.16, \ref LIBUSB_API_VERSION >= 0x01000102
 *
 * \param ctx            context of this notification
 * \param device         libusb_device this event occurred on
 * \param event          event that occurred
 * \param user_data      user data provided when this callback was registered
 * \returns bool whether this callback is finished processing events.
 *                       returning 1 will cause this callback to be deregistered
 *
 * 热插拔回调函数类型。 请求热插拔事件通知时，您将指针传递给此类型的回调函数。
 * 该回调可以由内部事件线程调用，因此建议在返回之前，使回调进行最少的处理。
 * 当匹配设备上发生匹配事件时，libusb将稍后调用此函数。 有关更多信息，请参见热插拔。
 * 从回调函数中调用libusb_hotplug_register_callback()或libusb_hotplug_deregister_callback()是安全的。
 * 从1.0.16版本开始，LIBUSB_API_VERSION >= 0x01000102
 */
typedef int (LIBUSB_CALL *libusb_hotplug_callback_fn)(libusb_context *ctx,
						libusb_device *device,
						libusb_hotplug_event event,
						void *user_data);

/** \ingroup hotplug
 * Register a hotplug callback function
 *
 * Register a callback with the libusb_context. The callback will fire
 * when a matching event occurs on a matching device. The callback is
 * armed until either it is deregistered with libusb_hotplug_deregister_callback()
 * or the supplied callback returns 1 to indicate it is finished processing events.
 *
 * If the \ref LIBUSB_HOTPLUG_ENUMERATE is passed the callback will be
 * called with a \ref LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED for all devices
 * already plugged into the machine. Note that libusb modifies its internal
 * device list from a separate thread, while calling hotplug callbacks from
 * libusb_handle_events(), so it is possible for a device to already be present
 * on, or removed from, its internal device list, while the hotplug callbacks
 * still need to be dispatched. This means that when using \ref
 * LIBUSB_HOTPLUG_ENUMERATE, your callback may be called twice for the arrival
 * of the same device, once from libusb_hotplug_register_callback() and once
 * from libusb_handle_events(); and/or your callback may be called for the
 * removal of a device for which an arrived call was never made.
 *
 * Since version 1.0.16, \ref LIBUSB_API_VERSION >= 0x01000102
 *
 * \param[in] ctx context to register this callback with
 * \param[in] events bitwise or of events that will trigger this callback. See \ref
 *            libusb_hotplug_event
 * \param[in] flags hotplug callback flags. See \ref libusb_hotplug_flag
 * \param[in] vendor_id the vendor id to match or \ref LIBUSB_HOTPLUG_MATCH_ANY
 * \param[in] product_id the product id to match or \ref LIBUSB_HOTPLUG_MATCH_ANY
 * \param[in] dev_class the device class to match or \ref LIBUSB_HOTPLUG_MATCH_ANY
 * \param[in] cb_fn the function to be invoked on a matching event/device
 * \param[in] user_data user data to pass to the callback function
 * \param[out] handle pointer to store the handle of the allocated callback (can be NULL)
 * \returns LIBUSB_SUCCESS on success LIBUSB_ERROR code on failure
 *
 * 注册一个热插拔回调函数用libusb_context注册一个回调。 
 * 当匹配设备上发生匹配事件时，将触发回调。 回调将处于预备状态，直到通过libusb_hotplug_deregister_callback()注销了该回调，或者提供的回调返回1表示完成处理事件。
 * 如果传递了LIBUSB_HOTPLUG_ENUMERATE，则将为所有已插入计算机的设备使用LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED调用回调。
 * 请注意，libusb从一个单独的线程修改其内部设备列表，同时从libusb_handle_events()调用热插拔回调，
 * 因此，某个设备可能已经存在于其内部设备列表中或从其内部设备列表中删除，而仍然需要热插拔回调待派遣。
 * 这意味着，当使用LIBUSB_HOTPLUG_ENUMERATE时，对于同一设备的到达，您的回调可能会被调用两次，一次是从libusb_hotplug_register_callback()，一次是从libusb_handle_events()；
 * 和/或您的回调可能会被调用，以删除从未对其进行过到达呼叫的设备。
 * 从1.0.16版本开始，LIBUSB_API_VERSION >= 0x01000102
 */
int LIBUSB_CALL libusb_hotplug_register_callback(libusb_context *ctx,
						libusb_hotplug_event events,
						libusb_hotplug_flag flags,
						int vendor_id, int product_id,
						int dev_class,
						libusb_hotplug_callback_fn cb_fn,
						void *user_data,
						libusb_hotplug_callback_handle *handle);

/** \ingroup hotplug
 * Deregisters a hotplug callback.
 *
 * Deregister a callback from a libusb_context. This function is safe to call from within
 * a hotplug callback.
 *
 * Since version 1.0.16, \ref LIBUSB_API_VERSION >= 0x01000102
 *
 * \param[in] ctx context this callback is registered with
 * \param[in] handle the handle of the callback to deregister
 *
 * 注销热插拔回调。
 * 从libusb_context注销回调。 从热插拔回调中可以安全地调用此函数。
 * 从1.0.16版本开始，LIBUSB_API_VERSION >= 0x01000102
 */
void LIBUSB_CALL libusb_hotplug_deregister_callback(libusb_context *ctx,
						libusb_hotplug_callback_handle handle);

#ifdef __cplusplus
}
#endif

#endif
