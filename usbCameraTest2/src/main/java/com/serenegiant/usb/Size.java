package com.serenegiant.usb;
/*
 * UVCCamera
 * library and sample to access to UVC web camera on non-rooted Android device
 *
 * Copyright (c) 2015 saki t_saki@serenegiant.com
 *
 * File name: Size.java
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 * All files in the folder are under this Apache License, Version 2.0.
 * Files in the jni/libjpeg, jni/libusb, jin/libuvc, jni/rapidjson folder may have a different license, see the respective files.
*/

import java.util.Locale;

import android.os.Parcel;
import android.os.Parcelable;

public class Size implements Parcelable {
	// 9999 is still image
	public int type;
	public int index;
	public int width;
	public int height;

	public Size(final int _type, final int _index, final int _width, final int _height) {
		type = _type;
		index = _index;
		width = _width;
		height = _height;
	}

	private Size(final Parcel source) {
		type = source.readInt();
		index = source.readInt();;
		width = source.readInt();;
		height = source.readInt();;
	}

	public Size set(final Size other) {
		if (other != null) {
			type = other.type;
			index = other.index;
			width = other.width;
			height = other.height;
		}
		return this;
	}

	@Override
	public int describeContents() {
		return 0;
	}

	@Override
	public void writeToParcel(final Parcel dest, final int flags) {
		dest.writeInt(type);
		dest.writeInt(index);
		dest.writeInt(width);
		dest.writeInt(height);
	}


	@Override
	public String toString() {
		return String.format(Locale.US, "Size(%dx%d,type:%d,index:%d)", width, height, type, index);
	}


	public static final Creator<Size> CREATOR = new Parcelable.Creator<Size>() {
		@Override
		public Size createFromParcel(final Parcel source) {
			return new Size(source);
		}
		@Override
		public Size[] newArray(final int size) {
			return new Size[size];
		}
	};
}
