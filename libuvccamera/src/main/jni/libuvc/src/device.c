/*********************************************************************
 *********************************************************************/
/*********************************************************************
 * add and modified some functions to support non-rooted Android devices
 * and modified some function for optimaization with gcc
 * Copyright (C) 2014-2016 saki@serenegiant All rights reserved.
 *********************************************************************/
/*********************************************************************
 * Software License Agreement (BSD License)
 *
 *  Copyright (C) 2010-2012 Ken Tossell
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above
 *     copyright notice, this list of conditions and the following
 *     disclaimer in the documentation and/or other materials provided
 *     with the distribution.
 *   * Neither the name of the author nor other contributors may be
 *     used to endorse or promote products derived from this software
 *     without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 *  FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 *  COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 *  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 *  BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 *  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 *  CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 *  LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 *  ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *  POSSIBILITY OF SUCH DAMAGE.
 *********************************************************************/
/**
 * @defgroup device Device handling and enumeration
 * @brief Support for finding, inspecting and opening UVC devices
 */

#define LOCAL_DEBUG 0

#define LOG_TAG "libuvc/device"
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

#include "libuvc/libuvc.h"
#include "libuvc/libuvc_internal.h"

// set this 1 attach/detach kernel driver by libuvc, set this 0 automatically attach/detach by libusb
// 通过libuvc设置此1附加/分离内核驱动程序，通过libusb设置此0自动附加/分离内核驱动程序
#define UVC_DETACH_ATTACH 0

int uvc_already_open(uvc_context_t *ctx, struct libusb_device *usb_dev);
void uvc_free_devh(uvc_device_handle_t *devh);

uvc_error_t uvc_get_device_info(uvc_device_t *dev, uvc_device_info_t **info);
void uvc_free_device_info(uvc_device_info_t *info);

uvc_error_t uvc_scan_control(uvc_device_t *dev, uvc_device_info_t *info);
uvc_error_t uvc_parse_vc(uvc_device_t *dev, uvc_device_info_t *info,
		const unsigned char *block, size_t block_size);
uvc_error_t uvc_parse_vc_extension_unit(uvc_device_t *dev,
		uvc_device_info_t *info, const unsigned char *block, size_t block_size);
uvc_error_t uvc_parse_vc_header(uvc_device_t *dev, uvc_device_info_t *info,
		const unsigned char *block, size_t block_size);
uvc_error_t uvc_parse_vc_input_terminal(uvc_device_t *dev,
		uvc_device_info_t *info, const unsigned char *block, size_t block_size);
uvc_error_t uvc_parse_vc_processing_unit(uvc_device_t *dev,
		uvc_device_info_t *info, const unsigned char *block, size_t block_size);

uvc_error_t uvc_scan_streaming(uvc_device_t *dev, uvc_device_info_t *info,
		int interface_idx);
uvc_error_t uvc_parse_vs(uvc_device_t *dev, uvc_device_info_t *info,
		uvc_streaming_interface_t *stream_if, const unsigned char *block,
		size_t block_size);
uvc_error_t uvc_parse_vs_format_uncompressed(
		uvc_streaming_interface_t *stream_if, const unsigned char *block,
		size_t block_size);
uvc_error_t uvc_parse_vs_format_mjpeg(uvc_streaming_interface_t *stream_if,
		const unsigned char *block, size_t block_size);
uvc_error_t uvc_parse_vs_frame_uncompressed(
		uvc_streaming_interface_t *stream_if, const unsigned char *block,
		size_t block_size);
uvc_error_t uvc_parse_vs_frame_format(uvc_streaming_interface_t *stream_if,
	    const unsigned char *block,
	    size_t block_size);
uvc_error_t uvc_parse_vs_frame_frame(uvc_streaming_interface_t *stream_if,
	    const unsigned char *block,
	    size_t block_size);
uvc_error_t uvc_parse_vs_input_header(uvc_streaming_interface_t *stream_if,
		const unsigned char *block, size_t block_size);

void _uvc_status_callback(struct libusb_transfer *transfer);

/** @internal
 * @brief Test whether the specified USB device has been opened as a UVC device
 * @ingroup device
 *
 * @param ctx Context in which to search for the UVC device
 * @param usb_dev USB device to find
 * @return true if the device is open in this context
 *
 * 测试指定的USB设备是否已作为UVC设备打开
 */
int uvc_already_open(uvc_context_t *ctx, struct libusb_device *usb_dev) {
	uvc_device_handle_t *devh;

	DL_FOREACH(ctx->open_devices, devh)
	{
		if (usb_dev == devh->dev->usb_dev)
			return 1;
	}

	return 0;
}

/** @brief Finds a camera identified by vendor, product and/or serial number
 * @ingroup device
 *
 * @param[in] ctx UVC context in which to search for the camera
 * @param[out] dev Reference to the camera, or NULL if not found
 * @param[in] vid Vendor ID number, optional
 * @param[in] pid Product ID number, optional
 * @param[in] sn Serial number or NULL
 * @return Error finding device or UVC_SUCCESS
 *
 * 查找由供应商，产品和/或序列号标识的摄像机
 */
uvc_error_t uvc_find_device(uvc_context_t *ctx, uvc_device_t **dev, int vid,
		int pid, const char *sn) {
	uvc_error_t ret = UVC_SUCCESS;

	uvc_device_t **list;
	uvc_device_t *test_dev;
	int dev_idx;
	int found_dev;

	UVC_ENTER();

	ret = uvc_get_device_list(ctx, &list);

	if (UNLIKELY(ret != UVC_SUCCESS)) {
		UVC_EXIT(ret);
		return ret;
	}

	dev_idx = 0;
	found_dev = 0;

	while (!found_dev && (test_dev = list[dev_idx++]) != NULL ) {
		uvc_device_descriptor_t *desc;

		if (uvc_get_device_descriptor(test_dev, &desc) != UVC_SUCCESS)
			continue;

		if ((!vid || desc->idVendor == vid) && (!pid || desc->idProduct == pid)
				&& (!sn || (desc->serialNumber && !strcmp(desc->serialNumber, sn))))
			found_dev = 1;

		uvc_free_device_descriptor(desc);
	}

	if (found_dev)
		uvc_ref_device(test_dev);

	uvc_free_device_list(list, 1);

	if (found_dev) {
		*dev = test_dev;
		UVC_EXIT(UVC_SUCCESS);
		return UVC_SUCCESS;
	} else {
		UVC_EXIT(UVC_ERROR_NO_DEVICE);
		return UVC_ERROR_NO_DEVICE;
	}
}

/**
 * XXX add for non-rooted Android devices
 * get uvc_device_t from specified vid/pid/serial
 * return UVC_ERROR_NO_DEVICE when device not found
 * return UVC_SUCCESS and set uvc_device_t when specific device found
 *
 * 为非根目录的Android设备添加，从指定的 vid/pid/serial 获取uvc_device_t，
 * 当找不到设备时返回UVC_ERROR_NO_DEVICE
 * 返回UVC_SUCCESS，并在找到特定设备时设置uvc_device_t
 */
uvc_error_t uvc_find_device2(uvc_context_t *ctx, uvc_device_t **device, int vid,
		int pid, const char *sn, int fd) {

	UVC_ENTER();

	LOGD("call libusb_find_device");
	struct libusb_device *usb_dev = libusb_find_device(ctx->usb_ctx, vid, pid, sn, fd);

	if (usb_dev) {
		*device = malloc(sizeof(uvc_device_t/* *device */));
		(*device)->ctx = ctx;
		(*device)->ref = 0;
		(*device)->usb_dev = usb_dev;
		libusb_set_device_fd(usb_dev, fd);	// assign fd to libusb_device for non-rooted Android devices  将fd分配给非root用户的Android设备的libusb_device
		uvc_ref_device(*device);
		UVC_EXIT(UVC_SUCCESS);
		return UVC_SUCCESS;
	} else {
		LOGE("could not find specific device");
		*device = NULL;
	}
	UVC_EXIT(UVC_ERROR_NO_DEVICE);
	return UVC_ERROR_NO_DEVICE;
}

/**
 * XXX add for non-rooted Android device, >= Android7
 * generate fake libusb_device according to specific params
 * and set it to uvc_device_t to access UVC device on Android7 or later
 *
 * 为非root用户的Android设备添加，>= Android7 根据特定的参数生成伪造的libusb_device并将其设置为uvc_device_t，以访问Android7或更高版本上的UVC设备
 */
uvc_error_t uvc_get_device_with_fd(uvc_context_t *ctx, uvc_device_t **device,
		int vid, int pid, const char *serial, int fd, int busnum, int devaddr) {

	ENTER();

	LOGD("call libusb_get_device_with_fd");
	struct libusb_device *usb_dev = libusb_get_device_with_fd(ctx->usb_ctx, vid, pid, serial, fd, busnum, devaddr);

	if (LIKELY(usb_dev)) {
		*device = malloc(sizeof(uvc_device_t/* *device */));
		(*device)->ctx = ctx;
		(*device)->ref = 0;
		(*device)->usb_dev = usb_dev;
//		libusb_set_device_fd(usb_dev, fd);	// assign fd to libusb_device for non-rooted Android devices  将fd分配给非root用户的Android设备的libusb_device
		uvc_ref_device(*device);
		UVC_EXIT(UVC_SUCCESS);
		RETURN(UVC_SUCCESS, int);
	} else {
		LOGE("could not find specific device");
		*device = NULL;
		RETURN(UVC_ERROR_NO_DEVICE, int);
	}

}

/** @brief Get the number of the bus to which the device is attached
 * @ingroup device
 *
 * 获取设备连接的总线号
 */
uint8_t uvc_get_bus_number(uvc_device_t *dev) {
	return libusb_get_bus_number(dev->usb_dev);
}

/** @brief Get the number assigned to the device within its bus
 * @ingroup device
 *
 * 获取分配给设备的总线编号
 */
uint8_t uvc_get_device_address(uvc_device_t *dev) {
	return libusb_get_device_address(dev->usb_dev);
}

/** @brief Open a UVC device
 * @ingroup device
 *
 * @param dev Device to open
 * @param[out] devh Handle on opened device
 * @return Error opening device or SUCCESS
 *
 * 打开UVC设备
 */
uvc_error_t uvc_open(uvc_device_t *dev, uvc_device_handle_t **devh) {
	uvc_error_t ret;
	struct libusb_device_handle *usb_devh;
	uvc_device_handle_t *internal_devh;
	struct libusb_device_descriptor desc;

	UVC_ENTER();

	ret = libusb_open(dev->usb_dev, &usb_devh);
	UVC_DEBUG("libusb_open() = %d", ret);

	if (UNLIKELY(ret != UVC_SUCCESS)) {
		UVC_EXIT(ret);
		return ret;
	}

	uvc_ref_device(dev);

	internal_devh = calloc(1, sizeof(*internal_devh));
	internal_devh->dev = dev;
	internal_devh->usb_devh = usb_devh;
	internal_devh->reset_on_release_if = 0;	// XXX
	ret = uvc_get_device_info(dev, &(internal_devh->info));
	pthread_mutex_init(&internal_devh->status_mutex, NULL);	// XXX saki

	if (UNLIKELY(ret != UVC_SUCCESS))
		goto fail2;	// uvc_claim_if was not called yet and we don't need to call uvc_release_if 尚未调用uvc_claim_if，我们不需要调用uvc_release_if
#if !UVC_DETACH_ATTACH
	/* enable automatic attach/detach kernel driver on supported platforms in libusb
	 * 在libusb中支持的平台上启用自动附加/分离内核驱动程序
	 */
	libusb_set_auto_detach_kernel_driver(usb_devh, 1);
#endif
	UVC_DEBUG("claiming control interface %d", internal_devh->info->ctrl_if.bInterfaceNumber);
	ret = uvc_claim_if(internal_devh, internal_devh->info->ctrl_if.bInterfaceNumber);
	if (UNLIKELY(ret != UVC_SUCCESS))
		goto fail;

    // 获取给定设备的USB设备描述符。
	libusb_get_device_descriptor(dev->usb_dev, &desc);
	internal_devh->is_isight = (desc.idVendor == 0x05ac && desc.idProduct == 0x8501);
	if (internal_devh->info->ctrl_if.bEndpointAddress) {
		UVC_DEBUG("status check transfer:bEndpointAddress=0x%02x", internal_devh->info->ctrl_if.bEndpointAddress);
		// 为libusb传输分配指定数量的同步数据包描述符
		internal_devh->status_xfer = libusb_alloc_transfer(0);
		if (UNLIKELY(!internal_devh->status_xfer)) {
			ret = UVC_ERROR_NO_MEM;
			goto fail;
		}

        // Helper函数填充中断传输所需的libusb_transfer字段。
		libusb_fill_interrupt_transfer(internal_devh->status_xfer, usb_devh,
				internal_devh->info->ctrl_if.bEndpointAddress,
				internal_devh->status_buf, sizeof(internal_devh->status_buf),
				_uvc_status_callback, internal_devh, 0);
		// 提交传输。 此功能将触发USB传输，然后立即返回。 发送中断
		ret = libusb_submit_transfer(internal_devh->status_xfer);
		UVC_DEBUG("libusb_submit_transfer() = %d", ret);

		if (UNLIKELY(ret)) {
			LOGE("device has a status interrupt endpoint, but unable to read from it");
			goto fail;
		}
	} else {
		LOGE("internal_devh->info->ctrl_if.bEndpointAddress is null");
	}

	if (dev->ctx->own_usb_ctx && dev->ctx->open_devices == NULL) {
		/* Since this is our first device, we need to spawn the event handler thread
		 * 由于这是我们的第一台设备，因此我们需要生成事件处理程序线程
		 */
		uvc_start_handler_thread(dev->ctx);
	}

	DL_APPEND(dev->ctx->open_devices, internal_devh);
	*devh = internal_devh;

	UVC_EXIT(ret);

	return ret;

fail:
	uvc_release_if(internal_devh, internal_devh->info->ctrl_if.bInterfaceNumber);	// XXX crash, assume when uvc_get_device_info failed.  崩溃，假设uvc_get_device_info失败。
fail2:
#if !UVC_DETACH_ATTACH
	/* disable automatic attach/detach kernel driver on supported platforms in libusb
	 * 在libusb中支持的平台上禁用自动附加/分离内核驱动程序
	 */
	libusb_set_auto_detach_kernel_driver(usb_devh, 0);
#endif
	libusb_close(usb_devh);
	uvc_unref_device(dev);
	uvc_free_devh(internal_devh);

	UVC_EXIT(ret);

	return ret;
}

/**
 * @internal
 * @brief Parses the complete device descriptor for a device
 * @ingroup device
 * @note Free *info with uvc_free_device_info when you're done
 *
 * @param dev Device to parse descriptor for
 * @param info Where to store a pointer to the new info struct
 *
 * 解析设备的完整设备描述符
 * 完成后，使用uvc_free_device_info释放*info
 */
uvc_error_t uvc_get_device_info(uvc_device_t *dev, uvc_device_info_t **info) {
	uvc_error_t ret;
	uvc_device_info_t *internal_info;

	UVC_ENTER();

	internal_info = calloc(1, sizeof(*internal_info));
	if (!internal_info) {
		UVC_EXIT(UVC_ERROR_NO_MEM);
		return UVC_ERROR_NO_MEM;
	}
	if (libusb_get_config_descriptor(dev->usb_dev, 0, &(internal_info->config)) != 0) {
//	if (libusb_get_active_config_descriptor(dev->usb_dev, &(internal_info->config)) != 0) {
		// XXX assume libusb_get_active_config_descriptor　is better
		// but some buggy device will return error when get active config.
		// so we will use libusb_get_config_descriptor...
		free(internal_info);
		UVC_EXIT(UVC_ERROR_IO);
		return UVC_ERROR_IO;
	}

	ret = uvc_scan_control(dev, internal_info);
	if (UNLIKELY(ret)) {
		uvc_free_device_info(internal_info);
		UVC_EXIT(ret);
		return ret;
	}

	*info = internal_info;

	UVC_EXIT(ret);
	return ret;
}

/**
 * @internal
 * @brief Frees the device descriptor for a device
 * @ingroup device
 *
 * @param info Which device info block to free
 *
 * 释放设备的设备描述符
 */
void uvc_free_device_info(uvc_device_info_t *info) {
	uvc_input_terminal_t *input_term, *input_term_tmp;
	uvc_output_terminal_t *output_term, *output_term_tmp;
	uvc_processing_unit_t *proc_unit, *proc_unit_tmp;
	uvc_extension_unit_t *ext_unit, *ext_unit_tmp;

	uvc_streaming_interface_t *stream_if, *stream_if_tmp;
	uvc_format_desc_t *format, *format_tmp;
	uvc_frame_desc_t *frame, *frame_tmp;

	UVC_ENTER();

	DL_FOREACH_SAFE(info->ctrl_if.input_term_descs, input_term, input_term_tmp)
	{
		DL_DELETE(info->ctrl_if.input_term_descs, input_term);
		free(input_term);
	}

	DL_FOREACH_SAFE(info->ctrl_if.output_term_descs, output_term, output_term_tmp)
	{
		DL_DELETE(info->ctrl_if.output_term_descs, output_term);
		free(output_term);
	}

	DL_FOREACH_SAFE(info->ctrl_if.processing_unit_descs, proc_unit, proc_unit_tmp)
	{
		DL_DELETE(info->ctrl_if.processing_unit_descs, proc_unit);
		free(proc_unit);
	}

	DL_FOREACH_SAFE(info->ctrl_if.extension_unit_descs, ext_unit, ext_unit_tmp)
	{
		DL_DELETE(info->ctrl_if.extension_unit_descs, ext_unit);
		free(ext_unit);
	}

	DL_FOREACH_SAFE(info->stream_ifs, stream_if, stream_if_tmp)
	{
		DL_FOREACH_SAFE(stream_if->format_descs, format, format_tmp)
		{
			DL_FOREACH_SAFE(format->frame_descs, frame, frame_tmp)
			{
				if (frame->intervals)
					free(frame->intervals);

				DL_DELETE(format->frame_descs, frame);
				free(frame);
			}

			DL_DELETE(stream_if->format_descs, format);
			free(format);
		}

		DL_DELETE(info->stream_ifs, stream_if);
		free(stream_if->bmaControls);	// XXX
		free(stream_if);
	}

	if (info->config)
		libusb_free_config_descriptor(info->config);

	free(info);

	UVC_EXIT_VOID();
}

/**
 * @brief Get a descriptor that contains the general information about
 * a device
 * @ingroup device
 *
 * Free *desc with uvc_free_device_descriptor when you're done.
 *
 * @param dev Device to fetch information about
 * @param[out] desc Descriptor structure
 * @return Error if unable to fetch information, else SUCCESS
 *
 * 获取包含有关设备常规信息的描述符
 * 完成后，使用uvc_free_device_descriptor释放*desc。
 */
uvc_error_t uvc_get_device_descriptor(uvc_device_t *dev,
		uvc_device_descriptor_t **desc) {

	uvc_device_descriptor_t *desc_internal;
	struct libusb_device_descriptor usb_desc;
	struct libusb_device_handle *usb_devh;
	uvc_error_t ret;

	UVC_ENTER();

	ret = libusb_get_device_descriptor(dev->usb_dev, &usb_desc);

	if (UNLIKELY(ret)) {
		UVC_EXIT(ret);
		return ret;
	}

	desc_internal = calloc(1, sizeof(*desc_internal));
	desc_internal->idVendor = usb_desc.idVendor;
	desc_internal->idProduct = usb_desc.idProduct;

	if (libusb_open(dev->usb_dev, &usb_devh) == 0) {
		unsigned char str_buf[255];	// XXX 64 => 255
		// get serialNumber
		// 获取序列号
		int str_bytes = libusb_get_string_descriptor_ascii(usb_devh,
			usb_desc.iSerialNumber, str_buf, sizeof(str_buf));

		if (str_bytes > 0)
			desc_internal->serialNumber = strdup((const char*) str_buf);

		// get manufacturer
		// 得到制造商
		str_bytes = libusb_get_string_descriptor_ascii(usb_devh,
			usb_desc.iManufacturer, str_buf, sizeof(str_buf));

		if (str_bytes > 0)
			desc_internal->manufacturer = strdup((const char*) str_buf);

		// get product name
		// 获取产品名称
		str_bytes = libusb_get_string_descriptor_ascii(usb_devh,
			usb_desc.iProduct, str_buf, sizeof(str_buf));

		if (str_bytes > 0)
			desc_internal->product = strdup((const char*) str_buf);

		libusb_close(usb_devh);
	} else {
		UVC_DEBUG("can't open device %04x:%04x, not fetching serial",
				usb_desc.idVendor, usb_desc.idProduct);
	}

	*desc = desc_internal;

	UVC_EXIT(ret);
	return ret;
}

/**
 * @brief Frees a device descriptor created with uvc_get_device_descriptor
 * @ingroup device
 *
 * @param desc Descriptor to free
 *
 * 释放使用uvc_get_device_descriptor创建的设备描述符
 */
void uvc_free_device_descriptor(uvc_device_descriptor_t *desc) {
	UVC_ENTER();

	if (desc->serialNumber)
		free((void*) desc->serialNumber);

	if (desc->manufacturer)
		free((void*) desc->manufacturer);

	if (desc->product)
		free((void*) desc->product);

	free(desc);

	UVC_EXIT_VOID();
}

/**
 * @brief Get a list of the UVC devices attached to the system
 * @ingroup device
 *
 * @note Free the list with uvc_free_device_list when you're done.
 *
 * @param ctx UVC context in which to list devices
 * @param list List of uvc_device structures
 * @return Error if unable to list devices, else SUCCESS
 *
 * 获取连接到系统的UVC设备的列表
 * 完成后，使用uvc_free_device_list释放列表。
 */
uvc_error_t uvc_get_device_list(uvc_context_t *ctx, uvc_device_t ***list) {
	uvc_error_t ret;
	struct libusb_device **usb_dev_list;
	struct libusb_device *usb_dev;
	int num_usb_devices;

	uvc_device_t **list_internal;
	int num_uvc_devices;

	/* per device 每个设备 */
	int dev_idx;
	struct libusb_device_handle *usb_devh;
	struct libusb_config_descriptor *config;
	struct libusb_device_descriptor desc;
	uint8_t got_interface;

	/* per interface 每个接口 */
	int interface_idx;
	const struct libusb_interface *interface;

	/* per altsetting  用于altsetting */
	int altsetting_idx;
	const struct libusb_interface_descriptor *if_desc;

	UVC_ENTER();

	num_usb_devices = libusb_get_device_list(ctx->usb_ctx, &usb_dev_list);

	if (UNLIKELY(num_usb_devices < 0)) {
		UVC_EXIT(UVC_ERROR_IO);
		return UVC_ERROR_IO;
	}

	list_internal = malloc(sizeof(*list_internal));
	*list_internal = NULL;

	num_uvc_devices = 0;
	dev_idx = -1;

	while ((usb_dev = usb_dev_list[++dev_idx]) != NULL ) {
		usb_devh = NULL;
		got_interface = 0;

		if (libusb_get_config_descriptor(usb_dev, 0, &config) != 0)
			continue;

		if (libusb_get_device_descriptor (usb_dev, &desc) != LIBUSB_SUCCESS)
			continue;

		// Special case for Imaging Source cameras
		// 成像源相机的特殊情况
		if ((0x199e == desc.idVendor) && (0x8101 == desc.idProduct)) {
			got_interface = 1;
		} else {
			for (interface_idx = 0;
					!got_interface && interface_idx < config->bNumInterfaces;
					++interface_idx) {
				interface = &config->interface[interface_idx];

				for (altsetting_idx = 0;
						!got_interface && altsetting_idx < interface->num_altsetting;
						++altsetting_idx) {
					if_desc = &interface->altsetting[altsetting_idx];

					/* Video, Streaming  视频流 */
					if (if_desc->bInterfaceClass == 14
							&& if_desc->bInterfaceSubClass == 2) {
						got_interface = 1;
					}
				}
			}
		}

		libusb_free_config_descriptor(config);

		if (got_interface) {
			uvc_device_t *uvc_dev = malloc(sizeof(*uvc_dev));
			uvc_dev->ctx = ctx;
			uvc_dev->ref = 0;
			uvc_dev->usb_dev = usb_dev;
			uvc_ref_device(uvc_dev);

			num_uvc_devices++;
			list_internal = realloc(list_internal,
					(num_uvc_devices + 1) * sizeof(*list_internal));

			list_internal[num_uvc_devices - 1] = uvc_dev;
			list_internal[num_uvc_devices] = NULL;

			UVC_DEBUG("    UVC: %d", dev_idx);
		} else {
			UVC_DEBUG("non-UVC: %d", dev_idx);
		}
	}

	libusb_free_device_list(usb_dev_list, 1);

	*list = list_internal;

	UVC_EXIT(UVC_SUCCESS);
	return UVC_SUCCESS;
}

/**
 * @brief Frees a list of device structures created with uvc_get_device_list.
 * @ingroup device
 *
 * @param list Device list to free
 * @param unref_devices Decrement the reference counter for each device
 * in the list, and destroy any entries that end up with zero references
 *
 * 释放使用uvc_get_device_list创建的设备结构的列表。
 */
void uvc_free_device_list(uvc_device_t **list, uint8_t unref_devices) {
	uvc_device_t *dev;
	int dev_idx = 0;

	UVC_ENTER();

	if (unref_devices) {
		while ((dev = list[dev_idx++]) != NULL ) {
			uvc_unref_device(dev);
		}
	}

	free(list);

	UVC_EXIT_VOID();
}

/**
 * @brief Get the uvc_device_t corresponding to an open device
 * @ingroup device
 *
 * @note Unref the uvc_device_t when you're done with it
 *
 * @param devh Device handle to an open UVC device
 *
 * 获取对应于打开的设备的uvc_device_t
 * 完成后，请取消引用uvc_device_t
 */
uvc_device_t *uvc_get_device(uvc_device_handle_t *devh) {
	uvc_ref_device(devh->dev);
	return devh->dev;
}

/**
 * @brief Get the underlying libusb device handle for an open device
 * @ingroup device
 *
 * This can be used to access other interfaces on the same device, e.g.
 * a webcam microphone.
 *
 * @note The libusb device handle is only valid while the UVC device is open;
 * it will be invalidated upon calling uvc_close.
 *
 * @param devh UVC device handle to an open device
 *
 * 获取打开设备的底层libusb设备句柄
 * 这可用于访问同一设备上的其他接口，例如 网络摄像头麦克风。
 * libusb设备句柄仅在UVC设备打开时才有效；调用uvc_close时它将失效。
 */
libusb_device_handle *uvc_get_libusb_handle(uvc_device_handle_t *devh) {
	return devh->usb_devh;
}

/**
 * @brief Get input terminal descriptors for the open device.
 *
 * @note Do not modify the returned structure.
 * @note The returned structure is part of a linked list. Iterate through
 *       it by using the 'next' pointers.
 *
 * @param devh Device handle to an open UVC device
 *
 * 获取打开设备的输入终端描述符。
 * 不要修改返回的结构。
 * 返回的结构是链表的一部分。 通过使用“next”指针对其进行迭代。
 */
const uvc_input_terminal_t *uvc_get_input_terminals(uvc_device_handle_t *devh) {
	return devh->info->ctrl_if.input_term_descs;
}

/**
 * @brief Get output terminal descriptors for the open device.
 *
 * @note Do not modify the returned structure.
 * @note The returned structure is part of a linked list. Iterate through
 *       it by using the 'next' pointers.
 *
 * @param devh Device handle to an open UVC device
 *
 * 获取打开设备的输出终端描述符。
 * 不要修改返回的结构。
 * 返回的结构是链表的一部分。 通过使用“next”指针对其进行迭代。
 */
const uvc_output_terminal_t *uvc_get_output_terminals(uvc_device_handle_t *devh) {
	return devh->info->ctrl_if.output_term_descs ;
}

/**
 * @brief Get processing unit descriptors for the open device.
 *
 * @note Do not modify the returned structure.
 * @note The returned structure is part of a linked list. Iterate through
 *       it by using the 'next' pointers.
 *
 * @param devh Device handle to an open UVC device
 *
 * 获取打开设备的处理单元描述符。
 * 不要修改返回的结构。
 * 返回的结构是链表的一部分。 通过使用“next”指针对其进行迭代。
 */
const uvc_processing_unit_t *uvc_get_processing_units(uvc_device_handle_t *devh) {
	return devh->info->ctrl_if.processing_unit_descs;
}

/**
 * @brief Get extension unit descriptors for the open device.
 *
 * @note Do not modify the returned structure.
 * @note The returned structure is part of a linked list. Iterate through
 *       it by using the 'next' pointers.
 *
 * @param devh Device handle to an open UVC device
 *
 * 获取打开设备的扩展单元描述符。
 * 不要修改返回的结构。
 * 返回的结构是链表的一部分。 通过使用“next”指针对其进行迭代。
 */
const uvc_extension_unit_t *uvc_get_extension_units(uvc_device_handle_t *devh) {
	return devh->info->ctrl_if.extension_unit_descs;
}

/**
 * @brief Increment the reference count for a device
 * @ingroup device
 *
 * @param dev Device to reference
 *
 * 增加设备的参考计数
 */
void uvc_ref_device(uvc_device_t *dev) {
	UVC_ENTER();

	dev->ref++;	// これ排他制御要るんちゃうかなぁ(｡･_･｡)   我想知道是否需要此独占控件（。･ _ ･。）
//	LOGI("ref=%d", dev->ref);
	libusb_ref_device(dev->usb_dev);

	UVC_EXIT_VOID();
}

/**
 * @brief Decrement the reference count for a device
 * @ingropu device
 * @note If the count reaches zero, the device will be discarded
 *
 * @param dev Device to unreference
 *
 * 减少设备的参考计数
 * 如果计数达到零，该设备将被丢弃
 */
void uvc_unref_device(uvc_device_t *dev) {
	UVC_ENTER();

	libusb_unref_device(dev->usb_dev);
	dev->ref--;	// これ排他制御要るんちゃうかなぁ(｡･_･｡)   我想知道是否需要此独占控件（。･ _ ･。）

//	LOGI("ref=%d", dev->ref);
	if (dev->ref == 0) {
		free(dev);
		dev = NULL;
	}

	UVC_EXIT_VOID();
}

/** @internal
 * Claim a UVC interface, detaching the kernel driver if necessary.
 * @ingroup device
 *
 * @param devh UVC device handle
 * @param idx UVC interface index
 *
 * 声明UVC接口，必要时分离内核驱动程序。
 */
uvc_error_t uvc_claim_if(uvc_device_handle_t *devh, int idx) {
	int ret;

	UVC_ENTER();
#if !UVC_DETACH_ATTACH
	// libusb automatically attach/detach kernel driver on supported platforms
	// libusb在支持的平台上自动附加/分离内核驱动程序
	UVC_DEBUG("claiming interface %d", idx);
	ret = libusb_claim_interface(devh->usb_devh, idx);
#else
	/* Tell libusb to detach any active kernel drivers. libusb will keep track of whether
	 * it found a kernel driver for this interface.
	 * 告诉libusb分离所有活动的内核驱动程序。 libusb将跟踪是否为该接口找到了内核驱动程序。
	 */
	ret = libusb_detach_kernel_driver(devh->usb_devh, idx);
	
	if LIKELY(!ret || ret == LIBUSB_ERROR_NOT_FOUND || ret == LIBUSB_ERROR_NOT_SUPPORTED) {
		UVC_DEBUG("claiming interface %d", idx);
		ret = libusb_claim_interface(devh->usb_devh, idx);
	} else {
	    UVC_DEBUG("not claiming interface %d: unable to detach kernel driver (%s)",
			idx, uvc_strerror(ret));
	}
#endif
	UVC_EXIT(ret);
	return ret;
}

/** @internal
 * Release a UVC interface.
 * @ingroup device
 *
 * @param devh UVC device handle
 * @param idx UVC interface index
 *
 * 释放UVC接口。
 */
uvc_error_t uvc_release_if(uvc_device_handle_t *devh, int idx) {
	int ret;

	UVC_ENTER();
	UVC_DEBUG("releasing interface %d", idx);
	/* libusb_release_interface *should* reset the alternate setting to the first available,
	 but sometimes (e.g. on Darwin) it doesn't. Thus, we do it explicitly here.
	 This is needed to de-initialize certain cameras.
	 libusb_release_interface应该将替代设置重置为第一个可用设置，但是有时（例如在达尔文市）却没有。
	 因此，我们在这里明确进行此操作，这是取消初始化某些摄像机所必需的。
	 */
	// XXX but resetting the alt setting here manytimes leads trouble
	// on GT-N7100(international Galaxy Note2 at lease with Android4.4.2)
	// so we add flag to avoid the issue
	// 但是在这里多次重置alt设置会导致GT-N7100（与Android4.4.2租赁的国际Galaxy Note 2）出现问题，因此我们添加了标记以避免此问题
	if (devh->reset_on_release_if)
		libusb_set_interface_alt_setting(devh->usb_devh, idx, 0);

	ret = libusb_release_interface(devh->usb_devh, idx);

#if !UVC_DETACH_ATTACH
	// libusb automatically attach/detach kernel driver on supported platforms and nothing to do here
	// libusb会自动在支持的平台上附加/分离内核驱动程序，而无需执行任何操作
#else
	if (UVC_SUCCESS == ret) {
		/* Reattach any kernel drivers that were disabled when we claimed this interface
		 * 重新附加我们声明此接口时已禁用的所有内核驱动程序
		 */
		ret = libusb_attach_kernel_driver(devh->usb_devh, idx);

		if LIKELY(!ret) {
			UVC_DEBUG("reattached kernel driver to interface %d", idx);
		} else if (ret == LIBUSB_ERROR_NOT_FOUND || ret == LIBUSB_ERROR_NOT_SUPPORTED) {
			ret = UVC_SUCCESS;  /* NOT_FOUND and NOT_SUPPORTED are OK: nothing to do   NOT_FOUND和NOT_SUPPORTED没问题：无事可做 */
		} else {
			UVC_DEBUG("error reattaching kernel driver to interface %d: %s",
                idx, uvc_strerror(ret));
		}
	}
#endif
	UVC_EXIT(ret);
	return ret;
}

/** @internal
 * Find a device's VideoControl interface and process its descriptor
 * @ingroup device
 *
 * 查找设备的VideoControl接口并处理其描述符
 */
uvc_error_t uvc_scan_control(uvc_device_t *dev, uvc_device_info_t *info) {
	const struct libusb_interface_descriptor *if_desc;
	uvc_error_t parse_ret, ret;
	int interface_idx;
	const unsigned char *buffer;
	size_t buffer_left, block_size;

	UVC_ENTER();

	ret = UVC_SUCCESS;
	if_desc = NULL;

	if (LIKELY(info && info->config)) {	// XXX add to avoid crash  添加以避免崩溃
		MARK("bNumInterfaces=%d", info->config->bNumInterfaces);
		for (interface_idx = 0; interface_idx < info->config->bNumInterfaces; ++interface_idx) {
			if_desc = &info->config->interface[interface_idx].altsetting[0];
			MARK("interface_idx=%d:bInterfaceClass=%02x,bInterfaceSubClass=%02x", interface_idx, if_desc->bInterfaceClass, if_desc->bInterfaceSubClass);
			// select first found Video control
			// 选择第一个找到的视频控件  查找视频类型的usb设备
			if (if_desc->bInterfaceClass == LIBUSB_CLASS_VIDEO/*14*/ && if_desc->bInterfaceSubClass == 1) // Video, Control  视频，控制
				break;

			// Another TIS camera hack.
			// 另一个TIS相机黑客。
			if (if_desc->bInterfaceClass == 255 && if_desc->bInterfaceSubClass == 1) {
				uvc_device_descriptor_t* dev_desc;
				int haveTISCamera = 0;
				uvc_get_device_descriptor (dev, &dev_desc);
				if (dev_desc->idVendor == 0x199e && dev_desc->idProduct == 0x8101) {
					haveTISCamera = 1;
				}
				uvc_free_device_descriptor (dev_desc);
				if (haveTISCamera) {
					break;
				}
			}
			if_desc = NULL;
		}
	}

	if (UNLIKELY(!if_desc)) {
		UVC_EXIT(UVC_ERROR_INVALID_DEVICE);
		LOGE("UVC_ERROR_INVALID_DEVICE");
		return UVC_ERROR_INVALID_DEVICE;
	}

	info->ctrl_if.bInterfaceNumber = interface_idx;
	if (if_desc->bNumEndpoints != 0) {
		info->ctrl_if.bEndpointAddress = if_desc->endpoint[0].bEndpointAddress;
	}

	buffer = if_desc->extra;
	buffer_left = if_desc->extra_length;

	while (buffer_left >= 3) { // parseX needs to see buf[0,2] = length,type
		block_size = buffer[0];
		parse_ret = uvc_parse_vc(dev, info, buffer, block_size);

		if (parse_ret != UVC_SUCCESS) {
			ret = parse_ret;
			break;
		}

		buffer_left -= block_size;
		buffer += block_size;
	}

	UVC_EXIT(ret);
	return ret;
}

/** @internal
 * @brief Parse a VideoControl header.
 * @ingroup device
 * 解析VideoControl标头。
 */
uvc_error_t uvc_parse_vc_header(uvc_device_t *dev, uvc_device_info_t *info,
		const unsigned char *block, size_t block_size) {
	size_t i;
	uvc_error_t scan_ret, ret = UVC_SUCCESS;

	UVC_ENTER();

	/*
	 int uvc_version;
	 uvc_version = (block[4] >> 4) * 1000 + (block[4] & 0x0f) * 100
	 + (block[3] >> 4) * 10 + (block[3] & 0x0f);
	 */

	info->ctrl_if.bcdUVC = SW_TO_SHORT(&block[3]);

	switch (info->ctrl_if.bcdUVC) {
	case 0x0100:
	case 0x010a:
	case 0x0110:
	case 0x0150:	// XXX add to support UVC 1.5  添加以支持UVC 1.5
		break;
	default:
		UVC_EXIT(UVC_ERROR_NOT_SUPPORTED);
		return UVC_ERROR_NOT_SUPPORTED;
	}

	for (i = 12; i < block_size; ++i) {
		scan_ret = uvc_scan_streaming(dev, info, block[i]);
		if (scan_ret != UVC_SUCCESS) {
			ret = scan_ret;
			break;
		}
	}

	UVC_EXIT(ret);
	return ret;
}

/** @internal
 * @brief Parse a VideoControl input terminal.
 * @ingroup device
 *
 * 解析VideoControl输入端子。
 */
uvc_error_t uvc_parse_vc_input_terminal(uvc_device_t *dev,
		uvc_device_info_t *info, const unsigned char *block, size_t block_size) {
	uvc_input_terminal_t *term;
	size_t i;

	UVC_ENTER();

	/* only supporting camera-type input terminals
	 * 仅支持相机类型的输入端子
	 */
	if (SW_TO_SHORT(&block[4]) != UVC_ITT_CAMERA) {
		UVC_EXIT(UVC_SUCCESS);
		return UVC_SUCCESS;
	}

	term = calloc(1, sizeof(*term));

	term->bTerminalID = block[3];
	term->wTerminalType = SW_TO_SHORT(&block[4]);
	term->wObjectiveFocalLengthMin = SW_TO_SHORT(&block[8]);
	term->wObjectiveFocalLengthMax = SW_TO_SHORT(&block[10]);
	term->wOcularFocalLength = SW_TO_SHORT(&block[12]);
	term->request = (term->bTerminalID << 8) | info->ctrl_if.bInterfaceNumber;
	term->bmControls = 0;	// XXX
	for (i = 14 + block[14]; i >= 15; i--)
		term->bmControls = block[i] + (term->bmControls << 8);

	DL_APPEND(info->ctrl_if.input_term_descs, term);

	UVC_EXIT(UVC_SUCCESS);
	return UVC_SUCCESS;
}

/** @internal
 * @brief Parse a output terminal.
 * @ingroup device
 *
 * 解析输出端子。
 */
uvc_error_t uvc_parse_vc_output_terminal(uvc_device_t *dev,
		uvc_device_info_t *info, const unsigned char *block, size_t block_size) {
	uvc_output_terminal_t *term;
	size_t i;

	UVC_ENTER();

	/* only supporting display-type input terminals
	 * 仅支持显示型输入端子
	 */
	if (SW_TO_SHORT(&block[4]) != UVC_OTT_DISPLAY) {
		UVC_EXIT(UVC_SUCCESS);
		return UVC_SUCCESS;
	}

	term = calloc(1, sizeof(*term));

	term->bTerminalID = block[3];
	term->wTerminalType = SW_TO_SHORT(&block[4]);
	term->bAssocTerminal = block[6];
	term->bSourceID = block[7];
	term->iTerminal = block[8];
	term->request = (term->bTerminalID << 8) | info->ctrl_if.bInterfaceNumber;	// XXX
	// TODO depending on the wTerminalType
	// 取决于wTerminalType

	DL_APPEND(info->ctrl_if.output_term_descs, term);

	UVC_EXIT(UVC_SUCCESS);
	return UVC_SUCCESS;
}

/** @internal
 * @brief Parse a VideoControl processing unit.
 * @ingroup device
 *
 * 解析VideoControl处理单元。
 */
uvc_error_t uvc_parse_vc_processing_unit(uvc_device_t *dev,
		uvc_device_info_t *info, const unsigned char *block, size_t block_size) {
	uvc_processing_unit_t *unit;
	size_t i;

	UVC_ENTER();

	unit = calloc(1, sizeof(*unit));
	unit->bUnitID = block[3];
	unit->bSourceID = block[4];
	unit->request = (unit->bUnitID << 8) | info->ctrl_if.bInterfaceNumber;	// XXX

	unit->bmControls = 0;	// XXX
	for (i = 7 + block[7]; i >= 8; i--)
		unit->bmControls = block[i] + (unit->bmControls << 8);

	DL_APPEND(info->ctrl_if.processing_unit_descs, unit);

	UVC_EXIT(UVC_SUCCESS);
	return UVC_SUCCESS;
}

/** @internal
 * @brief Parse a VideoControl extension unit.
 * @ingroup device
 *
 * 解析VideoControl扩展单元。
 */
uvc_error_t uvc_parse_vc_extension_unit(uvc_device_t *dev,
		uvc_device_info_t *info, const unsigned char *block, size_t block_size) {
	uvc_extension_unit_t *unit = calloc(1, sizeof(*unit));
	const uint8_t *start_of_controls;
	int size_of_controls, num_in_pins;
	int i;

	UVC_ENTER();

	unit->bUnitID = block[3];
	memcpy(unit->guidExtensionCode, &block[4], 16);

	num_in_pins = block[21];
	size_of_controls = block[22 + num_in_pins];
	start_of_controls = &block[23 + num_in_pins];
	unit->request = (unit->bUnitID << 8) | info->ctrl_if.bInterfaceNumber;

	unit->bmControls = 0;	// XXX
	for (i = size_of_controls - 1; i >= 0; i--)
		unit->bmControls = start_of_controls[i] + (unit->bmControls << 8);

	DL_APPEND(info->ctrl_if.extension_unit_descs, unit);

	UVC_EXIT(UVC_SUCCESS);
	return UVC_SUCCESS;
}

/** @internal
 * Process a single VideoControl descriptor block
 * @ingroup device
 *
 * 处理单个VideoControl描述符块
 */
uvc_error_t uvc_parse_vc(uvc_device_t *dev, uvc_device_info_t *info,
		const unsigned char *block, size_t block_size) {
	int descriptor_subtype;
	uvc_error_t ret = UVC_SUCCESS;

	UVC_ENTER();

	if (block[1] != LIBUSB_DT_CS_INTERFACE/*36*/) { // not a CS_INTERFACE descriptor??
		UVC_EXIT(UVC_SUCCESS);
		return UVC_SUCCESS; // UVC_ERROR_INVALID_DEVICE;
	}

	descriptor_subtype = block[2];

	switch (descriptor_subtype) {
	case UVC_VC_HEADER:
		ret = uvc_parse_vc_header(dev, info, block, block_size);
		break;
	case UVC_VC_INPUT_TERMINAL:
		ret = uvc_parse_vc_input_terminal(dev, info, block, block_size);
		break;
	case UVC_VC_OUTPUT_TERMINAL:
		break;
	case UVC_VC_SELECTOR_UNIT:
		break;
	case UVC_VC_PROCESSING_UNIT:
		ret = uvc_parse_vc_processing_unit(dev, info, block, block_size);
		break;
	case UVC_VC_EXTENSION_UNIT:
		ret = uvc_parse_vc_extension_unit(dev, info, block, block_size);
		break;
	default:
		LOGW("UVC_ERROR_INVALID_DEVICE:descriptor_subtype=0x%02x", descriptor_subtype);
		ret = UVC_ERROR_INVALID_DEVICE;
	}

	UVC_EXIT(ret);
	return ret;
}

/** @internal
 * Process a VideoStreaming interface
 * @ingroup device
 *
 * 处理VideoStreaming接口
 */
uvc_error_t uvc_scan_streaming(uvc_device_t *dev, uvc_device_info_t *info,
		int interface_idx) {
	const struct libusb_interface_descriptor *if_desc;
	const unsigned char *buffer;
	size_t buffer_left, block_size;
	uvc_error_t ret, parse_ret;
	uvc_streaming_interface_t *stream_if;

	UVC_ENTER();

	ret = UVC_SUCCESS;

	if_desc = &(info->config->interface[interface_idx].altsetting[0]);
	buffer = if_desc->extra;
	buffer_left = if_desc->extra_length;
	// XXX some device have it's format descriptions after the endpoint descriptor
	if (UNLIKELY(!buffer || !buffer_left)) {
		if (if_desc->bNumEndpoints && if_desc->endpoint) {
			// try to use extra data in endpoint[0]
			buffer = if_desc->endpoint[0].extra;
			buffer_left = if_desc->endpoint[0].extra_length;
		}
	}
	stream_if = calloc(1, sizeof(*stream_if));
	stream_if->parent = info;
	stream_if->bInterfaceNumber = if_desc->bInterfaceNumber;
	DL_APPEND(info->stream_ifs, stream_if);

	if (LIKELY(buffer_left >= 3)) {
		while (buffer_left >= 3) {
			block_size = buffer[0];
//			MARK("bDescriptorType=0x%02x", buffer[1]);
			parse_ret = uvc_parse_vs(dev, info, stream_if, buffer, block_size);

			if (parse_ret != UVC_SUCCESS) {
				ret = parse_ret;
				break;
			}

			buffer_left -= block_size;
			buffer += block_size;
		}
	} else {
		LOGW("This VideoStreaming interface has no extra data");
	}

	UVC_EXIT(ret);
	return ret;
}

/** @internal
 * @brief Parse a VideoStreaming header block.
 * @ingroup device
 *
 * 解析VideoStreaming标头块。
 */
uvc_error_t uvc_parse_vs_input_header(uvc_streaming_interface_t *stream_if,
		const unsigned char *block, size_t block_size) {
	UVC_ENTER();

	stream_if->bEndpointAddress = block[6] & 0x8f;
	stream_if->bTerminalLink = block[8];
	stream_if->bmInfo = block[7];	// XXX
	stream_if->bStillCaptureMethod = block[9];	// XXX
	stream_if->bTriggerSupport = block[10];	// XXX
	stream_if->bTriggerUsage = block[11];	// XXX
	stream_if->bmaControls = NULL;
	const uint8_t n = block[12];
	if (LIKELY(n)) {
		const uint8_t p = (block_size - 13) / n;
		if (LIKELY(p)) {
			uint64_t *bmaControls = (uint64_t *)calloc(p, sizeof(uint64_t));
			stream_if->bmaControls = bmaControls;
			const uint8_t *bma;
			int pp, nn;
			for (pp = 1; pp <= p; pp++) {
				bma = &block[12 + pp * n];
				for (nn = n - 1; nn >= 0; --nn) {
					*bmaControls = *bma-- + (*bmaControls << 8);
				}
				bmaControls++;
			}
		}
	}
	UVC_EXIT(UVC_SUCCESS);
	return UVC_SUCCESS;
}

/** @internal
 * @brief Parse a VideoStreaming uncompressed format block.
 * @ingroup device
 *
 * 解析VideoStreaming未压缩的格式块。
 */
uvc_error_t uvc_parse_vs_format_uncompressed(
		uvc_streaming_interface_t *stream_if, const unsigned char *block,
		size_t block_size) {
	UVC_ENTER();

	uvc_format_desc_t *format = calloc(1, sizeof(*format));

	format->parent = stream_if;
	format->bDescriptorSubtype = block[2];
	format->bFormatIndex = block[3];
	//format->bmCapabilities = block[4];
	//format->bmFlags = block[5];
	memcpy(format->guidFormat, &block[5], 16);
	format->bBitsPerPixel = block[21];
	format->bDefaultFrameIndex = block[22];
	format->bAspectRatioX = block[23];
	format->bAspectRatioY = block[24];
	format->bmInterlaceFlags = block[25];
	format->bCopyProtect = block[26];

	DL_APPEND(stream_if->format_descs, format);

	UVC_EXIT(UVC_SUCCESS);
	return UVC_SUCCESS;
}

/** @internal
 * @brief Parse a VideoStreaming frame format block.
 * @ingroup device
 *
 * 解析VideoStreaming帧格式块。
 */
uvc_error_t uvc_parse_vs_frame_format(uvc_streaming_interface_t *stream_if,
	const unsigned char *block, size_t block_size) {
	UVC_ENTER();

	uvc_format_desc_t *format = calloc(1, sizeof(*format));

	format->parent = stream_if;
	format->bDescriptorSubtype = block[2];
	format->bFormatIndex = block[3];
	format->bNumFrameDescriptors = block[4];
	memcpy(format->guidFormat, &block[5], 16);
	format->bBitsPerPixel = block[21];
	format->bDefaultFrameIndex = block[22];
	format->bAspectRatioX = block[23];
	format->bAspectRatioY = block[24];
	format->bmInterlaceFlags = block[25];
	format->bCopyProtect = block[26];
	format->bVariableSize = block[27];

	DL_APPEND(stream_if->format_descs, format);

	UVC_EXIT(UVC_SUCCESS);
	return UVC_SUCCESS;
}

/** @internal
 * @brief Parse a VideoStreaming MJPEG format block.
 * @ingroup device
 *
 * 解析VideoStreaming MJPEG格式块。
 */
uvc_error_t uvc_parse_vs_format_mjpeg(uvc_streaming_interface_t *stream_if,
		const unsigned char *block, size_t block_size) {
	UVC_ENTER();

	uvc_format_desc_t *format = calloc(1, sizeof(*format));

	format->parent = stream_if;
	format->bDescriptorSubtype = block[2];
	format->bFormatIndex = block[3];
	memcpy(format->fourccFormat, "MJPG", 4);
	format->bmFlags = block[5];
	format->bBitsPerPixel = 0;
	format->bDefaultFrameIndex = block[6];
	format->bAspectRatioX = block[7];
	format->bAspectRatioY = block[8];
	format->bmInterlaceFlags = block[9];
	format->bCopyProtect = block[10];

	DL_APPEND(stream_if->format_descs, format);

	UVC_EXIT(UVC_SUCCESS);
	return UVC_SUCCESS;
}

/** @internal
 * @brief Parse a VideoStreaming uncompressed frame block.
 * @ingroup device
 *
 * 解析VideoStreaming未压缩的帧块。
 */
uvc_error_t uvc_parse_vs_frame_frame(uvc_streaming_interface_t *stream_if,
					    const unsigned char *block,
					    size_t block_size) {
  uvc_format_desc_t *format;
  uvc_frame_desc_t *frame;

  const unsigned char *p;
  int i;

  UVC_ENTER();

  format = stream_if->format_descs->prev;
  frame = calloc(1, sizeof(*frame));

  frame->parent = format;

  frame->bDescriptorSubtype = block[2];
  frame->bFrameIndex = block[3];
  frame->bmCapabilities = block[4];
  frame->wWidth = block[5] + (block[6] << 8);
  frame->wHeight = block[7] + (block[8] << 8);
  frame->dwMinBitRate = DW_TO_INT(&block[9]);
  frame->dwMaxBitRate = DW_TO_INT(&block[13]);
  frame->dwDefaultFrameInterval = DW_TO_INT(&block[17]);
  frame->bFrameIntervalType = block[21];
  frame->dwBytesPerLine = DW_TO_INT(&block[22]);

  if (block[21] == 0) {
    frame->dwMinFrameInterval = DW_TO_INT(&block[26]);
    frame->dwMaxFrameInterval = DW_TO_INT(&block[30]);
    frame->dwFrameIntervalStep = DW_TO_INT(&block[34]);
  } else {
    frame->intervals = calloc(block[21] + 1, sizeof(frame->intervals[0]));
    p = &block[26];

    for (i = 0; i < block[21]; ++i) {
      frame->intervals[i] = DW_TO_INT(p);
      p += 4;
    }
    frame->intervals[block[21]] = 0;
  }

  DL_APPEND(format->frame_descs, frame);

  UVC_EXIT(UVC_SUCCESS);
  return UVC_SUCCESS;
}

/** @internal
 * @brief Parse a VideoStreaming uncompressed frame block.
 * @ingroup device
 *
 * 解析VideoStreaming未压缩的帧块。
 */
uvc_error_t uvc_parse_vs_frame_uncompressed(
		uvc_streaming_interface_t *stream_if, const unsigned char *block,
		size_t block_size) {
	uvc_format_desc_t *format;
	uvc_frame_desc_t *frame;
	uint8_t frame_type;
	uint8_t n;
	uint32_t interval;

	const unsigned char *p;
	int i;

	UVC_ENTER();

	format = stream_if->format_descs->prev;
	frame = calloc(1, sizeof(*frame));

	frame->parent = format;

	frame_type = frame->bDescriptorSubtype = block[2];
	frame->bFrameIndex = block[3];
	frame->bmCapabilities = block[4];
	frame->wWidth = block[5] + (block[6] << 8);
	frame->wHeight = block[7] + (block[8] << 8);
	frame->dwMinBitRate = DW_TO_INT(&block[9]);
	frame->dwMaxBitRate = DW_TO_INT(&block[13]);
	frame->dwMaxVideoFrameBufferSize = DW_TO_INT(&block[17]);
	frame->dwDefaultFrameInterval = DW_TO_INT(&block[21]);
	n = frame->bFrameIntervalType = block[25];

	if (!n) {
		frame->dwMinFrameInterval = DW_TO_INT(&block[26]);
		frame->dwMaxFrameInterval = DW_TO_INT(&block[30]);
		frame->dwFrameIntervalStep = DW_TO_INT(&block[34]);
	} else {
		frame->intervals = calloc(n + 1, sizeof(frame->intervals[0]));
		p = &block[26];

		for (i = 0; i < n; ++i) {
			interval = DW_TO_INT(p);
			frame->intervals[i] = interval ? interval : 1;
			p += 4;
		}
		frame->intervals[n] = 0;
		
		frame->dwDefaultFrameInterval
			= MIN(frame->intervals[n-1],
				MAX(frame->intervals[0], frame->dwDefaultFrameInterval));
	}
	
	if (frame_type == UVC_VS_FRAME_UNCOMPRESSED) {
		frame->dwMaxVideoFrameBufferSize
			= format->bBitsPerPixel * frame->wWidth * frame->wHeight / 8;
	}
	
	
	DL_APPEND(format->frame_descs, frame);

	UVC_EXIT(UVC_SUCCESS);
	return UVC_SUCCESS;
}

/** @internal
 * Process a single VideoStreaming descriptor block
 * @ingroup device
 *
 * 处理单个VideoStreaming描述符块
 */
uvc_error_t uvc_parse_vs(uvc_device_t *dev, uvc_device_info_t *info,
		uvc_streaming_interface_t *stream_if, const unsigned char *block,
		size_t block_size) {
	uvc_error_t ret;
	int descriptor_subtype;

	UVC_ENTER();

	ret = UVC_SUCCESS;
	descriptor_subtype = block[2];
//	MARK("descriptor_subtype=0x%02x", descriptor_subtype);
	switch (descriptor_subtype) {
	case UVC_VS_INPUT_HEADER:
		ret = uvc_parse_vs_input_header(stream_if, block, block_size);
		break;
//	case UVC_VS_STILL_IMAGE_FRAME:	// FIXME unsupported now  现在不支持
//		break;
	case UVC_VS_FORMAT_UNCOMPRESSED:
		ret = uvc_parse_vs_format_uncompressed(stream_if, block, block_size);
		break;
	case UVC_VS_FORMAT_MJPEG:
		ret = uvc_parse_vs_format_mjpeg(stream_if, block, block_size);
		break;
	case UVC_VS_FRAME_UNCOMPRESSED:
	case UVC_VS_FRAME_MJPEG:
		ret = uvc_parse_vs_frame_uncompressed(stream_if, block, block_size);
		break;
	case UVC_VS_FORMAT_FRAME_BASED:
		ret = uvc_parse_vs_frame_format(stream_if, block, block_size );
		break;
	case UVC_VS_FRAME_FRAME_BASED:
		ret = uvc_parse_vs_frame_frame(stream_if, block, block_size );
		break;
//	case UVC_VS_COLORFORMAT:	// FIXME unsupported now  现在不支持
//		break;
	default:
		/** @todo handle JPEG and maybe still frames or even DV...
		 * 处理JPEG甚至静止帧甚至DV ...
		 */
		LOGV("unsupported descriptor_subtype(0x%02x)", descriptor_subtype);
		break;
	}

	UVC_EXIT(ret);
	return ret;
}

/** @internal
 * @brief Free memory associated with a UVC device
 * @pre Streaming must be stopped, and threads must have died
 *
 * 与UVC设备关联的可用内存
 * 流必须停止，并且线程必须已终止
 */
void uvc_free_devh(uvc_device_handle_t *devh) {
	UVC_ENTER();

	pthread_mutex_destroy(&devh->status_mutex);	// XXX saki
	if (devh->info)
		uvc_free_device_info(devh->info);

	if (devh->status_xfer)
		libusb_free_transfer(devh->status_xfer);

	free(devh);

	UVC_EXIT_VOID();
}

/** @brief Close a device
 *
 * @ingroup device
 *
 * Ends any stream that's in progress.
 *
 * The device handle and frame structures will be invalidated.
 *
 * 关闭设备
 * 结束任何正在进行的流。
 * 设备句柄和框架结构将失效。
 */
void uvc_close(uvc_device_handle_t *devh) {

	UVC_ENTER();

	uvc_context_t *ctx = devh->dev->ctx;

	if (devh->streams)
		uvc_stop_streaming(devh);

	uvc_release_if(devh, devh->info->ctrl_if.bInterfaceNumber);

#if !UVC_DETACH_ATTACH
	/* disable automatic attach/detach kernel driver on supported platforms in libusb
	 * 在libusb中支持的平台上禁用自动附加/分离内核驱动程序
	 */
	libusb_set_auto_detach_kernel_driver(devh->usb_devh, 0);
#endif
	/* If we are managing the libusb context and this is the last open device,
	 * then we need to cancel the handler thread. When we call libusb_close,
	 * it'll cause a return from the thread's libusb_handle_events call, after
	 * which the handler thread will check the flag we set and then exit.
	 * 如果我们正在管理libusb上下文，并且这是最后打开的设备，则需要取消处理程序线程。
	 * 当我们调用libusb_close时，它将导致线程的libusb_handle_events调用返回，此后处理程序线程将检查我们设置的标志，然后退出。
	 */
	if (ctx->own_usb_ctx && ctx->open_devices == devh && devh->next == NULL) {
		ctx->kill_handler_thread = 1;
		libusb_close(devh->usb_devh);
		pthread_join(ctx->handler_thread, NULL);
	} else {
		libusb_close(devh->usb_devh);
	}

	DL_DELETE(ctx->open_devices, devh);

	uvc_unref_device(devh->dev);

	uvc_free_devh(devh);

	UVC_EXIT_VOID();
}

uvc_error_t uvc_set_reset_altsetting(uvc_device_handle_t *devh, uint8_t reset_on_release_if) {
	if (UNLIKELY(!devh))
		RETURN(UVC_ERROR_INVALID_PARAM, uvc_error_t);
	devh->reset_on_release_if = reset_on_release_if;
	RETURN(UVC_SUCCESS, uvc_error_t);
}

/** @internal
 * @brief Get number of open devices
 *
 * 获取打开设备的数量
 */
size_t uvc_num_devices(uvc_context_t *ctx) {
	size_t count = 0;

	uvc_device_handle_t *devh;

	UVC_ENTER();

	DL_FOREACH(ctx->open_devices, devh)
	{
		count++;
	}

	UVC_EXIT((int) count);
	return count;
}

void uvc_process_control_status(uvc_device_handle_t *devh, unsigned char *data, int len) {
	enum uvc_status_class status_class;
	uint8_t originator = 0, selector = 0, event = 0;
	enum uvc_status_attribute attribute = UVC_STATUS_ATTRIBUTE_UNKNOWN;
	void *content = NULL;
	size_t content_len = 0;
	int found_entity = 0;
	struct uvc_input_terminal *input_terminal;
	struct uvc_processing_unit *processing_unit;

	UVC_ENTER();

	if (len < 5) {
		UVC_DEBUG("Short read of VideoControl status update (%d bytes)", len);
		UVC_EXIT_VOID();
		return;
	}

	originator = data[1];
	event = data[2];
	selector = data[3];

	if (originator == 0) {
		UVC_DEBUG("Unhandled update from VC interface");
		UVC_EXIT_VOID();
		return;  /* @todo VideoControl virtual entity interface updates   视频控制虚拟实体接口更新 */
	}

	if (event != 0) {
		UVC_DEBUG("Unhandled VC event %d", (int) event);
		UVC_EXIT_VOID();
		return;
	}

	/* printf("bSelector: %d\n", selector); */

	DL_FOREACH(devh->info->ctrl_if.input_term_descs, input_terminal) {
		if (input_terminal->bTerminalID == originator) {
			status_class = UVC_STATUS_CLASS_CONTROL_CAMERA;
			found_entity = 1;
			break;
		}
	}

	if (!found_entity) {
		DL_FOREACH(devh->info->ctrl_if.processing_unit_descs, processing_unit) {
			if (processing_unit->bUnitID == originator) {
				status_class = UVC_STATUS_CLASS_CONTROL_PROCESSING;
				found_entity = 1;
				break;
			}
		}
	}

	if (!found_entity) {
		UVC_DEBUG("Got status update for unknown VideoControl entity %d",
				  (int) originator);
		UVC_EXIT_VOID();
		return;
	}

	attribute = data[4];
	content = data + 5;
	content_len = len - 5;

	UVC_DEBUG("Event: class=%d, event=%d, selector=%d, attribute=%d, content_len=%zd",
			  status_class, event, selector, attribute, content_len);

	pthread_mutex_lock(&devh->status_mutex);	// XXX saki
	{
	if(devh->status_cb) {
		UVC_DEBUG("Running user-supplied status callback");
		devh->status_cb(status_class,
						event,
						selector,
						attribute,
						content, content_len,
						devh->status_user_ptr);
	}
	}
	pthread_mutex_unlock(&devh->status_mutex);	// XXX saki

	UVC_EXIT_VOID();
}

void uvc_process_streaming_status(uvc_device_handle_t *devh, unsigned char *data, int len) {

	UVC_ENTER();

	if (len < 3) {
		UVC_DEBUG("Invalid streaming status event received.\n");
		UVC_EXIT_VOID();
		return;
	}

	if (data[2] == 0) {
		if (len < 4) {
			UVC_DEBUG("Short read of status update (%d bytes)", len);
			UVC_EXIT_VOID();
			return;
		}
		UVC_DEBUG("Button (intf %u) %s len %d\n", data[1], data[3] ? "pressed" : "released", len);

		pthread_mutex_lock(&devh->status_mutex);	// XXX saki
		{
			if(devh->button_cb) {
				UVC_DEBUG("Running user-supplied button callback");
				devh->button_cb(data[1],
								data[3],
								devh->button_user_ptr);
		}
		}
		pthread_mutex_unlock(&devh->status_mutex);	// XXX saki
	} else {
		UVC_DEBUG("Stream %u error event %02x %02x len %d.\n", data[1], data[2], data[3], len);
	}

	UVC_EXIT_VOID();
}

void uvc_process_status_xfer(uvc_device_handle_t *devh, struct libusb_transfer *transfer) {

	UVC_ENTER();

	/* printf("Got transfer of aLen = %d\n", transfer->actual_length); */

	if (transfer->actual_length > 0) {
		switch (transfer->buffer[0] & 0x0f) {
			case 1: /* VideoControl interface  视频控制接口 */
				uvc_process_control_status(devh, transfer->buffer, transfer->actual_length);
				break;
			case 2:  /* VideoStreaming interface  视频流接口 */
				uvc_process_streaming_status(devh, transfer->buffer, transfer->actual_length);
				break;
		}
	}

	UVC_EXIT_VOID();
}

/** @internal
 * @brief Process asynchronous status updates from the device.
 *
 * 从设备处理异步状态更新。
 */
void _uvc_status_callback(struct libusb_transfer *transfer) {
	UVC_ENTER();

	uvc_device_handle_t *devh = (uvc_device_handle_t *) transfer->user_data;

	switch (transfer->status) {
        case LIBUSB_TRANSFER_ERROR:
        case LIBUSB_TRANSFER_CANCELLED:
        case LIBUSB_TRANSFER_NO_DEVICE:
            UVC_DEBUG("not processing/resubmitting, status = %d", transfer->status);
            UVC_EXIT_VOID();
            return;
        case LIBUSB_TRANSFER_COMPLETED:
            uvc_process_status_xfer(devh, transfer);
            break;
        case LIBUSB_TRANSFER_TIMED_OUT:
        case LIBUSB_TRANSFER_STALL:
        case LIBUSB_TRANSFER_OVERFLOW:
            UVC_DEBUG("retrying transfer, status = %d", transfer->status);
            break;
	}
    // 提交传输。 此功能将触发USB传输，然后立即返回。
	uvc_error_t ret = libusb_submit_transfer(transfer);
	UVC_DEBUG("libusb_submit_transfer() = %d", ret);

	UVC_EXIT_VOID();
}

/** @brief Set a callback function to receive status updates
 *
 * @ingroup device
 *
 * 设置回调函数以接收状态更新
 */
void uvc_set_status_callback(uvc_device_handle_t *devh,
		uvc_status_callback_t cb, void *user_ptr) {
	UVC_ENTER();

	pthread_mutex_lock(&devh->status_mutex);	// XXX saki
	{
		devh->status_cb = cb;
		devh->status_user_ptr = user_ptr;
	}
	pthread_mutex_unlock(&devh->status_mutex);	// XXX saki

	UVC_EXIT_VOID();
}

/** @brief Set a callback function to receive button events
 *
 * @ingroup device
 *
 * 设置回调函数以接收按钮事件
 */
void uvc_set_button_callback(uvc_device_handle_t *devh,
		uvc_button_callback_t cb, void *user_ptr) {
	UVC_ENTER();

	pthread_mutex_lock(&devh->status_mutex);	// XXX saki
	{
		devh->button_cb = cb;
		devh->button_user_ptr = user_ptr;
	}
	pthread_mutex_unlock(&devh->status_mutex);	// XXX saki

	UVC_EXIT_VOID();
}

/**
 * @brief Get format descriptions for the open device.
 *
 * @note Do not modify the returned structure.
 *
 * @param devh Device handle to an open UVC device
 *
 * 获取打开的设备的格式描述。
 * 不要修改返回的结构。
 */
const uvc_format_desc_t *uvc_get_format_descs(uvc_device_handle_t *devh) {
  return devh->info->stream_ifs->format_descs;
}
