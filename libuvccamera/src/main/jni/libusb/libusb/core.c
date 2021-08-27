/* -*- Mode: C; indent-tabs-mode:t ; c-basic-offset:8 -*- */
/*
 * add some functions for no-rooted Android
 * add optimaization when compiling with gcc
 * Copyright © 2014-2017 saki <t_saki@serenegiant.com>
 *
 * Core functions for libusb
 * Copyright © 2012-2013 Nathan Hjelm <hjelmn@cs.unm.edu>
 * Copyright © 2007-2008 Daniel Drake <dsd@gentoo.org>
 * Copyright © 2001 Johannes Erdfelt <johannes@erdfelt.com>
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

#define LOCAL_DEBUG 0

#define LOG_TAG "libusb/core"
#if 1	// デバッグ情報を出さない時1
	#ifndef LOG_NDEBUG
		#define	LOG_NDEBUG		// LOGV/LOGD/MARKを出力しない時
		#endif
	#undef USE_LOGALL			// 指定したLOGxだけを出力
#else
	#define USE_LOGALL
	#undef LOG_NDEBUG
	#undef NDEBUG
	#define GET_RAW_DESCRIPTOR
#endif

#include "config.h"
#include <assert.h>		// XXX add assert for debugging

#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#endif
#ifdef HAVE_SYSLOG_H
#include <syslog.h>
#endif

#ifdef __ANDROID__
#include <android/log.h>
#endif

#include "libusbi.h"
#include "hotplug.h"

#if defined(OS_ANDROID)	// XXX for non rooted android device  对于无根的android设备
const struct usbi_os_backend * const usbi_backend = &android_usbfs_backend;
#elif defined(OS_LINUX)
const struct usbi_os_backend * const usbi_backend = &linux_usbfs_backend;
#elif defined(OS_DARWIN)
const struct usbi_os_backend * const usbi_backend = &darwin_backend;
#elif defined(OS_OPENBSD)
const struct usbi_os_backend * const usbi_backend = &openbsd_backend;
#elif defined(OS_NETBSD)
const struct usbi_os_backend * const usbi_backend = &netbsd_backend;
#elif defined(OS_WINDOWS)
const struct usbi_os_backend * const usbi_backend = &windows_backend;
#elif defined(OS_WINCE)
const struct usbi_os_backend * const usbi_backend = &wince_backend;
#else
#error "Unsupported OS"
#endif

struct libusb_context *usbi_default_context = NULL;
static const struct libusb_version libusb_version_internal =
	{ LIBUSB_MAJOR, LIBUSB_MINOR, LIBUSB_MICRO, LIBUSB_NANO,
	  LIBUSB_RC, "http://libusb.info" };
static int default_context_refcnt = 0;
static usbi_mutex_static_t default_context_lock = USBI_MUTEX_INITIALIZER;
static struct timeval timestamp_origin = { 0, 0 };

usbi_mutex_static_t active_contexts_lock = USBI_MUTEX_INITIALIZER;
struct list_head active_contexts_list;

#ifdef __ANDROID__
int android_generate_device(struct libusb_context *ctx, struct libusb_device **dev,
	int vid, int pid, const char *serial, int fd, int busnum, int devaddr);
#endif

/**
 * \mainpage libusb-1.0 API Reference
 *
 * \section intro Introduction
 *
 * libusb is an open source library that allows you to communicate with USB
 * devices from userspace. For more info, see the
 * <a href="http://libusb.info">libusb homepage</a>.
 *
 * This documentation is aimed at application developers wishing to
 * communicate with USB peripherals from their own software. After reviewing
 * this documentation, feedback and questions can be sent to the
 * <a href="http://mailing-list.libusb.info">libusb-devel mailing list</a>.
 *
 * This documentation assumes knowledge of how to operate USB devices from
 * a software standpoint (descriptors, configurations, interfaces, endpoints,
 * control/bulk/interrupt/isochronous transfers, etc). Full information
 * can be found in the <a href="http://www.usb.org/developers/docs/">USB 3.0
 * Specification</a> which is available for free download. You can probably
 * find less verbose introductions by searching the web.
 *
 * \section features Library features
 *
 * - All transfer types supported (control/bulk/interrupt/isochronous)
 * - 2 transfer interfaces:
 *    -# Synchronous (simple)
 *    -# Asynchronous (more complicated, but more powerful)
 * - Thread safe (although the asynchronous interface means that you
 *   usually won't need to thread)
 * - Lightweight with lean API
 * - Compatible with libusb-0.1 through the libusb-compat-0.1 translation layer
 * - Hotplug support (on some platforms). See \ref hotplug.
 *
 * \section gettingstarted Getting Started
 *
 * To begin reading the API documentation, start with the Modules page which
 * links to the different categories of libusb's functionality.
 *
 * One decision you will have to make is whether to use the synchronous
 * or the asynchronous data transfer interface. The \ref io documentation
 * provides some insight into this topic.
 *
 * Some example programs can be found in the libusb source distribution under
 * the "examples" subdirectory. The libusb homepage includes a list of
 * real-life project examples which use libusb.
 *
 * \section errorhandling Error handling
 *
 * libusb functions typically return 0 on success or a negative error code
 * on failure. These negative error codes relate to LIBUSB_ERROR constants
 * which are listed on the \ref misc "miscellaneous" documentation page.
 *
 * \section msglog Debug message logging
 *
 * libusb uses stderr for all logging. By default, logging is set to NONE,
 * which means that no output will be produced. However, unless the library
 * has been compiled with logging disabled, then any application calls to
 * libusb_set_debug(), or the setting of the environmental variable
 * LIBUSB_DEBUG outside of the application, can result in logging being
 * produced. Your application should therefore not close stderr, but instead
 * direct it to the null device if its output is undesireable.
 *
 * The libusb_set_debug() function can be used to enable logging of certain
 * messages. Under standard configuration, libusb doesn't really log much
 * so you are advised to use this function to enable all error/warning/
 * informational messages. It will help debug problems with your software.
 *
 * The logged messages are unstructured. There is no one-to-one correspondence
 * between messages being logged and success or failure return codes from
 * libusb functions. There is no format to the messages, so you should not
 * try to capture or parse them. They are not and will not be localized.
 * These messages are not intended to being passed to your application user;
 * instead, you should interpret the error codes returned from libusb functions
 * and provide appropriate notification to the user. The messages are simply
 * there to aid you as a programmer, and if you're confused because you're
 * getting a strange error code from a libusb function, enabling message
 * logging may give you a suitable explanation.
 *
 * The LIBUSB_DEBUG environment variable can be used to enable message logging
 * at run-time. This environment variable should be set to a log level number,
 * which is interpreted the same as the libusb_set_debug() parameter. When this
 * environment variable is set, the message logging verbosity level is fixed
 * and libusb_set_debug() effectively does nothing.
 *
 * libusb can be compiled without any logging functions, useful for embedded
 * systems. In this case, libusb_set_debug() and the LIBUSB_DEBUG environment
 * variable have no effects.
 *
 * libusb can also be compiled with verbose debugging messages always. When
 * the library is compiled in this way, all messages of all verbosities are
 * always logged. libusb_set_debug() and the LIBUSB_DEBUG environment variable
 * have no effects.
 *
 * \section remarks Other remarks
 *
 * libusb does have imperfections. The \ref caveats "caveats" page attempts
 * to document these.
 */

/**
 * \page caveats Caveats
 *
 * \section devresets Device resets
 *
 * The libusb_reset_device() function allows you to reset a device. If your
 * program has to call such a function, it should obviously be aware that
 * the reset will cause device state to change (e.g. register values may be
 * reset).
 *
 * The problem is that any other program could reset the device your program
 * is working with, at any time. libusb does not offer a mechanism to inform
 * you when this has happened, so if someone else resets your device it will
 * not be clear to your own program why the device state has changed.
 *
 * Ultimately, this is a limitation of writing drivers in userspace.
 * Separation from the USB stack in the underlying kernel makes it difficult
 * for the operating system to deliver such notifications to your program.
 * The Linux kernel USB stack allows such reset notifications to be delivered
 * to in-kernel USB drivers, but it is not clear how such notifications could
 * be delivered to second-class drivers that live in userspace.
 *
 * \section blockonly Blocking-only functionality
 *
 * The functionality listed below is only available through synchronous,
 * blocking functions. There are no asynchronous/non-blocking alternatives,
 * and no clear ways of implementing these.
 *
 * - Configuration activation (libusb_set_configuration())
 * - Interface/alternate setting activation (libusb_set_interface_alt_setting())
 * - Releasing of interfaces (libusb_release_interface())
 * - Clearing of halt/stall condition (libusb_clear_halt())
 * - Device resets (libusb_reset_device())
 *
 * \section configsel Configuration selection and handling
 *
 * When libusb presents a device handle to an application, there is a chance
 * that the corresponding device may be in unconfigured state. For devices
 * with multiple configurations, there is also a chance that the configuration
 * currently selected is not the one that the application wants to use.
 *
 * The obvious solution is to add a call to libusb_set_configuration() early
 * on during your device initialization routines, but there are caveats to
 * be aware of:
 * -# If the device is already in the desired configuration, calling
 *    libusb_set_configuration() using the same configuration value will cause
 *    a lightweight device reset. This may not be desirable behaviour.
 * -# libusb will be unable to change configuration if the device is in
 *    another configuration and other programs or drivers have claimed
 *    interfaces under that configuration.
 * -# In the case where the desired configuration is already active, libusb
 *    may not even be able to perform a lightweight device reset. For example,
 *    take my USB keyboard with fingerprint reader: I'm interested in driving
 *    the fingerprint reader interface through libusb, but the kernel's
 *    USB-HID driver will almost always have claimed the keyboard interface.
 *    Because the kernel has claimed an interface, it is not even possible to
 *    perform the lightweight device reset, so libusb_set_configuration() will
 *    fail. (Luckily the device in question only has a single configuration.)
 *
 * One solution to some of the above problems is to consider the currently
 * active configuration. If the configuration we want is already active, then
 * we don't have to select any configuration:
 \code
 cfg = libusb_get_configuration(dev);
 if (cfg != desired)
 libusb_set_configuration(dev, desired);
 \endcode
 *
 * This is probably suitable for most scenarios, but is inherently racy:
 * another application or driver may change the selected configuration
 * <em>after</em> the libusb_get_configuration() call.
 *
 * Even in cases where libusb_set_configuration() succeeds, consider that other
 * applications or drivers may change configuration after your application
 * calls libusb_set_configuration().
 *
 * One possible way to lock your device into a specific configuration is as
 * follows:
 * -# Set the desired configuration (or use the logic above to realise that
 *    it is already in the desired configuration)
 * -# Claim the interface that you wish to use
 * -# Check that the currently active configuration is the one that you want
 *    to use.
 *
 * The above method works because once an interface is claimed, no application
 * or driver is able to select another configuration.
 *
 * \section earlycomp Early transfer completion
 *
 * NOTE: This section is currently Linux-centric. I am not sure if any of these
 * considerations apply to Darwin or other platforms.
 *
 * When a transfer completes early (i.e. when less data is received/sent in
 * any one packet than the transfer buffer allows for) then libusb is designed
 * to terminate the transfer immediately, not transferring or receiving any
 * more data unless other transfers have been queued by the user.
 *
 * On legacy platforms, libusb is unable to do this in all situations. After
 * the incomplete packet occurs, "surplus" data may be transferred. For recent
 * versions of libusb, this information is kept (the data length of the
 * transfer is updated) and, for device-to-host transfers, any surplus data was
 * added to the buffer. Still, this is not a nice solution because it loses the
 * information about the end of the short packet, and the user probably wanted
 * that surplus data to arrive in the next logical transfer.
 *
 *
 * \section zlp Zero length packets
 *
 * - libusb is able to send a packet of zero length to an endpoint simply by
 * submitting a transfer of zero length.
 * - The \ref libusb_transfer_flags::LIBUSB_TRANSFER_ADD_ZERO_PACKET
 * "LIBUSB_TRANSFER_ADD_ZERO_PACKET" flag is currently only supported on Linux.
 */

/**
 * \page contexts Contexts
 *
 * It is possible that libusb may be used simultaneously from two independent
 * libraries linked into the same executable. For example, if your application
 * has a plugin-like system which allows the user to dynamically load a range
 * of modules into your program, it is feasible that two independently
 * developed modules may both use libusb.
 *
 * libusb is written to allow for these multiple user scenarios. The two
 * "instances" of libusb will not interfere: libusb_set_debug() calls
 * from one user will not affect the same settings for other users, other
 * users can continue using libusb after one of them calls libusb_exit(), etc.
 *
 * This is made possible through libusb's <em>context</em> concept. When you
 * call libusb_init(), you are (optionally) given a context. You can then pass
 * this context pointer back into future libusb functions.
 *
 * In order to keep things simple for more simplistic applications, it is
 * legal to pass NULL to all functions requiring a context pointer (as long as
 * you're sure no other code will attempt to use libusb from the same process).
 * When you pass NULL, the default context will be used. The default context
 * is created the first time a process calls libusb_init() when no other
 * context is alive. Contexts are destroyed during libusb_exit().
 *
 * The default context is reference-counted and can be shared. That means that
 * if libusb_init(NULL) is called twice within the same process, the two
 * users end up sharing the same context. The deinitialization and freeing of
 * the default context will only happen when the last user calls libusb_exit().
 * In other words, the default context is created and initialized when its
 * reference count goes from 0 to 1, and is deinitialized and destroyed when
 * its reference count goes from 1 to 0.
 *
 * You may be wondering why only a subset of libusb functions require a
 * context pointer in their function definition. Internally, libusb stores
 * context pointers in other objects (e.g. libusb_device instances) and hence
 * can infer the context from those objects.
 */

/**
 * @defgroup lib Library initialization/deinitialization
 * This page details how to initialize and deinitialize libusb. Initialization
 * must be performed before using any libusb functionality, and similarly you
 * must not call any libusb functions after deinitialization.
 */

/**
 * @defgroup dev Device handling and enumeration
 * The functionality documented below is designed to help with the following
 * operations:
 * - Enumerating the USB devices currently attached to the system
 * - Choosing a device to operate from your software
 * - Opening and closing the chosen device
 *
 * \section nutshell In a nutshell...
 *
 * The description below really makes things sound more complicated than they
 * actually are. The following sequence of function calls will be suitable
 * for almost all scenarios and does not require you to have such a deep
 * understanding of the resource management issues:
 * \code
 // discover devices
 libusb_device **list;
 libusb_device *found = NULL;
 ssize_t cnt = libusb_get_device_list(NULL, &list);
 ssize_t i = 0;
 int err = 0;
 if (cnt < 0)
 error();

 for (i = 0; i < cnt; i++) {
 libusb_device *device = list[i];
 if (is_interesting(device)) {
 found = device;
 break;
 }
 }

 if (found) {
 libusb_device_handle *handle;

 err = libusb_open(found, &handle);
 if (err)
 error();
 // etc
 }

 libusb_free_device_list(list, 1);
 \endcode
 *
 * The two important points:
 * - You asked libusb_free_device_list() to unreference the devices (2nd
 *   parameter)
 * - You opened the device before freeing the list and unreferencing the
 *   devices
 *
 * If you ended up with a handle, you can now proceed to perform I/O on the
 * device.
 *
 * \section devshandles Devices and device handles
 * libusb has a concept of a USB device, represented by the
 * \ref libusb_device opaque type. A device represents a USB device that
 * is currently or was previously connected to the system. Using a reference
 * to a device, you can determine certain information about the device (e.g.
 * you can read the descriptor data).
 *
 * The libusb_get_device_list() function can be used to obtain a list of
 * devices currently connected to the system. This is known as device
 * discovery.
 *
 * Just because you have a reference to a device does not mean it is
 * necessarily usable. The device may have been unplugged, you may not have
 * permission to operate such device, or another program or driver may be
 * using the device.
 *
 * When you've found a device that you'd like to operate, you must ask
 * libusb to open the device using the libusb_open() function. Assuming
 * success, libusb then returns you a <em>device handle</em>
 * (a \ref libusb_device_handle pointer). All "real" I/O operations then
 * operate on the handle rather than the original device pointer.
 *
 * \section devref Device discovery and reference counting
 *
 * Device discovery (i.e. calling libusb_get_device_list()) returns a
 * freshly-allocated list of devices. The list itself must be freed when
 * you are done with it. libusb also needs to know when it is OK to free
 * the contents of the list - the devices themselves.
 *
 * To handle these issues, libusb provides you with two separate items:
 * - A function to free the list itself
 * - A reference counting system for the devices inside
 *
 * New devices presented by the libusb_get_device_list() function all have a
 * reference count of 1. You can increase and decrease reference count using
 * libusb_ref_device() and libusb_unref_device(). A device is destroyed when
 * its reference count reaches 0.
 *
 * With the above information in mind, the process of opening a device can
 * be viewed as follows:
 * -# Discover devices using libusb_get_device_list().
 * -# Choose the device that you want to operate, and call libusb_open().
 * -# Unref all devices in the discovered device list.
 * -# Free the discovered device list.
 *
 * The order is important - you must not unreference the device before
 * attempting to open it, because unreferencing it may destroy the device.
 *
 * For convenience, the libusb_free_device_list() function includes a
 * parameter to optionally unreference all the devices in the list before
 * freeing the list itself. This combines steps 3 and 4 above.
 *
 * As an implementation detail, libusb_open() actually adds a reference to
 * the device in question. This is because the device remains available
 * through the handle via libusb_get_device(). The reference is deleted during
 * libusb_close().
 */

/** @defgroup misc Miscellaneous */

/* we traverse usbfs without knowing how many devices we are going to find.
 * so we create this discovered_devs model which is similar to a linked-list
 * which grows when required. it can be freed once discovery has completed,
 * eliminating the need for a list node in the libusb_device structure
 * itself.
 *
 * 我们遍历usbfs却不知道要找到多少个设备。
 * 因此，我们创建了一个Discovered_devs模型，该模型类似于一个链表，该链表在需要时会增长。
 * 发现完成后可以将其释放，从而无需在libusb_device结构本身中使用列表节点。
 */
#define DISCOVERED_DEVICES_SIZE_STEP 8

static struct discovered_devs *discovered_devs_alloc(void) {

	struct discovered_devs *ret =
		malloc(sizeof(*ret) + (sizeof(void *) * DISCOVERED_DEVICES_SIZE_STEP));

	if (ret) {
		ret->len = 0;
		ret->capacity = DISCOVERED_DEVICES_SIZE_STEP;
	}
	return ret;
}

/* append a device to the discovered devices collection. may realloc itself,
 * returning new discdevs. returns NULL on realloc failure.
 * 将设备追加到发现的设备集合中。可能会重新分配自身，返回新的discdev。重新分配失败时返回NULL。
 */
struct discovered_devs *discovered_devs_append(
	struct discovered_devs *discdevs, struct libusb_device *dev) {
	
	size_t len = discdevs->len;
	size_t capacity;

	/* if there is space, just append the device
	 * 如果有空间，只需附加设备
	 */
	if (LIKELY(len < discdevs->capacity)) {
		discdevs->devices[len] = libusb_ref_device(dev);
		discdevs->len++;
		return discdevs;
	}

	/* exceeded capacity, need to grow
	 * 超出容量，需要增长
	 */
	usbi_dbg("need to increase capacity");
	capacity = discdevs->capacity + DISCOVERED_DEVICES_SIZE_STEP;
	discdevs = usbi_reallocf(discdevs, sizeof(*discdevs) + (sizeof(void *) * capacity));
	if (LIKELY(discdevs)) {
		discdevs->capacity = capacity;
		discdevs->devices[len] = libusb_ref_device(dev);
		discdevs->len++;
	}

	return discdevs;
}

static void discovered_devs_free(struct discovered_devs *discdevs) {

	size_t i;

	for (i = 0; i < discdevs->len; i++)
		libusb_unref_device(discdevs->devices[i]);

	free(discdevs);
}

/* Allocate a new device with a specific session ID. The returned device has a reference count of 1.
 * 使用特定的会话ID分配新设备。 返回的设备的参考计数为1。
 */
struct libusb_device *usbi_alloc_device(struct libusb_context *ctx,
	unsigned long session_id) {
	
	size_t priv_size = usbi_backend->device_priv_size;
	struct libusb_device *dev = calloc(1, sizeof(*dev) + priv_size);
	int r;

	if (UNLIKELY(!dev))
		return NULL ;

	r = usbi_mutex_init(&dev->lock, NULL);
	if (UNLIKELY(r)) {
		free(dev);
		return NULL;
	}

	dev->ctx = ctx;
	dev->refcnt = 1;
	dev->session_data = session_id;
	dev->speed = LIBUSB_SPEED_UNKNOWN;

	if (!libusb_has_capability(LIBUSB_CAP_HAS_HOTPLUG)) {
		usbi_connect_device(dev);
	}

	return dev;
}

void usbi_connect_device(struct libusb_device *dev) {

	libusb_hotplug_message message;
	ssize_t ret;

	memset(&message, 0, sizeof(message));
	message.event = LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED;
	message.device = dev;
	dev->attached = 1;

	usbi_mutex_lock(&dev->ctx->usb_devs_lock);
	{
		list_add(&dev->list, &dev->ctx->usb_devs);
	}
	usbi_mutex_unlock(&dev->ctx->usb_devs_lock);

	/* Signal that an event has occurred for this device if we support hotplug AND
	 * the hotplug pipe is ready. This prevents an event from getting raised during
	 * initial enumeration.
	 * 如果我们支持热插拔并且热插拔管道已准备就绪，则表明此设备发生了事件。 这样可以防止在初始枚举期间引发事件。
	 */
	if (libusb_has_capability(LIBUSB_CAP_HAS_HOTPLUG) && dev->ctx->hotplug_pipe[1] > 0) {
		ret = usbi_write(dev->ctx->hotplug_pipe[1], &message, sizeof(message));
		if (UNLIKELY(sizeof(message) != ret)) {
			usbi_err(DEVICE_CTX(dev), "error writing hotplug message");
		}
	}
}

void usbi_disconnect_device(struct libusb_device *dev) {

	libusb_hotplug_message message;
	struct libusb_context *ctx = dev->ctx;
	ssize_t ret;

	memset(&message, 0, sizeof(message));
	message.event = LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT;
	message.device = dev;
	usbi_mutex_lock(&dev->lock);
	{
		dev->attached = 0;
	}
	usbi_mutex_unlock(&dev->lock);

	usbi_mutex_lock(&ctx->usb_devs_lock);
	{
		list_del(&dev->list);
	}
	usbi_mutex_unlock(&ctx->usb_devs_lock);

	/* Signal that an event has occurred for this device if we support hotplug AND
	 * the hotplug pipe is ready. This prevents an event from getting raised during
	 * initial enumeration. libusb_handle_events will take care of dereferencing the
	 * device.
	 * 如果我们支持热插拔并且热插拔管道已准备就绪，则表明此设备发生了事件。这样可以防止在初始枚举期间引发事件。libusb_handle_events将负责取消对设备的引用。
	 */
	if (libusb_has_capability(LIBUSB_CAP_HAS_HOTPLUG) && dev->ctx->hotplug_pipe[1] > 0) {
		ret = usbi_write(dev->ctx->hotplug_pipe[1], &message, sizeof(message));
		if (UNLIKELY(sizeof(message) != ret)) {
			usbi_err(DEVICE_CTX(dev), "error writing hotplug message");
		}
	}
}

/* Perform some final sanity checks on a newly discovered device. If this
 * function fails (negative return code), the device should not be added
 * to the discovered device list.
 * 在新发现的设备上执行一些最终的健全性检查。如果此功能失败（返回码为负），则不应将设备添加到发现的设备列表中。
 */
int usbi_sanitize_device(struct libusb_device *dev) {

	int r;
	uint8_t num_configurations;

	r = usbi_device_cache_descriptor(dev);
	if (UNLIKELY(r < 0))
		return r;

	num_configurations = dev->device_descriptor.bNumConfigurations;
	if UNLIKELY(num_configurations > USB_MAXCONFIG) {
		usbi_err(DEVICE_CTX(dev), "too many configurations");
		return LIBUSB_ERROR_IO;
	} else if (0 == num_configurations)
		usbi_dbg("zero configurations, maybe an unauthorized device");

	dev->num_configurations = num_configurations;
	return LIBUSB_SUCCESS;
}

/* Examine libusb's internal list of known devices, looking for one with
 * a specific session ID. Returns the matching device if it was found, and
 * NULL otherwise.
 * 检查libusb的内部已知设备列表，以查找具有特定会话ID的设备。返回匹配的设备（如果找到），否则返回NULL。
 */
struct libusb_device *usbi_get_device_by_session_id(struct libusb_context *ctx,
		unsigned long session_id) {
		
	struct libusb_device *dev;
	struct libusb_device *ret = NULL;

	usbi_mutex_lock(&ctx->usb_devs_lock);
	{
		list_for_each_entry(dev, &ctx->usb_devs, list, struct libusb_device)
			if (dev->session_data == session_id) {
				ret = libusb_ref_device(dev);
				break;
			}
	}
	usbi_mutex_unlock(&ctx->usb_devs_lock);

	return ret;
}

/** @ingroup dev
 * Returns a list of USB devices currently attached to the system. This is
 * your entry point into finding a USB device to operate.
 *
 * You are expected to unreference all the devices when you are done with
 * them, and then free the list with libusb_free_device_list(). Note that
 * libusb_free_device_list() can unref all the devices for you. Be careful
 * not to unreference a device you are about to open until after you have
 * opened it.
 *
 * This return value of this function indicates the number of devices in
 * the resultant list. The list is actually one element larger, as it is
 * NULL-terminated.
 *
 * \param ctx the context to operate on, or NULL for the default context
 * \param list output location for a list of devices. Must be later freed with
 * libusb_free_device_list().
 * \returns the number of devices in the outputted list, or any
 * \ref libusb_error according to errors encountered by the backend.
 *
 * 返回当前连接到系统的USB设备的列表。 这是寻找可操作的USB设备的切入点。
 * 当您完成对所有设备的使用后，应先取消对它们的引用，然后使用libusb_free_device_list()释放列表。
 * 请注意，libusb_free_device_list()可以为您取消引用所有设备。在打开设备之前，请小心不要取消引用即将打开的设备。
 * 此函数的此返回值指示结果列表中的设备数。 该列表实际上是一个大元素，因为它以NULL终止。
 */
ssize_t API_EXPORTED libusb_get_device_list(libusb_context *ctx,
		libusb_device ***list) {

	ENTER();

	struct discovered_devs *discdevs = discovered_devs_alloc();
	struct libusb_device **ret;
	int r = 0;
	ssize_t i, len;
	USBI_GET_CONTEXT(ctx);
	usbi_dbg("");

	if (UNLIKELY(!discdevs))
		return LIBUSB_ERROR_NO_MEM;

	if (libusb_has_capability(LIBUSB_CAP_HAS_HOTPLUG)) {
		LOGD("backend provides hotplug support");
		struct libusb_device *dev;

		if (usbi_backend->hotplug_poll)
			usbi_backend->hotplug_poll();

		usbi_mutex_lock(&ctx->usb_devs_lock);
		{
			list_for_each_entry(dev, &ctx->usb_devs, list, struct libusb_device)
			{
				discdevs = discovered_devs_append(discdevs, dev);

				if (UNLIKELY(!discdevs)) {
					r = LIBUSB_ERROR_NO_MEM;
					break;
				}
			}
		}
		usbi_mutex_unlock(&ctx->usb_devs_lock);
	} else {
		LOGD("backend does not provide hotplug support");
		r = usbi_backend->get_device_list(ctx, &discdevs);
	}

	if (UNLIKELY(r < 0)) {
		len = r;
		goto out;
	}

	/* convert discovered_devs into a list
	 * 将found_devs转换为列表
	 */
	len = discdevs->len;
	ret = calloc(len + 1, sizeof(struct libusb_device *));
	if (UNLIKELY(!ret)) {
		LOGE("LIBUSB_ERROR_NO_MEM");
		len = LIBUSB_ERROR_NO_MEM;
		goto out;
	}

	ret[len] = NULL;
	for (i = 0; i < len; i++) {
		struct libusb_device *dev = discdevs->devices[i];
		ret[i] = libusb_ref_device(dev);
	}
	*list = ret;

out:
	discovered_devs_free(discdevs);
	RETURN(len, int);
}

/**
 * search device with specific vender ID and product ID
 * TODO it is better to check serial number for multiple device connection with same vender ID and product ID
 * @return null if not found
 * @param vid: vender ID, 0 means don't care
 * @param pid: product ID, 0 means don't care
 * @param sn: serial number(currently not use)
 * @param fd: file descripter that need to access device on no-rooted Android
 * @return null if not found
 * 具有特定供应商ID和产品ID的搜索设备
 * TODO 最好检查具有相同供应商ID和产品ID的多个设备连接的序列号
 */
libusb_device *libusb_find_device(libusb_context *ctx, const int vid,
		const int pid, const char* sn, int fd) {

	ENTER();

	libusb_device **devs;
	// get list of devices
	// 获取设备列表
	int cnt = libusb_get_device_list(ctx, &devs);
	if (UNLIKELY(cnt < 0)) {
		LOGI("failed to get device list");
		usbi_dbg("failed to get device list");
		return NULL ;
	}

	int r, i;
	libusb_device *device = NULL;
	struct libusb_device_descriptor desc;
	LOGI("try to find specific device:cnt=%d", cnt);
	for (i = 0; i < cnt; i++) {
		r = libusb_get_device_descriptor(devs[i], &desc);
		if (UNLIKELY(r < 0)) {
			LOGI("failed to get device descriptor");
			usbi_dbg("failed to get device descriptor");
			continue;
		}
		if ((!vid || (desc.idVendor == vid))
				&& (!pid || (desc.idProduct == pid))) {
			LOGI("found");
			device = devs[i];
			libusb_ref_device(device);
			break;
		}
	}

	libusb_free_device_list(devs, 1);
	RET(device);
}

/** \ingroup dev
 * Frees a list of devices previously discovered using
 * libusb_get_device_list(). If the unref_devices parameter is set, the
 * reference count of each device in the list is decremented by 1.
 * \param list the list to free
 * \param unref_devices whether to unref the devices in the list
 *
 * 释放先前使用libusb_get_device_list()发现的设备列表。如果设置了unref_devices参数，则列表中每个设备的引用计数都会减少1。
 */
void API_EXPORTED libusb_free_device_list(libusb_device **list,
		int unref_devices) {
		
	if (UNLIKELY(!list))
		return;

	if (unref_devices) {
		int i = 0;
		struct libusb_device *dev;

		while ((dev = list[i++]) != NULL)
			libusb_unref_device(dev);
	}
	free(list);
}

/** \ingroup dev
 * Get the number of the bus that a device is connected to.
 * \param dev a device
 * \returns the bus number
 * 获取设备连接的总线号。
 */
uint8_t API_EXPORTED libusb_get_bus_number(libusb_device *dev) {

	return dev->bus_number;
}

/** \ingroup dev
 * Get the number of the port that a device is connected to.
 * Unless the OS does something funky, or you are hot-plugging USB extension cards,
 * the port number returned by this call is usually guaranteed to be uniquely tied
 * to a physical port, meaning that different devices plugged on the same physical
 * port should return the same port number.
 *
 * But outside of this, there is no guarantee that the port number returned by this
 * call will remain the same, or even match the order in which ports have been
 * numbered by the HUB/HCD manufacturer.
 *
 * \param dev a device
 * \returns the port number (0 if not available)
 *
 * 获取设备连接的端口号。
 * 除非操作系统做一些时髦的事情，或者您正在热插拔USB扩展卡，否则通常会保证此调用返回的端口号唯一地绑定到物理端口，这意味着插入同一物理端口的不同设备应返回相同的端口号。
 * 但是除此之外，不能保证此调用返回的端口号将保持不变，甚至与HUB/HCD制造商对端口编号的顺序匹配。
 */
uint8_t API_EXPORTED libusb_get_port_number(libusb_device *dev) {

	return dev->port_number;
}

/** \ingroup dev
 * Get the list of all port numbers from root for the specified device
 *
 * Since version 1.0.16, \ref LIBUSB_API_VERSION >= 0x01000102
 * \param dev a device
 * \param port_numbers the array that should contain the port numbers
 * \param port_numbers_len the maximum length of the array. As per the USB 3.0
 * specs, the current maximum limit for the depth is 7.
 * \returns the number of elements filled
 * \returns LIBUSB_ERROR_OVERFLOW if the array is too small
 *
 * 从根获取指定设备的所有端口号的列表
 * 从1.0.16版本开始，LIBUSB_API_VERSION >= 0x01000102
 */
int API_EXPORTED libusb_get_port_numbers(libusb_device *dev,
		uint8_t* port_numbers, int port_numbers_len) {

	int i = port_numbers_len;
	struct libusb_context *ctx = DEVICE_CTX(dev);

	if UNLIKELY(port_numbers_len <= 0)
		return LIBUSB_ERROR_INVALID_PARAM;

	// HCDs can be listed as devices with port #0
	// HCD可以列为端口号为0的设备
	while ((dev) && (dev->port_number != 0)) {
		if (--i < 0) {
			usbi_warn(ctx, "port numbers array is too small");
			return LIBUSB_ERROR_OVERFLOW;
		}
		port_numbers[i] = dev->port_number;
		dev = dev->parent_dev;
	}
	if (i < port_numbers_len)
		memmove(port_numbers, &port_numbers[i], port_numbers_len - i);
	return port_numbers_len - i;
}

/** \ingroup dev
 * Deprecated please use libusb_get_port_numbers instead.
 * 不推荐使用，请改用libusb_get_port_numbers。
 */
int API_EXPORTED libusb_get_port_path(libusb_context *ctx, libusb_device *dev,
		uint8_t* port_numbers, uint8_t port_numbers_len) {
		
	UNUSED(ctx);

	return libusb_get_port_numbers(dev, port_numbers, port_numbers_len);
}

/** \ingroup dev
 * Get the the parent from the specified device.
 * \param dev a device
 * \returns the device parent or NULL if not available
 * You should issue a \ref libusb_get_device_list() before calling this
 * function and make sure that you only access the parent before issuing
 * \ref libusb_free_device_list(). The reason is that libusb currently does
 * not maintain a permanent list of device instances, and therefore can
 * only guarantee that parents are fully instantiated within a 
 * libusb_get_device_list() - libusb_free_device_list() block.
 *
 * 从指定设备获取父级。
 */
DEFAULT_VISIBILITY
libusb_device * LIBUSB_CALL libusb_get_parent(libusb_device *dev) {

	return dev->parent_dev;
}

/** \ingroup dev
 * Get the address of the device on the bus it is connected to.
 * \param dev a device
 * \returns the device address
 *
 * 获取设备在其连接的总线上的地址。
 */
uint8_t API_EXPORTED libusb_get_device_address(libusb_device *dev) {

	return dev->device_address;
}

/** \ingroup dev
 * Get the negotiated connection speed for a device.
 * \param dev a device
 * \returns a \ref libusb_speed code, where LIBUSB_SPEED_UNKNOWN means that
 * the OS doesn't know or doesn't support returning the negotiated speed.
 *
 * 获取设备的协商连接速度。
 */
int API_EXPORTED libusb_get_device_speed(libusb_device *dev) {

	return dev->speed;
}

static const struct libusb_endpoint_descriptor *find_endpoint(
	struct libusb_config_descriptor *config, unsigned char endpoint) {
	
	int iface_idx;
	for (iface_idx = 0; iface_idx < config->bNumInterfaces; iface_idx++) {
		const struct libusb_interface *iface = &config->interface[iface_idx];
		int altsetting_idx;

		for (altsetting_idx = 0; altsetting_idx < iface->num_altsetting;
				altsetting_idx++) {
			const struct libusb_interface_descriptor *altsetting
				= &iface->altsetting[altsetting_idx];
			int ep_idx;

			for (ep_idx = 0; ep_idx < altsetting->bNumEndpoints; ep_idx++) {
				const struct libusb_endpoint_descriptor *ep =
						&altsetting->endpoint[ep_idx];
				if (ep->bEndpointAddress == endpoint)
					return ep;
			}
		}
	}
	return NULL;
}

/** \ingroup dev
 * Convenience function to retrieve the wMaxPacketSize value for a particular
 * endpoint in the active device configuration.
 *
 * This function was originally intended to be of assistance when setting up
 * isochronous transfers, but a design mistake resulted in this function
 * instead. It simply returns the wMaxPacketSize value without considering
 * its contents. If you're dealing with isochronous transfers, you probably
 * want libusb_get_max_iso_packet_size() instead.
 *
 * \param dev a device
 * \param endpoint address of the endpoint in question
 * \returns the wMaxPacketSize value
 * \returns LIBUSB_ERROR_NOT_FOUND if the endpoint does not exist
 * \returns LIBUSB_ERROR_OTHER on other failure
 *
 * 便利功能，用于检索活动设备配置中特定端点的wMaxPacketSize值。
 * 此功能最初是在设置同步传输时提供帮助的，但由于设计错误而导致使用此功能。
 * 它仅返回wMaxPacketSize值，而不考虑其内容。如果要处理同步传输，则可能需要libusb_get_max_iso_packet_size()。
 */
int API_EXPORTED libusb_get_max_packet_size(libusb_device *dev,
		unsigned char endpoint) {

	struct libusb_config_descriptor *config;
	const struct libusb_endpoint_descriptor *ep;
	int r;

	r = libusb_get_active_config_descriptor(dev, &config);
	if (UNLIKELY(r < 0)) {
		usbi_err(DEVICE_CTX(dev),
				"could not retrieve active config descriptor");
		return LIBUSB_ERROR_OTHER;
	}

	ep = find_endpoint(config, endpoint);
	if (UNLIKELY(!ep)) {
		r = LIBUSB_ERROR_NOT_FOUND;
		goto out;
	}

	r = ep->wMaxPacketSize;

out:
	libusb_free_config_descriptor(config);
	return r;
}

/** \ingroup dev
 * Calculate the maximum packet size which a specific endpoint is capable is
 * sending or receiving in the duration of 1 microframe
 *
 * Only the active configuration is examined. The calculation is based on the
 * wMaxPacketSize field in the endpoint descriptor as described in section
 * 9.6.6 in the USB 2.0 specifications.
 *
 * If acting on an isochronous or interrupt endpoint, this function will
 * multiply the value found in bits 0:10 by the number of transactions per
 * microframe (determined by bits 11:12). Otherwise, this function just
 * returns the numeric value found in bits 0:10.
 *
 * This function is useful for setting up isochronous transfers, for example
 * you might pass the return value from this function to
 * libusb_set_iso_packet_lengths() in order to set the length field of every
 * isochronous packet in a transfer.
 *
 * Since v1.0.3.
 *
 * \param dev a device
 * \param endpoint address of the endpoint in question
 * \returns the maximum packet size which can be sent/received on this endpoint
 * \returns LIBUSB_ERROR_NOT_FOUND if the endpoint does not exist
 * \returns LIBUSB_ERROR_OTHER on other failure
 *
 * 计算特定端点能够在1个微帧的持续时间内发送或接收的最大数据包大小。仅检查活动配置。该计算基于端点描述符中的wMaxPacketSize字段，如USB 2.0规范中的9.6.6节所述。
 * 如果作用于同步或中断端点，则此功能会将位0:10中的值乘以每个微帧的事务数（由位11:12确定）。 否则，此函数仅返回在位0:10中找到的数值。
 * 此函数对于设置同步传输很有用，例如，您可以将此函数的返回值传递给libusb_set_iso_packet_lengths()，以设置传输中每个同步数据包的长度字段。
 * 从v1.0.3开始
 */
int API_EXPORTED libusb_get_max_iso_packet_size(libusb_device *dev,
		unsigned char endpoint) {

	struct libusb_config_descriptor *config;
	const struct libusb_endpoint_descriptor *ep;
	enum libusb_transfer_type ep_type;
	uint16_t val;
	int r;

	r = libusb_get_active_config_descriptor(dev, &config);
	if (UNLIKELY(r < 0)) {
		usbi_err(DEVICE_CTX(dev),
				"could not retrieve active config descriptor");
		return LIBUSB_ERROR_OTHER;
	}

	ep = find_endpoint(config, endpoint);
	if (UNLIKELY(!ep)) {
		r = LIBUSB_ERROR_NOT_FOUND;
		goto out;
	}

	val = ep->wMaxPacketSize;
	ep_type = (enum libusb_transfer_type) (ep->bmAttributes & 0x3);

	r = val & 0x07ff;
	if (ep_type == LIBUSB_TRANSFER_TYPE_ISOCHRONOUS
			|| ep_type == LIBUSB_TRANSFER_TYPE_INTERRUPT)
		r *= (1 + ((val >> 11) & 3));

out:
	libusb_free_config_descriptor(config);
	return r;
}

/** \ingroup dev
 * Increment the reference count of a device.
 * 增加设备的引用计数。
 *
 * \param dev the device to reference
 * \returns the same device
 */
DEFAULT_VISIBILITY
libusb_device * LIBUSB_CALL libusb_ref_device(libusb_device *dev) {

	int refcnt;
	usbi_mutex_lock(&dev->lock);
	{
		refcnt = ++dev->refcnt;
	}
	usbi_mutex_unlock(&dev->lock);
//	LOGI("refcnt=%d", refcnt);
	return dev;
}

/** \ingroup dev
 * Decrement the reference count of a device. If the decrement operation
 * causes the reference count to reach zero, the device shall be destroyed.
 * \param dev the device to unreference
 *
 * 减少设备的引用计数。 如果递减操作导致参考计数达到零，则应销毁该设备。
 */
void API_EXPORTED libusb_unref_device(libusb_device *dev) {

	int refcnt;

	if (UNLIKELY(!dev))
		return;

	usbi_mutex_lock(&dev->lock);
	{
		refcnt = --dev->refcnt;
	}
	usbi_mutex_unlock(&dev->lock);
//	LOGI("refcnt=%d", dev->refcnt);

	if (refcnt == 0) {
		usbi_dbg("destroy device %d.%d", dev->bus_number, dev->device_address);

		libusb_unref_device(dev->parent_dev);

		if (usbi_backend->destroy_device)
			usbi_backend->destroy_device(dev);

		if (!libusb_has_capability(LIBUSB_CAP_HAS_HOTPLUG)) {
			/* backend does not support hotplug
			 * 后端不支持热插拔
			 */
			usbi_disconnect_device(dev);
		}

		usbi_mutex_destroy(&dev->lock);
		free(dev);
	}
}

/*
 * Interrupt the iteration of the event handling thread, so that it picks
 * up the new fd.
 *
 * 中断事件处理线程的迭代，以便它获取新的fd。
 */
void usbi_fd_notification(struct libusb_context *ctx) {

	unsigned char dummy = 1;
	ssize_t r;

	if (UNLIKELY(ctx == NULL))
		return;

	/* record that we are messing with poll fds
	 * 记录我们丢失的poll fds
	 */
	usbi_mutex_lock(&ctx->pollfd_modify_lock);
	{
		ctx->pollfd_modify++;
	}
	usbi_mutex_unlock(&ctx->pollfd_modify_lock);

	/* write some data on control pipe to interrupt event handlers
	 * 在控制管道上写入一些数据以中断事件处理程序
	 */
	r = usbi_write(ctx->ctrl_pipe[1], &dummy, sizeof(dummy));
	if (UNLIKELY(r <= 0)) {
		usbi_warn(ctx, "internal signalling write failed");
		usbi_mutex_lock(&ctx->pollfd_modify_lock);
		{
			ctx->pollfd_modify--;
		}
		usbi_mutex_unlock(&ctx->pollfd_modify_lock);
		return;
	}

	/* take event handling lock
	 * 取得事件处理锁
	 */
	libusb_lock_events(ctx);
	{
		/* read the dummy data
		 * 读取虚拟数据
		 */
		r = usbi_read(ctx->ctrl_pipe[0], &dummy, sizeof(dummy));
		if (UNLIKELY(r <= 0))
			usbi_warn(ctx, "internal signalling read failed");

		/* we're done with modifying poll fds
		* 我们已经完成了对poll fds的修改
		 */
		usbi_mutex_lock(&ctx->pollfd_modify_lock);
		{
			ctx->pollfd_modify--;
		}
		usbi_mutex_unlock(&ctx->pollfd_modify_lock);
	}
	/* Release event handling lock and wake up event waiters
	 * 释放事件处理锁定并唤醒事件等待者
	 */
	libusb_unlock_events(ctx);
}

/** \ingroup dev
 * Open a device and obtain a device handle. A handle allows you to perform
 * I/O on the device in question.
 *
 * Internally, this function adds a reference to the device and makes it
 * available to you through libusb_get_device(). This reference is removed
 * during libusb_close().
 *
 * This is a non-blocking function; no requests are sent over the bus.
 *
 * \param dev the device to open
 * \param handle output location for the returned device handle pointer. Only
 * populated when the return code is 0.
 * \returns 0 on success
 * \returns LIBUSB_ERROR_NO_MEM on memory allocation failure
 * \returns LIBUSB_ERROR_ACCESS if the user has insufficient permissions
 * \returns LIBUSB_ERROR_NO_DEVICE if the device has been disconnected
 * \returns another LIBUSB_ERROR code on other failure
 *
 * 打开设备并获取设备句柄。 您可以使用句柄在设备上执行I/O请求。
 * 在内部，此函数添加对设备的引用，并通过libusb_get_device()使您可以使用它。 在libusb_close()期间删除了此引用。
 * 这是一个非阻塞功能； 没有请求通过总线发送。
 */
int API_EXPORTED libusb_open(libusb_device *dev, libusb_device_handle **handle) {

	struct libusb_context *ctx = DEVICE_CTX(dev);
	struct libusb_device_handle *_handle;
	size_t priv_size = usbi_backend->device_handle_priv_size;
	int r;
	usbi_dbg("open (bus/addr)=(%d.%d)", dev->bus_number, dev->device_address);

	if (UNLIKELY(!dev->attached)) {
		return LIBUSB_ERROR_NO_DEVICE;
	}

	_handle = malloc(sizeof(*_handle) + priv_size);
	if (UNLIKELY(!_handle))
		return LIBUSB_ERROR_NO_MEM;

	r = usbi_mutex_init(&_handle->lock, NULL);
	if (UNLIKELY(r)) {
		free(_handle);
		return LIBUSB_ERROR_OTHER;
	}

	_handle->dev = libusb_ref_device(dev);
	_handle->auto_detach_kernel_driver = 0;
	_handle->claimed_interfaces = 0;
	memset(&_handle->os_priv, 0, priv_size);

	r = usbi_backend->open(_handle);
	if (UNLIKELY(r < 0)) {
		usbi_dbg("open %d.%d returns %d", dev->bus_number, dev->device_address, r);
		libusb_unref_device(dev);
		usbi_mutex_destroy(&_handle->lock);
		free(_handle);
		return r;
	}

	usbi_mutex_lock(&ctx->open_devs_lock);
	{
		list_add(&_handle->list, &ctx->open_devs);
	}
	usbi_mutex_unlock(&ctx->open_devs_lock);
	*handle = _handle;

	/* At this point, we want to interrupt any existing event handlers so
	 * that they realise the addition of the new device's poll fd. One
	 * example when this is desirable is if the user is running a separate
	 * dedicated libusb events handling thread, which is running with a long
	 * or infinite timeout. We want to interrupt that iteration of the loop,
	 * so that it picks up the new fd, and then continues.
	 * 在这一点上，我们想中断任何现有的事件处理程序，以便它们实现新设备的poll fd的添加。
	 * 一个理想的例子是用户正在运行一个单独的专用libusb事件处理线程，该线程正在长时间或无限超时下运行。
	 * 我们要中断循环的迭代，以便它拾取新的fd，然后继续。
	 */
	usbi_fd_notification(ctx);

	return LIBUSB_SUCCESS;
}

int API_EXPORTED libusb_set_device_fd(libusb_device *dev, int fd) {

	return usbi_backend->set_device_fd(dev, fd);
}

libusb_device * LIBUSB_CALL libusb_get_device_with_fd(libusb_context *ctx,
	int vid, int pid, const char *serial, int fd, int busnum, int devaddr) {

	ENTER();

	struct libusb_device *device = NULL;
	// android_generate_device内でusbi_alloc_deviceが呼ばれた時に参照カウンタは1
	// 在android_generate_device中调用usbi_alloc_device时，参考计数器为1
	int ret = android_generate_device(ctx, &device, vid, pid, serial, fd, busnum, devaddr);
	if (ret) {
		LOGD("android_generate_device failed:err=%d", ret);
		device = NULL;
	}

	RET(device);
}

/** \ingroup dev
 * Convenience function for finding a device with a particular
 * <tt>idVendor</tt>/<tt>idProduct</tt> combination. This function is intended
 * for those scenarios where you are using libusb to knock up a quick test
 * application - it allows you to avoid calling libusb_get_device_list() and
 * worrying about traversing/freeing the list.
 *
 * This function has limitations and is hence not intended for use in real
 * applications: if multiple devices have the same IDs it will only
 * give you the first one, etc.
 *
 * \param ctx the context to operate on, or NULL for the default context
 * \param vendor_id the idVendor value to search for
 * \param product_id the idProduct value to search for
 * \returns a handle for the first found device, or NULL on error or if the
 * device could not be found.
 *
 * 便捷功能，用于查找具有特定 idVendor/idProduct 组合的设备。
 * 此函数适用于使用libusb调用快速测试应用程序的情况-它使您避免调用libusb_get_device_list()并避免遍历/释放列表的麻烦。
 * 此功能有局限性，因此不适合在实际应用中使用：如果多个设备具有相同的ID，则只会为您提供第一个，依此类推。
 */
DEFAULT_VISIBILITY
libusb_device_handle * LIBUSB_CALL libusb_open_device_with_vid_pid(
		libusb_context *ctx, uint16_t vendor_id, uint16_t product_id) {

	struct libusb_device **devs;
	struct libusb_device *found = NULL;
	struct libusb_device *dev;
	struct libusb_device_handle *handle = NULL;
	size_t i = 0;
	int r;

	if (libusb_get_device_list(ctx, &devs) < 0)
		return NULL;

	while ((dev = devs[i++]) != NULL) {
		struct libusb_device_descriptor desc;
		r = libusb_get_device_descriptor(dev, &desc);
		if (UNLIKELY(r < 0))
			goto out;
		if (desc.idVendor == vendor_id && desc.idProduct == product_id) {
			found = dev;
			break;
		}
	}

	if (found) {
		r = libusb_open(found, &handle);
		if (UNLIKELY(r < 0))
			handle = NULL;
	}

out:
	libusb_free_device_list(devs, 1);
	return handle;
}

static void do_close(struct libusb_context *ctx,
	struct libusb_device_handle *dev_handle) {

	struct usbi_transfer *itransfer;
	struct usbi_transfer *tmp;

	libusb_lock_events(ctx);
	{
		/* remove any transfers in flight that are for this device
		 * 删除此设备正在传输中的所有传输
		 */
		usbi_mutex_lock(&ctx->flying_transfers_lock);
		{
			/* safe iteration because transfers may be being deleted
			 * 安全的迭代，因为传输可能会被删除
			 */
			list_for_each_entry_safe(itransfer, tmp, &ctx->flying_transfers, list, struct usbi_transfer)
			{
				struct libusb_transfer *transfer =
					USBI_TRANSFER_TO_LIBUSB_TRANSFER(itransfer);

				if (transfer->dev_handle != dev_handle)
					continue;

				if (!(itransfer->flags & USBI_TRANSFER_DEVICE_DISAPPEARED)) {
					usbi_err(ctx,
						"Device handle closed while transfer was still being processed, but the device is still connected as far as we know");

					if (itransfer->flags & USBI_TRANSFER_CANCELLING)
						usbi_warn(ctx,
							"A cancellation for an in-flight transfer hasn't completed but closing the device handle");
					else
						usbi_err(ctx,
							"A cancellation hasn't even been scheduled on the transfer for which the device is closing");
				}

				/* remove from the list of in-flight transfers and make sure
				 * we don't accidentally use the device handle in the future
				 * (or that such accesses will be easily caught and identified as a crash)
				 * 从机上传输列表中删除，并确保将来我们不会意外使用设备句柄（否则此类访问将很容易被捕获并识别为崩溃）
				 */
				usbi_mutex_lock(&itransfer->lock);
				{
					list_del(&itransfer->list);
					transfer->dev_handle = NULL;
				}
				usbi_mutex_unlock(&itransfer->lock);

				/* it is up to the user to free up the actual transfer struct.  this is
				 * just making sure that we don't attempt to process the transfer after
				 * the device handle is invalid
				 * 用户可以自行释放实际的传输结构。 这只是确保我们不会在设备句柄无效之后尝试处理传输
				 */
				usbi_dbg(
						"Removed transfer %p from the in-flight list because device handle %p closed",
						transfer, dev_handle);
			}
		}
		usbi_mutex_unlock(&ctx->flying_transfers_lock);
	}
	libusb_unlock_events(ctx);

	usbi_mutex_lock(&ctx->open_devs_lock);
	{
		list_del(&dev_handle->list);
	}
	usbi_mutex_unlock(&ctx->open_devs_lock);

	usbi_backend->close(dev_handle);
	libusb_unref_device(dev_handle->dev);
	usbi_mutex_destroy(&dev_handle->lock);
	free(dev_handle);
}

/** \ingroup dev
 * Close a device handle. Should be called on all open handles before your
 * application exits.
 *
 * Internally, this function destroys the reference that was added by
 * libusb_open() on the given device.
 *
 * This is a non-blocking function; no requests are sent over the bus.
 *
 * \param dev_handle the handle to close
 *
 * 关闭设备手柄。 在应用程序退出之前，应在所有打开的句柄上调用它。
 * 在内部，此函数将破坏由libusb_open()在给定设备上添加的引用。
 * 这是一个非阻塞功能； 没有请求通过总线发送。
 */
void API_EXPORTED libusb_close(libusb_device_handle *dev_handle) {

	struct libusb_context *ctx;
	unsigned char dummy = 1;
	ssize_t r;

	if (UNLIKELY(!dev_handle))
		return;
	usbi_dbg("");

	ctx = HANDLE_CTX(dev_handle);

	/* Similarly to libusb_open(), we want to interrupt all event handlers
	 * at this point. More importantly, we want to perform the actual close of
	 * the device while holding the event handling lock (preventing any other
	 * thread from doing event handling) because we will be removing a file
	 * descriptor from the polling loop.
	 *
	 * 与libusb_open()类似，我们现在要中断所有事件处理程序。
	 * 更重要的是，我们要在保持事件处理锁定的同时执行设备的实际关闭操作（防止任何其他线程进行事件处理），因为我们将从轮询循环中删除文件描述符。
	 */

	/* record that we are messing with poll fds
	 * 记录我们丢失的 poll fds
	 */
	usbi_mutex_lock(&ctx->pollfd_modify_lock);
	{
		ctx->pollfd_modify++;
	}
	usbi_mutex_unlock(&ctx->pollfd_modify_lock);

	/* write some data on control pipe to interrupt event handlers
	 * 在控制管道上写入一些数据以中断事件处理程序
	 */
	r = usbi_write(ctx->ctrl_pipe[1], &dummy, sizeof(dummy));
	if (UNLIKELY(r <= 0)) {
		usbi_warn(ctx, "internal signalling write failed, closing anyway");
		do_close(ctx, dev_handle);
		usbi_mutex_lock(&ctx->pollfd_modify_lock);
		{
			ctx->pollfd_modify--;
		}
		usbi_mutex_unlock(&ctx->pollfd_modify_lock);
		return;
	}

	/* take event handling lock
	 * 取得事件处理锁
	 */
	libusb_lock_events(ctx);	// XXX crash
	{
		/* read the dummy data
		 * 读取虚拟数据
		 */
		r = usbi_read(ctx->ctrl_pipe[0], &dummy, sizeof(dummy));	// XXX crash
		if (UNLIKELY(r <= 0)) {
			usbi_warn(ctx, "internal signalling read failed, closing anyway");
		}

		/* Close the device
		 * 关闭设备
		 */
		do_close(ctx, dev_handle);	// XXX this function internally call libusb_lock_events/libusb_unlock_events while libusb_lock_events is already called and will hang-up on some OS?
									// 这个函数在内部调用libusb_lock_events/libusb_unlock_events，而libusb_lock_events已经被调用并且会在某些OS上挂断？

		/* we're done with modifying poll fds
		 * 我们已经完成了对poll fds的修改
		 */
		usbi_mutex_lock(&ctx->pollfd_modify_lock);
		{
			ctx->pollfd_modify--;
		}
		usbi_mutex_unlock(&ctx->pollfd_modify_lock);
	}
	/* Release event handling lock and wake up event waiters
	 * 释放事件处理锁定并唤醒事件等待者
	 */
	libusb_unlock_events(ctx);
}

/** \ingroup dev
 * Get the underlying device for a handle. This function does not modify
 * the reference count of the returned device, so do not feel compelled to
 * unreference it when you are done.
 * \param dev_handle a device handle
 * \returns the underlying device
 *
 * 获取句柄的基础设备。 此功能不会修改返回的设备的引用计数，因此在完成操​​作时不要感到被迫取消引用。
 */
DEFAULT_VISIBILITY
libusb_device * LIBUSB_CALL libusb_get_device(libusb_device_handle *dev_handle) {

	return dev_handle->dev;
}

/** \ingroup dev
 * Determine the bConfigurationValue of the currently active configuration.
 *
 * You could formulate your own control request to obtain this information,
 * but this function has the advantage that it may be able to retrieve the
 * information from operating system caches (no I/O involved).
 *
 * If the OS does not cache this information, then this function will block
 * while a control transfer is submitted to retrieve the information.
 *
 * This function will return a value of 0 in the <tt>config</tt> output
 * parameter if the device is in unconfigured state.
 *
 * \param dev a device handle
 * \param config output location for the bConfigurationValue of the active
 * configuration (only valid for return code 0)
 * \returns 0 on success
 * \returns LIBUSB_ERROR_NO_DEVICE if the device has been disconnected
 * \returns another LIBUSB_ERROR code on other failure
 *
 * 确定当前活动配置的bConfigurationValue。
 * 您可以制定自己的控制请求来获取此信息，但是此功能的优点是它可以从操作系统缓存中检索信息（不涉及I/O）。
 * 如果操作系统没有缓存该信息，则在提交控制权以检索信息时该功能将阻塞。
 * 如果设备处于未配置状态，此函数将在 config 输出参数中返回值0。
 */
int API_EXPORTED libusb_get_configuration(libusb_device_handle *dev,
		int *config) {
	int r = LIBUSB_ERROR_NOT_SUPPORTED;

	usbi_dbg("");
	if (usbi_backend->get_configuration)
		r = usbi_backend->get_configuration(dev, config);

	if (r == LIBUSB_ERROR_NOT_SUPPORTED) {
		uint8_t tmp = 0;
		usbi_dbg("falling back to control message");
		r = libusb_control_transfer(dev, LIBUSB_ENDPOINT_IN,
				LIBUSB_REQUEST_GET_CONFIGURATION, 0, 0, &tmp, 1, 1000);
		if (r == 0) {
			usbi_err(HANDLE_CTX(dev), "zero bytes returned in ctrl transfer?");
			r = LIBUSB_ERROR_IO;
		} else if (r == 1) {
			r = 0;
			*config = tmp;
		} else {
			usbi_dbg("control failed, error %d", r);
		}
	}

	if (r == 0)
		usbi_dbg("active config %d", *config);

	return r;
}

/** \ingroup dev
 * Set the active configuration for a device.
 *
 * The operating system may or may not have already set an active
 * configuration on the device. It is up to your application to ensure the
 * correct configuration is selected before you attempt to claim interfaces
 * and perform other operations.
 *
 * If you call this function on a device already configured with the selected
 * configuration, then this function will act as a lightweight device reset:
 * it will issue a SET_CONFIGURATION request using the current configuration,
 * causing most USB-related device state to be reset (altsetting reset to zero,
 * endpoint halts cleared, toggles reset).
 *
 * You cannot change/reset configuration if your application has claimed
 * interfaces. It is advised to set the desired configuration before claiming
 * interfaces.
 *
 * Alternatively you can call libusb_release_interface() first. Note if you
 * do things this way you must ensure that auto_detach_kernel_driver for
 * <tt>dev</tt> is 0, otherwise the kernel driver will be re-attached when you
 * release the interface(s).
 *
 * You cannot change/reset configuration if other applications or drivers have
 * claimed interfaces.
 *
 * A configuration value of -1 will put the device in unconfigured state.
 * The USB specifications state that a configuration value of 0 does this,
 * however buggy devices exist which actually have a configuration 0.
 *
 * You should always use this function rather than formulating your own
 * SET_CONFIGURATION control request. This is because the underlying operating
 * system needs to know when such changes happen.
 *
 * This is a blocking function.
 *
 * \param dev a device handle
 * \param configuration the bConfigurationValue of the configuration you
 * wish to activate, or -1 if you wish to put the device in unconfigured state
 * \returns 0 on success
 * \returns LIBUSB_ERROR_NOT_FOUND if the requested configuration does not exist
 * \returns LIBUSB_ERROR_BUSY if interfaces are currently claimed
 * \returns LIBUSB_ERROR_NO_DEVICE if the device has been disconnected
 * \returns another LIBUSB_ERROR code on other failure
 * \see libusb_set_auto_detach_kernel_driver()
 *
 * 设置设备的活动配置。
 * 操作系统可能已经或可能尚未在设备上设置活动配置。 在尝试声明接口并执行其他操作之前，由应用程序确定是否选择了正确的配置。
 * 如果在已经配置了所选配置的设备上调用此功能，则此功能将充当轻型设备重置：它将使用当前配置发出SET_CONFIGURATION请求，
 * 从而导致大多数与USB相关的设备状态被重置（设置为零，清除端点停止，切换复位）。
 * 如果您的应用程序声明了接口，则无法更改/重置配置。 建议在声明接口之前设置所需的配置。
 * 或者，您可以先调用libusb_release_interface()。 请注意，如果以这种方式进行操作，则必须确保 dev 的auto_detach_kernel_driver为0，否则在释放接口时将重新附加内核驱动程序。
 * 如果其他应用程序或驱动程序声明了接口，则无法更改/重置配置。
 * 配置值为-1将使设备处于未配置状态。  USB规范指出，配置值为0可以做到这一点，但实际上存在配置为0的有问题设备。
 * 您应该始终使用此功能，而不是制定自己的SET_CONFIGURATION控制请求。 这是因为底层操作系统需要知道何时发生此类更改。
 * 这是一个阻止功能。
 */
int API_EXPORTED libusb_set_configuration(libusb_device_handle *dev,
		int configuration) {

	usbi_dbg("configuration %d", configuration);
	return usbi_backend->set_configuration(dev, configuration);
}

/** \ingroup dev
 * Claim an interface on a given device handle. You must claim the interface
 * you wish to use before you can perform I/O on any of its endpoints.
 *
 * It is legal to attempt to claim an already-claimed interface, in which
 * case libusb just returns 0 without doing anything.
 *
 * If auto_detach_kernel_driver is set to 1 for <tt>dev</tt>, the kernel driver
 * will be detached if necessary, on failure the detach error is returned.
 *
 * Claiming of interfaces is a purely logical operation; it does not cause
 * any requests to be sent over the bus. Interface claiming is used to
 * instruct the underlying operating system that your application wishes
 * to take ownership of the interface.
 *
 * This is a non-blocking function.
 *
 * \param dev a device handle
 * \param interface_number the <tt>bInterfaceNumber</tt> of the interface you
 * wish to claim
 * \returns 0 on success
 * \returns LIBUSB_ERROR_NOT_FOUND if the requested interface does not exist
 * \returns LIBUSB_ERROR_BUSY if another program or driver has claimed the
 * interface
 * \returns LIBUSB_ERROR_NO_DEVICE if the device has been disconnected
 * \returns a LIBUSB_ERROR code on other failure
 * \see libusb_set_auto_detach_kernel_driver()
 *
 * 声明给定设备句柄上的接口。 您必须先声明要使用的接口，然后才能在其任何端点上执行I/O。
 * 尝试声明一个已经声明的接口是合法的，在这种情况下，libusb 仅返回0而没有执行任何操作。
 * 如果将 dev 的 auto_detach_kernel_driver 设置为1，则将在必要时分离内核驱动程序，如果失败，则返回分离错误。
 * 声明接口是纯粹的逻辑操作； 它不会导致任何请求通过总线发送。 接口声明用于指示基础操作系统您的应用程序希望获得该接口的所有权。
 * 这是一个非阻塞功能。
 */
int API_EXPORTED libusb_claim_interface(libusb_device_handle *dev,
		int interface_number) {

	ENTER();

	int r = LIBUSB_SUCCESS;

	usbi_dbg("interface %d", interface_number);
	LOGD("interface %d", interface_number);

	if (interface_number >= USB_MAXINTERFACES) {
		RETURN(LIBUSB_ERROR_INVALID_PARAM, int);
	}

	if (UNLIKELY(!dev->dev->attached)) {
		RETURN(LIBUSB_ERROR_NO_DEVICE, int);
	}

	usbi_mutex_lock(&dev->lock);
	if (!(dev->claimed_interfaces & (1 << interface_number))) {
		r = usbi_backend->claim_interface(dev, interface_number);
		if (r == LIBUSB_ERROR_BUSY) {
			// EBUSYが返ってきた時はたぶんカーネルドライバーがアタッチされているから
			// デタッチ要求してから再度claimしてみる
			// 当EBUSY回来时，也许内核驱动程序已附加
			// 请求分队，然后回收
			LOGV("request detach kernel driver and retry claim interface");
			r = usbi_backend->release_interface(dev, interface_number);
			libusb_detach_kernel_driver(dev, interface_number);
			if (!r) {
				r = usbi_backend->claim_interface(dev, interface_number);
			}
		}
		if (!r) {
			dev->claimed_interfaces |= 1 << interface_number;
		}
	} else {
		LOGV("already claimed");
	}
	usbi_mutex_unlock(&dev->lock);

	RETURN(r, int);
}

/** \ingroup dev
 * Release an interface previously claimed with libusb_claim_interface(). You
 * should release all claimed interfaces before closing a device handle.
 *
 * This is a blocking function. A SET_INTERFACE control request will be sent
 * to the device, resetting interface state to the first alternate setting.
 *
 * If auto_detach_kernel_driver is set to 1 for <tt>dev</tt>, the kernel
 * driver will be re-attached after releasing the interface.
 *
 * \param dev a device handle
 * \param interface_number the <tt>bInterfaceNumber</tt> of the
 * previously-claimed interface
 * \returns 0 on success
 * \returns LIBUSB_ERROR_NOT_FOUND if the interface was not claimed
 * \returns LIBUSB_ERROR_NO_DEVICE if the device has been disconnected
 * \returns another LIBUSB_ERROR code on other failure
 * \see libusb_set_auto_detach_kernel_driver()
 *
 * 释放先前使用libusb_claim_interface()声明的接口。 您应该在关闭设备句柄之前释放所有声明的接口。
 * 这是一个阻止功能。SET_INTERFACE控制请求将发送到设备，将接口状态重置为第一个备用设置。
 * 如果将 dev 的auto_detach_kernel_driver设置为1，则在释放接口后将重新附加内核驱动程序。
 */
int API_EXPORTED libusb_release_interface(libusb_device_handle *dev,
		int interface_number) {

	ENTER();

	int r;

	LOGD("interface %d", interface_number);
	usbi_dbg("interface %d", interface_number);
	if (UNLIKELY(interface_number >= USB_MAXINTERFACES))
		RETURN(LIBUSB_ERROR_INVALID_PARAM, int);

	usbi_mutex_lock(&dev->lock);
	{
		if (dev->claimed_interfaces & (1 << interface_number)) {
			r = usbi_backend->release_interface(dev, interface_number);
			if (!r) {
				LOGV("released");
				dev->claimed_interfaces &= ~(1 << interface_number);
			}
		} else {
			// already released
			// 已经释放
			r = LIBUSB_ERROR_NOT_FOUND;
		}
	}
	usbi_mutex_unlock(&dev->lock);

	RETURN(r, int);
}

/** \ingroup dev
 * Activate an alternate setting for an interface. The interface must have
 * been previously claimed with libusb_claim_interface().
 *
 * You should always use this function rather than formulating your own
 * SET_INTERFACE control request. This is because the underlying operating
 * system needs to know when such changes happen.
 *
 * This is a blocking function.
 *
 * \param dev a device handle
 * \param interface_number the <tt>bInterfaceNumber</tt> of the
 * previously-claimed interface
 * \param alternate_setting the <tt>bAlternateSetting</tt> of the alternate
 * setting to activate
 * \returns 0 on success
 * \returns LIBUSB_ERROR_NOT_FOUND if the interface was not claimed, or the
 * requested alternate setting does not exist
 * \returns LIBUSB_ERROR_NO_DEVICE if the device has been disconnected
 * \returns another LIBUSB_ERROR code on other failure
 *
 * 激活接口的备用设置。 该接口必须先前已通过libusb_claim_interface()声明了所有权。
 * 您应该始终使用此功能，而不是制定自己的SET_INTERFACE控制请求。 这是因为底层操作系统需要知道何时发生此类更改。
 * 这是一个阻塞功能。
 */
int API_EXPORTED libusb_set_interface_alt_setting(libusb_device_handle *dev,
		int interface_number, int alternate_setting) {

	usbi_dbg("interface %d altsetting %d", interface_number, alternate_setting);
	if (interface_number >= USB_MAXINTERFACES) {
	    // 不合法参数
	    return LIBUSB_ERROR_INVALID_PARAM;
	}

	usbi_mutex_lock(&dev->lock);
	{
		if (UNLIKELY(!dev->dev->attached)) {
			usbi_mutex_unlock(&dev->lock);
			// 没有设备
			return LIBUSB_ERROR_NO_DEVICE;
		}

		if (UNLIKELY(!(dev->claimed_interfaces & (1 << interface_number)))) {
			usbi_mutex_unlock(&dev->lock);
			// 接口不存在
			return LIBUSB_ERROR_NOT_FOUND;
		}
	}
	usbi_mutex_unlock(&dev->lock);

	return usbi_backend->set_interface_altsetting(dev, interface_number,
			alternate_setting);
}

/** \ingroup dev
 * Clear the halt/stall condition for an endpoint. Endpoints with halt status
 * are unable to receive or transmit data until the halt condition is stalled.
 *
 * You should cancel all pending transfers before attempting to clear the halt
 * condition.
 *
 * This is a blocking function.
 *
 * \param dev a device handle
 * \param endpoint the endpoint to clear halt status
 * \returns 0 on success
 * \returns LIBUSB_ERROR_NOT_FOUND if the endpoint does not exist
 * \returns LIBUSB_ERROR_NO_DEVICE if the device has been disconnected
 * \returns another LIBUSB_ERROR code on other failure
 *
 * 清除端点的暂停/停止条件。 具有暂停状态的端点在暂停条件停止之前无法接收或传输数据。
 * 在尝试清除暂停条件之前，您应该取消所有挂起的传输。
 * 这是一个阻止功能。
 */
int API_EXPORTED libusb_clear_halt(libusb_device_handle *dev,
		unsigned char endpoint) {

	usbi_dbg("endpoint %x", endpoint);
	if (UNLIKELY(!dev->dev->attached))
		return LIBUSB_ERROR_NO_DEVICE;

	return usbi_backend->clear_halt(dev, endpoint);
}

/** \ingroup dev
 * Perform a USB port reset to reinitialize a device. The system will attempt
 * to restore the previous configuration and alternate settings after the
 * reset has completed.
 *
 * If the reset fails, the descriptors change, or the previous state cannot be
 * restored, the device will appear to be disconnected and reconnected. This
 * means that the device handle is no longer valid (you should close it) and
 * rediscover the device. A return code of LIBUSB_ERROR_NOT_FOUND indicates
 * when this is the case.
 *
 * This is a blocking function which usually incurs a noticeable delay.
 *
 * \param dev a handle of the device to reset
 * \returns 0 on success
 * \returns LIBUSB_ERROR_NOT_FOUND if re-enumeration is required, or if the
 * device has been disconnected
 * \returns another LIBUSB_ERROR code on other failure
 *
 * 执行USB端口重置以重新初始化设备。 重置完成后，系统将尝试恢复先前的配置和备用设置。
 * 如果重置失败，描述符更改或无法恢复以前的状态，则设备似乎已断开连接并重新连接。这意味着设备句柄不再有效（您应将其关闭）并重新发现设备。 出现这种情况时，将返回LIBUSB_ERROR_NOT_FOUND的返回码。
 * 这是一个阻止功能，通常会引起明显的延迟。
 */
int API_EXPORTED libusb_reset_device(libusb_device_handle *dev) {

	usbi_dbg("");
	if (UNLIKELY(!dev->dev->attached))
		return LIBUSB_ERROR_NO_DEVICE;

	return usbi_backend->reset_device(dev);
}

/** \ingroup asyncio
 * Allocate up to num_streams usb bulk streams on the specified endpoints. This
 * function takes an array of endpoints rather then a single endpoint because
 * some protocols require that endpoints are setup with similar stream ids.
 * All endpoints passed in must belong to the same interface.
 *
 * Note this function may return less streams then requested. Also note that the
 * same number of streams are allocated for each endpoint in the endpoint array.
 *
 * Stream id 0 is reserved, and should not be used to communicate with devices.
 * If libusb_alloc_streams() returns with a value of N, you may use stream ids
 * 1 to N.
 *
 * Since version 1.0.19, \ref LIBUSB_API_VERSION >= 0x01000103
 *
 * \param dev a device handle
 * \param num_streams number of streams to try to allocate
 * \param endpoints array of endpoints to allocate streams on
 * \param num_endpoints length of the endpoints array
 * \returns number of streams allocated, or a LIBUSB_ERROR code on failure
 *
 * 在指定的端点上最多分配num_streams个USB块流。 此功能采用一组端点而不是单个端点，因为某些协议要求使用类似的流ID设置端点。 传入的所有端点必须属于同一接口。
 * 请注意，此函数返回的流可能少于请求的流。 还要注意，为端点数组中的每个端点分配了相同数量的流。
 * 流ID 0是保留的，不应用于与设备通信。 如果libusb_alloc_streams() 返回的值为N，则可以使用流ID 1到N。
 * 从1.0.19版本开始，LIBUSB_API_VERSION >= 0x01000103
 */
int API_EXPORTED libusb_alloc_streams(libusb_device_handle *dev,
	uint32_t num_streams, unsigned char *endpoints, int num_endpoints)
{
	usbi_dbg("streams %u eps %d", (unsigned) num_streams, num_endpoints);

	if UNLIKELY(!dev->dev->attached)
		return LIBUSB_ERROR_NO_DEVICE;

	if LIKELY(usbi_backend->alloc_streams)
		return usbi_backend->alloc_streams(dev, num_streams, endpoints,
						   num_endpoints);
	else
		return LIBUSB_ERROR_NOT_SUPPORTED;
}

/** \ingroup asyncio
 * Free usb bulk streams allocated with libusb_alloc_streams().
 *
 * Note streams are automatically free-ed when releasing an interface.
 *
 * Since version 1.0.19, \ref LIBUSB_API_VERSION >= 0x01000103
 *
 * \param dev a device handle
 * \param endpoints array of endpoints to free streams on
 * \param num_endpoints length of the endpoints array
 * \returns LIBUSB_SUCCESS, or a LIBUSB_ERROR code on failure
 *
 * 用libusb_alloc_streams()分配的免费USB块流。
 * 释放接口时，注释流会自动释放。
 * 自版本1.0.19起，LIBUSB_API_VERSION >= 0x01000103
 */
int API_EXPORTED libusb_free_streams(libusb_device_handle *dev,
	unsigned char *endpoints, int num_endpoints)
{
	ENTER();

	LOGD("eps %d", num_endpoints);
	usbi_dbg("eps %d", num_endpoints);

	if UNLIKELY(!dev->dev->attached) {
		RETURN(LIBUSB_ERROR_NO_DEVICE, int);
	}

	if LIKELY(usbi_backend->free_streams) {
		RETURN(usbi_backend->free_streams(dev, endpoints, num_endpoints), int);
	} else {
		RETURN(LIBUSB_ERROR_NOT_SUPPORTED, int);
	}
}

/** \ingroup dev
 * Determine if a kernel driver is active on an interface. If a kernel driver
 * is active, you cannot claim the interface, and libusb will be unable to
 * perform I/O.
 *
 * This functionality is not available on Windows.
 *
 * \param dev a device handle
 * \param interface_number the interface to check
 * \returns 0 if no kernel driver is active
 * \returns 1 if a kernel driver is active
 * \returns LIBUSB_ERROR_NO_DEVICE if the device has been disconnected
 * \returns LIBUSB_ERROR_NOT_SUPPORTED on platforms where the functionality
 * is not available
 * \returns another LIBUSB_ERROR code on other failure
 * \see libusb_detach_kernel_driver()
 *
 * 确定内核驱动程序在接口上是否处于活动状态。 如果内核驱动程序处于活动状态，则无法声明该接口，并且libusb将无法执行I/O。
 * 此功能在Windows上不可用。
 */
int API_EXPORTED libusb_kernel_driver_active(libusb_device_handle *dev,
		int interface_number) {

	ENTER();

	LOGD("interface %d", interface_number);
	usbi_dbg("interface %d", interface_number);

	if (UNLIKELY(!dev->dev->attached)) {
		RETURN(LIBUSB_ERROR_NO_DEVICE, int);
	}

	if LIKELY(usbi_backend->kernel_driver_active) {
		RETURN(usbi_backend->kernel_driver_active(dev, interface_number), int);
	} else {
		RETURN(LIBUSB_ERROR_NOT_SUPPORTED, int);
	}
}

/** \ingroup dev
 * Detach a kernel driver from an interface. If successful, you will then be
 * able to claim the interface and perform I/O.
 *
 * This functionality is not available on Darwin or Windows.
 *
 * Note that libusb itself also talks to the device through a special kernel
 * driver, if this driver is already attached to the device, this call will
 * not detach it and return LIBUSB_ERROR_NOT_FOUND.
 *
 * \param dev a device handle
 * \param interface_number the interface to detach the driver from
 * \returns 0 on success
 * \returns LIBUSB_ERROR_NOT_FOUND if no kernel driver was active
 * \returns LIBUSB_ERROR_INVALID_PARAM if the interface does not exist
 * \returns LIBUSB_ERROR_NO_DEVICE if the device has been disconnected
 * \returns LIBUSB_ERROR_NOT_SUPPORTED on platforms where the functionality
 * is not available
 * \returns another LIBUSB_ERROR code on other failure
 * \see libusb_kernel_driver_active()
 *
 * 从接口分离内核驱动程序。 如果成功，则可以声明该接口并执行I/O。
 * 此功能在Darwin或Windows上不可用。
 * 请注意，libusb本身也通过特殊的内核驱动程序与该设备通信，如果该驱动程序已连接到该设备，则此调用不会分离该驱动程序并返回LIBUSB_ERROR_NOT_FOUND。
 */
int API_EXPORTED libusb_detach_kernel_driver(libusb_device_handle *dev,
		int interface_number) {

	usbi_dbg("interface %d", interface_number);

	if (UNLIKELY(!dev->dev->attached))
		return LIBUSB_ERROR_NO_DEVICE;

	if (LIKELY(usbi_backend->detach_kernel_driver))
		return usbi_backend->detach_kernel_driver(dev, interface_number);
	else
		return LIBUSB_ERROR_NOT_SUPPORTED;
}

/** \ingroup dev
 * Re-attach an interface's kernel driver, which was previously detached
 * using libusb_detach_kernel_driver(). This call is only effective on
 * Linux and returns LIBUSB_ERROR_NOT_SUPPORTED on all other platforms.
 *
 * This functionality is not available on Darwin or Windows.
 *
 * \param dev a device handle
 * \param interface_number the interface to attach the driver from
 * \returns 0 on success
 * \returns LIBUSB_ERROR_NOT_FOUND if no kernel driver was active
 * \returns LIBUSB_ERROR_INVALID_PARAM if the interface does not exist
 * \returns LIBUSB_ERROR_NO_DEVICE if the device has been disconnected
 * \returns LIBUSB_ERROR_NOT_SUPPORTED on platforms where the functionality
 * is not available
 * \returns LIBUSB_ERROR_BUSY if the driver cannot be attached because the
 * interface is claimed by a program or driver
 * \returns another LIBUSB_ERROR code on other failure
 * \see libusb_kernel_driver_active()
 *
 * 重新连接接口的内核驱动程序，该驱动程序以前是使用libusb_detach_kernel_driver()分离的。 该调用仅在Linux上有效，并在所有其他平台上返回LIBUSB_ERROR_NOT_SUPPORTED。
 * 此功能在Darwin或Windows上不可用。
 */
int API_EXPORTED libusb_attach_kernel_driver(libusb_device_handle *dev,
		int interface_number) {

	ENTER();

	LOGD("interface %d", interface_number);
	usbi_dbg("interface %d", interface_number);

	if (UNLIKELY(!dev->dev->attached)) {
		RETURN(LIBUSB_ERROR_NO_DEVICE, int);
	}

	if (LIKELY(usbi_backend->attach_kernel_driver)) {
		RETURN(usbi_backend->attach_kernel_driver(dev, interface_number), int);
	} else {
		RETURN(LIBUSB_ERROR_NOT_SUPPORTED, int);
	}
}

/** \ingroup dev
 * Enable/disable libusb's automatic kernel driver detachment. When this is
 * enabled libusb will automatically detach the kernel driver on an interface
 * when claiming the interface, and attach it when releasing the interface.
 *
 * Automatic kernel driver detachment is disabled on newly opened device
 * handles by default.
 *
 * On platforms which do not have LIBUSB_CAP_SUPPORTS_DETACH_KERNEL_DRIVER
 * this function will return LIBUSB_ERROR_NOT_SUPPORTED, and libusb will
 * continue as if this function was never called.
 *
 * \param dev a device handle
 * \param enable whether to enable or disable auto kernel driver detachment
 *
 * \returns LIBUSB_SUCCESS on success
 * \returns LIBUSB_ERROR_NOT_SUPPORTED on platforms where the functionality
 * is not available
 * \see libusb_claim_interface()
 * \see libusb_release_interface()
 * \see libusb_set_configuration()
 *
 * 启用/禁用libusb的自动内核驱动程序分离。 启用此选项后，libusb将在声明接口时自动分离接口上的内核驱动程序，并在释放接口时将其附加。
 * 默认情况下，在新打开的设备句柄上禁用自动内核驱动程序分离。
 * 在没有LIBUSB_CAP_SUPPORTS_DETACH_KERNEL_DRIVER的平台上，此函数将返回LIBUSB_ERROR_NOT_SUPPORTED，并且libusb将继续运行，就像从未调用此函数一样。
 */
int API_EXPORTED libusb_set_auto_detach_kernel_driver(libusb_device_handle *dev,
		int enable) {

	ENTER();

	LOGD("enable=%d", enable);
	if (!(usbi_backend->caps & USBI_CAP_SUPPORTS_DETACH_KERNEL_DRIVER)) {
		LOGD("does not support detach kernel driver");
		RETURN(LIBUSB_ERROR_NOT_SUPPORTED, int);
	}

	dev->auto_detach_kernel_driver = enable;
	RETURN(LIBUSB_SUCCESS, int);
}

/** \ingroup lib
 * Set log message verbosity.
 *
 * The default level is LIBUSB_LOG_LEVEL_NONE, which means no messages are ever
 * printed. If you choose to increase the message verbosity level, ensure
 * that your application does not close the stdout/stderr file descriptors.
 *
 * You are advised to use level LIBUSB_LOG_LEVEL_WARNING. libusb is conservative
 * with its message logging and most of the time, will only log messages that
 * explain error conditions and other oddities. This will help you debug
 * your software.
 *
 * If the LIBUSB_DEBUG environment variable was set when libusb was
 * initialized, this function does nothing: the message verbosity is fixed
 * to the value in the environment variable.
 *
 * If libusb was compiled without any message logging, this function does
 * nothing: you'll never get any messages.
 *
 * If libusb was compiled with verbose debug message logging, this function
 * does nothing: you'll always get messages from all levels.
 *
 * \param ctx the context to operate on, or NULL for the default context
 * \param level debug level to set
 *
 * 设置日志消息的详细程度。
 * 默认级别为LIBUSB_LOG_LEVEL_NONE，这意味着不会打印任何消息。 如果选择增加消息的详细程度，请确保您的应用程序未关闭stdout/stderr文件描述符。
 * 建议您使用级别LIBUSB_LOG_LEVEL_WARNING。  libusb的消息记录很保守，在大多数情况下，只会记录说明错误情况和其他异常情况的消息。 这将帮助您调试软件。
 * 如果在初始化libusb时设置了LIBUSB_DEBUG环境变量，则此函数不执行任何操作：消息详细程度固定为环境变量中的值。
 * 如果libusb编译时未记录任何消息，则此功能不执行任何操作：您将永远不会收到任何消息。
 * 如果libusb是用详细的调试消息日志记录编译的，则此函数将不执行任何操作：您将始终从各个级别获取消息。
 */
void API_EXPORTED libusb_set_debug(libusb_context *ctx, int level) {

	USBI_GET_CONTEXT(ctx);
	if (!ctx->debug_fixed)
		ctx->debug = level;
}

int API_EXPORTED libusb_init2(libusb_context **context, const char *usbfs) {
	ENTER();
	struct libusb_device *dev, *next;
	char *dbg = getenv("LIBUSB_DEBUG");
	struct libusb_context *ctx;
	static int first_init = 1;
	int r = 0;

	usbi_mutex_static_lock(&default_context_lock);
	{
		if (!timestamp_origin.tv_sec) {
			usbi_gettimeofday(&timestamp_origin, NULL);
		}

		if (!context && usbi_default_context) {
			usbi_dbg("reusing default context");
			LOGI("reusing default context");
			default_context_refcnt++;
			usbi_mutex_static_unlock(&default_context_lock);
			return LIBUSB_SUCCESS;
		}

		ctx = calloc(1, sizeof(*ctx));
		if (UNLIKELY(!ctx)) {
			r = LIBUSB_ERROR_NO_MEM;
			goto err_unlock;
		}

#ifdef ENABLE_DEBUG_LOGGING
		ctx->debug = LIBUSB_LOG_LEVEL_DEBUG;
#endif

		if (UNLIKELY(dbg)) {
			ctx->debug = atoi(dbg);
			if (ctx->debug)
				ctx->debug_fixed = 1;
		}

		/* default context should be initialized before calling usbi_dbg
		 * 调用usbi_dbg之前，应初始化默认上下文
		 */
		if (!usbi_default_context) {
			usbi_default_context = ctx;
			default_context_refcnt++;
			usbi_dbg("created default context");
		}

		LOGI("libusb v%d.%d.%d.%d", libusb_version_internal.major, libusb_version_internal.minor,
			libusb_version_internal.micro, libusb_version_internal.nano);

		usbi_dbg("libusb v%d.%d.%d.%d", libusb_version_internal.major, libusb_version_internal.minor,
			libusb_version_internal.micro, libusb_version_internal.nano);

		usbi_mutex_init(&ctx->usb_devs_lock, NULL);
		usbi_mutex_init(&ctx->open_devs_lock, NULL);
		usbi_mutex_init(&ctx->hotplug_cbs_lock, NULL);
		list_init(&ctx->usb_devs);
		list_init(&ctx->open_devs);
		list_init(&ctx->hotplug_cbs);

		usbi_mutex_static_lock(&active_contexts_lock);
		{
			if (first_init) {
				first_init = 0;
				list_init(&active_contexts_list);
			}
			list_add(&ctx->list, &active_contexts_list);
		}
		usbi_mutex_static_unlock(&active_contexts_lock);

		if (LIKELY(usbfs && strlen(usbfs) > 0)) {
			LOGD("call usbi_backend->init2");
			if (usbi_backend->init2) {
				r = usbi_backend->init2(ctx, usbfs);
				if (UNLIKELY(r)) {
					LOGE("failed to call usbi_backend->init2, err=%d", r);
					goto err_free_ctx;
				}
			} else {
				LOGE("has no usbi_backend->init2");
				goto err_free_ctx;
			}
		} else {
			LOGD("call usbi_backend->init");
			if (usbi_backend->init) {
				r = usbi_backend->init(ctx);
				if (UNLIKELY(r))
					goto err_free_ctx;
			} else
				goto err_free_ctx;
		}

		r = usbi_io_init(ctx);
		if (UNLIKELY(r < 0))
			goto err_backend_exit;
	}
	usbi_mutex_static_unlock(&default_context_lock);

	if (context)
		*context = ctx;

	RETURN(LIBUSB_SUCCESS, int);

err_backend_exit:
	LOGI("err_backend_exit");
	if (usbi_backend->exit)
		usbi_backend->exit();
err_free_ctx:
	LOGI("err_free_ctx");
	if (ctx == usbi_default_context)
		usbi_default_context = NULL;

	usbi_mutex_static_lock(&active_contexts_lock);
	{
		list_del(&ctx->list);
	}
	usbi_mutex_static_unlock(&active_contexts_lock);

	usbi_mutex_lock(&ctx->usb_devs_lock);
	{
		list_for_each_entry_safe(dev, next, &ctx->usb_devs, list, struct libusb_device)
		{
			list_del(&dev->list);
			libusb_unref_device(dev);
		}
	}
	usbi_mutex_unlock(&ctx->usb_devs_lock);

	usbi_mutex_destroy(&ctx->open_devs_lock);
	usbi_mutex_destroy(&ctx->usb_devs_lock);
	usbi_mutex_destroy(&ctx->hotplug_cbs_lock);

	free(ctx);
err_unlock:
	LOGI("err_unlock");
	usbi_mutex_static_unlock(&default_context_lock);
	RETURN(r, int);
}

/** \ingroup lib
 * Initialize libusb. This function must be called before calling any other
 * libusb function.
 *
 * If you do not provide an output location for a context pointer, a default
 * context will be created. If there was already a default context, it will
 * be reused (and nothing will be initialized/reinitialized).
 *
 * \param context Optional output location for context pointer.
 * Only valid on return code 0.
 * \returns 0 on success, or a LIBUSB_ERROR code on failure
 * \see contexts
 *
 * 初始化libusb。 必须在调用任何其他libusb函数之前调用此函数。
 * 如果不为上下文指针提供输出位置，则将创建默认上下文。 如果已经有一个默认上下文，它将被重用（不会初始化/重新初始化任何内容）。
 */
int API_EXPORTED libusb_init(libusb_context **context) {

	return libusb_init2(context, NULL);
#if 0
	struct libusb_device *dev, *next;
	char *dbg = getenv("LIBUSB_DEBUG");
	struct libusb_context *ctx;
	static int first_init = 1;
	int r = 0;

	usbi_mutex_static_lock(&default_context_lock);
	{
		if (!timestamp_origin.tv_sec) {
			usbi_gettimeofday(&timestamp_origin, NULL);
		}

		if (!context && usbi_default_context) {
			usbi_dbg("reusing default context");
			default_context_refcnt++;
			usbi_mutex_static_unlock(&default_context_lock);
			return LIBUSB_SUCCESS;
		}

		ctx = calloc(1, sizeof(*ctx));
		if (UNLIKELY(!ctx)) {
			r = LIBUSB_ERROR_NO_MEM;
			goto err_unlock;
		}

#ifdef ENABLE_DEBUG_LOGGING
		ctx->debug = LIBUSB_LOG_LEVEL_DEBUG;
#endif

		if (UNLIKELY(dbg)) {
			ctx->debug = atoi(dbg);
			if (ctx->debug)
				ctx->debug_fixed = 1;
		}

		/* default context should be initialized before calling usbi_dbg
		 * 调用usbi_dbg之前，应初始化默认上下文
		 */
		if (!usbi_default_context) {
			usbi_default_context = ctx;
			default_context_refcnt++;
			usbi_dbg("created default context");
		}

		usbi_dbg("libusb v%d.%d.%d.%d", libusb_version_internal.major, libusb_version_internal.minor,
			libusb_version_internal.micro, libusb_version_internal.nano);

		usbi_mutex_init(&ctx->usb_devs_lock, NULL);
		usbi_mutex_init(&ctx->open_devs_lock, NULL);
		usbi_mutex_init(&ctx->hotplug_cbs_lock, NULL);
		list_init(&ctx->usb_devs);
		list_init(&ctx->open_devs);
		list_init(&ctx->hotplug_cbs);

		usbi_mutex_static_lock(&active_contexts_lock);
		{
			if (first_init) {
				first_init = 0;
				list_init(&active_contexts_list);
			}
			list_add(&ctx->list, &active_contexts_list);
		}
		usbi_mutex_static_unlock(&active_contexts_lock);

		if (usbi_backend->init) {
			r = usbi_backend->init(ctx);
			if (UNLIKELY(r))
				goto err_free_ctx;
		}

		r = usbi_io_init(ctx);
		if (UNLIKELY(r < 0))
			goto err_backend_exit;
	}
	usbi_mutex_static_unlock(&default_context_lock);

	if (context)
		*context = ctx;

	return LIBUSB_SUCCESS;

err_backend_exit:
	if (usbi_backend->exit)
		usbi_backend->exit();
err_free_ctx:
	if (ctx == usbi_default_context)
		usbi_default_context = NULL;

	usbi_mutex_static_lock(&active_contexts_lock);
	{
		list_del(&ctx->list);
	}
	usbi_mutex_static_unlock(&active_contexts_lock);

	usbi_mutex_lock(&ctx->usb_devs_lock);
	{
		list_for_each_entry_safe(dev, next, &ctx->usb_devs, list, struct libusb_device)
		{
			list_del(&dev->list);
			libusb_unref_device(dev);
		}
	}
	usbi_mutex_unlock(&ctx->usb_devs_lock);

	usbi_mutex_destroy(&ctx->open_devs_lock);
	usbi_mutex_destroy(&ctx->usb_devs_lock);
	usbi_mutex_destroy(&ctx->hotplug_cbs_lock);

	free(ctx);
err_unlock:
	usbi_mutex_static_unlock(&default_context_lock);
	return r;
#endif
}

/** \ingroup lib
 * Deinitialize libusb. Should be called after closing all open devices and
 * before your application terminates.
 * \param ctx the context to deinitialize, or NULL for the default context
 *
 * 取消初始化libusb。 在关闭所有打开的设备之后且应用程序终止之前应调用此方法。
 */
void API_EXPORTED libusb_exit(struct libusb_context *ctx) {

	struct libusb_device *dev, *next;
	struct timeval tv = { 0, 0 };

	usbi_dbg("");
	USBI_GET_CONTEXT(ctx);

	/* if working with default context, only actually do the deinitialization if we're the last user
	 * 如果使用默认上下文，则只有在我们是最后一个用户的情况下才进行反初始化
	 */
	usbi_mutex_static_lock(&default_context_lock);
	if (ctx == usbi_default_context) {
		if (--default_context_refcnt > 0) {
			usbi_dbg("not destroying default context");
			usbi_mutex_static_unlock(&default_context_lock);
			return;
		}
		usbi_dbg("destroying default context");
		usbi_default_context = NULL;
	}
	usbi_mutex_static_unlock(&default_context_lock);

	usbi_mutex_static_lock(&active_contexts_lock);
	{
		list_del(&ctx->list);
	}
	usbi_mutex_static_unlock(&active_contexts_lock);

	if (libusb_has_capability(LIBUSB_CAP_HAS_HOTPLUG)) {
		usbi_hotplug_deregister_all(ctx);

		/*
		 * Ensure any pending unplug events are read from the hotplug
		 * pipe. The usb_device-s hold in the events are no longer part
		 * of usb_devs, but the events still hold a reference!
		 *
		 * Note we don't do this if the application has left devices
		 * open (which implies a buggy app) to avoid packet completion
		 * handlers running when the app does not expect them to run.
		 *
		 * 确保从热插拔管道读取任何未决的拔出事件。 事件中的usb_device-s不再是usb_devs的一部分，但事件仍然具有引用！
         * 请注意，如果应用程序未打开设备（这意味着有问题的应用程序），则我们不会这样做，以避免在应用程序不希望它们运行时运行数据包完成处理程序。
		 */
		if (list_empty(&ctx->open_devs))
			libusb_handle_events_timeout(ctx, &tv);

		usbi_mutex_lock(&ctx->usb_devs_lock);
		{
			list_for_each_entry_safe(dev, next, &ctx->usb_devs, list, struct libusb_device)
			{
				list_del(&dev->list);
				libusb_unref_device(dev);
			}
		}
		usbi_mutex_unlock(&ctx->usb_devs_lock);
	}

	/* a few sanity checks. don't bother with locking because unless
	 * there is an application bug, nobody will be accessing these.
	 * 一些健全性检查。 不必担心锁定，因为除非有应用程序错误，否则没有人会访问它们。
	 */
	if (!list_empty(&ctx->usb_devs))
		usbi_warn(ctx, "some libusb_devices were leaked");
	if (!list_empty(&ctx->open_devs))
		usbi_warn(ctx, "application left some devices open");

	usbi_io_exit(ctx);
	if (usbi_backend->exit)
		usbi_backend->exit();

	usbi_mutex_destroy(&ctx->open_devs_lock);
	usbi_mutex_destroy(&ctx->usb_devs_lock);
	usbi_mutex_destroy(&ctx->hotplug_cbs_lock);
	free(ctx);
}

/** \ingroup misc
 * Check at runtime if the loaded library has a given capability.
 * This call should be performed after \ref libusb_init(), to ensure the
 * backend has updated its capability set.
 * 在运行时检查加载的库是否具有给定的功能。 此调用应在 libusb_init() 之后执行，以确保后端已更新其功能集。
 *
 * \param capability the \ref libusb_capability to check for
 * \returns nonzero if the running library has the capability, 0 otherwise
 */
int API_EXPORTED libusb_has_capability(uint32_t capability) {

	switch (capability) {
	case LIBUSB_CAP_HAS_CAPABILITY:
		return 1;
	case LIBUSB_CAP_HAS_HOTPLUG:
		return !(usbi_backend->get_device_list);
	case LIBUSB_CAP_HAS_HID_ACCESS:
		return (usbi_backend->caps & USBI_CAP_HAS_HID_ACCESS);
	case LIBUSB_CAP_SUPPORTS_DETACH_KERNEL_DRIVER:
		return (usbi_backend->caps & USBI_CAP_SUPPORTS_DETACH_KERNEL_DRIVER);
	}
	return LIBUSB_SUCCESS;
}

/* this is defined in libusbi.h if needed
 * 如果需要，可以在libusbi.h中定义
 */
#ifdef LIBUSB_GETTIMEOFDAY_WIN32
/*
 * gettimeofday
 * Implementation according to:
 * The Open Group Base Specifications Issue 6
 * IEEE Std 1003.1, 2004 Edition
 * 根据以下内容的实施：开放组基本规范第6版IEEE标准1003.1，2004版
 */

/*
 *  THIS SOFTWARE IS NOT COPYRIGHTED
 *
 *  This source code is offered for use in the public domain. You may
 *  use, modify or distribute it freely.
 *
 *  This code is distributed in the hope that it will be useful but
 *  WITHOUT ANY WARRANTY. ALL WARRANTIES, EXPRESS OR IMPLIED ARE HEREBY
 *  DISCLAIMED. This includes but is not limited to warranties of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 *  Contributed by:
 *  Danny Smith <dannysmith@users.sourceforge.net>
 *
 * 本软件不受版权保护本源代码提供在公共领域中使用。 您可以自由使用，修改或分发它。
 * 分发此代码是希望它会有用，但没有任何保证。 此处不作任何明示或暗示的担保。 这包括但不限于对适销性或特定用途适用性的保证。
 */

/* Offset between 1/1/1601 and 1/1/1970 in 100 nanosec units
 * 偏移量在1/1/1601和1/1/1970之间，以100纳秒为单位
 */
#define _W32_FT_OFFSET (116444736000000000)

int usbi_gettimeofday(struct timeval *tp, void *tzp)
{
	union {
		unsigned __int64 ns100; /* Time since 1 Jan 1601, in 100ns units  自1601年1月1日以来的时间，以100ns为单位 */
		FILETIME ft;
	} _now;
	UNUSED(tzp);

	if(tp) {
#if defined(OS_WINCE)
		SYSTEMTIME st;
		GetSystemTime(&st);
		SystemTimeToFileTime(&st, &_now.ft);
#else
		GetSystemTimeAsFileTime (&_now.ft);
#endif
		tp->tv_usec=(long)((_now.ns100 / 10) % 1000000 );
		tp->tv_sec= (long)((_now.ns100 - _W32_FT_OFFSET) / 10000000);
	}
	/* Always return 0 as per Open Group Base Specifications Issue 6. Do not set errno on error.
	 * 根据开放组基本规范第6期，始终返回0。 不要将errno设置为错误。
	 */
	return LIBUSB_SUCCESS;
}
#endif

static void usbi_log_str(struct libusb_context *ctx,
	enum libusb_log_level level, const char * str) {

#if defined(USE_SYSTEM_LOGGING_FACILITY)
#if defined(OS_WINDOWS) || defined(OS_WINCE)
	/* Windows CE only supports the Unicode version of OutputDebugString.
	 * Windows CE仅支持OutputDebugString的Unicode版本。
	 */
	WCHAR wbuf[USBI_MAX_LOG_LEN];
	MultiByteToWideChar(CP_UTF8, 0, str, -1, wbuf, sizeof(wbuf));
	OutputDebugStringW(wbuf);
#elif defined(__ANDROID__)
	int priority = ANDROID_LOG_UNKNOWN;
	switch (level) {
	case LIBUSB_LOG_LEVEL_NONE: break;	// XXX add to avoid warning when compiling with clang  添加以避免在使用clang编译时发出警告
	case LIBUSB_LOG_LEVEL_INFO: priority = ANDROID_LOG_INFO; break;
	case LIBUSB_LOG_LEVEL_WARNING: priority = ANDROID_LOG_WARN; break;
	case LIBUSB_LOG_LEVEL_ERROR: priority = ANDROID_LOG_ERROR; break;
	case LIBUSB_LOG_LEVEL_DEBUG: priority = ANDROID_LOG_DEBUG; break;
	}
	__android_log_write(priority, "libusb", str);
#elif defined(HAVE_SYSLOG_FUNC)
	int syslog_level = LOG_INFO;
	switch (level) {
	case LIBUSB_LOG_LEVEL_INFO: syslog_level = LOG_INFO; break;
	case LIBUSB_LOG_LEVEL_WARNING: syslog_level = LOG_WARNING; break;
	case LIBUSB_LOG_LEVEL_ERROR: syslog_level = LOG_ERR; break;
	case LIBUSB_LOG_LEVEL_DEBUG: syslog_level = LOG_DEBUG; break;
	}
	syslog(syslog_level, "%s", str);
#else /* All of gcc, Clang, XCode seem to use #warning  所有的gcc，Clang和XCode似乎都使用#warning */
#warning System logging is not supported on this platform. Logging to stderr will be used instead.
	fputs(str, stderr);
#endif
#else
	fputs(str, stderr);
#endif /* USE_SYSTEM_LOGGING_FACILITY */
	UNUSED(ctx);
	UNUSED(level);
}

void usbi_log_v(struct libusb_context *ctx, enum libusb_log_level level,
	const char *function, const char *format, va_list args) {

#ifndef __ANDROID__
	const char *prefix = "";
#endif
	char buf[USBI_MAX_LOG_LEN];
	struct timeval now;
	int global_debug, header_len, text_len;
	static int has_debug_header_been_displayed = 0;

#ifdef ENABLE_DEBUG_LOGGING
	global_debug = 1;
	UNUSED(ctx);
#else
	int ctx_level = 0;

	USBI_GET_CONTEXT(ctx);
	if (ctx) {
		ctx_level = ctx->debug;
	} else {
		char *dbg = getenv("LIBUSB_DEBUG");
		if (dbg)
			ctx_level = atoi(dbg);
	}
#ifdef __ANDROID__
	global_debug = 0;
#else
	global_debug = (ctx_level == LIBUSB_LOG_LEVEL_DEBUG);
#endif
	if (!ctx_level)
		return;
	if (level == LIBUSB_LOG_LEVEL_WARNING && ctx_level < LIBUSB_LOG_LEVEL_WARNING)
		return;
	if (level == LIBUSB_LOG_LEVEL_INFO && ctx_level < LIBUSB_LOG_LEVEL_INFO)
		return;
	if (level == LIBUSB_LOG_LEVEL_DEBUG && ctx_level < LIBUSB_LOG_LEVEL_DEBUG)
		return;
#endif

	usbi_gettimeofday(&now, NULL);
	if ((global_debug) && (!has_debug_header_been_displayed)) {
		has_debug_header_been_displayed = 1;
		usbi_log_str(ctx, LIBUSB_LOG_LEVEL_DEBUG, "[timestamp] [threadID] facility level [function call] <message>\n");
		usbi_log_str(ctx, LIBUSB_LOG_LEVEL_DEBUG, "--------------------------------------------------------------------------------\n");
	}
	if (now.tv_usec < timestamp_origin.tv_usec) {
		now.tv_sec--;
		now.tv_usec += 1000000;
	}
	now.tv_sec -= timestamp_origin.tv_sec;
	now.tv_usec -= timestamp_origin.tv_usec;
#ifndef __ANDROID__
	switch (level) {
	case LIBUSB_LOG_LEVEL_INFO:
		prefix = "info";
		break;
	case LIBUSB_LOG_LEVEL_WARNING:
		prefix = "warning";
		break;
	case LIBUSB_LOG_LEVEL_ERROR:
		prefix = "error";
		break;
	case LIBUSB_LOG_LEVEL_DEBUG:
		prefix = "debug";
		break;
	case LIBUSB_LOG_LEVEL_NONE:
		return;
	default:
		prefix = "unknown";
		break;
	}
#endif
#ifdef __ANDROID__
	header_len = snprintf(buf, sizeof(buf), "[%s] ", function);
#else
	if (global_debug) {
		header_len = snprintf(buf, sizeof(buf),
			"[%2d.%06d] [%08x] libusb: %s [%s] ", (int) now.tv_sec,
			(int) now.tv_usec, usbi_get_tid(), prefix, function);
	} else {
		header_len = snprintf(buf, sizeof(buf),
			"libusb:%s [%s] ", prefix, function);
	}
#endif

	if (header_len < 0 || header_len >= sizeof(buf)) {
		/* Somehow snprintf failed to write to the buffer,
		 * remove the header so something useful is output.
		 * snprintf不知何故无法写入缓冲区，请删除标头，以便输出有用的信息。
		 */
		header_len = 0;
	}
	/* Make sure buffer is NUL terminated
	 * 确保缓冲区以NULL终止
	 */
	buf[header_len] = '\0';
	text_len = vsnprintf(buf + header_len, sizeof(buf) - header_len, format, args);
	if (text_len < 0 || text_len + header_len >= sizeof(buf)) {
		/* Truncated log output. On some platforms a -1 return value means
		 * that the output was truncated.
		 * 日志输出被截断。 在某些平台上，返回值-1表示输出被截断。
		 */
		text_len = sizeof(buf) - header_len;
	}
	if (header_len + text_len + sizeof(USBI_LOG_LINE_END) >= sizeof(buf)) {
		/* Need to truncate the text slightly to fit on the terminator.
		 * 需要略微截断文本以适合终止符。
		 */
		text_len -= (header_len + text_len + sizeof(USBI_LOG_LINE_END)) - sizeof(buf);
	}
	strcpy(buf + header_len + text_len, USBI_LOG_LINE_END);

	usbi_log_str(ctx, level, buf);
}

void usbi_log(struct libusb_context *ctx, enum libusb_log_level level,
	const char *function, const char *format, ...) {

	va_list args;

	va_start(args, format);
	usbi_log_v(ctx, level, function, format, args);
	va_end(args);
}

/** \ingroup misc
 * Returns a constant NULL-terminated string with the ASCII name of a libusb
 * error or transfer status code. The caller must not free() the returned
 * string.
 *
 * \param error_code The \ref libusb_error or libusb_transfer_status code to
 * return the name of.
 * \returns The error name, or the string **UNKNOWN** if the value of
 * error_code is not a known error / status code.
 *
 * 返回一个以NULL终止的常量字符串，其中带有libusb错误或传输状态代码的ASCII名称。 调用者不得free()返回的字符串。
 */
DEFAULT_VISIBILITY const char * LIBUSB_CALL libusb_error_name(int error_code) {

	switch (error_code) {
	case LIBUSB_ERROR_IO:
		return "LIBUSB_ERROR_IO";
	case LIBUSB_ERROR_INVALID_PARAM:
		return "LIBUSB_ERROR_INVALID_PARAM";
	case LIBUSB_ERROR_ACCESS:
		return "LIBUSB_ERROR_ACCESS";
	case LIBUSB_ERROR_NO_DEVICE:
		return "LIBUSB_ERROR_NO_DEVICE";
	case LIBUSB_ERROR_NOT_FOUND:
		return "LIBUSB_ERROR_NOT_FOUND";
	case LIBUSB_ERROR_BUSY:
		return "LIBUSB_ERROR_BUSY";
	case LIBUSB_ERROR_TIMEOUT:
		return "LIBUSB_ERROR_TIMEOUT";
	case LIBUSB_ERROR_OVERFLOW:
		return "LIBUSB_ERROR_OVERFLOW";
	case LIBUSB_ERROR_PIPE:
		return "LIBUSB_ERROR_PIPE";
	case LIBUSB_ERROR_INTERRUPTED:
		return "LIBUSB_ERROR_INTERRUPTED";
	case LIBUSB_ERROR_NO_MEM:
		return "LIBUSB_ERROR_NO_MEM";
	case LIBUSB_ERROR_NOT_SUPPORTED:
		return "LIBUSB_ERROR_NOT_SUPPORTED";
	case LIBUSB_ERROR_OTHER:
		return "LIBUSB_ERROR_OTHER";

	case LIBUSB_TRANSFER_ERROR:
		return "LIBUSB_TRANSFER_ERROR";
	case LIBUSB_TRANSFER_TIMED_OUT:
		return "LIBUSB_TRANSFER_TIMED_OUT";
	case LIBUSB_TRANSFER_CANCELLED:
		return "LIBUSB_TRANSFER_CANCELLED";
	case LIBUSB_TRANSFER_STALL:
		return "LIBUSB_TRANSFER_STALL";
	case LIBUSB_TRANSFER_NO_DEVICE:
		return "LIBUSB_TRANSFER_NO_DEVICE";
	case LIBUSB_TRANSFER_OVERFLOW:
		return "LIBUSB_TRANSFER_OVERFLOW";

	case 0:
		return "LIBUSB_SUCCESS / LIBUSB_TRANSFER_COMPLETED";
	default:
		return "**UNKNOWN**";
	}
}

/** \ingroup misc
 * Returns a pointer to const struct libusb_version with the version
 * (major, minor, micro, nano and rc) of the running library.
 * 返回具有运行库版本（主要，次要，微型，nano和rc）的const struct libusb_version的指针。
 */
DEFAULT_VISIBILITY
const struct libusb_version * LIBUSB_CALL libusb_get_version(void) {

	return &libusb_version_internal;
}
