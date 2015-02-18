package com.serenegiant.usb;
/*
 * UVCCamera
 * library and sample to access to UVC web camera on non-rooted Android device
 *
 * Copyright (c) 2015 saki t_saki@serenegiant.com
 *
 * File name: IFrameCallback.java
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
 * Files in the jni/libjpeg, jni/libusb and jin/libuvc folder may have a different license, see the respective files.
*/

import java.nio.ByteBuffer;
/**
 * Callback interface for UVCCamera class
 * If you need frame data as ByteBuffer, you can use this callback interface with UVCCamera#setFrameCallback
 */
public interface IFrameCallback {
	/**
	 * This method is called from native library via JNI on the same thread as UVCCamera#startCapture.
	 * You can use both UVCCamera#startCapture and #setFrameCallback
	 * but it is better to use either for better performance.
	 * You can also pass pixel format type to UVCCamera#setFrameCallback for this method.
	 * Some frames may drops if this method takes a time.
	 * @param frame
	 */
	public void onFrame(ByteBuffer frame);
}
