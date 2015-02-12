package com.serenegiant.usb;
/*
 * UVCCamera
 * library and sample to access to UVC web camera on non-rooted Android device
 *
 * Copyright (c) 2014-2015 saki t_saki@serenegiant.com
 *
 * File name: UVCCamera.java
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

import android.graphics.SurfaceTexture;
import android.text.TextUtils;
import android.util.Log;
import android.view.Surface;
import android.view.SurfaceHolder;

import com.serenegiant.usb.USBMonitor.UsbControlBlock;

public class UVCCamera {

	private static final String TAG = UVCCamera.class.getSimpleName();
	private static final String DEFAULT_USBFS = "/dev/bus/usb";

	public static final int DEFAULT_PREVIEW_WIDTH = 640;
	public static final int DEFAULT_PREVIEW_HEIGHT = 480;
	public static final int DEFAULT_PREVIEW_MODE = 0;

	public static final int FRAME_FORMAT_YUYV = 0;
	public static final int FRAME_FORMAT_MJPEG = 1;

	public static final int PIXEL_FORMAT_RAW = 0;
	public static final int PIXEL_FORMAT_YUV = 1;
	public static final int PIXEL_FORMAT_RGB565 = 2;
	public static final int PIXEL_FORMAT_RGBX = 3;
	public static final int PIXEL_FORMAT_NV21 = 4;		// = YUV420SemiPlanar

	private static boolean isLoaded;
	static {
		if (!isLoaded) {
			System.loadLibrary("usb100");
			System.loadLibrary("uvc");
			System.loadLibrary("UVCCamera");
			isLoaded = true;
		}
	}

	private UsbControlBlock mCtrlBlock;
    protected long mNativePtr;	// this field is accessed from native code and do not change name and remove

    /**
     * the sonctructor of this class should be call within the thread that has a looper
     * (UI thread or a thread that called Looper.prepare)
     */
    public UVCCamera() {
    	mNativePtr = nativeCreate();
	}

    /**
     * connect to a UVC camera
     * USB permission is necessary before this method is called
     * @param ctrlBlock
     */
    public void open(final UsbControlBlock ctrlBlock) {
		mCtrlBlock = ctrlBlock;
		nativeConnect(mNativePtr,
			mCtrlBlock.getVenderId(), mCtrlBlock.getProductId(),
			mCtrlBlock.getFileDescriptor(),
			getUSBFSName(mCtrlBlock));
		nativeSetPreviewSize(mNativePtr, DEFAULT_PREVIEW_WIDTH, DEFAULT_PREVIEW_HEIGHT, DEFAULT_PREVIEW_MODE);
    }

    /**
     * close and release UVC camera
     */
    public void close() {
    	stopPreview();
    	if (mNativePtr != 0) {
    		nativeRelease(mNativePtr);
    	}
   		mCtrlBlock = null;
    }

	/**
	 * Set preview size and preview mode
	 * @param width
	   @param height
	   @param mode 0:yuyv, other:MJPEG
	 */
	public void setPreviewSize(final int width, final int height, final int mode) {
		if ((width == 0) || (height == 0))
			throw new IllegalArgumentException("invalid preview size");
		if (mNativePtr != 0) {
			final int result = nativeSetPreviewSize(mNativePtr, width, height, mode);
			if (result != 0)
				throw new IllegalArgumentException("Failed to set preview size");
		}
	}

    /**
     * set preview surface with SurfaceHolder</br>
     * you can use SurfaceHolder came from SurfaceView/GLSurfaceView
     * @param holder
     */
    public void setPreviewDisplay(final SurfaceHolder holder) {
   		nativeSetPreviewDisplay(mNativePtr, holder.getSurface());
    }

    /**
     * set preview surface with SurfaceTexture.
     * this method require API >= 14
     * @param texture
     */
    public void setPreviewTexture(final SurfaceTexture texture) {	// API >= 11
    	final Surface surface = new Surface(texture);	// XXX API >= 14
    	nativeSetPreviewDisplay(mNativePtr, surface);
    }

    /**
     * set preview surface with Surface
     * @param Surface
     */
    public void setPreviewDisplay(final Surface surface) {
    	nativeSetPreviewDisplay(mNativePtr, surface);
    }

    /**
     * set frame callback
     * @param callback
     * @param pixelFormat
     */
    public void setFrameCallback(final IFrameCallback callback, final int pixelFormat) {
    	if (mNativePtr != 0) {
        	nativeSetFrameCallback(mNativePtr, callback, pixelFormat);
    	}
    }

    /**
     * start preview
     */
    public void startPreview() {
    	if (mCtrlBlock != null) {
    		nativeStartPreview(mNativePtr);
    	}
    }

    /**
     * stop preview
     */
    public void stopPreview() {
    	setFrameCallback(null, 0);
    	if (mCtrlBlock != null) {
    		nativeStopPreview(mNativePtr);
    	}
    }

    /**
     * destroy UVCCamera object
     */
    public void destroy() {
    	close();
    	if (mNativePtr != 0) {
    		nativeDestroy(mNativePtr);
    		mNativePtr = 0;
    	}
    }

	private final String getUSBFSName(final UsbControlBlock ctrlBlock) {
		String result = null;
		final String name = ctrlBlock.getDeviceName();
		final String[] v = !TextUtils.isEmpty(name) ? name.split("/") : null;
		if ((v != null) && (v.length > 2)) {
			final StringBuilder sb = new StringBuilder(v[0]);
			for (int i = 1; i < v.length - 2; i++)
				sb.append("/").append(v[i]);
			result = sb.toString();
		}
		if (TextUtils.isEmpty(result)) {
			Log.w(TAG, "failed to get USBFS path, try to use default path:" + name);
			result = DEFAULT_USBFS;
		}
		return result;
	}

    // #nativeCreate and #nativeDestroy are not static methods.
    private final native long nativeCreate();
    private final native void nativeDestroy(long id_camera);

    private static final native int nativeConnect(long id_camera, int venderId, int productId, int fileDescriptor, String usbfs);
    private static final native int nativeRelease(long id_camera);

    private static final native int nativeSetPreviewSize(long id_camera, int width, int height, int mode);
    private static final native int nativeStartPreview(long id_camera);
    private static final native int nativeStopPreview(long id_camera);
    private static final native int nativeSetPreviewDisplay(long id_camera, Surface surface);
    private static final native int nativeSetFrameCallback(long mNativePtr, IFrameCallback callback, int pixelFormat);

//**********************************************************************
    /**
     * start movie capturing(this should call while previewing)
     * @param surface
     */
    public void startCapture(final Surface surface) {
    	if (mCtrlBlock != null && surface != null) {
    		nativeSetCaptureDisplay(mNativePtr, surface);
    	} else
    		throw new NullPointerException("startCapture");
    }

    /**
     * stop movie capturing
     */
    public void stopCapture() {
    	if (mCtrlBlock != null) {
    		nativeSetCaptureDisplay(mNativePtr, null);
    	}
    }
    private static final native int nativeSetCaptureDisplay(long id_camera, Surface surface);

}
