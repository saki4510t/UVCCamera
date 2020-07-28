/*
 * Internal header for libusb
 * Copyright © 2007-2009 Daniel Drake <dsd@gentoo.org>
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

#ifndef LIBUSBI_H
#define LIBUSBI_H

#include "config.h"

#include <stdlib.h>

#include <stddef.h>
#include <stdint.h>
#include <time.h>
#include <stdarg.h>
#ifdef HAVE_POLL_H
#include <poll.h>
#endif

#ifdef HAVE_MISSING_H
#include "missing.h"
#endif
#include "libusb.h"
#include "version.h"
#include "utilbase.h"

/* Inside the libusb code, mark all public functions as follows:
 *   return_type API_EXPORTED function_name(params) { ... }
 * But if the function returns a pointer, mark it as follows:
 *   DEFAULT_VISIBILITY return_type * LIBUSB_CALL function_name(params) { ... }
 * In the libusb public header, mark all declarations as:
 *   return_type LIBUSB_CALL function_name(params);
 *
 * 在libusb代码中，将所有公共函数标记如下：
 * return_type API_EXPORTED function_name（params）{...}
 * 但是，如果函数返回一个指针，则将其标记为：
 * DEFAULT_VISIBILITY return_type * LIBUSB_CALL function_name（params）{...}
 * 在 libusb公共头文件，将所有声明标记为：
 * return_type LIBUSB_CALL function_name（params）;
 */
#define API_EXPORTED LIBUSB_CALL DEFAULT_VISIBILITY

#define DEVICE_DESC_LENGTH		18

#define USB_MAXENDPOINTS	32
#define USB_MAXINTERFACES	32
#define USB_MAXCONFIG		8

/* Backend specific capabilities
 * 后端特定功能
 */
#define USBI_CAP_HAS_HID_ACCESS					0x00010000
#define USBI_CAP_SUPPORTS_DETACH_KERNEL_DRIVER	0x00020000

/* Maximum number of bytes in a log line
 * 日志行中的最大字节数
 */
#define USBI_MAX_LOG_LEN	1024
/* Terminator for log lines
 * 日志行的终止符
 */
#define USBI_LOG_LINE_END	"\n"

/* The following is used to silence warnings for unused variables
 * 以下内容用于使未使用变量的警告静音
 */
#define UNUSED(var)			do { (void)(var); } while(0)

#if !defined(ARRAYSIZE)
#define ARRAYSIZE(array) (sizeof(array)/sizeof(array[0]))
#endif

struct list_head {
	struct list_head *prev, *next;
};

/* Get an entry from the list
 *  ptr - the address of this list_head element in "type"
 *  type - the data type that contains "member"
 *  member - the list_head element in "type"
 * 从列表中获取一个实体
 */
#define list_entry(ptr, type, member) \
	((type *)((uintptr_t)(ptr) - (uintptr_t)offsetof(type, member)))

/* Get each entry from a list
 *  pos - A structure pointer has a "member" element
 *  head - list head
 *  member - the list_head element in "pos"
 *  type - the type of the first parameter
 * 从列表中获取每个实体
 */
#define list_for_each_entry(pos, head, member, type)			\
	for (pos = list_entry((head)->next, type, member);			\
		 &pos->member != (head);								\
		 pos = list_entry(pos->member.next, type, member))

#define list_for_each_entry_safe(pos, n, head, member, type)	\
	for (pos = list_entry((head)->next, type, member),			\
		 n = list_entry(pos->member.next, type, member);		\
		 &pos->member != (head);								\
		 pos = n, n = list_entry(n->member.next, type, member))

#define list_empty(entry) ((entry)->next == (entry))

static inline void list_init(struct list_head *entry) {
	entry->prev = entry->next = entry;
}

static inline void list_add(struct list_head *entry, struct list_head *head) {
	entry->next = head->next;
	entry->prev = head;

	head->next->prev = entry;
	head->next = entry;
}

static inline void list_add_tail(struct list_head *entry, struct list_head *head) {
	entry->next = head;
	entry->prev = head->prev;

	head->prev->next = entry;
	head->prev = entry;
}

static inline void list_del(struct list_head *entry) {
	if (!list_empty(entry)) {	// XXX add saki@serenegiant 因为有时会崩溃
		entry->next->prev = entry->prev;
		entry->prev->next = entry->next;
		entry->next = entry->prev = NULL;
	}
}

static inline void *usbi_reallocf(void *ptr, size_t size) {
	void *ret = realloc(ptr, size);
	if (UNLIKELY(!ret))
		free(ptr);
	return ret;
}

#define container_of(ptr, type, member) ({                      \
        const typeof( ((type *)0)->member ) *mptr = (ptr);    \
        (type *)( (char *)mptr - offsetof(type,member) );})

#define MIN(a, b)	((a) < (b) ? (a) : (b))
#define MAX(a, b)	((a) > (b) ? (a) : (b))

#define TIMESPEC_IS_SET(ts) ((ts)->tv_sec != 0 || (ts)->tv_nsec != 0)

/* Some platforms don't have this define
 * 一些平台没有这个定义
 */
#ifndef TIMESPEC_TO_TIMEVAL
#define TIMESPEC_TO_TIMEVAL(tv, ts)                                     \
        do {                                                            \
                (tv)->tv_sec = (ts)->tv_sec;                            \
                (tv)->tv_usec = (ts)->tv_nsec / 1000;                   \
        } while (0)
#endif

void usbi_log(struct libusb_context *ctx, enum libusb_log_level level,
		const char *function, const char *format, ...);

void usbi_log_v(struct libusb_context *ctx, enum libusb_log_level level,
		const char *function, const char *format, va_list args);

#if !defined(_MSC_VER) || _MSC_VER >= 1400

#ifdef ENABLE_LOGGING
#define _usbi_log(ctx, level, ...) usbi_log(ctx, level, __FUNCTION__, __VA_ARGS__)
#define usbi_dbg(...) _usbi_log(NULL, LIBUSB_LOG_LEVEL_DEBUG, __VA_ARGS__)
#else
#define _usbi_log(ctx, level, ...) do { (void)(ctx); } while(0)
#define usbi_dbg(...) do {} while(0)
#endif

#define usbi_info(ctx, ...) _usbi_log(ctx, LIBUSB_LOG_LEVEL_INFO, __VA_ARGS__)
#define usbi_warn(ctx, ...) _usbi_log(ctx, LIBUSB_LOG_LEVEL_WARNING, __VA_ARGS__)
#define usbi_err(ctx, ...) _usbi_log(ctx, LIBUSB_LOG_LEVEL_ERROR, __VA_ARGS__)

#else /* !defined(_MSC_VER) || _MSC_VER >= 1400 */

#ifdef ENABLE_LOGGING
#define LOG_BODY(ctxt, level) \
{                             \
	va_list args;             \
	va_start (args, format);  \
	usbi_log_v(ctxt, level, "", format, args); \
	va_end(args);             \
}
#else
#define LOG_BODY(ctxt, level) do { (void)(ctxt); } while(0)
#endif

static inline void usbi_info(struct libusb_context *ctx, const char *format,
		...)
LOG_BODY(ctx,LIBUSB_LOG_LEVEL_INFO)
static inline void usbi_warn(struct libusb_context *ctx, const char *format,
		...)
LOG_BODY(ctx,LIBUSB_LOG_LEVEL_WARNING)
static inline void usbi_err( struct libusb_context *ctx, const char *format,
		...)
LOG_BODY(ctx,LIBUSB_LOG_LEVEL_ERROR)

static inline void usbi_dbg(const char *format, ...)
LOG_BODY(NULL,LIBUSB_LOG_LEVEL_DEBUG)

#endif /* !defined(_MSC_VER) || _MSC_VER >= 1400 */

#define USBI_GET_CONTEXT(ctx) if (!(ctx)) (ctx) = usbi_default_context
#define DEVICE_CTX(dev) ((dev)->ctx)
#define HANDLE_CTX(handle) (DEVICE_CTX((handle)->dev))
#define TRANSFER_CTX(transfer) (HANDLE_CTX((transfer)->dev_handle))
#define ITRANSFER_CTX(transfer) \
	(TRANSFER_CTX(USBI_TRANSFER_TO_LIBUSB_TRANSFER(transfer)))

#define IS_EPIN(ep) (0 != ((ep) & LIBUSB_ENDPOINT_IN))
#define IS_EPOUT(ep) (!IS_EPIN(ep))
#define IS_XFERIN(xfer) (0 != ((xfer)->endpoint & LIBUSB_ENDPOINT_IN))
#define IS_XFEROUT(xfer) (!IS_XFERIN(xfer))

/* Internal abstraction for thread synchronization
 * 线程同步的内部抽象
 */
#if defined(THREADS_POSIX)
#include "os/threads_posix.h"
#elif defined(OS_WINDOWS) || defined(OS_WINCE)
#include <os/threads_windows.h>
#endif

extern struct libusb_context *usbi_default_context;

struct libusb_context {
	int debug;
	int debug_fixed;

	/* internal control pipe, used for interrupting event handling when something needs to modify poll fds.
	 * 内部控制管道，用于在需要修改poll fds时中断事件处理。
	 */
	int ctrl_pipe[2];

	struct list_head usb_devs;
	usbi_mutex_t usb_devs_lock;

	/* A list of open handles. Backends are free to traverse this if required.
	 * 打开句柄列表。 后端可以根据需要随意遍历。
	 */
	struct list_head open_devs;
	usbi_mutex_t open_devs_lock;

	/* A list of registered hotplug callbacks
	 * 已注册的热插拔回调列表
	 */
	struct list_head hotplug_cbs;
	usbi_mutex_t hotplug_cbs_lock;
	int hotplug_pipe[2];

	/* this is a list of in-flight transfer handles, sorted by timeout
	 * expiration. URBs to timeout the soonest are placed at the beginning of
	 * the list, URBs that will time out later are placed after, and urbs with
	 * infinite timeout are always placed at the very end.
	 * 这是一个空中传输句柄的列表，按超时时间排序。
	 * 将最快超时的URB放置在列表的开头，将稍后超时的URB放置在列表的后面，将将具有无限超时的urb始终放置在列表的最后。
	 */
	struct list_head flying_transfers;
	usbi_mutex_t flying_transfers_lock;

	/* list of poll fds
	 */
	struct list_head pollfds;
	usbi_mutex_t pollfds_lock;

	/* a counter that is set when we want to interrupt event handling, in order to modify the poll fd set. and a lock to protect it.
	 * 我们想要中断事件处理时设置的计数器，以修改 poll fd 设置。 和一把锁来保护它。
	 */
	unsigned int pollfd_modify;
	usbi_mutex_t pollfd_modify_lock;

	/* user callbacks for pollfd changes
	 * pollfd 变更的 回调方法
	 */
	libusb_pollfd_added_cb fd_added_cb;
	libusb_pollfd_removed_cb fd_removed_cb;
	void *fd_cb_user_data;

	/* ensures that only one thread is handling events at any one time
	 * 确保在任何时间只有一个线程正在处理事件
	 */
	usbi_mutex_t events_lock;

	/* used to see if there is an active thread doing event handling
	 * 用于查看是否有活动线程在进行事件处理
	 */
	int event_handler_active;

	/* used to wait for event completion in threads other than the one that is event handling
	 * 用于等待事件处理以外的线程中的事件完成
	 */
	usbi_mutex_t event_waiters_lock;
	usbi_cond_t event_waiters_cond;

#ifdef USBI_TIMERFD_AVAILABLE
	/* used for timeout handling, if supported by OS.
	 * this timerfd is maintained to trigger on the next pending timeout
	 * 用于超时处理（如果操作系统支持）。
	 * 维护此timerfd以在下一个未决超时时触发
	 */
	int timerfd;
#endif

	struct list_head list;
};

#ifdef USBI_TIMERFD_AVAILABLE
#define usbi_using_timerfd(ctx) ((ctx)->timerfd >= 0)
#else
#define usbi_using_timerfd(ctx) (0)
#endif

struct libusb_device {
	/* lock protects refcnt, everything else is finalized at initialization time
	 * 锁保护引用，其他所有操作在初始化时完成
	 */
	usbi_mutex_t lock;
	int refcnt;

	struct libusb_context *ctx;

	uint8_t bus_number;
	uint8_t port_number;
	struct libusb_device* parent_dev;
	uint8_t device_address;
	uint8_t num_configurations;
	enum libusb_speed speed;

	struct list_head list;
	unsigned long session_data;

	struct libusb_device_descriptor device_descriptor;
	int attached;

	unsigned char os_priv
#if defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 199901L)
	[] /* valid C99 code 有效的C99代码 */
#else
	[0] /* non-standard, but usually working code 非标准，但通常是工作代码 */
#endif
	;
};

struct libusb_device_handle {
	/* lock protects claimed_interfaces
	 * 锁保护 claimed_interfaces
	 */
	usbi_mutex_t lock;
	unsigned long claimed_interfaces;

	struct list_head list;
	struct libusb_device *dev;
	int auto_detach_kernel_driver;
	unsigned char os_priv
#if defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 199901L)
	[] /* valid C99 code 有效的C99代码 */
#else
	[0] /* non-standard, but usually working code  非标准，但通常是工作代码 */
#endif
	;
};

enum {
	USBI_CLOCK_MONOTONIC,
	USBI_CLOCK_REALTIME
};

/* in-memory transfer layout:
 *
 * 1. struct usbi_transfer
 * 2. struct libusb_transfer (which includes iso packets) [variable size]
 * 3. os private data [variable size]
 *
 * from a libusb_transfer, you can get the usbi_transfer by rewinding the
 * appropriate number of bytes.
 * the usbi_transfer includes the number of allocated packets, so you can
 * determine the size of the transfer and hence the start and length of the
 * OS-private data.
 *
 * 内存中的传输层：
 * 1. struct usbi_transfer
 * 2. struct libusb_transfer（包括iso数据包）[可变大小]
 * 3.从libusb_transfer中获取os私有数据[可变大小]，您可以通过倒带相应的字节数来获取usbi_transfer。
 * usbi_transfer包括分配的数据包数量，因此您可以确定传输的大小，从而确定OS专用数据的开始和长度。
 */

struct usbi_transfer {
    // 数据包数量
	int num_iso_packets;
	struct list_head list;
	struct timeval timeout;
	int transferred;
	uint32_t stream_id;
	uint8_t flags;

	/* this lock is held during libusb_submit_transfer() and
	 * libusb_cancel_transfer() (allowing the OS backend to prevent duplicate
	 * cancellation, submission-during-cancellation, etc). the OS backend
	 * should also take this lock in the handle_events path, to prevent the user
	 * cancelling the transfer from another thread while you are processing
	 * its completion (presumably there would be races within your OS backend
	 * if this were possible).
	 *
	 * 此锁在 libusb_submit_transfer() 和 libusb_cancel_transfer() 期间保持（允许OS后端防止重复取消，取消时提交）。
	 * 操作系统后端也应在 handle_events 路径中使用此锁定，以防止用户在处理线程完成时取消从另一个线程进行的传输（如果可能，则可能在您的操作系统后端中发生竞争）。
	 */
	usbi_mutex_t lock;
};

enum usbi_transfer_flags {
	/* The transfer has timed out
	 * 传输已超时
	 */
	USBI_TRANSFER_TIMED_OUT = 1 << 0,

	/* Set by backend submit_transfer() if the OS handles timeout
	 * 如果操作系统处理超时，则由后端 submit_transfer() 设置
	 */
	USBI_TRANSFER_OS_HANDLES_TIMEOUT = 1 << 1,

	/* Cancellation was requested via libusb_cancel_transfer()
	 * 通过 libusb_cancel_transfer() 请求取消
	 */
	USBI_TRANSFER_CANCELLING = 1 << 2,

	/* Operation on the transfer failed because the device disappeared
	 * 传输操作失败，因为设备消失了
	 */
	USBI_TRANSFER_DEVICE_DISAPPEARED = 1 << 3,

	/* Set by backend submit_transfer() if the fds in use have been updated
	 * 如果正在使用的fds已更新，则由后端 submit_transfer() 设置
	 */
	USBI_TRANSFER_UPDATED_FDS = 1 << 4,
};

#define USBI_TRANSFER_TO_LIBUSB_TRANSFER(transfer) \
	((struct libusb_transfer *)(((unsigned char *)(transfer)) \
		+ sizeof(struct usbi_transfer)))
#define LIBUSB_TRANSFER_TO_USBI_TRANSFER(transfer) \
	((struct usbi_transfer *)(((unsigned char *)(transfer)) \
		- sizeof(struct usbi_transfer)))

static inline void *usbi_transfer_get_os_priv(struct usbi_transfer *transfer) {
	return ((unsigned char *) transfer) + sizeof(struct usbi_transfer)
			+ sizeof(struct libusb_transfer)
			+ (transfer->num_iso_packets
					* sizeof(struct libusb_iso_packet_descriptor));
}

/* bus structures
 * 总线结构
 */

/* All standard descriptors have these 2 fields in common
 * 所有标准描述符都有这两个字段
 */
struct usb_descriptor_header {
	uint8_t bLength;
	uint8_t bDescriptorType;
};

/* shared data and functions
 * 共享数据和功能
 */

int usbi_io_init(struct libusb_context *ctx);
void usbi_io_exit(struct libusb_context *ctx);

struct libusb_device *usbi_alloc_device(struct libusb_context *ctx,
		unsigned long session_id);
struct libusb_device *usbi_get_device_by_session_id(struct libusb_context *ctx,
		unsigned long session_id);
int usbi_sanitize_device(struct libusb_device *dev);
void usbi_handle_disconnect(struct libusb_device_handle *handle);

int usbi_handle_transfer_completion(struct usbi_transfer *itransfer,
		enum libusb_transfer_status status);
int usbi_handle_transfer_cancellation(struct usbi_transfer *transfer);

int usbi_parse_descriptor(const unsigned char *source, const char *descriptor,
		void *dest, int host_endian);
int usbi_device_cache_descriptor(libusb_device *dev);
int usbi_get_config_index_by_value(struct libusb_device *dev,
		uint8_t bConfigurationValue, int *idx);

void usbi_connect_device(struct libusb_device *dev);
void usbi_disconnect_device(struct libusb_device *dev);

/* Internal abstraction for poll (needs struct usbi_transfer on Windows)
 * 内部抽象轮询（在Windows上需要 struct usbi_transfer ）
 */
#if defined(OS_ANDROID) || defined(OS_LINUX) || defined(OS_DARWIN) || defined(OS_OPENBSD) || defined(OS_NETBSD)	// XXX
#include <unistd.h>
#include "os/poll_posix.h"
#elif defined(OS_WINDOWS) || defined(OS_WINCE)
#include "os/poll_windows.h"
#endif

#if (defined(OS_WINDOWS) || defined(OS_WINCE)) && !defined(__GNUC__)
#define snprintf _snprintf
#define vsnprintf _vsnprintf
int usbi_gettimeofday(struct timeval *tp, void *tzp);
#define LIBUSB_GETTIMEOFDAY_WIN32
#define HAVE_USBI_GETTIMEOFDAY
#else
#ifdef HAVE_GETTIMEOFDAY
#define usbi_gettimeofday(tv, tz) gettimeofday((tv), (tz))
#define HAVE_USBI_GETTIMEOFDAY
#endif
#endif

struct usbi_pollfd {
	/* must come first
	 * 必须先来
	 */
	struct libusb_pollfd pollfd;

	struct list_head list;
};

int usbi_add_pollfd(struct libusb_context *ctx, int fd, short events);
void usbi_remove_pollfd(struct libusb_context *ctx, int fd);
void usbi_fd_notification(struct libusb_context *ctx);

/* device discovery
 * 设备发现
 */

/* we traverse usbfs without knowing how many devices we are going to find.
 * so we create this discovered_devs model which is similar to a linked-list
 * which grows when required. it can be freed once discovery has completed,
 * eliminating the need for a list node in the libusb_device structure
 * itself.
 *
 * 我们遍历usbfs却不知道要找到多少个设备。因此，我们创建了一个Discovered_devs模型，该模型类似于一个链表，该链表在需要时会增长。
 * 发现完成后可以将其释放，从而无需在 libusb_device 结构本身中使用列表节点。
 */
struct discovered_devs {
	size_t len;
	size_t capacity;
	struct libusb_device *devices
#if defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 199901L)
	[] /* valid C99 code */
#else
	[0] /* non-standard, but usually working code */
#endif
	;
};

struct discovered_devs *discovered_devs_append(struct discovered_devs *discdevs,
		struct libusb_device *dev);

/* OS abstraction
 * 操作系统抽象
 */

/* This is the interface that OS backends need to implement.
 * All fields are mandatory, except ones explicitly noted as optional.
 * 这是OS后端需要实现的接口。除明确指出为可选的字段外，所有字段均为必填字段。
 */
struct usbi_os_backend {
	/* A human-readable name for your backend, e.g. "Linux usbfs"
	 * 后端的易于理解的名称，例如 “ Linux usbfs”
	 */
	const char *name;

	/* Binary mask for backend specific capabilities
	 * 用于后端特定功能的二进制掩码
	 */
	uint32_t caps;

	/* Perform initialization of your backend. You might use this function
	 * to determine specific capabilities of the system, allocate required
	 * data structures for later, etc.
	 *
	 * This function is called when a libusb user initializes the library
	 * prior to use.
	 *
	 * Return 0 on success, or a LIBUSB_ERROR code on failure.
	 *
	 * 执行后端的初始化。您可以使用此功能来确定系统的特定功能，为以后分配所需的数据结构，等等。
	 * 当libusb用户在使用之前初始化库时，将调用此函数。
	 * 成功返回0，失败返回LIBUSB_ERROR代码。
	 */
	int (*init)(struct libusb_context *ctx);
	int (*init2)(struct libusb_context *ctx, const char *usbfs);

	/* Deinitialization. Optional. This function should destroy anything
	 * that was set up by init.
	 *
	 * This function is called when the user deinitializes the library.
	 *
	 * 取消初始化。 可选的。 此函数应销毁由init设置的任何内容。
	 * 用户取消初始化库时，将调用此函数。
	 */
	void (*exit)(void);

	/* Enumerate all the USB devices on the system, returning them in a list
	 * of discovered devices.
	 *
	 * Your implementation should enumerate all devices on the system,
	 * regardless of whether they have been seen before or not.
	 *
	 * When you have found a device, compute a session ID for it. The session
	 * ID should uniquely represent that particular device for that particular
	 * connection session since boot (i.e. if you disconnect and reconnect a
	 * device immediately after, it should be assigned a different session ID).
	 * If your OS cannot provide a unique session ID as described above,
	 * presenting a session ID of (bus_number << 8 | device_address) should
	 * be sufficient. Bus numbers and device addresses wrap and get reused,
	 * but that is an unlikely case.
	 *
	 * After computing a session ID for a device, call
	 * usbi_get_device_by_session_id(). This function checks if libusb already
	 * knows about the device, and if so, it provides you with a reference
	 * to a libusb_device structure for it.
	 *
	 * If usbi_get_device_by_session_id() returns NULL, it is time to allocate
	 * a new device structure for the device. Call usbi_alloc_device() to
	 * obtain a new libusb_device structure with reference count 1. Populate
	 * the bus_number and device_address attributes of the new device, and
	 * perform any other internal backend initialization you need to do. At
	 * this point, you should be ready to provide device descriptors and so
	 * on through the get_*_descriptor functions. Finally, call
	 * usbi_sanitize_device() to perform some final sanity checks on the
	 * device. Assuming all of the above succeeded, we can now continue.
	 * If any of the above failed, remember to unreference the device that
	 * was returned by usbi_alloc_device().
	 *
	 * At this stage we have a populated libusb_device structure (either one
	 * that was found earlier, or one that we have just allocated and
	 * populated). This can now be added to the discovered devices list
	 * using discovered_devs_append(). Note that discovered_devs_append()
	 * may reallocate the list, returning a new location for it, and also
	 * note that reallocation can fail. Your backend should handle these
	 * error conditions appropriately.
	 *
	 * This function should not generate any bus I/O and should not block.
	 * If I/O is required (e.g. reading the active configuration value), it is
	 * OK to ignore these suggestions :)
	 *
	 * This function is executed when the user wishes to retrieve a list
	 * of USB devices connected to the system.
	 *
	 * If the backend has hotplug support, this function is not used!
	 *
	 * Return 0 on success, or a LIBUSB_ERROR code on failure.
	 *
	 * 枚举系统上的所有USB设备，并将它们返回到发现的设备列表中。
	 * 您的实现应枚举系统上的所有设备，无论之前是否曾见过它们。
	 * 找到设备后，请为其计算会话ID。 自启动以来，会话ID应该唯一地代表该特定连接会话的特定设备（即，如果您在断开连接后立即断开连接并重新连接设备，则应为其分配一个不同的会话ID）。
	 * 如果您的操作系统无法如上所述提供唯一的会话ID，则提供会话ID为（bus_number << 8 | device_address）就足够了。 总线号和设备地址会包装并重用，但这是不太可能的情况。
	 * 计算设备的会话ID后，请调用usbi_get_device_by_session_id()。 此函数检查libusb是否已经知道该设备，如果知道，则为您提供对libusb_device结构的引用。
	 * 如果usbi_get_device_by_session_id()返回NULL，则该为该设备分配新的设备结构了。
	 * 调用usbi_alloc_device()以获取具有引用计数1的新libusb_device结构。填充新设备的bus_number和device_address属性，并执行您需要执行的任何其他内部后端初始化。
	 * 此时，您应该准备通过 get_*_descriptor 函数提供设备描述符等。
	 * 最后，调用usbi_sanitize_device()对设备执行一些最终的健康检查。
	 * 假设以上所有条件均成功，我们现在可以继续。
	 * 如果以上任何一项失败，请记住要取消引用usbi_alloc_device()返回的设备。
	 * 在这一阶段，我们有一个已填充的libusb_device结构（可以是先前找到的一个，也可以是我们刚刚分配并填充的一个）。
	 * 现在，可以使用discover_devs_append()将其添加到发现的设备列表中。
	 * 请注意，founded_devs_append()可能会重新分配列表，为其返回一个新位置，并且还请注意，重新分配可能会失败。
	 * 您的后端应适当处理这些错误情况。
	 * 该函数不应产生任何总线I/O，也不应阻塞。如果需要I/O（例如，读取活动的配置值），则可以忽略这些建议：)
	 * 当用户希望检索一个I/O时执行此函数。连接到系统的USB设备列表。
	 * 如果后端支持热插拔，则不使用此功能！
	 * 成功返回0，失败返回LIBUSB_ERROR代码。
	 */
	int (*get_device_list)(struct libusb_context *ctx,
			struct discovered_devs **discdevs);

	/* Apps which were written before hotplug support, may listen for
	 * hotplug events on their own and call libusb_get_device_list on
	 * device addition. In this case libusb_get_device_list will likely
	 * return a list without the new device in there, as the hotplug
	 * event thread will still be busy enumerating the device, which may
	 * take a while, or may not even have seen the event yet.
	 *
	 * To avoid this libusb_get_device_list will call this optional
	 * function for backends with hotplug support before copying
	 * ctx->usb_devs to the user. In this function the backend should
	 * ensure any pending hotplug events are fully processed before
	 * returning.
	 *
	 * Optional, should be implemented by backends with hotplug support.
	 *
	 * 在支持热插拔之前编写的应用程序可以自己侦听热插拔事件，并在添加设备时调用libusb_get_device_list。
	 * 为了避免这种情况，在将ctx->usb_devs复制到用户之前，libusb_get_device_list将为支持热插拔的后端调用此可选函数。
	 * 在此功能中，后端应确保返回之前已完全处理所有未决的热插拔事件。
	 * 可选，应由具有热插拔支持的后端实现。
	 */
	void (*hotplug_poll)(void);

	/* Open a device for I/O and other USB operations. The device handle
	 * is preallocated for you, you can retrieve the device in question
	 * through handle->dev.
	 *
	 * Your backend should allocate any internal resources required for I/O
	 * and other operations so that those operations can happen (hopefully)
	 * without hiccup. This is also a good place to inform libusb that it
	 * should monitor certain file descriptors related to this device -
	 * see the usbi_add_pollfd() function.
	 *
	 * This function should not generate any bus I/O and should not block.
	 *
	 * This function is called when the user attempts to obtain a device
	 * handle for a device.
	 *
	 * Return:
	 * - 0 on success
	 * - LIBUSB_ERROR_ACCESS if the user has insufficient permissions
	 * - LIBUSB_ERROR_NO_DEVICE if the device has been disconnected since
	 *   discovery
	 * - another LIBUSB_ERROR code on other failure
	 *
	 * Do not worry about freeing the handle on failed open, the upper layers
	 * do this for you.
	 *
	 * 打开用于I/O和其他USB操作的设备。设备句柄已为您预先分配，您可以通过handle->dev检索有问题的设备。
     * 您的后端应该分配I/O和其他操作所需的所有内部资源，以便这些操作可以（希望）发生而不会打ic。
     * 这也是通知libusb应该监视与该设备有关的某些文件描述符的好地方-请参见usbi_add_pollfd()函数。
     * 此功能不应生成任何总线I/O，也不应阻塞。
     * 当用户尝试获取设备的设备句柄时，将调用此函数。
     * 返回：
     * -成功时为0
     * -如果用户权限不足，则为LIBUSB_ERROR_ACCESS
     * -如果自发现以来已断开设备的连接，则为LIBUSB_ERROR_NO_DEVICE
     * -其他故障时的另一个LIBUSB_ERROR
     * 代码不用担心在打开失败时释放手柄，上层将为您执行此操作。
	 */
	int (*open)(struct libusb_device_handle *handle);

	/*
	 * XXX function to set file descriptor, added for mainly non-rooted Android
	 * 设置文件描述符的功能，主要针对非root用户的Android添加
	 */
	int (*set_device_fd)(struct libusb_device *device, int fd);
	/* Close a device such that the handle cannot be used again. Your backend
	 * should destroy any resources that were allocated in the open path.
	 * This may also be a good place to call usbi_remove_pollfd() to inform
	 * libusb of any file descriptors associated with this device that should
	 * no longer be monitored.
	 *
	 * This function is called when the user closes a device handle.
	 *
	 * 关闭设备，使手柄无法再次使用。您的后端应销毁在开放路径中分配的所有资源。这也可能是调用usbi_remove_pollfd()的好地方，
	 * 以通知libusb与该设备关联的所有文件描述符，这些文件描述符将不再受到监视。
     * 用户关闭设备句柄时将调用此函数。
	 */
	void (*close)(struct libusb_device_handle *handle);

#ifdef ACCESS_RAW_DESCRIPTORS
	int (*get_raw_descriptor)(struct libusb_device *device,
			unsigned char *buffer, int *descriptors_len, int *host_endian);	// XXX
#endif
	/* Retrieve the device descriptor from a device.
	 *
	 * The descriptor should be retrieved from memory, NOT via bus I/O to the
	 * device. This means that you may have to cache it in a private structure
	 * during get_device_list enumeration. Alternatively, you may be able
	 * to retrieve it from a kernel interface (some Linux setups can do this)
	 * still without generating bus I/O.
	 *
	 * This function is expected to write DEVICE_DESC_LENGTH (18) bytes into
	 * buffer, which is guaranteed to be big enough.
	 *
	 * This function is called when sanity-checking a device before adding
	 * it to the list of discovered devices, and also when the user requests
	 * to read the device descriptor.
	 *
	 * This function is expected to return the descriptor in bus-endian format
	 * (LE). If it returns the multi-byte values in host-endian format,
	 * set the host_endian output parameter to "1".
	 *
	 * Return 0 on success or a LIBUSB_ERROR code on failure.
	 *
	 * 从设备检索设备描述符。
	 * 应该从存​​储器而不是通过总线I/O到设备检索描述符。 这意味着在枚举get_device_list时可能必须将其缓存在私有结构中。
	 * 或者，您仍然可以从内核接口（某些Linux设置可以执行此操作）中检索它，而无需生成总线I/O。
	 * 预期此函数会将DEVICE_DESC_LENGTH(18)字节写入缓冲区，保证足够大。
	 * 在将设备添加到发现的设备列表之前对其进行完整性检查时，以及在用户请求读取设备描述符时，都会调用此函数。
	 * 预期此函数将以bus-endian格式(LE)返回描述符。如果它以host-endian格式返回多字节值，则将host_endian输出参数设置为“ 1”。
	 * 如果成功，则返回0；如果失败，则返回LIBUSB_ERROR代码。
	 */
	int (*get_device_descriptor)(struct libusb_device *device,
			unsigned char *buffer, int *host_endian);

	/* Get the ACTIVE configuration descriptor for a device.
	 *
	 * The descriptor should be retrieved from memory, NOT via bus I/O to the
	 * device. This means that you may have to cache it in a private structure
	 * during get_device_list enumeration. You may also have to keep track
	 * of which configuration is active when the user changes it.
	 *
	 * This function is expected to write len bytes of data into buffer, which
	 * is guaranteed to be big enough. If you can only do a partial write,
	 * return an error code.
	 *
	 * This function is expected to return the descriptor in bus-endian format
	 * (LE). If it returns the multi-byte values in host-endian format,
	 * set the host_endian output parameter to "1".
	 *
	 * Return:
	 * - 0 on success
	 * - LIBUSB_ERROR_NOT_FOUND if the device is in unconfigured state
	 * - another LIBUSB_ERROR code on other failure
	 *
	 * 获取设备的ACTIVE配置描述符。
	 * 应该从存​​储器而不是通过总线I/O到设备检索描述符。这意味着在枚举get_device_list时可能必须将其缓存在私有结构中。当用户更改配置时，您可能还必须跟踪哪个配置处于活动状态。
	 * 预期该函数会将len个字节的数据写入缓冲区，这保证足够大。 如果只能进行部分写入，请返回错误代码。
	 * 预期此函数将以bus-endian格式（LE）返回描述符。 如果它以host-endian格式返回多字节值，则将host_endian输出参数设置为“ 1”。
	 * 返回值：
	 * -0表示成功
	 * -LIBUSB_ERROR_NOT_FOUND（如果设备处于未配置状态）
	 * -另一个LIBUSB_ERROR代码出现其他故障
	 */
	int (*get_active_config_descriptor)(struct libusb_device *device,
			unsigned char *buffer, size_t len, int *host_endian);

	/* Get a specific configuration descriptor for a device.
	 *
	 * The descriptor should be retrieved from memory, NOT via bus I/O to the
	 * device. This means that you may have to cache it in a private structure
	 * during get_device_list enumeration.
	 *
	 * The requested descriptor is expressed as a zero-based index (i.e. 0
	 * indicates that we are requesting the first descriptor). The index does
	 * not (necessarily) equal the bConfigurationValue of the configuration
	 * being requested.
	 *
	 * This function is expected to write len bytes of data into buffer, which
	 * is guaranteed to be big enough. If you can only do a partial write,
	 * return an error code.
	 *
	 * This function is expected to return the descriptor in bus-endian format
	 * (LE). If it returns the multi-byte values in host-endian format,
	 * set the host_endian output parameter to "1".
	 *
	 * Return the length read on success or a LIBUSB_ERROR code on failure.
	 *
	 * 获取设备的特定配置描述符。
	 * 应该从存​​储器而不是通过总线I/O到设备检索描述符。这意味着在枚举get_device_list时可能必须将其缓存在私有结构中。
	 * 请求的描述符表示为从零开始的索引（即0表示我们正在请求第一个描述符）。 该索引不（必须）等于所请求配置的bConfigurationValue。
	 * 预期该函数会将len个字节的数据写入缓冲区，这保证足够大。 如果只能进行部分写入，请返回错误代码。
	 * 预期此函数将以bus-endian格式（LE）返回描述符。 如果它以host-endian格式返回多字节值，则将host_endian输出参数设置为“ 1”。
	 * 返回成功读取的长度或失败读取的LIBUSB_ERROR代码。
	 */
	int (*get_config_descriptor)(struct libusb_device *device,
			uint8_t config_index, unsigned char *buffer, size_t len,
			int *host_endian);

	/* Like get_config_descriptor but then by bConfigurationValue instead
	 * of by index.
	 *
	 * Optional, if not present the core will call get_config_descriptor
	 * for all configs until it finds the desired bConfigurationValue.
	 *
	 * Returns a pointer to the raw-descriptor in *buffer, this memory
	 * is valid as long as device is valid.
	 *
	 * Returns the length of the returned raw-descriptor on success,
	 * or a LIBUSB_ERROR code on failure.
	 *
	 * 像get_config_descriptor一样，但是通过bConfigurationValue而不是通过索引。
	 * 可选，如果不存在，则内核将为所有配置调用get_config_descriptor，直到找到所需的bConfigurationValue为止。
	 * 返回指向 *buffer 中原始描述符的指针，只要设备有效，此内存就有效。
	 * 如果成功，则返回返回的原始描述符的长度；如果失败，则返回LIBUSB_ERROR代码。
	 */
	int (*get_config_descriptor_by_value)(struct libusb_device *device,
			uint8_t bConfigurationValue, unsigned char **buffer,
			int *host_endian);

	/* Get the bConfigurationValue for the active configuration for a device.
	 * Optional. This should only be implemented if you can retrieve it from
	 * cache (don't generate I/O).
	 *
	 * If you cannot retrieve this from cache, either do not implement this
	 * function, or return LIBUSB_ERROR_NOT_SUPPORTED. This will cause
	 * libusb to retrieve the information through a standard control transfer.
	 *
	 * This function must be non-blocking.
	 * Return:
	 * - 0 on success
	 * - LIBUSB_ERROR_NO_DEVICE if the device has been disconnected since it
	 *   was opened
	 * - LIBUSB_ERROR_NOT_SUPPORTED if the value cannot be retrieved without
	 *   blocking
	 * - another LIBUSB_ERROR code on other failure.
	 *
	 * 获取设备的活动配置的bConfigurationValue。
	 * 可选的。 仅当您可以从缓存中检索它（不生成I / O）时，才应实施此方法。
	 * 如果您无法从缓存中检索到此消息，则不执行此功能，或者返回LIBUSB_ERROR_NOT_SUPPORTED。 这将导致libusb通过标准控件传输来检索信息。
	 * 此功能必须是非阻塞的。
	 * 返回值：
	 * -0表示成功
	 * -LIBUSB_ERROR_NO_DEVICE（如果设备自打开以来就已断开连接）
	 * -LIBUSB_ERROR_NOT_SUPPORTED（如果无法阻止访问该值）
	 * -其他故障时出现另一个LIBUSB_ERROR代码。
	 */
	int (*get_configuration)(struct libusb_device_handle *handle, int *config);

	/* Set the active configuration for a device.
	 *
	 * A configuration value of -1 should put the device in unconfigured state.
	 *
	 * This function can block.
	 *
	 * Return:
	 * - 0 on success
	 * - LIBUSB_ERROR_NOT_FOUND if the configuration does not exist
	 * - LIBUSB_ERROR_BUSY if interfaces are currently claimed (and hence
	 *   configuration cannot be changed)
	 * - LIBUSB_ERROR_NO_DEVICE if the device has been disconnected since it
	 *   was opened
	 * - another LIBUSB_ERROR code on other failure.
	 *
	 * 设置设备的活动配置。
	 * 配置值-1应该使设备处于未配置状态。
	 * 该功能可能会阻止。
	 * 返回值：
	 * -成功时为0
	 * -如果该配置不存在，则为LIBUSB_ERROR_NOT_FOUND
	 * -如果当前声明了接口（因此无法更改配置），则为LIBUSB_ERROR_BUSY
	 * -如果该设备自打开以来已断开连接，则为LIBUSB_ERROR_NO_DEVICE
	 * -在其他故障时，另一个LIBUSB_ERROR代码。
	 */
	int (*set_configuration)(struct libusb_device_handle *handle, int config);

	/* Claim an interface. When claimed, the application can then perform
	 * I/O to an interface's endpoints.
	 *
	 * This function should not generate any bus I/O and should not block.
	 * Interface claiming is a logical operation that simply ensures that
	 * no other drivers/applications are using the interface, and after
	 * claiming, no other drivers/applicatiosn can use the interface because
	 * we now "own" it.
	 *
	 * Return:
	 * - 0 on success
	 * - LIBUSB_ERROR_NOT_FOUND if the interface does not exist
	 * - LIBUSB_ERROR_BUSY if the interface is in use by another driver/app
	 * - LIBUSB_ERROR_NO_DEVICE if the device has been disconnected since it
	 *   was opened
	 * - another LIBUSB_ERROR code on other failure
	 *
	 * 声明接口。 声明所有权后，应用程序即可对接口的端点执行I/O。
	 * 此功能不应生成任何总线I/O，也不应阻塞。
	 * 接口声明是一种逻辑操作，可以确保没有其他驱动程序/应用程序在使用该接口，并且声明之后，其他任何驱动程序/应用程序都不能使用该接口，因为我们现在“拥有”它。
	 * 返回值：
	 * -成功时为0
	 * -如果该接口不存在，则为LIBUSB_ERROR_NOT_FOUND
	 * -如果该接口被另一个驱动程序/应用程序使用，则为LIBUSB_ERROR_BUSY
	 * -如果该设备自打开以来已断开连接，则为LIBUSB_ERROR_NO_DEVICE
	 * -在其他故障时，另一个LIBUSB_ERROR代码
	 */
	int (*claim_interface)(struct libusb_device_handle *handle, int interface_number);

	/* Release a previously claimed interface.
	 *
	 * This function should also generate a SET_INTERFACE control request,
	 * resetting the alternate setting of that interface to 0. It's OK for
	 * this function to block as a result.
	 *
	 * You will only ever be asked to release an interface which was
	 * successfully claimed earlier.
	 *
	 * Return:
	 * - 0 on success
	 * - LIBUSB_ERROR_NO_DEVICE if the device has been disconnected since it
	 *   was opened
	 * - another LIBUSB_ERROR code on other failure
	 *
	 * 释放以前声明的界面。
	 * 该函数还应该生成一个SET_INTERFACE控制请求，将该接口的替代设置重置为0。因此，该函数可以阻塞。
	 * 您只会被要求释放之前已成功声明的接口。
	 * 返回值：
	 * -0成功
	 * -LIBUSB_ERROR_NO_DEVICE（如果设备自打开以来就已断开连接）
	 * -其他LIBUSB_ERROR代码出现其他故障
	 */
	int (*release_interface)(struct libusb_device_handle *handle, int interface_number);

	/* Set the alternate setting for an interface.
	 *
	 * You will only ever be asked to set the alternate setting for an
	 * interface which was successfully claimed earlier.
	 *
	 * It's OK for this function to block.
	 *
	 * Return:
	 * - 0 on success
	 * - LIBUSB_ERROR_NOT_FOUND if the alternate setting does not exist
	 * - LIBUSB_ERROR_NO_DEVICE if the device has been disconnected since it
	 *   was opened
	 * - another LIBUSB_ERROR code on other failure
	 *
	 * 设置接口的备用设置。
	 * 仅要求您为先前已成功声明的接口设置备用设置。
	 * 可以阻止此功能。
	 * 返回值：-0表示成功
	 * -LIBUSB_ERROR_NOT_FOUND（如果不存在其他设置）
	 * -LIBUSB_ERROR_NO_DEVICE（如果设备自打开以来就已断开连接）
	 * -其他LIBUSB_ERROR代码出现其他故障
	 */
	int (*set_interface_altsetting)(struct libusb_device_handle *handle,
			int interface_number, int altsetting);

	/* Clear a halt/stall condition on an endpoint.
	 *
	 * It's OK for this function to block.
	 *
	 * Return:
	 * - 0 on success
	 * - LIBUSB_ERROR_NOT_FOUND if the endpoint does not exist
	 * - LIBUSB_ERROR_NO_DEVICE if the device has been disconnected since it
	 *   was opened
	 * - another LIBUSB_ERROR code on other failure
	 *
	 * 清除端点上的停止/停止条件。
	 * 可以阻止此功能。
	 * 返回值：-0表示成功
	 * -LIBUSB_ERROR_NOT_FOUND（如果端点不存在）
	 * -LIBUSB_ERROR_NO_DEVICE（如果设备自打开以来就已断开连接）
	 * -其他LIBUSB_ERROR代码出现其他故障
	 */
	int (*clear_halt)(struct libusb_device_handle *handle,
			unsigned char endpoint);

	/* Perform a USB port reset to reinitialize a device.
	 *
	 * If possible, the handle should still be usable after the reset
	 * completes, assuming that the device descriptors did not change during
	 * reset and all previous interface state can be restored.
	 *
	 * If something changes, or you cannot easily locate/verify the resetted
	 * device, return LIBUSB_ERROR_NOT_FOUND. This prompts the application
	 * to close the old handle and re-enumerate the device.
	 *
	 * Return:
	 * - 0 on success
	 * - LIBUSB_ERROR_NOT_FOUND if re-enumeration is required, or if the device
	 *   has been disconnected since it was opened
	 * - another LIBUSB_ERROR code on other failure
	 *
	 * 执行USB端口重置以重新初始化设备。
	 * 如果可能，该句柄应该在重置完成后仍然可用，假设设备描述符在重置期间没有更改并且可以恢复所有先前的接口状态。
	 * 如果发生更改，或者您无法轻松找到/验证已重置的设备，请返回LIBUSB_ERROR_NOT_FOUND。 这会提示应用程序关闭旧句柄并重新枚举设备。
	 * 返回值：
	 * -0表示成功
	 * -LIBUSB_ERROR_NOT_FOUND（如果需要重新枚举，或者设备自打开以来就已断开连接）
	 * -其他LIBUSB_ERROR代码出现其他故障
	 */
	int (*reset_device)(struct libusb_device_handle *handle);

	/* Alloc num_streams usb3 bulk streams on the passed in endpoints
	 * 传入端点上的Alloc num_streams usb3块流
	 */
	int (*alloc_streams)(struct libusb_device_handle *handle,
		uint32_t num_streams, unsigned char *endpoints, int num_endpoints);

	/* Free usb3 bulk streams allocated with alloc_streams
	 * 使用alloc_streams分配的免费USB3批量流
	 */
	int (*free_streams)(struct libusb_device_handle *handle,
		unsigned char *endpoints, int num_endpoints);

	/* Determine if a kernel driver is active on an interface. Optional.
	 *
	 * The presence of a kernel driver on an interface indicates that any
	 * calls to claim_interface would fail with the LIBUSB_ERROR_BUSY code.
	 *
	 * Return:
	 * - 0 if no driver is active
	 * - 1 if a driver is active
	 * - LIBUSB_ERROR_NO_DEVICE if the device has been disconnected since it
	 *   was opened
	 * - another LIBUSB_ERROR code on other failure
	 *
	 * 确定内核驱动程序在接口上是否处于活动状态。 可选的。
	 * 接口上存在内核驱动程序，这表明使用LIBUSB_ERROR_BUSY代码对Claim_interface的任何调用都会失败。
	 * 返回值：
	 * -0，如果没有驱动程序处于活动状态
	 * -1，如果驱动程序处于活动状态
	 * -LIBUSB_ERROR_NO_DEVICE，如果设备自打开以来就已断开连接
	 * -另一个LIBUSB_ERROR代码出现其他故障
	 */
	int (*kernel_driver_active)(struct libusb_device_handle *handle,
			int interface_number);

	/* Detach a kernel driver from an interface. Optional.
	 *
	 * After detaching a kernel driver, the interface should be available
	 * for claim.
	 *
	 * Return:
	 * - 0 on success
	 * - LIBUSB_ERROR_NOT_FOUND if no kernel driver was active
	 * - LIBUSB_ERROR_INVALID_PARAM if the interface does not exist
	 * - LIBUSB_ERROR_NO_DEVICE if the device has been disconnected since it
	 *   was opened
	 * - another LIBUSB_ERROR code on other failure
	 *
	 * 从接口分离内核驱动程序。 可选的。
	 * 分离内核驱动程序后，该接口应可用于声明。
	 * 返回值：
	 * -成功时为0
	 * -如果没有内核驱动程序处于活动状态，则为LIBUSB_ERROR_NOT_FOUND
	 * -如果该接口不存在，则为LIBUSB_ERROR_INVALID_PARAM
	 * -如果该设备自打开以来已断开连接，则为LIBUSB_ERROR_NO_DEVICE
	 * -在其他故障时，另一个LIBUSB_ERROR代码
	 */
	int (*detach_kernel_driver)(struct libusb_device_handle *handle,
			int interface_number);

	/* Attach a kernel driver to an interface. Optional.
	 *
	 * Reattach a kernel driver to the device.
	 *
	 * Return:
	 * - 0 on success
	 * - LIBUSB_ERROR_NOT_FOUND if no kernel driver was active
	 * - LIBUSB_ERROR_INVALID_PARAM if the interface does not exist
	 * - LIBUSB_ERROR_NO_DEVICE if the device has been disconnected since it
	 *   was opened
	 * - LIBUSB_ERROR_BUSY if a program or driver has claimed the interface,
	 *   preventing reattachment
	 * - another LIBUSB_ERROR code on other failure
	 *
	 * 将内核驱动程序附加到接口。 可选的。
	 * 将内核驱动程序重新连接到设备。
	 * 返回值：
	 * -成功时为0
	 * -如果没有内核驱动程序处于活动状态，则为LIBUSB_ERROR_NOT_FOUND
	 * -如果该接口不存在，则为LIBUSB_ERROR_INVALID_PARAM
	 * -如果该设备自打开以来已断开连接，则为LIBUSB_ERROR_NO_DEVICE
	 * -如果程序或驱动程序声明了该接口，则阻止了LIBUSB_ERROR_BUSY-防止重新连接
	 * -另一个 其他失败的LIBUSB_ERROR代码
	 */
	int (*attach_kernel_driver)(struct libusb_device_handle *handle,
			int interface_number);

	/* Destroy a device. Optional.
	 *
	 * This function is called when the last reference to a device is
	 * destroyed. It should free any resources allocated in the get_device_list
	 * path.
	 *
	 * 销毁设备。 可选的。
	 * 当对设备的最后一个引用被破坏时，将调用此函数。 它应该释放在get_device_list路径中分配的所有资源。
	 */
	void (*destroy_device)(struct libusb_device *dev);

	/* Submit a transfer. Your implementation should take the transfer,
	 * morph it into whatever form your platform requires, and submit it
	 * asynchronously.
	 *
	 * This function must not block.
	 *
	 * This function gets called with the flying_transfers_lock locked!
	 *
	 * Return:
	 * - 0 on success
	 * - LIBUSB_ERROR_NO_DEVICE if the device has been disconnected
	 * - another LIBUSB_ERROR code on other failure
	 *
	 * 提交传输。 您的实现应采用传输方式，将其转换为平台所需的任何形式，然后异步提交。
	 * 此功能不得阻止。
	 * 在锁定flying_transfers_lock的情况下调用此函数！
	 * 返回值：
	 * -0表示成功
	 * -LIBUSB_ERROR_NO_DEVICE（如果设备已断开连接）
	 * -另一个LIBUSB_ERROR代码出现其他故障
	 */
	int (*submit_transfer)(struct usbi_transfer *itransfer);

	/* Cancel a previously submitted transfer.
	 *
	 * This function must not block. The transfer cancellation must complete
	 * later, resulting in a call to usbi_handle_transfer_cancellation()
	 * from the context of handle_events.
	 *
	 * 取消先前提交的传输。
	 * 此功能不得阻止。 传输取消必须稍后完成，从而导致从handle_events上下文调用usbi_handle_transfer_cancellation()。
	 */
	int (*cancel_transfer)(struct usbi_transfer *itransfer);

	/* Clear a transfer as if it has completed or cancelled, but do not
	 * report any completion/cancellation to the library. You should free
	 * all private data from the transfer as if you were just about to report
	 * completion or cancellation.
	 *
	 * This function might seem a bit out of place. It is used when libusb
	 * detects a disconnected device - it calls this function for all pending
	 * transfers before reporting completion (with the disconnect code) to
	 * the user. Maybe we can improve upon this internal interface in future.
	 *
	 * 清除传输，就好像传输已完成或取消一样，但不要向磁带库报告任何完成/取消。 您应该从传输中释放所有私人数据，就好像您将要报告完成或取消一样。
	 * 该功能可能看起来有点不合适。 当libusb检测到断开连接的设备时使用它-在向用户报告完成情况（带有断开连接代码）之前，它将为所有挂起的传输调用此函数。 也许将来我们可以改进此内部接口。
	 */
	void (*clear_transfer_priv)(struct usbi_transfer *itransfer);

	/* Handle any pending events. This involves monitoring any active
	 * transfers and processing their completion or cancellation.
	 *
	 * The function is passed an array of pollfd structures (size nfds)
	 * as a result of the poll() system call. The num_ready parameter
	 * indicates the number of file descriptors that have reported events
	 * (i.e. the poll() return value). This should be enough information
	 * for you to determine which actions need to be taken on the currently
	 * active transfers.
	 *
	 * For any cancelled transfers, call usbi_handle_transfer_cancellation().
	 * For completed transfers, call usbi_handle_transfer_completion().
	 * For control/bulk/interrupt transfers, populate the "transferred"
	 * element of the appropriate usbi_transfer structure before calling the
	 * above functions. For isochronous transfers, populate the status and
	 * transferred fields of the iso packet descriptors of the transfer.
	 *
	 * This function should also be able to detect disconnection of the
	 * device, reporting that situation with usbi_handle_disconnect().
	 *
	 * When processing an event related to a transfer, you probably want to
	 * take usbi_transfer.lock to prevent races. See the documentation for
	 * the usbi_transfer structure.
	 *
	 * Return 0 on success, or a LIBUSB_ERROR code on failure.
	 *
	 * 处理所有未决事件。 这涉及监视任何活动的传输并处理其完成或取消。
	 * 作为poll()系统调用的结果，该函数将传递一个pollfd结构数组（大小为nfds）。
	 * num_ready参数指示已报告事件（即poll()返回值）的文件描述符的数量。 这应该是足够的信息，以便您确定需要对当前活动的传输执行哪些操作。
	 * 对于任何取消的传输，请调用usbi_handle_transfer_cancellation()。
	 * 要完成传输，请致电usbi_handle_transfer_completion()。
	 * 对于控制/批量/中断传输，在调用上述函数之前，请填充适当的usbi_transfer结构的“transferred”元素。
	 * 对于同步传输，请填充传输的iso数据包描述符的状态和已传输字段。
	 * 此函数还应该能够检测设备的断开连接，并通过usbi_handle_disconnect()报告这种情况。
	 * 在处理与传输相关的事件时，您可能希望使用usbi_transfer.lock来防止比赛。 请参阅有关usbi_transfer结构的文档。
	 * 成功返回0，失败返回LIBUSB_ERROR代码。
	 */
	int (*handle_events)(struct libusb_context *ctx, struct pollfd *fds,
			POLL_NFDS_TYPE nfds, int num_ready);

	/* Get time from specified clock. At least two clocks must be implemented
	 by the backend: USBI_CLOCK_REALTIME, and USBI_CLOCK_MONOTONIC.

	 Description of clocks:
	 USBI_CLOCK_REALTIME : clock returns time since system epoch.
	 USBI_CLOCK_MONOTONIC: clock returns time since unspecified start
	 time (usually boot).

	 从指定的时钟获取时间。 后端必须至少实现两个时钟：USBI_CLOCK_REALTIME 和 USBI_CLOCK_MONOTONIC。
	 时钟说明：
	 USBI_CLOCK_REALTIME：时钟返回自系统纪元以来的时间。
     USBI_CLOCK_MONOTONIC：时钟返回自未指定的开始时间起的时间（通常为启动时间）。
	 */
	int (*clock_gettime)(int clkid, struct timespec *tp);

#ifdef USBI_TIMERFD_AVAILABLE
	/* clock ID of the clock that should be used for timerfd
	 * 应该用于timerfd的时钟的时钟ID
	 */
	clockid_t (*get_timerfd_clockid)(void);
#endif

	/* Number of bytes to reserve for per-device private backend data.
	 * This private data area is accessible through the "os_priv" field of
	 * struct libusb_device.
	 * 为每个设备的专用后端数据保留的字节数。可通过结构libusb_device的“os_priv”字段访问此私有数据区域。
	 */
	size_t device_priv_size;

	/* Number of bytes to reserve for per-handle private backend data.
	 * This private data area is accessible through the "os_priv" field of
	 * struct libusb_device.
	 * 每个句柄专用后端数据要保留的字节数。 可通过结构libusb_device的“ os_priv”字段访问此私有数据区域。
	 */
	size_t device_handle_priv_size;

	/* Number of bytes to reserve for per-transfer private backend data.
	 * This private data area is accessible by calling
	 * usbi_transfer_get_os_priv() on the appropriate usbi_transfer instance.
	 * 每次传输专用后端数据要保留的字节数。通过在适当的usbi_transfer实例上调用usbi_transfer_get_os_priv()可以访问此私有数据区域。
	 */
	size_t transfer_priv_size;

	/* Mumber of additional bytes for os_priv for each iso packet.
	 * Can your backend use this?
	 * 每个iso数据包的os_priv附加字节数。
     * 您的后端可以使用吗？
	 */
	/* FIXME: linux can't use this any more. if other OS's cannot either, then remove this
	 * linux不能再使用它了。如果其他操作系统也不能，请删除此
	 */
	size_t add_iso_packet_size;
};

extern const struct usbi_os_backend * const usbi_backend;

extern const struct usbi_os_backend android_usbfs_backend;	// XXX added for mainly non-rooted Android 为主要是非root用户的Android添加
extern const struct usbi_os_backend linux_usbfs_backend;
extern const struct usbi_os_backend darwin_backend;
extern const struct usbi_os_backend openbsd_backend;
extern const struct usbi_os_backend netbsd_backend;
extern const struct usbi_os_backend windows_backend;
extern const struct usbi_os_backend wince_backend;

extern struct list_head active_contexts_list;
extern usbi_mutex_static_t active_contexts_lock;

#endif
