/*
 *  UVCCamera
 *  library and sample to access to UVC web camera on non-rooted Android device
 *
 * Copyright (c) 2014-2017 saki t_saki@serenegiant.com
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *   You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *   Unless required by applicable law or agreed to in writing, software
 *   distributed under the License is distributed on an "AS IS" BASIS,
 *   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *   See the License for the specific language governing permissions and
 *   limitations under the License.
 *
 *  All files in the folder are under this Apache License, Version 2.0.
 *  Files in the libjpeg-turbo, libusb, libuvc, rapidjson folder
 *  may have a different license, see the respective files.
 */

package com.serenegiant.usb;

import java.io.IOException;
import java.util.ArrayList;
import java.util.Collections;
import java.util.List;

import org.xmlpull.v1.XmlPullParser;
import org.xmlpull.v1.XmlPullParserException;

import android.content.Context;
import android.content.res.Resources.NotFoundException;
import android.hardware.usb.UsbDevice;
import android.hardware.usb.UsbInterface;
import android.text.TextUtils;
import android.util.Log;

public final class DeviceFilter {

	private static final String TAG = "DeviceFilter";

	// USB Vendor ID (or -1 for unspecified)
	public final int mVendorId;
	// USB Product ID (or -1 for unspecified)
	public final int mProductId;
	// USB device or interface class (or -1 for unspecified)
	public final int mClass;
	// USB device subclass (or -1 for unspecified)
	public final int mSubclass;
	// USB device protocol (or -1 for unspecified)
	public final int mProtocol;
	// USB device manufacturer name string (or null for unspecified)
	public final String mManufacturerName;
	// USB device product name string (or null for unspecified)
	public final String mProductName;
	// USB device serial number string (or null for unspecified)
	public final String mSerialNumber;
	// set true if specific device(s) should exclude
	public final boolean isExclude;

	public DeviceFilter(final int vid, final int pid, final int clasz, final int subclass,
			final int protocol, final String manufacturer, final String product, final String serialNum) {
		this(vid, pid, clasz, subclass, protocol, manufacturer, product, serialNum, false);
	}

	public DeviceFilter(final int vid, final int pid, final int clasz, final int subclass,
			final int protocol, final String manufacturer, final String product, final String serialNum, final boolean isExclude) {
		mVendorId = vid;
		mProductId = pid;
		mClass = clasz;
		mSubclass = subclass;
		mProtocol = protocol;
		mManufacturerName = manufacturer;
		mProductName = product;
		mSerialNumber = serialNum;
		this.isExclude = isExclude;
/*		Log.i(TAG, String.format("vendorId=0x%04x,productId=0x%04x,class=0x%02x,subclass=0x%02x,protocol=0x%02x",
			mVendorId, mProductId, mClass, mSubclass, mProtocol)); */
	}

	public DeviceFilter(final UsbDevice device) {
		this(device, false);
	}

	public DeviceFilter(final UsbDevice device, final boolean isExclude) {
		mVendorId = device.getVendorId();
		mProductId = device.getProductId();
		mClass = device.getDeviceClass();
		mSubclass = device.getDeviceSubclass();
		mProtocol = device.getDeviceProtocol();
		mManufacturerName = null;	// device.getManufacturerName();
		mProductName = null;		// device.getProductName();
		mSerialNumber = null;		// device.getSerialNumber();
		this.isExclude = isExclude;
/*		Log.i(TAG, String.format("vendorId=0x%04x,productId=0x%04x,class=0x%02x,subclass=0x%02x,protocol=0x%02x",
			mVendorId, mProductId, mClass, mSubclass, mProtocol)); */
	}

	/**
	 * 指定したxmlリソースからDeviceFilterリストを生成する
	 * @param context
	 * @param deviceFilterXmlId
	 * @return
	 */
	public static List<DeviceFilter> getDeviceFilters(final Context context, final int deviceFilterXmlId) {
		final XmlPullParser parser = context.getResources().getXml(deviceFilterXmlId);
		final List<DeviceFilter> deviceFilters = new ArrayList<DeviceFilter>();
		try {
			int eventType = parser.getEventType();
			while (eventType != XmlPullParser.END_DOCUMENT) {
	            if (eventType == XmlPullParser.START_TAG) {
					final DeviceFilter deviceFilter = readEntryOne(context, parser);
					if (deviceFilter != null) {
						deviceFilters.add(deviceFilter);
					}
	            }
				eventType = parser.next();
			}
		} catch (final XmlPullParserException e) {
			Log.d(TAG, "XmlPullParserException", e);
		} catch (final IOException e) {
			Log.d(TAG, "IOException", e);
		}

		return Collections.unmodifiableList(deviceFilters);
	}

	/**
	 * read as integer values with default value from xml(w/o exception throws)
	 * resource integer id is also resolved into integer
	 * @param parser
	 * @param namespace
	 * @param name
	 * @param defaultValue
	 * @return
	 */
	private static final int getAttributeInteger(final Context context, final XmlPullParser parser, final String namespace, final String name, final int defaultValue) {
		int result = defaultValue;
		try {
			String v = parser.getAttributeValue(namespace, name);
			if (!TextUtils.isEmpty(v) && v.startsWith("@")) {
				final String r = v.substring(1);
				final int resId = context.getResources().getIdentifier(r, null, context.getPackageName());
				if (resId > 0) {
					result = context.getResources().getInteger(resId);
				}
			} else {
                int radix = 10;
                if (v != null && v.length() > 2 && v.charAt(0) == '0' &&
                    (v.charAt(1) == 'x' || v.charAt(1) == 'X')) {
                    // allow hex values starting with 0x or 0X
                    radix = 16;
                    v = v.substring(2);
                }
				result = Integer.parseInt(v, radix);
			}
		} catch (final NotFoundException e) {
			result = defaultValue;
		} catch (final NumberFormatException e) {
			result = defaultValue;
		} catch (final NullPointerException e) {
			result = defaultValue;
		}
		return result;
	}

	/**
	 * read as boolean values with default value from xml(w/o exception throws)
	 * resource boolean id is also resolved into boolean
	 * if the value is zero, return false, if the value is non-zero integer, return true
	 * @param context
	 * @param parser
	 * @param namespace
	 * @param name
	 * @param defaultValue
	 * @return
	 */
	private static final boolean getAttributeBoolean(final Context context, final XmlPullParser parser, final String namespace, final String name, final boolean defaultValue) {
		boolean result = defaultValue;
		try {
			String v = parser.getAttributeValue(namespace, name);
			if ("TRUE".equalsIgnoreCase(v)) {
				result = true;
			} else if ("FALSE".equalsIgnoreCase(v)) {
				result = false;
			} else if (!TextUtils.isEmpty(v) && v.startsWith("@")) {
				final String r = v.substring(1);
				final int resId = context.getResources().getIdentifier(r, null, context.getPackageName());
				if (resId > 0) {
					result = context.getResources().getBoolean(resId);
				}
			} else {
                int radix = 10;
                if (v != null && v.length() > 2 && v.charAt(0) == '0' &&
                    (v.charAt(1) == 'x' || v.charAt(1) == 'X')) {
                    // allow hex values starting with 0x or 0X
                    radix = 16;
                    v = v.substring(2);
                }
				final int val = Integer.parseInt(v, radix);
				result = val != 0;
			}
		} catch (final NotFoundException e) {
			result = defaultValue;
		} catch (final NumberFormatException e) {
			result = defaultValue;
		} catch (final NullPointerException e) {
			result = defaultValue;
		}
		return result;
	}

	/**
	 * read as String attribute with default value from xml(w/o exception throws)
	 * resource string id is also resolved into string
	 * @param parser
	 * @param namespace
	 * @param name
	 * @param defaultValue
	 * @return
	 */
	private static final String getAttributeString(final Context context, final XmlPullParser parser, final String namespace, final String name, final String defaultValue) {
		String result = defaultValue;
		try {
			result = parser.getAttributeValue(namespace, name);
			if (result == null)
				result = defaultValue;
			if (!TextUtils.isEmpty(result) && result.startsWith("@")) {
				final String r = result.substring(1);
				final int resId = context.getResources().getIdentifier(r, null, context.getPackageName());
				if (resId > 0)
					result = context.getResources().getString(resId);
			}
		} catch (final NotFoundException e) {
			result = defaultValue;
		} catch (final NumberFormatException e) {
			result = defaultValue;
		} catch (final NullPointerException e) {
			result = defaultValue;
		}
		return result;
	}

	public static DeviceFilter readEntryOne(final Context context, final XmlPullParser parser)
			throws XmlPullParserException, IOException {
		int vendorId = -1;
		int productId = -1;
		int deviceClass = -1;
		int deviceSubclass = -1;
		int deviceProtocol = -1;
		boolean exclude = false;
		String manufacturerName = null;
		String productName = null;
		String serialNumber = null;
		boolean hasValue = false;

		String tag;
        int eventType = parser.getEventType();
        while (eventType != XmlPullParser.END_DOCUMENT) {
        	tag = parser.getName();
        	if (!TextUtils.isEmpty(tag) && (tag.equalsIgnoreCase("usb-device"))) {
        		if (eventType == XmlPullParser.START_TAG) {
        			hasValue = true;
					vendorId = getAttributeInteger(context, parser, null, "vendor-id", -1);
        			if (vendorId == -1) {
        				vendorId = getAttributeInteger(context, parser, null, "vendorId", -1);
        				if (vendorId == -1)
                			vendorId = getAttributeInteger(context, parser, null, "venderId", -1);
        			}
    				productId = getAttributeInteger(context, parser, null, "product-id", -1);
        			if (productId == -1)
            			productId = getAttributeInteger(context, parser, null, "productId", -1);
        			deviceClass = getAttributeInteger(context, parser, null, "class", -1);
        			deviceSubclass = getAttributeInteger(context, parser, null, "subclass", -1);
        			deviceProtocol = getAttributeInteger(context, parser, null, "protocol", -1);
        			manufacturerName = getAttributeString(context, parser, null, "manufacturer-name", null);
        			if (TextUtils.isEmpty(manufacturerName))
        				manufacturerName = getAttributeString(context, parser, null, "manufacture", null);
        			productName = getAttributeString(context, parser, null, "product-name", null);
        			if (TextUtils.isEmpty(productName))
        				productName = getAttributeString(context, parser, null, "product", null);
        			serialNumber = getAttributeString(context, parser, null, "serial-number", null);
        			if (TextUtils.isEmpty(serialNumber))
            			serialNumber = getAttributeString(context, parser, null, "serial", null);
					exclude = getAttributeBoolean(context, parser, null, "exclude", false);
        		} else if (eventType == XmlPullParser.END_TAG) {
        			if (hasValue) {
	        			return new DeviceFilter(vendorId, productId, deviceClass,
	        					deviceSubclass, deviceProtocol, manufacturerName, productName,
	        					serialNumber, exclude);
        			}
        		}
        	}
        	eventType = parser.next();
        }
        return null;
	}

/*	public void write(XmlSerializer serializer) throws IOException {
		serializer.startTag(null, "usb-device");
		if (mVendorId != -1) {
			serializer
					.attribute(null, "vendor-id", Integer.toString(mVendorId));
		}
		if (mProductId != -1) {
			serializer.attribute(null, "product-id",
					Integer.toString(mProductId));
		}
		if (mClass != -1) {
			serializer.attribute(null, "class", Integer.toString(mClass));
		}
		if (mSubclass != -1) {
			serializer.attribute(null, "subclass", Integer.toString(mSubclass));
		}
		if (mProtocol != -1) {
			serializer.attribute(null, "protocol", Integer.toString(mProtocol));
		}
		if (mManufacturerName != null) {
			serializer.attribute(null, "manufacturer-name", mManufacturerName);
		}
		if (mProductName != null) {
			serializer.attribute(null, "product-name", mProductName);
		}
		if (mSerialNumber != null) {
			serializer.attribute(null, "serial-number", mSerialNumber);
		}
		serializer.attribute(null, "serial-number", Boolean.toString(isExclude));
		serializer.endTag(null, "usb-device");
	} */

	/**
	 * 指定したクラス・サブクラス・プロトコルがこのDeviceFilterとマッチするかどうかを返す
	 * mExcludeフラグは別途#isExcludeか自前でチェックすること
	 * @param clasz
	 * @param subclass
	 * @param protocol
	 * @return
	 */
	private boolean matches(final int clasz, final int subclass, final int protocol) {
		return ((mClass == -1 || clasz == mClass)
				&& (mSubclass == -1 || subclass == mSubclass) && (mProtocol == -1 || protocol == mProtocol));
	}

	/**
	 * 指定したUsbDeviceがこのDeviceFilterにマッチするかどうかを返す
	 * mExcludeフラグは別途#isExcludeか自前でチェックすること
	 * @param device
	 * @return
	 */
	public boolean matches(final UsbDevice device) {
		if (mVendorId != -1 && device.getVendorId() != mVendorId) {
			return false;
		}
		if (mProductId != -1 && device.getProductId() != mProductId) {
			return false;
		}
/*		if (mManufacturerName != null && device.getManufacturerName() == null)
			return false;
		if (mProductName != null && device.getProductName() == null)
			return false;
		if (mSerialNumber != null && device.getSerialNumber() == null)
			return false;
		if (mManufacturerName != null && device.getManufacturerName() != null
				&& !mManufacturerName.equals(device.getManufacturerName()))
			return false;
		if (mProductName != null && device.getProductName() != null
				&& !mProductName.equals(device.getProductName()))
			return false;
		if (mSerialNumber != null && device.getSerialNumber() != null
				&& !mSerialNumber.equals(device.getSerialNumber()))
			return false; */

		// check device class/subclass/protocol
		if (matches(device.getDeviceClass(), device.getDeviceSubclass(), device.getDeviceProtocol())) {
			return true;
		}

		// if device doesn't match, check the interfaces
		final int count = device.getInterfaceCount();
		for (int i = 0; i < count; i++) {
			final UsbInterface intf = device.getInterface(i);
			if (matches(intf.getInterfaceClass(), intf.getInterfaceSubclass(), intf.getInterfaceProtocol())) {
				return true;
			}
		}

		return false;
	}

	/**
	 * このDeviceFilterに一致してかつmExcludeがtrueならtrueを返す
	 * @param device
	 * @return
	 */
	public boolean isExclude(final UsbDevice device) {
		return isExclude && matches(device);
	}

	/**
	 * これって要らんかも, equalsでできる気が
	 * @param f
	 * @return
	 */
	public boolean matches(final DeviceFilter f) {
		if (isExclude != f.isExclude) {
			return false;
		}
		if (mVendorId != -1 && f.mVendorId != mVendorId) {
			return false;
		}
		if (mProductId != -1 && f.mProductId != mProductId) {
			return false;
		}
		if (f.mManufacturerName != null && mManufacturerName == null) {
			return false;
		}
		if (f.mProductName != null && mProductName == null) {
			return false;
		}
		if (f.mSerialNumber != null && mSerialNumber == null) {
			return false;
		}
		if (mManufacturerName != null && f.mManufacturerName != null
				&& !mManufacturerName.equals(f.mManufacturerName)) {
			return false;
		}
		if (mProductName != null && f.mProductName != null
				&& !mProductName.equals(f.mProductName)) {
			return false;
		}
		if (mSerialNumber != null && f.mSerialNumber != null
				&& !mSerialNumber.equals(f.mSerialNumber)) {
			return false;
		}

		// check device class/subclass/protocol
		return matches(f.mClass, f.mSubclass, f.mProtocol);
	}

	@Override
	public boolean equals(final Object obj) {
		// can't compare if we have wildcard strings
		if (mVendorId == -1 || mProductId == -1 || mClass == -1
				|| mSubclass == -1 || mProtocol == -1) {
			return false;
		}
		if (obj instanceof DeviceFilter) {
			final DeviceFilter filter = (DeviceFilter) obj;

			if (filter.mVendorId != mVendorId
					|| filter.mProductId != mProductId
					|| filter.mClass != mClass || filter.mSubclass != mSubclass
					|| filter.mProtocol != mProtocol) {
				return false;
			}
			if ((filter.mManufacturerName != null && mManufacturerName == null)
					|| (filter.mManufacturerName == null && mManufacturerName != null)
					|| (filter.mProductName != null && mProductName == null)
					|| (filter.mProductName == null && mProductName != null)
					|| (filter.mSerialNumber != null && mSerialNumber == null)
					|| (filter.mSerialNumber == null && mSerialNumber != null)) {
				return false;
			}
			if ((filter.mManufacturerName != null && mManufacturerName != null && !mManufacturerName
					.equals(filter.mManufacturerName))
					|| (filter.mProductName != null && mProductName != null && !mProductName
							.equals(filter.mProductName))
					|| (filter.mSerialNumber != null && mSerialNumber != null && !mSerialNumber
							.equals(filter.mSerialNumber))) {
				return false;
			}
			return (filter.isExclude != isExclude);
		}
		if (obj instanceof UsbDevice) {
			final UsbDevice device = (UsbDevice) obj;
			if (isExclude
					|| (device.getVendorId() != mVendorId)
					|| (device.getProductId() != mProductId)
					|| (device.getDeviceClass() != mClass)
					|| (device.getDeviceSubclass() != mSubclass)
					|| (device.getDeviceProtocol() != mProtocol) ) {
				return false;
			}
/*			if ((mManufacturerName != null && device.getManufacturerName() == null)
					|| (mManufacturerName == null && device
							.getManufacturerName() != null)
					|| (mProductName != null && device.getProductName() == null)
					|| (mProductName == null && device.getProductName() != null)
					|| (mSerialNumber != null && device.getSerialNumber() == null)
					|| (mSerialNumber == null && device.getSerialNumber() != null)) {
				return (false);
			} */
/*			if ((device.getManufacturerName() != null && !mManufacturerName
					.equals(device.getManufacturerName()))
					|| (device.getProductName() != null && !mProductName
							.equals(device.getProductName()))
					|| (device.getSerialNumber() != null && !mSerialNumber
							.equals(device.getSerialNumber()))) {
				return (false);
			} */
			return true;
		}
		return false;
	}

	@Override
	public int hashCode() {
		return (((mVendorId << 16) | mProductId) ^ ((mClass << 16)
				| (mSubclass << 8) | mProtocol));
	}

	@Override
	public String toString() {
		return "DeviceFilter[mVendorId=" + mVendorId + ",mProductId="
			+ mProductId + ",mClass=" + mClass + ",mSubclass=" + mSubclass
			+ ",mProtocol=" + mProtocol
			+ ",mManufacturerName=" + mManufacturerName
			+ ",mProductName=" + mProductName
			+ ",mSerialNumber=" + mSerialNumber
			+ ",isExclude=" + isExclude
			+ "]";
	}

}
