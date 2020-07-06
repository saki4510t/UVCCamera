/**
 * modified to improve compatibility with some cameras.
 * Copyright(c) 2014 saki saki@serenegiant.com
 */
/* -*- Mode: C; indent-tabs-mode:t ; c-basic-offset:8 -*- */
/*
 * USB descriptor handling functions for libusb
 * Copyright © 2007 Daniel Drake <dsd@gentoo.org>
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

#include <errno.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "libusbi.h"

// comment out because duplicate definitions already exit in libusb.h
//#define DESC_HEADER_LENGTH			2	// XXX this is same as LIBUSB_DT_HEADER_SIZE in libusb.h
//#define DEVICE_DESC_LENGTH			18	// XXX this is same as LIBUSB_DT_DEVICE_SIZE in libusb.h
//#define CONFIG_DESC_LENGTH			9	// XXX this is same as LIBUSB_DT_CONFIG_SIZE in libusb.h
//#define INTERFACE_DESC_LENGTH			9	// XXX this is same as LIBUSB_DT_INTERFACE_SIZE in libusb.h
//#define ENDPOINT_DESC_LENGTH			7	// XXX this is same as LIBUSB_DT_ENDPOINT_SIZE in libusb.h
//#define ENDPOINT_AUDIO_DESC_LENGTH	9	// XXX this is same as LIBUSB_DT_ENDPOINT_AUDIO_SIZE in libusb.h
//#define ASSOCIATION_DESC_LENGTH		8	// XXX this is same as LIBUSB_DT_ASSOCIATION_SIZE in libusb.h

/** @defgroup desc USB descriptors
 * This page details how to examine the various standard USB descriptors
 * for detected devices
 * 本页详细介绍如何检查检测到的设备的各种标准USB描述符
 */

static inline int is_known_descriptor_type(int type) {
	return ((type == LIBUSB_DT_ENDPOINT)
		|| (type == LIBUSB_DT_INTERFACE)
		|| (type == LIBUSB_DT_CONFIG)
		|| (type == LIBUSB_DT_DEVICE)
		|| (type == LIBUSB_DT_ASSOCIATION) );
}

/* set host_endian if the w values are already in host endian format,
 * as opposed to bus endian.
 * 如果w值已经是主机字节序格式（与总线字节序相反），则设置host_endian。
 */
int usbi_parse_descriptor(const unsigned char *source, const char *descriptor,
	void *dest, int host_endian)
{
	const unsigned char *sp = source;
	unsigned char *dp = dest;
	uint16_t w;
	const char *cp;
	uint32_t d;

	for (cp = descriptor; *cp; cp++) {
		switch (*cp) {
			case 'b':	/* 8-bit byte 8位字节 */
				*dp++ = *sp++;
				break;
			case 'w':	/* 16-bit word, convert from little endian to CPU  16位字，从小尾数转换为CPU */
				dp += ((uintptr_t)dp & 1);	/* Align to word boundary  对齐单词边界 */

				if (host_endian) {
					memcpy(dp, sp, 2);
				} else {
					w = (sp[1] << 8) | sp[0];
					*((uint16_t *)dp) = w;
				}
				sp += 2;
				dp += 2;
				break;
			case 'd':	/* 32-bit word, convert from little endian to CPU  32位字，从小尾数转换为CPU */
				dp += ((uintptr_t)dp & 1);	/* Align to word boundary  对齐单词边界 */

				if (host_endian) {
					memcpy(dp, sp, 4);
				} else {
					d = (sp[3] << 24) | (sp[2] << 16) |
						(sp[1] << 8) | sp[0];
					*((uint32_t *)dp) = d;
				}
				sp += 4;
				dp += 4;
				break;
			case 'u':	/* 16 byte UUID  16字节的UUID */
				memcpy(dp, sp, 16);
				sp += 16;
				dp += 16;
				break;
		}
	}

	return (int) (sp - source);
}

static void clear_endpoint(struct libusb_endpoint_descriptor *endpoint)
{
	if LIKELY(endpoint && endpoint->extra) {
		free((unsigned char *) endpoint->extra);
		endpoint->extra = NULL; // XXX
		endpoint->extra_length = 0;
	}
}

static int parse_endpoint(struct libusb_context *ctx,
	struct libusb_endpoint_descriptor *endpoint, unsigned char *buffer,
	int size, int host_endian)
{
	ENTER();

	struct usb_descriptor_header header;
	unsigned char *extra;
	unsigned char *begin;
	int parsed = 0;
	int len;

	if UNLIKELY(size < LIBUSB_DT_HEADER_SIZE/*DESC_HEADER_LENGTH*/) {
		usbi_err(ctx, "short endpoint descriptor read %d/%d",
			 size, LIBUSB_DT_HEADER_SIZE/*DESC_HEADER_LENGTH*/);
		RETURN(LIBUSB_ERROR_IO, int);
	}

	usbi_parse_descriptor(buffer, "bb", &header, 0);
	if UNLIKELY(header.bDescriptorType != LIBUSB_DT_ENDPOINT) {
		usbi_err(ctx, "unexpected descriptor %x (expected %x)",
			header.bDescriptorType, LIBUSB_DT_ENDPOINT);
		RETURN(parsed, int);
	}
	if UNLIKELY(header.bLength > size) {
		usbi_warn(ctx, "short endpoint descriptor read %d/%d",
			  size, header.bLength);
		RETURN(parsed, int);
	}
	if (header.bLength >= LIBUSB_DT_ENDPOINT_AUDIO_SIZE/*ENDPOINT_AUDIO_DESC_LENGTH*/)
		usbi_parse_descriptor(buffer, "bbbbwbbb", endpoint, host_endian);
	else if (header.bLength >= LIBUSB_DT_ENDPOINT_SIZE/*ENDPOINT_DESC_LENGTH*/)
		usbi_parse_descriptor(buffer, "bbbbwb", endpoint, host_endian);
	else {
		usbi_err(ctx, "invalid endpoint bLength (%d)", header.bLength);
		RETURN(LIBUSB_ERROR_IO, int);
	}

	buffer += header.bLength;
	size -= header.bLength;
	parsed += header.bLength;

	/* Skip over the rest of the Class Specific or Vendor Specific descriptors
	 * 跳过其余的特定于类或供应商的描述符 */
	begin = buffer;
	while (size >= LIBUSB_DT_HEADER_SIZE/*DESC_HEADER_LENGTH*/) {
		usbi_parse_descriptor(buffer, "bb", &header, 0);
		if UNLIKELY(header.bLength < LIBUSB_DT_HEADER_SIZE/*DESC_HEADER_LENGTH*/) {
			usbi_err(ctx, "invalid extra ep desc len (%d)",
				 header.bLength);
			RETURN(LIBUSB_ERROR_IO, int);
		} else if (header.bLength > size) {
			usbi_warn(ctx, "short extra ep desc read %d/%d",
				  size, header.bLength);
			RETURN(parsed, int);
		}

		/* If we find another "proper" descriptor then we're done  如果找到另一个“适当的”描述符，那么我们就完成了  */
		if (is_known_descriptor_type(header.bDescriptorType))
			break;

		usbi_dbg("skipping descriptor 0x%02x", header.bDescriptorType);
		buffer += header.bLength;
		size -= header.bLength;
		parsed += header.bLength;
	}

	/* Copy any unknown descriptors into a storage area for drivers to later parse
	 * 将所有未知的描述符复制到存储区中，以供驱动程序以后解析 */
	len = (int)(buffer - begin);
	if (!len) {
		endpoint->extra = NULL;
		endpoint->extra_length = 0;
		RETURN(parsed, int);
	}

	endpoint->extra = extra = malloc(len);
	if UNLIKELY(!extra) {
		endpoint->extra_length = 0;
		RETURN(LIBUSB_ERROR_NO_MEM, int);
	}

	memcpy(extra, begin, len);
	endpoint->extra_length = len;

	RETURN(parsed, int);
}

static void clear_interface(struct libusb_interface *usb_interface)
{
	int i;
	int j;

	if (usb_interface->altsetting) {
		for (i = 0; i < usb_interface->num_altsetting; i++) {
			struct libusb_interface_descriptor *ifp =
				(struct libusb_interface_descriptor *)
				usb_interface->altsetting + i;
			if (ifp->extra)
				free((void *) ifp->extra);
			if (ifp->endpoint) {
				for (j = 0; j < ifp->bNumEndpoints; j++)
					clear_endpoint((struct libusb_endpoint_descriptor *)
						ifp->endpoint + j);
				free((void *) ifp->endpoint);
			}
		}
		free((void *) usb_interface->altsetting);
		usb_interface->altsetting = NULL;
	}

}

static int parse_interface(libusb_context *ctx,
	struct libusb_interface *usb_interface, unsigned char *buffer, int size,
	int host_endian)
{
	ENTER();

	int i;
	int len;
	int r;
	int parsed = 0;
	int interface_number = -1;
	size_t tmp;
	struct usb_descriptor_header header;
	struct libusb_interface_descriptor *ifp;
	unsigned char *begin;

	usb_interface->num_altsetting = 0;

	while (size >= LIBUSB_DT_INTERFACE_SIZE/*INTERFACE_DESC_LENGTH*/) {
		struct libusb_interface_descriptor *altsetting =
			(struct libusb_interface_descriptor *) usb_interface->altsetting;
		altsetting = usbi_reallocf(altsetting,
			sizeof(struct libusb_interface_descriptor) * (usb_interface->num_altsetting + 1));
		if UNLIKELY(!altsetting) {
			r = LIBUSB_ERROR_NO_MEM;
			goto err;
		}
		usb_interface->altsetting = altsetting;

		ifp = altsetting + usb_interface->num_altsetting;
		usbi_parse_descriptor(buffer, "bbbbbbbbb", ifp, 0);
		if UNLIKELY(ifp->bDescriptorType != LIBUSB_DT_INTERFACE) {
			usbi_err(ctx, "unexpected descriptor %x (expected %x)",
				 ifp->bDescriptorType, LIBUSB_DT_INTERFACE);
			RETURN(parsed, int);
		}
		if UNLIKELY(ifp->bLength < LIBUSB_DT_INTERFACE_SIZE/*INTERFACE_DESC_LENGTH*/) {
			usbi_err(ctx, "invalid interface bLength (%d)",
				 ifp->bLength);
			r = LIBUSB_ERROR_IO;
			goto err;
		}
		if UNLIKELY(ifp->bLength > size) {
			usbi_warn(ctx, "short intf descriptor read %d/%d",
				 size, ifp->bLength);
			RETURN(parsed, int);
		}
		if UNLIKELY(ifp->bNumEndpoints > USB_MAXENDPOINTS) {
			usbi_err(ctx, "too many endpoints (%d)", ifp->bNumEndpoints);
			r = LIBUSB_ERROR_IO;
			goto err;
		}

		usb_interface->num_altsetting++;
		ifp->extra = NULL;
		ifp->extra_length = 0;
		ifp->endpoint = NULL;

		if (interface_number == -1)
			interface_number = ifp->bInterfaceNumber;

		/* Skip over the interface
		 * 跳过接口 */
		buffer += ifp->bLength;
		parsed += ifp->bLength;
		size -= ifp->bLength;

		begin = buffer;

		/* Skip over any interface, class or vendor descriptors
		 * 跳过任何接口，类或供应商描述符 */
		while (size >= LIBUSB_DT_HEADER_SIZE/*DESC_HEADER_LENGTH*/) {
			usbi_parse_descriptor(buffer, "bb", &header, 0);
			if UNLIKELY(header.bLength < LIBUSB_DT_HEADER_SIZE/*DESC_HEADER_LENGTH*/) {
				usbi_err(ctx,
					 "invalid extra intf desc len (%d)",
					 header.bLength);
				r = LIBUSB_ERROR_IO;
				goto err;
			} else if (header.bLength > size) {
				usbi_warn(ctx,
					  "short extra intf desc read %d/%d",
					  size, header.bLength);
				RETURN(parsed, int);
			}

			MARK("bDescriptorType=0x%02x", header.bDescriptorType);
			/* If we find another "proper" descriptor then we're done
			 * 如果找到另一个“适当的”描述符，那么我们就完成了 */
			if (is_known_descriptor_type(header.bDescriptorType))
				break;

			buffer += header.bLength;
			parsed += header.bLength;
			size -= header.bLength;
		}

		/* Copy any unknown descriptors into a storage area for drivers to later parse */
		/* 将所有未知的描述符复制到存储区中，以供驱动程序以后解析  */
		len = (int)(buffer - begin);
		if (len) {
			MARK("save unknown descriptors into ifp->extra:lebgth=%d", len);
			ifp->extra = usbi_reallocf((unsigned char *)ifp->extra, ifp->extra_length + len);
			if UNLIKELY(!ifp->extra) {
				r = LIBUSB_ERROR_NO_MEM;
				goto err;
			}
			memcpy((unsigned char *)(ifp->extra + ifp->extra_length), begin, len);
			ifp->extra_length += len;
		}

		MARK("bNumEndpoints=%d", ifp->bNumEndpoints);
		if (ifp->bNumEndpoints > 0) {
			struct libusb_endpoint_descriptor *endpoint;
			tmp = ifp->bNumEndpoints * sizeof(struct libusb_endpoint_descriptor);
			ifp->endpoint = endpoint = malloc(tmp);
			if UNLIKELY(!endpoint) {
				r = LIBUSB_ERROR_NO_MEM;
				goto err;
			}

			memset(endpoint, 0, tmp);
			for (i = 0; i < ifp->bNumEndpoints; i++) {
				MARK("parse endpoint%d", i);
				r = parse_endpoint(ctx, endpoint + i, buffer, size,
					host_endian);
				if UNLIKELY(r < 0)
					goto err;
				if (r == 0) {
					ifp->bNumEndpoints = (uint8_t)i;
					break;;
				}

				buffer += r;
				parsed += r;
				size -= r;
			}
		}

		/* We check to see if it's an alternate to this one
		 * 我们检查一下是否可以替代此 */
		ifp = (struct libusb_interface_descriptor *) buffer;
		if (size < LIBUSB_DT_INTERFACE_SIZE ||
				ifp->bDescriptorType != LIBUSB_DT_INTERFACE ||
				ifp->bInterfaceNumber != interface_number)
			RETURN(parsed, int);
	}

	RETURN(parsed, int);
err:
	clear_interface(usb_interface);
	RETURN(r, int);
}

static void clear_association(struct libusb_association_descriptor *association) {
	if LIKELY(association && association->extra) {
		free((unsigned char *) association->extra);
		association->extra = NULL;
		association->extra_length = 0;
	}
}

static int parse_association(struct libusb_context *ctx,
		struct libusb_config_descriptor *config, unsigned char *buffer,
	int size, int host_endian) {

	ENTER();

	struct usb_descriptor_header header;
	struct libusb_association_descriptor *association, *temp;
	unsigned char *begin;
	int parsed = 0;
	int len;

	if UNLIKELY(size < LIBUSB_DT_HEADER_SIZE/*DESC_HEADER_LENGTH*/) {
		usbi_err(ctx, "short association descriptor read %d/%d",
			 size, LIBUSB_DT_HEADER_SIZE/*DESC_HEADER_LENGTH*/);
		RETURN(LIBUSB_ERROR_IO, int);
	}
	// ディスクリプタの先頭2バイトだけ解析して長さとディスクリプタの種類を取得
	// 仅解析描述符的前2个字节以获取长度和描述符类型
	usbi_parse_descriptor(buffer, "bb", &header, 0);
	if UNLIKELY(header.bDescriptorType != LIBUSB_DT_ASSOCIATION) {	// 種類が違う時  当不同的类型
		usbi_err(ctx, "unexpected descriptor %x (expected %x)",
			header.bDescriptorType, LIBUSB_DT_ASSOCIATION);
		RETURN(parsed, int);	// return 0;
	}
	if UNLIKELY(header.bLength > size) {	// IADに長さが足りない時  当IAD不够长时
		usbi_warn(ctx, "short association descriptor read %d/%d",
			  size, header.bLength);
		RETURN(parsed, int);	// return 0;
	}
	if (header.bLength >= LIBUSB_DT_ASSOCIATION_SIZE/*ASSOCIATION_DESC_LENGTH*/) {
		config->association_descriptor = usbi_reallocf(config->association_descriptor,
			sizeof(struct libusb_association_descriptor) * (config->num_associations + 1));
		if UNLIKELY(!config->association_descriptor) {
			parsed = LIBUSB_ERROR_NO_MEM;
			goto err;
		}
		association = config->association_descriptor + config->num_associations;
		association->extra = NULL;
		association->extra_length = 0;
		len = usbi_parse_descriptor(buffer, "bbbbbbbb", association, host_endian);
		if LIKELY(len > 0) {
			config->num_associations++;
#if 0
			LOGI("\t association:bLength=%d", association->bLength);
			LOGI("\t association:bDescriptorType=0x%02d", association->bDescriptorType);
			LOGI("\t association:bFirstInterface=%d", association->bFirstInterface);
			LOGI("\t association:bInterfaceCount=%d", association->bInterfaceCount);
			LOGI("\t association:bFunctionClass=0x%02x", association->bFunctionClass);
			LOGI("\t association:bFunctionSubClass=0x%02x", association->bFunctionSubClass);
			LOGI("\t association:bFunctionProtocol=0x%02x", association->bFunctionProtocol);
			LOGI("\t association:iFunction=%d", association->iFunction);
#endif
		} else {
			// 解析に失敗した時は未使用部分を削除
			// 分析失败时，删除未使用的部分
			config->association_descriptor = usbi_reallocf(association,
				sizeof(struct libusb_association_descriptor) * config->num_associations);
		}
	} else {
		// 種類はIADで有るにも関わらず長さが足りない時
		// 当类型为IAD但长度不足时
		usbi_err(ctx, "invalid interface association descriptor bLength (%d)", header.bLength);
		RETURN(LIBUSB_ERROR_IO, int);
	}
	// 次の解析開始位置・残りサイズをセット
	// 设置下一个分析开始位置/剩余大小
	buffer += header.bLength;
	size -= header.bLength;
	parsed += header.bLength;

	/* Skip over the rest of the Class Specific or Vendor Specific descriptors
	 * 跳过其余的特定于类或供应商的描述符
	 */
	begin = buffer;
	while (size >= LIBUSB_DT_HEADER_SIZE/*DESC_HEADER_LENGTH*/) {
		usbi_parse_descriptor(buffer, "bb", &header, 0);
		if UNLIKELY(header.bLength < LIBUSB_DT_HEADER_SIZE/*DESC_HEADER_LENGTH*/) {
			usbi_err(ctx, "invalid extra ia desc len (%d)",
				 header.bLength);
			RETURN(LIBUSB_ERROR_IO, int);
		} else if (header.bLength > size) {
			usbi_warn(ctx, "short extra ia desc read %d/%d",
				  size, header.bLength);
			RETURN(parsed, int);
		}

		MARK("bDescriptorType=0x%02x", header.bDescriptorType);
		/* If we find another "proper" descriptor then we're done
		 * 如果找到另一个“适当的”描述符，那么我们就完成了 */
		if (is_known_descriptor_type(header.bDescriptorType))
			break;

		usbi_dbg("skipping descriptor 0x%02x", header.bDescriptorType);
		buffer += header.bLength;
		size -= header.bLength;
		parsed += header.bLength;
	}

	// Append/Copy any unknown descriptors into a storage area for drivers to later parse
	// 将未知描述符追加/复制到存储区中，以供驱动程序以后解析
	len = (int)(buffer - begin);
	if (!len) {
		RETURN(parsed, int);
	}

	MARK("save unknown descriptors into config->extra:length=%d", len);
	config->extra = usbi_reallocf((unsigned char *)config->extra, config->extra_length + len);
	if UNLIKELY(!config->extra) {
		config->extra_length = 0;
		RETURN(LIBUSB_ERROR_NO_MEM, int);
	}
	memcpy((unsigned char *)config->extra + config->extra_length, begin, len);
	config->extra_length += len;

	RETURN(parsed, int);
err:
	clear_association(config->association_descriptor);
	config->association_descriptor = NULL;
	RETURN(parsed, int);
}

static void clear_configuration(struct libusb_config_descriptor *config)
{
	if UNLIKELY(!config) return;

	if LIKELY(config->interface) {
		int i;
		for (i = 0; i < config->bNumInterfaces; i++)
			clear_interface((struct libusb_interface *)
				config->interface + i);
		free((void *) config->interface);
		config->interface = NULL;	// XXX
	}
	if (config->extra) {
		free((void *) config->extra);
		config->extra = NULL;	// XXX
	}
	if LIKELY(config->association_descriptor) {
		int i;
		for (i = 0; i < config->num_associations; i++)
			clear_association(config->association_descriptor + i);
		free((void *)config->association_descriptor);
		config->association_descriptor = NULL;
	}
}

static int parse_configuration(struct libusb_context *ctx,
	struct libusb_config_descriptor *config, unsigned char *buffer,
	int size, int host_endian) {

	ENTER();

	int parsed_if;
	int r;
	size_t tmp;
	struct usb_descriptor_header header;
	struct libusb_interface *usb_interface;
	struct libusb_association_descriptor *association_desc;

	if UNLIKELY(size < LIBUSB_DT_CONFIG_SIZE) {
		usbi_err(ctx, "short config descriptor read %d/%d",
			 size, LIBUSB_DT_CONFIG_SIZE);
		RETURN(LIBUSB_ERROR_IO, int);
	}

	usbi_parse_descriptor(buffer, "bbwbbbbb", config, host_endian);
	if UNLIKELY(config->bDescriptorType != LIBUSB_DT_CONFIG) {
		usbi_err(ctx, "unexpected descriptor %x (expected %x)",
			 config->bDescriptorType, LIBUSB_DT_CONFIG);
		RETURN(LIBUSB_ERROR_IO, int);
	}
	if UNLIKELY(config->bLength < LIBUSB_DT_CONFIG_SIZE) {
		usbi_err(ctx, "invalid config bLength (%d)", config->bLength);
		RETURN(LIBUSB_ERROR_IO, int);
	}
	if UNLIKELY(config->bLength > size) {
		usbi_err(ctx, "short config descriptor read %d/%d",
			 size, config->bLength);
		RETURN(LIBUSB_ERROR_IO, int);
	}
	if UNLIKELY(config->bNumInterfaces > USB_MAXINTERFACES) {
		usbi_err(ctx, "too many interfaces (%d)", config->bNumInterfaces);
		RETURN(LIBUSB_ERROR_IO, int);
	}
	// インターフェースディスクリプタ配列を確保(長さはconfig->bNumInterfaces)
	// 分配接口描述符数组（长度为config->bNumInterfaces）
	tmp = config->bNumInterfaces * sizeof(struct libusb_interface);
	config->interface = usb_interface = malloc(tmp);
	// インターフェースディスクリプタ配列を確保できなかった
	// 无法保护接口描述符数组
	if UNLIKELY(!config->interface)
		RETURN(LIBUSB_ERROR_NO_MEM, int);

	config->association_descriptor = NULL;
	config->num_associations = 0;

	memset(usb_interface, 0, tmp);
	buffer += config->bLength;
	size -= config->bLength;

	config->extra = NULL;
	config->extra_length = 0;
	MARK("bNumInterfaces=%d", config->bNumInterfaces);
	for (parsed_if = 0; (parsed_if < config->bNumInterfaces) && (size > 0); /*parsed_if++*/) {
		int len;
		unsigned char *begin;

		/* Skip over the rest of the Class Specific or Vendor Specific descriptors
		 * 跳过其余的特定于类或供应商的描述符 */
		begin = buffer;
		while (size >= LIBUSB_DT_HEADER_SIZE/*DESC_HEADER_LENGTH*/) {
			usbi_parse_descriptor(buffer, "bb", &header, 0);

			if UNLIKELY(header.bLength < LIBUSB_DT_HEADER_SIZE/*DESC_HEADER_LENGTH*/) {
				usbi_err(ctx,
					 "invalid extra config desc len (%d)",
					 header.bLength);
				r = LIBUSB_ERROR_IO;
				goto err;
			} else if UNLIKELY(header.bLength > size) {
				usbi_warn(ctx,
					  "short extra config desc read %d/%d",
					  size, header.bLength);
				config->bNumInterfaces = (uint8_t)parsed_if;
				return size;
			}

			MARK("bDescriptorType=0x%02x", header.bDescriptorType);
			/* If we find another "proper" descriptor then we're done
			 * 如果找到另一个“适当的”描述符，那么我们就完成了 */
			if (is_known_descriptor_type(header.bDescriptorType))
				break;

			usbi_dbg("skipping descriptor 0x%02x\n", header.bDescriptorType);
			buffer += header.bLength;
			size -= header.bLength;
		}

		/* Copy any unknown descriptors into a storage area for drivers to later parse */
		/* 将所有未知的描述符复制到存储区中，以供驱动程序以后解析 */
		len = (int)(buffer - begin);
		if (len) {
			MARK("save skipped unknown descriptors into config->extra:len=%d", len);
			config->extra = usbi_reallocf((void *) config->extra, config->extra_length + len);
			if UNLIKELY(!config->extra) {
				r = LIBUSB_ERROR_NO_MEM;
				goto err;
			}
			memcpy((unsigned char *)(config->extra + config->extra_length), begin, len);
			config->extra_length += len;
		}
		switch (header.bDescriptorType) {
		case LIBUSB_DT_ASSOCIATION:
			r = parse_association(ctx, config, buffer, size, host_endian);
			if (r < 0)
				goto err;
			break;
		default:
		case LIBUSB_DT_INTERFACE:
			r = parse_interface(ctx, usb_interface + parsed_if, buffer, size, host_endian);
			parsed_if++;
			if (r < 0)
				goto err;
			break;
		}
		if (r == 0) {
			config->bNumInterfaces = (uint8_t)parsed_if;
			break;
		}

		buffer += r;
		size -= r;
	}
	RETURN(size, int);

err:
	clear_configuration(config);
	RETURN(r, int);
}

#if PRINT_DIAG
static void dump_descriptors(unsigned char *buffer, int size) {
	struct usb_descriptor_header header;
	struct libusb_config_descriptor config;
	struct libusb_interface_descriptor interface;
	struct libusb_endpoint_descriptor endpoint;
	int i;

	LOGI("DUMP DESCRIPTIONS");
	for (i = 0; size >= 0; i += header.bLength, size -= header.bLength) {
		if (size == 0) {
			LOGI("END");
			return;
		}

		if (size < LIBUSB_DT_HEADER_SIZE) {
			LOGE("short descriptor read %d/2", size);
			return;
		}
		usbi_parse_descriptor(buffer + i, "bb", &header, 0);
		switch (header.bDescriptorType) {
		case LIBUSB_DT_DEVICE:
			LOGI("LIBUSB_DT_DEVICE(0x%02x),length=%d", header.bDescriptorType, header.bLength);
			break;
		case LIBUSB_DT_CONFIG:
			usbi_parse_descriptor(buffer, "bbwbbbbb", &config, 0);
			LOGI("LIBUSB_DT_CONFIG(0x%02x)", config.bDescriptorType);
			LOGI("\tbLength=%d", config.bLength);
			LOGI("\tbDescriptorType=0x%02x", config.bDescriptorType);
			LOGI("\twTotalLength=%d", config.wTotalLength);
			LOGI("\tbNumInterfaces=%d", config.bNumInterfaces);
			LOGI("\tbConfigurationValue=%d", config.bConfigurationValue);
			LOGI("\tiConfiguration=%d", config.iConfiguration);
			LOGI("\tbmAttributes=%d", config.bmAttributes);
			LOGI("\tMaxPower=%d", config.MaxPower);
			LOGI("\textra_length=%d", config.bLength - LIBUSB_DT_CONFIG_SIZE);
			break;
		case LIBUSB_DT_STRING:
			LOGI("LIBUSB_DT_STRING(0x%02x),length=%d", header.bDescriptorType, header.bLength);
			break;
		case LIBUSB_DT_INTERFACE:
			usbi_parse_descriptor(buffer + i, "bbbbbbbbb", &interface, 0);
			LOGI("LIBUSB_DT_INTERFACE(0x%02x):", header.bDescriptorType);
			LOGI("\tbLength=%d", interface.bLength);
			LOGI("\tbDescriptorType=0x%02x", interface.bDescriptorType);
			LOGI("\tbInterfaceNumber=%d", interface.bInterfaceNumber);
			LOGI("\tbAlternateSetting=%d", interface.bAlternateSetting);
			LOGI("\tbNumEndpoints=%d", interface.bNumEndpoints);
			LOGI("\tbInterfaceClass=0x%02x", interface.bInterfaceClass);
			LOGI("\tbInterfaceSubClass=0x%02x", interface.bInterfaceSubClass);
			LOGI("\tbInterfaceProtocol=0x%02x", interface.bInterfaceProtocol);
			LOGI("\tiInterface=%d", interface.iInterface);
			LOGI("\textra_length=%d", interface.bLength - LIBUSB_DT_INTERFACE_SIZE);
			break;
		case LIBUSB_DT_ENDPOINT:
			usbi_parse_descriptor(buffer + i, "bbbbwbbb", &endpoint, 0);
			LOGI("LIBUSB_DT_ENDPOINT(0x%02x):", header.bDescriptorType);
			LOGI("\tbLength=%d", endpoint.bLength);
			LOGI("\tbDescriptorType=0x%02x", endpoint.bDescriptorType);
			LOGI("\tbEndpointAddress=%d", endpoint.bEndpointAddress);
			LOGI("\tbmAttributes=%d", endpoint.bmAttributes);
			LOGI("\twMaxPacketSize=%d", endpoint.wMaxPacketSize);
			LOGI("\tbInterval=%d", endpoint.bInterval);
			LOGI("\tbRefresh=%d", endpoint.bRefresh);
			LOGI("\tbSynchAddress=%d", endpoint.bSynchAddress);
			LOGI("\textra_length=%d", endpoint.bLength - LIBUSB_DT_ENDPOINT_SIZE);
			break;
		case LIBUSB_DT_DEVICE_QUALIFIER:
			LOGI("LIBUSB_DT_DEVICE_QUALIFIER(0x%02x),length=%d", header.bDescriptorType, header.bLength);
			LOGI("\textra_length=%d", header.bLength - LIBUSB_DT_QUALIFER_SIZE);
			break;
		case LIBUSB_DT_OTHER_SPEED_CONFIGURATION:
			LOGI("LIBUSB_DT_OTHER_SPEED_CONFIGURATION(0x%02x),length=%d", header.bDescriptorType, header.bLength);
			LOGI("\textra_length=%d", header.bLength - LIBUSB_DT_OTHER_SPEED_SIZE);
			break;
		case LIBUSB_DT_INTERFACE_POWER:
			LOGI("LIBUSB_DT_INTERFACE_POWER(0x%02x),length=%d", header.bDescriptorType, header.bLength);
			break;
		case LIBUSB_DT_OTG:
			LOGI("LIBUSB_DT_OTG(0x%02x),length=%d", header.bDescriptorType, header.bLength);
			break;
		case LIBUSB_DT_DEBUG:
			LOGI("LIBUSB_DT_DEBUG(0x%02x),length=%d", header.bDescriptorType, header.bLength);
			break;
		case LIBUSB_DT_ASSOCIATION:
			LOGI("LIBUSB_DT_ASSOCIATION(0x%02x),length=%d", header.bDescriptorType, header.bLength);
			LOGI("\textra_length=%d", header.bLength - LIBUSB_DT_ASSOCIATION_SIZE);
			break;
		case LIBUSB_DT_BOS:
			LOGI("LIBUSB_DT_BOS(0x%02x),length=%d", header.bDescriptorType, header.bLength);
			LOGI("\textra_length=%d", header.bLength - LIBUSB_DT_BOS_SIZE);
			break;
		case LIBUSB_DT_DEVICE_CAPABILITY:
			LOGI("LIBUSB_DT_DEVICE_CAPABILITY(0x%02x),length=%d", header.bDescriptorType, header.bLength);
			LOGI("\textra_length=%d", header.bLength - LIBUSB_DT_DEVICE_CAPABILITY_SIZE);
			break;
		case LIBUSB_DT_HID:
			LOGI("LIBUSB_DT_HID(0x%02x),length=%d", header.bDescriptorType, header.bLength);
			break;
		case LIBUSB_DT_HID_REPORT:
			LOGI("LIBUSB_DT_REPORT(0x%02x),length=%d", header.bDescriptorType, header.bLength);
			break;
		case LIBUSB_DT_HID_PHYSICAL:
			LOGI("LIBUSB_DT_PHYSICAL(0x%02x),length=%d", header.bDescriptorType, header.bLength);
			break;
		case LIBUSB_DT_CS_INTERFACE:
			LOGI("LIBUSB_DT_CS_INTERFACE(0x%02x),length=%d", header.bDescriptorType, header.bLength);
			break;
		case LIBUSB_DT_CS_ENDPOINT:
			LOGI("LIBUSB_DT_CS_ENDPOINT(0x%02x),length=%d", header.bDescriptorType, header.bLength);
			break;
		case LIBUSB_DT_HUB:
			LOGI("LIBUSB_DT_HUB(0x%02x),length=%d", header.bDescriptorType, header.bLength);
			break;
		case LIBUSB_DT_SUPERSPEED_HUB:
			LOGI("LIBUSB_DT_SUPERSPEED_HUB(0x%02x),length=%d", header.bDescriptorType, header.bLength);
			break;
		case LIBUSB_DT_SS_ENDPOINT_COMPANION:
			LOGI("LIBUSB_DT_SS_ENDPOINT_COMPANION(0x%02x),length=%d", header.bDescriptorType, header.bLength);
			break;
		default:
			LOGI("unknown Descriptor(0x%02x),length=0x%02x", header.bDescriptorType, header.bLength);
			break;
		}
	}
}
#endif

static int raw_desc_to_config(struct libusb_context *ctx,
	unsigned char *buf, int size, int host_endian,
	struct libusb_config_descriptor **config)
{
	ENTER();

	struct libusb_config_descriptor *_config = malloc(sizeof(*_config));
	int r;
	
	if UNLIKELY(!_config)
		RETURN(LIBUSB_ERROR_NO_MEM, int);

#if PRINT_DIAG
	dump_descriptors(buf, size);
#endif
	r = parse_configuration(ctx, _config, buf, size, host_endian);
	if UNLIKELY(r < 0) {
		usbi_err(ctx, "parse_configuration failed with error %d", r);
		free(_config);
		return r;
	} else if (r > 0) {
		usbi_warn(ctx, "still %d bytes of descriptor data left", r);
	}
	
	*config = _config;
	RETURN(LIBUSB_SUCCESS, int);
}

int usbi_device_cache_descriptor(libusb_device *dev)
{
	int r, host_endian = 0;

	r = usbi_backend->get_device_descriptor(dev, (unsigned char *) &dev->device_descriptor,
						&host_endian);
	if UNLIKELY(r < 0)
		return r;

	if (!host_endian) {
		dev->device_descriptor.bcdUSB = libusb_le16_to_cpu(dev->device_descriptor.bcdUSB);
		dev->device_descriptor.idVendor = libusb_le16_to_cpu(dev->device_descriptor.idVendor);
		dev->device_descriptor.idProduct = libusb_le16_to_cpu(dev->device_descriptor.idProduct);
		dev->device_descriptor.bcdDevice = libusb_le16_to_cpu(dev->device_descriptor.bcdDevice);
	}

	return LIBUSB_SUCCESS;
}

int API_EXPORTED libusb_get_raw_descriptor(libusb_device *dev,
		unsigned char **buffer, int *descriptors_len, int *host_endian)
{
	if UNLIKELY(!buffer || !descriptors_len || !host_endian)
		return LIBUSB_ERROR_INVALID_PARAM;

	int len, r;
	r = usbi_backend->get_raw_descriptor(dev, NULL, &len, host_endian);
	if (!r) {
		unsigned char *temp = realloc(*buffer, len);
		if UNLIKELY(!temp)
			return LIBUSB_ERROR_NO_MEM;
		*buffer = temp;
		*descriptors_len = len;
		r = usbi_backend->get_raw_descriptor(dev, temp, &len, host_endian);
	}
	return r;
}

/** \ingroup desc
 * Get the USB device descriptor for a given device.
 *
 * This is a non-blocking function; the device descriptor is cached in memory.
 *
 * Note since libusb-1.0.16, \ref LIBUSB_API_VERSION >= 0x01000102, this
 * function always succeeds.
 *
 * \param dev the device
 * \param desc output location for the descriptor data
 * \returns 0 on success or a LIBUSB_ERROR code on failure
 *
 * 获取给定设备的USB设备描述符。
 * 这是一个非阻塞功能； 设备描述符缓存在内存中。
 * 请注意，由于libusb-1.0.16，LIBUSB_API_VERSION >= 0x01000102，此功能始终成功。
 */
int API_EXPORTED libusb_get_device_descriptor(libusb_device *dev,
	struct libusb_device_descriptor *desc)
{
	usbi_dbg("");
	// FIXME add IAD support
	LOGD("desc=%p,dev=%p,device_descriptor=%p", desc, dev, &dev->device_descriptor);
	memcpy((unsigned char *) desc, (unsigned char *) &dev->device_descriptor,
	       sizeof (dev->device_descriptor));
	return 0;
}

/** \ingroup desc
 * Get the USB configuration descriptor for the currently active configuration.
 * This is a non-blocking function which does not involve any requests being
 * sent to the device.
 *
 * \param dev a device
 * \param config output location for the USB configuration descriptor. Only
 * valid if 0 was returned. Must be freed with libusb_free_config_descriptor()
 * after use.
 * \returns 0 on success
 * \returns LIBUSB_ERROR_NOT_FOUND if the device is in unconfigured state
 * \returns another LIBUSB_ERROR code on error
 * \see libusb_get_config_descriptor
 *
 * 获取当前活动配置的USB配置描述符。
 * 这是一种非阻塞功能，不涉及任何发送到设备的请求。
 */
int API_EXPORTED libusb_get_active_config_descriptor(libusb_device *dev,
	struct libusb_config_descriptor **config)
{
	struct libusb_config_descriptor _config;
	unsigned char tmp[LIBUSB_DT_CONFIG_SIZE];
	unsigned char *buf = NULL;
	int host_endian = 0;
	int r;

	r = usbi_backend->get_active_config_descriptor(dev, tmp,	// XXX this function will return error on some buggy device  此功能将在某些故障设备上返回错误
		LIBUSB_DT_CONFIG_SIZE, &host_endian);
	if UNLIKELY(r < 0)
		return r;
	if UNLIKELY(r < LIBUSB_DT_CONFIG_SIZE) {
		usbi_err(dev->ctx, "short config descriptor read %d/%d",
			 r, LIBUSB_DT_CONFIG_SIZE);
		return LIBUSB_ERROR_IO;
	}

	usbi_parse_descriptor(tmp, "bbw", &_config, host_endian);
	buf = malloc(_config.wTotalLength);
	if UNLIKELY(!buf)
		return LIBUSB_ERROR_NO_MEM;

	r = usbi_backend->get_active_config_descriptor(dev, buf,	// XXX this function will return error on some buggy device  此功能将在某些故障设备上返回错误
		_config.wTotalLength, &host_endian);
	if (r >= 0)
		r = raw_desc_to_config(dev->ctx, buf, r, host_endian, config);

	free(buf);
	return r;
}

/** \ingroup desc
 * Get a USB configuration descriptor based on its index.
 * This is a non-blocking function which does not involve any requests being
 * sent to the device.
 *
 * \param dev a device
 * \param config_index the index of the configuration you wish to retrieve
 * \param config output location for the USB configuration descriptor. Only
 * valid if 0 was returned. Must be freed with libusb_free_config_descriptor()
 * after use.
 * \returns 0 on success
 * \returns LIBUSB_ERROR_NOT_FOUND if the configuration does not exist
 * \returns another LIBUSB_ERROR code on error
 * \see libusb_get_active_config_descriptor()
 * \see libusb_get_config_descriptor_by_value()
 *
 * 根据其索引获取USB配置描述符。 这是一种非阻塞功能，不涉及任何发送到设备的请求。
 */
int API_EXPORTED libusb_get_config_descriptor(libusb_device *dev,
	uint8_t config_index, struct libusb_config_descriptor **config)
{
	struct libusb_config_descriptor _config;
	unsigned char tmp[LIBUSB_DT_CONFIG_SIZE];
	unsigned char *buf = NULL;
	int host_endian = 0;
	int r;

	usbi_dbg("index %d", config_index);
	if UNLIKELY(config_index >= dev->num_configurations)
		return LIBUSB_ERROR_NOT_FOUND;

	r = usbi_backend->get_config_descriptor(dev, config_index, tmp,
		LIBUSB_DT_CONFIG_SIZE, &host_endian);
	if UNLIKELY(r < 0)
		return r;
	if UNLIKELY(r < LIBUSB_DT_CONFIG_SIZE) {
		usbi_err(dev->ctx, "short config descriptor read %d/%d",
			 r, LIBUSB_DT_CONFIG_SIZE);
		return LIBUSB_ERROR_IO;
	}

	usbi_parse_descriptor(tmp, "bbw", &_config, host_endian);
	buf = malloc(_config.wTotalLength);
	if UNLIKELY(!buf)
		return LIBUSB_ERROR_NO_MEM;

	r = usbi_backend->get_config_descriptor(dev, config_index, buf,
		_config.wTotalLength, &host_endian);
	if LIKELY(r >= 0)
		r = raw_desc_to_config(dev->ctx, buf, r, host_endian, config);

	free(buf);
	return r;
}

/* iterate through all configurations, returning the index of the configuration
 * matching a specific bConfigurationValue in the idx output parameter, or -1
 * if the config was not found.
 * returns 0 on success or a LIBUSB_ERROR code
 *
 * 遍历所有配置，在idx输出参数中返回与特定bConfigurationValue匹配的配置索引，如果未找到该配置，则返回-1。 成功返回0或LIBUSB_ERROR代码
 */
int usbi_get_config_index_by_value(struct libusb_device *dev,
	uint8_t bConfigurationValue, int *idx)
{
	uint8_t i;

	usbi_dbg("value %d", bConfigurationValue);
	for (i = 0; i < dev->num_configurations; i++) {
		unsigned char tmp[6];
		int host_endian;
		int r = usbi_backend->get_config_descriptor(dev, i, tmp, sizeof(tmp),
			&host_endian);
		if UNLIKELY(r < 0) {
			*idx = -1;
			return r;
		}
		if (tmp[5] == bConfigurationValue) {
			*idx = i;
			return 0;
		}
	}

	*idx = -1;
	return 0;
}

/** \ingroup desc
 * Get a USB configuration descriptor with a specific bConfigurationValue.
 * This is a non-blocking function which does not involve any requests being
 * sent to the device.
 *
 * \param dev a device
 * \param bConfigurationValue the bConfigurationValue of the configuration you
 * wish to retrieve
 * \param config output location for the USB configuration descriptor. Only
 * valid if 0 was returned. Must be freed with libusb_free_config_descriptor()
 * after use.
 * \returns 0 on success
 * \returns LIBUSB_ERROR_NOT_FOUND if the configuration does not exist
 * \returns another LIBUSB_ERROR code on error
 * \see libusb_get_active_config_descriptor()
 * \see libusb_get_config_descriptor()
 *
 * 获取具有特定bConfigurationValue的USB配置描述符。 这是一种非阻塞功能，不涉及任何发送到设备的请求。
 */
int API_EXPORTED libusb_get_config_descriptor_by_value(libusb_device *dev,
	uint8_t bConfigurationValue, struct libusb_config_descriptor **config)
{
	int r, idx, host_endian;
	unsigned char *buf = NULL;

	if (usbi_backend->get_config_descriptor_by_value) {
		r = usbi_backend->get_config_descriptor_by_value(dev,
			bConfigurationValue, &buf, &host_endian);
		if UNLIKELY(r < 0)
			return r;
		return raw_desc_to_config(dev->ctx, buf, r, host_endian, config);
	}

	r = usbi_get_config_index_by_value(dev, bConfigurationValue, &idx);
	if UNLIKELY(r < 0)
		return r;
	else if UNLIKELY(idx == -1)
		return LIBUSB_ERROR_NOT_FOUND;
	else
		return libusb_get_config_descriptor(dev, (uint8_t) idx, config);
}

/** \ingroup desc
 * Free a configuration descriptor obtained from
 * libusb_get_active_config_descriptor() or libusb_get_config_descriptor().
 * It is safe to call this function with a NULL config parameter, in which
 * case the function simply returns.
 *
 * \param config the configuration descriptor to free
 *
 * 释放从libusb_get_active_config_descriptor()或libusb_get_config_descriptor()获得的配置描述符。
 * 使用NULL config参数调用此函数是安全的，在这种情况下，该函数只是返回。
 */
void API_EXPORTED libusb_free_config_descriptor(
	struct libusb_config_descriptor *config)
{
	if UNLIKELY(!config)
		return;

	clear_configuration(config);
	free(config);
}

/** \ingroup desc
 * Get an endpoints superspeed endpoint companion descriptor (if any)
 *
 * \param ctx the context to operate on, or NULL for the default context
 * \param endpoint endpoint descriptor from which to get the superspeed
 * endpoint companion descriptor
 * \param ep_comp output location for the superspeed endpoint companion
 * descriptor. Only valid if 0 was returned. Must be freed with
 * libusb_free_ss_endpoint_companion_descriptor() after use.
 * \returns 0 on success
 * \returns LIBUSB_ERROR_NOT_FOUND if the configuration does not exist
 * \returns another LIBUSB_ERROR code on error
 *
 * 获取端点超速端点伴随描述符（如果有）
 */
int API_EXPORTED libusb_get_ss_endpoint_companion_descriptor(
	struct libusb_context *ctx,
	const struct libusb_endpoint_descriptor *endpoint,
	struct libusb_ss_endpoint_companion_descriptor **ep_comp)
{
	struct usb_descriptor_header header;
	int size = endpoint->extra_length;
	const unsigned char *buffer = endpoint->extra;

	*ep_comp = NULL;

	while (size >= LIBUSB_DT_HEADER_SIZE/*DESC_HEADER_LENGTH*/) {
		usbi_parse_descriptor(buffer, "bb", &header, 0);
		if UNLIKELY(header.bLength < 2 || header.bLength > size) {
			usbi_err(ctx, "invalid descriptor length %d",
				 header.bLength);
			return LIBUSB_ERROR_IO;
		}
		if (header.bDescriptorType != LIBUSB_DT_SS_ENDPOINT_COMPANION) {
			buffer += header.bLength;
			size -= header.bLength;
			continue;
		}
		if UNLIKELY(header.bLength < LIBUSB_DT_SS_ENDPOINT_COMPANION_SIZE) {
			usbi_err(ctx, "invalid ss-ep-comp-desc length %d",
				 header.bLength);
			return LIBUSB_ERROR_IO;
		}
		*ep_comp = malloc(sizeof(**ep_comp));
		if UNLIKELY(!*ep_comp)
			return LIBUSB_ERROR_NO_MEM;
		usbi_parse_descriptor(buffer, "bbbbw", *ep_comp, 0);
		return LIBUSB_SUCCESS;
	}
	return LIBUSB_ERROR_NOT_FOUND;
}

/** \ingroup desc
 * Free a superspeed endpoint companion descriptor obtained from
 * libusb_get_ss_endpoint_companion_descriptor().
 * It is safe to call this function with a NULL ep_comp parameter, in which
 * case the function simply returns.
 *
 * \param ep_comp the superspeed endpoint companion descriptor to free
 *
 * 释放从libusb_get_ss_endpoint_companion_descriptor()获得的超高速端点伴随描述符。
 * 使用NULL ep_comp参数调用此函数是安全的，在这种情况下，该函数只是返回。
 */
void API_EXPORTED libusb_free_ss_endpoint_companion_descriptor(
	struct libusb_ss_endpoint_companion_descriptor *ep_comp)
{
	free(ep_comp);
}

static int parse_bos(struct libusb_context *ctx,
	struct libusb_bos_descriptor **bos,
	unsigned char *buffer, int size, int host_endian)
{
	struct libusb_bos_descriptor bos_header, *_bos;
	struct libusb_bos_dev_capability_descriptor dev_cap;
	int i;

	if UNLIKELY(size < LIBUSB_DT_BOS_SIZE) {
		usbi_err(ctx, "short bos descriptor read %d/%d",
			 size, LIBUSB_DT_BOS_SIZE);
		return LIBUSB_ERROR_IO;
	}

	usbi_parse_descriptor(buffer, "bbwb", &bos_header, host_endian);
	if UNLIKELY(bos_header.bDescriptorType != LIBUSB_DT_BOS) {
		usbi_err(ctx, "unexpected descriptor %x (expected %x)",
			 bos_header.bDescriptorType, LIBUSB_DT_BOS);
		return LIBUSB_ERROR_IO;
	}
	if UNLIKELY(bos_header.bLength < LIBUSB_DT_BOS_SIZE) {
		usbi_err(ctx, "invalid bos bLength (%d)", bos_header.bLength);
		return LIBUSB_ERROR_IO;
	}
	if UNLIKELY(bos_header.bLength > size) {
		usbi_err(ctx, "short bos descriptor read %d/%d",
			 size, bos_header.bLength);
		return LIBUSB_ERROR_IO;
	}

	_bos = calloc (1,
		sizeof(*_bos) + bos_header.bNumDeviceCaps * sizeof(void *));
	if UNLIKELY(!_bos)
		return LIBUSB_ERROR_NO_MEM;

	usbi_parse_descriptor(buffer, "bbwb", _bos, host_endian);
	buffer += bos_header.bLength;
	size -= bos_header.bLength;

	/* Get the device capability descriptors
	 * 获取设备功能描述符
	 */
	for (i = 0; i < bos_header.bNumDeviceCaps; i++) {
		if (size < LIBUSB_DT_DEVICE_CAPABILITY_SIZE) {
			usbi_warn(ctx, "short dev-cap descriptor read %d/%d",
				  size, LIBUSB_DT_DEVICE_CAPABILITY_SIZE);
			break;
		}
		usbi_parse_descriptor(buffer, "bbb", &dev_cap, host_endian);
		if (dev_cap.bDescriptorType != LIBUSB_DT_DEVICE_CAPABILITY) {
			usbi_warn(ctx, "unexpected descriptor %x (expected %x)",
				  dev_cap.bDescriptorType, LIBUSB_DT_DEVICE_CAPABILITY);
			break;
		}
		if UNLIKELY(dev_cap.bLength < LIBUSB_DT_DEVICE_CAPABILITY_SIZE) {
			usbi_err(ctx, "invalid dev-cap bLength (%d)",
				 dev_cap.bLength);
			libusb_free_bos_descriptor(_bos);
			return LIBUSB_ERROR_IO;
		}
		if (dev_cap.bLength > size) {
			usbi_warn(ctx, "short dev-cap descriptor read %d/%d",
				  size, dev_cap.bLength);
			break;
		}

		_bos->dev_capability[i] = malloc(dev_cap.bLength);
		if UNLIKELY(!_bos->dev_capability[i]) {
			libusb_free_bos_descriptor(_bos);
			return LIBUSB_ERROR_NO_MEM;
		}
		memcpy(_bos->dev_capability[i], buffer, dev_cap.bLength);
		buffer += dev_cap.bLength;
		size -= dev_cap.bLength;
	}
	_bos->bNumDeviceCaps = (uint8_t)i;
	*bos = _bos;

	return LIBUSB_SUCCESS;
}

/** \ingroup desc
 * Get a Binary Object Store (BOS) descriptor
 * This is a BLOCKING function, which will send requests to the device.
 *
 * \param handle the handle of an open libusb device
 * \param bos output location for the BOS descriptor. Only valid if 0 was returned.
 * Must be freed with \ref libusb_free_bos_descriptor() after use.
 * \returns 0 on success
 * \returns LIBUSB_ERROR_NOT_FOUND if the device doesn't have a BOS descriptor
 * \returns another LIBUSB_ERROR code on error
 *
 * 获取二进制对象存储（BOS）描述符这是一个BLOCKING函数，它将请求发送给设备。
 */
int API_EXPORTED libusb_get_bos_descriptor(libusb_device_handle *handle,
	struct libusb_bos_descriptor **bos)
{
	struct libusb_bos_descriptor _bos;
	uint8_t bos_header[LIBUSB_DT_BOS_SIZE] = {0};
	unsigned char *bos_data = NULL;
	const int host_endian = 0;
	int r;

	/* Read the BOS. This generates 2 requests on the bus,
	 * one for the header, and one for the full BOS
	 * 阅读BOS。 这会在总线上生成2个请求，一个用于标头，一个用于完整的BOS。 */
	r = libusb_get_descriptor(handle, LIBUSB_DT_BOS, 0, bos_header,
				  LIBUSB_DT_BOS_SIZE);
	if UNLIKELY(r < 0) {
		if (r != LIBUSB_ERROR_PIPE)
			usbi_err(handle->dev->ctx, "failed to read BOS (%d)", r);
		return r;
	}
	if UNLIKELY(r < LIBUSB_DT_BOS_SIZE) {
		usbi_err(handle->dev->ctx, "short BOS read %d/%d",
			 r, LIBUSB_DT_BOS_SIZE);
		return LIBUSB_ERROR_IO;
	}

	usbi_parse_descriptor(bos_header, "bbwb", &_bos, host_endian);
	usbi_dbg("found BOS descriptor: size %d bytes, %d capabilities",
		 _bos.wTotalLength, _bos.bNumDeviceCaps);
	bos_data = calloc(_bos.wTotalLength, 1);
	if UNLIKELY(!bos_data)
		return LIBUSB_ERROR_NO_MEM;

	r = libusb_get_descriptor(handle, LIBUSB_DT_BOS, 0, bos_data,
				  _bos.wTotalLength);
	if LIKELY(r >= 0)
		r = parse_bos(handle->dev->ctx, bos, bos_data, r, host_endian);
	else
		usbi_err(handle->dev->ctx, "failed to read BOS (%d)", r);

	free(bos_data);
	return r;
}

/** \ingroup desc
 * Free a BOS descriptor obtained from libusb_get_bos_descriptor().
 * It is safe to call this function with a NULL bos parameter, in which
 * case the function simply returns.
 *
 * \param bos the BOS descriptor to free
 *
 * 释放从libusb_get_bos_descriptor() 获得的BOS描述符。 使用NULL bos参数调用此函数是安全的，在这种情况下，该函数只是返回。
 */
void API_EXPORTED libusb_free_bos_descriptor(struct libusb_bos_descriptor *bos)
{
	int i;

	if (!bos)
		return;

	for (i = 0; i < bos->bNumDeviceCaps; i++)
		free(bos->dev_capability[i]);
	free(bos);
}

/** \ingroup desc
 * Get an USB 2.0 Extension descriptor
 *
 * \param ctx the context to operate on, or NULL for the default context
 * \param dev_cap Device Capability descriptor with a bDevCapabilityType of
 * \ref libusb_capability_type::LIBUSB_BT_USB_2_0_EXTENSION
 * LIBUSB_BT_USB_2_0_EXTENSION
 * \param usb_2_0_extension output location for the USB 2.0 Extension
 * descriptor. Only valid if 0 was returned. Must be freed with
 * libusb_free_usb_2_0_extension_descriptor() after use.
 * \returns 0 on success
 * \returns a LIBUSB_ERROR code on error
 *
 * 获取USB 2.0扩展描述符
 */
int API_EXPORTED libusb_get_usb_2_0_extension_descriptor(
	struct libusb_context *ctx,
	struct libusb_bos_dev_capability_descriptor *dev_cap,
	struct libusb_usb_2_0_extension_descriptor **usb_2_0_extension)
{
	struct libusb_usb_2_0_extension_descriptor *_usb_2_0_extension;
	const int host_endian = 0;

	if UNLIKELY(dev_cap->bDevCapabilityType != LIBUSB_BT_USB_2_0_EXTENSION) {
		usbi_err(ctx, "unexpected bDevCapabilityType %x (expected %x)",
			 dev_cap->bDevCapabilityType,
			 LIBUSB_BT_USB_2_0_EXTENSION);
		return LIBUSB_ERROR_INVALID_PARAM;
	}
	if UNLIKELY(dev_cap->bLength < LIBUSB_BT_USB_2_0_EXTENSION_SIZE) {
		usbi_err(ctx, "short dev-cap descriptor read %d/%d",
			 dev_cap->bLength, LIBUSB_BT_USB_2_0_EXTENSION_SIZE);
		return LIBUSB_ERROR_IO;
	}

	_usb_2_0_extension = malloc(sizeof(*_usb_2_0_extension));
	if UNLIKELY(!_usb_2_0_extension)
		return LIBUSB_ERROR_NO_MEM;

	usbi_parse_descriptor((unsigned char *)dev_cap, "bbbd",
			      _usb_2_0_extension, host_endian);

	*usb_2_0_extension = _usb_2_0_extension;
	return LIBUSB_SUCCESS;
}

/** \ingroup desc
 * Free a USB 2.0 Extension descriptor obtained from
 * libusb_get_usb_2_0_extension_descriptor().
 * It is safe to call this function with a NULL usb_2_0_extension parameter,
 * in which case the function simply returns.
 *
 * \param usb_2_0_extension the USB 2.0 Extension descriptor to free
 *
 * 释放从libusb_get_usb_2_0_extension_descriptor() 获得的USB 2.0扩展描述符。 使用NULL usb_2_0_extension参数调用此函数是安全的，在这种情况下，该函数只是返回。
 */
void API_EXPORTED libusb_free_usb_2_0_extension_descriptor(
	struct libusb_usb_2_0_extension_descriptor *usb_2_0_extension)
{
	free(usb_2_0_extension);
}

/** \ingroup desc
 * Get a SuperSpeed USB Device Capability descriptor
 *
 * \param ctx the context to operate on, or NULL for the default context
 * \param dev_cap Device Capability descriptor with a bDevCapabilityType of
 * \ref libusb_capability_type::LIBUSB_BT_SS_USB_DEVICE_CAPABILITY
 * LIBUSB_BT_SS_USB_DEVICE_CAPABILITY
 * \param ss_usb_device_cap output location for the SuperSpeed USB Device
 * Capability descriptor. Only valid if 0 was returned. Must be freed with
 * libusb_free_ss_usb_device_capability_descriptor() after use.
 * \returns 0 on success
 * \returns a LIBUSB_ERROR code on error
 *
 * 获取SuperSpeed USB设备功能描述符
 */
int API_EXPORTED libusb_get_ss_usb_device_capability_descriptor(
	struct libusb_context *ctx,
	struct libusb_bos_dev_capability_descriptor *dev_cap,
	struct libusb_ss_usb_device_capability_descriptor **ss_usb_device_cap)
{
	struct libusb_ss_usb_device_capability_descriptor *_ss_usb_device_cap;
	const int host_endian = 0;

	if UNLIKELY(dev_cap->bDevCapabilityType != LIBUSB_BT_SS_USB_DEVICE_CAPABILITY) {
		usbi_err(ctx, "unexpected bDevCapabilityType %x (expected %x)",
			 dev_cap->bDevCapabilityType,
			 LIBUSB_BT_SS_USB_DEVICE_CAPABILITY);
		return LIBUSB_ERROR_INVALID_PARAM;
	}
	if UNLIKELY(dev_cap->bLength < LIBUSB_BT_SS_USB_DEVICE_CAPABILITY_SIZE) {
		usbi_err(ctx, "short dev-cap descriptor read %d/%d",
			 dev_cap->bLength, LIBUSB_BT_SS_USB_DEVICE_CAPABILITY_SIZE);
		return LIBUSB_ERROR_IO;
	}

	_ss_usb_device_cap = malloc(sizeof(*_ss_usb_device_cap));
	if UNLIKELY(!_ss_usb_device_cap)
		return LIBUSB_ERROR_NO_MEM;

	usbi_parse_descriptor((unsigned char *)dev_cap, "bbbbwbbw",
			      _ss_usb_device_cap, host_endian);

	*ss_usb_device_cap = _ss_usb_device_cap;
	return LIBUSB_SUCCESS;
}

/** \ingroup desc
 * Free a SuperSpeed USB Device Capability descriptor obtained from
 * libusb_get_ss_usb_device_capability_descriptor().
 * It is safe to call this function with a NULL ss_usb_device_cap
 * parameter, in which case the function simply returns.
 *
 * \param ss_usb_device_cap the USB 2.0 Extension descriptor to free
 *
 * 释放从libusb_get_ss_usb_device_capability_descriptor()获得的SuperSpeed USB设备功能描述符。 使用NULL ss_usb_device_cap参数调用此函数是安全的，在这种情况下，该函数只是返回。
 */
void API_EXPORTED libusb_free_ss_usb_device_capability_descriptor(
	struct libusb_ss_usb_device_capability_descriptor *ss_usb_device_cap)
{
	free(ss_usb_device_cap);
}

/** \ingroup desc
 * Get a Container ID descriptor
 *
 * \param ctx the context to operate on, or NULL for the default context
 * \param dev_cap Device Capability descriptor with a bDevCapabilityType of
 * \ref libusb_capability_type::LIBUSB_BT_CONTAINER_ID
 * LIBUSB_BT_CONTAINER_ID
 * \param container_id output location for the Container ID descriptor.
 * Only valid if 0 was returned. Must be freed with
 * libusb_free_container_id_descriptor() after use.
 * \returns 0 on success
 * \returns a LIBUSB_ERROR code on error
 *
 * 获取容器ID描述符
 */
int API_EXPORTED libusb_get_container_id_descriptor(struct libusb_context *ctx,
	struct libusb_bos_dev_capability_descriptor *dev_cap,
	struct libusb_container_id_descriptor **container_id)
{
	struct libusb_container_id_descriptor *_container_id;
	const int host_endian = 0;

	if UNLIKELY(dev_cap->bDevCapabilityType != LIBUSB_BT_CONTAINER_ID) {
		usbi_err(ctx, "unexpected bDevCapabilityType %x (expected %x)",
			 dev_cap->bDevCapabilityType,
			 LIBUSB_BT_CONTAINER_ID);
		return LIBUSB_ERROR_INVALID_PARAM;
	}
	if UNLIKELY(dev_cap->bLength < LIBUSB_BT_CONTAINER_ID_SIZE) {
		usbi_err(ctx, "short dev-cap descriptor read %d/%d",
			 dev_cap->bLength, LIBUSB_BT_CONTAINER_ID_SIZE);
		return LIBUSB_ERROR_IO;
	}

	_container_id = malloc(sizeof(*_container_id));
	if UNLIKELY(!_container_id)
		return LIBUSB_ERROR_NO_MEM;

	usbi_parse_descriptor((unsigned char *)dev_cap, "bbbbu",
			      _container_id, host_endian);

	*container_id = _container_id;
	return LIBUSB_SUCCESS;
}

/** \ingroup desc
 * Free a Container ID descriptor obtained from
 * libusb_get_container_id_descriptor().
 * It is safe to call this function with a NULL container_id parameter,
 * in which case the function simply returns.
 *
 * \param container_id the USB 2.0 Extension descriptor to free
 *
 * 释放从libusb_get_container_id_descriptor()获得的容器ID描述符。 使用NULL container_id参数调用此函数是安全的，在这种情况下，该函数只是返回。
 */
void API_EXPORTED libusb_free_container_id_descriptor(
	struct libusb_container_id_descriptor *container_id)
{
	free(container_id);
}

/** \ingroup desc
 * Retrieve a string descriptor in C style ASCII.
 *
 * Wrapper around libusb_get_string_descriptor(). Uses the first language
 * supported by the device.
 *
 * \param dev a device handle
 * \param desc_index the index of the descriptor to retrieve
 * \param data output buffer for ASCII string descriptor
 * \param length size of data buffer
 * \returns number of bytes returned in data, or LIBUSB_ERROR code on failure
 *
 * 以C样式ASCII检索字符串描述符。
 * 围绕libusb_get_string_descriptor()的包装。 使用设备支持的第一语言。
 */
int API_EXPORTED libusb_get_string_descriptor_ascii(libusb_device_handle *dev,
	uint8_t desc_index, unsigned char *data, int length)
{
	unsigned char tbuf[255]; /* Some devices choke on size > 255 */
	int r, si, di;
	uint16_t langid;

	/* Asking for the zero'th index is special - it returns a string
	 * descriptor that contains all the language IDs supported by the
	 * device. Typically there aren't many - often only one. Language
	 * IDs are 16 bit numbers, and they start at the third byte in the
	 * descriptor. There's also no point in trying to read descriptor 0
	 * with this function. See USB 2.0 specification section 9.6.7 for
	 * more information.
	 *
	 * 要求第零个索引很特殊-它返回一个字符串描述符，其中包含设备支持的所有语言ID。
	 * 通常没有很多-通常只有一个。 语言ID是16位数字，它们从描述符的第三个字节开始。
	 * 尝试使用此函数读取描述符0也没有意义。 有关更多信息，请参见USB 2.0规范第9.6.7节。
	 */

	if UNLIKELY(!desc_index)
		return LIBUSB_ERROR_INVALID_PARAM;

	r = libusb_get_string_descriptor(dev, 0, 0, tbuf, sizeof(tbuf));
	if UNLIKELY(r < 0)
		return r;

	if UNLIKELY(r < 4)
		return LIBUSB_ERROR_IO;

	langid = tbuf[2] | (tbuf[3] << 8);

	r = libusb_get_string_descriptor(dev, desc_index, langid, tbuf, sizeof(tbuf));
	if UNLIKELY(r < 0)
		return r;

	if UNLIKELY(tbuf[1] != LIBUSB_DT_STRING)
		return LIBUSB_ERROR_IO;

	if UNLIKELY(tbuf[0] > r)
		return LIBUSB_ERROR_IO;

	for (di = 0, si = 2; si < tbuf[0]; si += 2) {
		if (di >= (length - 1))
			break;

		if ((tbuf[si] & 0x80) || (tbuf[si + 1])) /* non-ASCII */
			data[di++] = '?';
		else
			data[di++] = tbuf[si];
	}

	data[di] = 0;
	return di;
}
