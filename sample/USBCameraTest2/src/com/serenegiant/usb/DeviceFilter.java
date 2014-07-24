package com.serenegiant.usb;

/*
 * This class came from com.android.server.usb.UsbSettingsManager.DeviceFilter
 * in UsbSettingsManager.java in Android SDK
 * 
 * Copyright (C) 2011 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

import java.io.IOException;
import java.util.ArrayList;
import java.util.Collections;
import java.util.List;

import org.xmlpull.v1.XmlPullParser;
import org.xmlpull.v1.XmlPullParserException;
import org.xmlpull.v1.XmlSerializer;

import android.content.Context;
import android.hardware.usb.UsbDevice;
import android.hardware.usb.UsbInterface;
import android.util.Log;

public class DeviceFilter {

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

	public DeviceFilter(int vid, int pid, int clasz, int subclass,
			int protocol, String manufacturer, String product, String serialnum) {
		mVendorId = vid;
		mProductId = pid;
		mClass = clasz;
		mSubclass = subclass;
		mProtocol = protocol;
		mManufacturerName = manufacturer;
		mProductName = product;
		mSerialNumber = serialnum;
	}

	public DeviceFilter(UsbDevice device) {
		mVendorId = device.getVendorId();
		mProductId = device.getProductId();
		mClass = device.getDeviceClass();
		mSubclass = device.getDeviceSubclass();
		mProtocol = device.getDeviceProtocol();
		mManufacturerName = null;	// device.getManufacturerName();
		mProductName = null;		// device.getProductName();
		mSerialNumber = null;		// device.getSerialNumber();
	}

	/**
	 * create DeviceFilter list from specified xml file.
	 * @param context
	 * @param deviceFilterXmlId
	 * @return
	 */
	public static List<DeviceFilter> getDeviceFilters(Context context, int deviceFilterXmlId) {
		final XmlPullParser parser = context.getResources().getXml(deviceFilterXmlId);
		final List<DeviceFilter> deviceFilters = new ArrayList<DeviceFilter>();
		try {
			int hasNext = XmlPullParser.START_DOCUMENT;
			while (hasNext != XmlPullParser.END_DOCUMENT) {
				hasNext = parser.next();
				if (hasNext == XmlPullParser.START_TAG) {
					final String tag = parser.getName();
					if ("usb-device".equals(tag)) {
						DeviceFilter deviceFilter = read(parser);
						if (deviceFilter != null) {
							deviceFilters.add(deviceFilter);
						}
					}
				}
			}
		} catch (XmlPullParserException e) {
			Log.d(TAG, "XmlPullParserException", e);
		} catch (IOException e) {
			Log.d(TAG, "IOException", e);
		}
		
		return Collections.unmodifiableList(deviceFilters);
	}
	
	public static DeviceFilter read(XmlPullParser parser)
			throws XmlPullParserException, IOException {
		int vendorId = -1;
		int productId = -1;
		int deviceClass = -1;
		int deviceSubclass = -1;
		int deviceProtocol = -1;
		String manufacturerName = null;
		String productName = null;
		String serialNumber = null;

		final int count = parser.getAttributeCount();
		for (int i = 0; i < count; i++) {
			String name = parser.getAttributeName(i);
			String value = parser.getAttributeValue(i);
			// Attribute values are ints or strings
			if ("manufacturer-name".equals(name)) {
				manufacturerName = value;
			} else if ("product-name".equals(name)) {
				productName = value;
			} else if ("serial-number".equals(name)) {
				serialNumber = value;
			} else {
				int intValue = -1;
				int radix = 10;
				if (value != null && value.length() > 2
						&& value.charAt(0) == '0'
						&& (value.charAt(1) == 'x' || value.charAt(1) == 'X')) {
					// allow hex values starting with 0x or 0X
					radix = 16;
					value = value.substring(2);
				}
				try {
					intValue = Integer.parseInt(value, radix);
				} catch (NumberFormatException e) {
					Log.e(TAG, "invalid number for field " + name, e);
					continue;
				}
				if ("vendor-id".equals(name)) {
					vendorId = intValue;
				} else if ("product-id".equals(name)) {
					productId = intValue;
				} else if ("class".equals(name)) {
					deviceClass = intValue;
				} else if ("subclass".equals(name)) {
					deviceSubclass = intValue;
				} else if ("protocol".equals(name)) {
					deviceProtocol = intValue;
				}
			}
		}
		return new DeviceFilter(vendorId, productId, deviceClass,
				deviceSubclass, deviceProtocol, manufacturerName, productName,
				serialNumber);
	}

	public void write(XmlSerializer serializer) throws IOException {
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
		serializer.endTag(null, "usb-device");
	}

	private boolean matches(int clasz, int subclass, int protocol) {
		return ((mClass == -1 || clasz == mClass)
				&& (mSubclass == -1 || subclass == mSubclass) && (mProtocol == -1 || protocol == mProtocol));
	}

	public boolean matches(UsbDevice device) {
		if (mVendorId != -1 && device.getVendorId() != mVendorId)
			return false;
		if (mProductId != -1 && device.getProductId() != mProductId)
			return false;
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
		if (matches(device.getDeviceClass(), device.getDeviceSubclass(),
				device.getDeviceProtocol()))
			return true;

		// if device doesn't match, check the interfaces
		int count = device.getInterfaceCount();
		for (int i = 0; i < count; i++) {
			UsbInterface intf = device.getInterface(i);
			if (matches(intf.getInterfaceClass(), intf.getInterfaceSubclass(),
					intf.getInterfaceProtocol()))
				return true;
		}

		return false;
	}

	public boolean matches(DeviceFilter f) {
		if (mVendorId != -1 && f.mVendorId != mVendorId)
			return false;
		if (mProductId != -1 && f.mProductId != mProductId)
			return false;
		if (f.mManufacturerName != null && mManufacturerName == null)
			return false;
		if (f.mProductName != null && mProductName == null)
			return false;
		if (f.mSerialNumber != null && mSerialNumber == null)
			return false;
		if (mManufacturerName != null && f.mManufacturerName != null
				&& !mManufacturerName.equals(f.mManufacturerName))
			return false;
		if (mProductName != null && f.mProductName != null
				&& !mProductName.equals(f.mProductName))
			return false;
		if (mSerialNumber != null && f.mSerialNumber != null
				&& !mSerialNumber.equals(f.mSerialNumber))
			return false;

		// check device class/subclass/protocol
		return matches(f.mClass, f.mSubclass, f.mProtocol);
	}

	@Override
	public boolean equals(Object obj) {
		// can't compare if we have wildcard strings
		if (mVendorId == -1 || mProductId == -1 || mClass == -1
				|| mSubclass == -1 || mProtocol == -1) {
			return false;
		}
		if (obj instanceof DeviceFilter) {
			DeviceFilter filter = (DeviceFilter) obj;

			if (filter.mVendorId != mVendorId
					|| filter.mProductId != mProductId
					|| filter.mClass != mClass || filter.mSubclass != mSubclass
					|| filter.mProtocol != mProtocol) {
				return (false);
			}
			if ((filter.mManufacturerName != null && mManufacturerName == null)
					|| (filter.mManufacturerName == null && mManufacturerName != null)
					|| (filter.mProductName != null && mProductName == null)
					|| (filter.mProductName == null && mProductName != null)
					|| (filter.mSerialNumber != null && mSerialNumber == null)
					|| (filter.mSerialNumber == null && mSerialNumber != null)) {
				return (false);
			}
			if ((filter.mManufacturerName != null && mManufacturerName != null && !mManufacturerName
					.equals(filter.mManufacturerName))
					|| (filter.mProductName != null && mProductName != null && !mProductName
							.equals(filter.mProductName))
					|| (filter.mSerialNumber != null && mSerialNumber != null && !mSerialNumber
							.equals(filter.mSerialNumber))) {
				return (false);
			}
			return (true);
		}
		if (obj instanceof UsbDevice) {
			UsbDevice device = (UsbDevice) obj;
			if (device.getVendorId() != mVendorId
					|| device.getProductId() != mProductId
					|| device.getDeviceClass() != mClass
					|| device.getDeviceSubclass() != mSubclass
					|| device.getDeviceProtocol() != mProtocol) {
				return (false);
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
			+ "]";
	}

}
