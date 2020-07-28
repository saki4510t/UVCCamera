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
	 * When you use some color format like NV21, this library never execute color space conversion,
	 * just execute pixel format conversion. If you want to get same result as on screen, please try to
	 * consider to get images via texture(SurfaceTexture) and read pixel buffer from it using OpenGL|ES2/3
	 * instead of using IFrameCallback(this way is much efficient in most case than using IFrameCallback).
	 * @param frame this is direct ByteBuffer from JNI layer and you should handle it's byte order and limitation.
	 *              如果需要加工该帧数据，建议复制一份出来加工处理，推荐使用线程异步处理，否则会阻塞uvc
	 */
	public void onFrame(ByteBuffer frame);
}
