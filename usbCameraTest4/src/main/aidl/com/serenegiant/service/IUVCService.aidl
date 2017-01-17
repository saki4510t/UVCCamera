package com.serenegiant.service;
/*
 * UVCCamera
 * library and sample to access to UVC web camera on non-rooted Android device
 * 
 * Copyright (c) 2014-2017 saki t_saki@serenegiant.com
 * 
 * File name: IUVCService.aidl
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

import com.serenegiant.service.IUVCServiceCallback;
import android.hardware.usb.UsbDevice;
import android.view.Surface;

/**
	<select						select UVC camera
		<connect				open device and start streaming
		disconnect>				stop streaming and close device
	release>					release camera
*/
interface IUVCService {
	int select(in UsbDevice device, IUVCServiceCallback callback);
	void release(int serviceId);
	boolean isSelected(int serviceId);
	void releaseAll();
	void resize(int serviceId, int width, int height);
	void connect(int serviceId);
	void disconnect(int serviceId);
	boolean isConnected(int serviceId);
	void addSurface(int serviceId, int id_surface, in Surface surface, boolean isRecordable);
	void removeSurface(int serviceId, int id_surface);
	boolean isRecording(int serviceId);
	void startRecording(int serviceId);
	void stopRecording(int serviceId);
	void captureStillImage(int serviceId, String path);
}