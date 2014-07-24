package com.serenegiant.usb;
/*
 * UVCCamera
 * library and sample to access to UVC web camera on non-rooted Android device
 * 
 * Copyright (c) 2014 saki t_saki@serenegiant.com
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
import android.view.Surface;
import android.view.SurfaceHolder;

import com.serenegiant.usb.USBMonitor.UsbControlBlock;

public class UVCCamera {

	private static final String TAG = UVCCamera.class.getSimpleName();

	private static boolean isLoaded;
	static {
		if (!isLoaded) {
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
    public void open(UsbControlBlock ctrlBlock) {
    	mCtrlBlock = ctrlBlock;
    	nativeConnect(mNativePtr,
       		mCtrlBlock.getVenderId(), mCtrlBlock.getProductId(),
       		mCtrlBlock.getFileDescriptor());
    }

    /**
     * close and release UVC camera
     */
    public void close() {
    	if (mNativePtr != 0) {
    		nativeRelease(mNativePtr);
    	}
   		mCtrlBlock = null;
    }

    /**
     * set preview surface with SurfaceHolder</br>
     * you can use SurfaceHolder came from SurfaceView/GLSurfaceView
     * @param holder
     */
    public void setPreviewDisplay(SurfaceHolder holder) {
   		nativeSetPreviewDisplay(mNativePtr, holder.getSurface());
    }
    
    /**
     * set preview surface with SurfaceTexture.
     * this method require API >= 14
     * @param texture
     */
    public void setPreviewTexture(SurfaceTexture texture) {	// API >= 11
    	final Surface surface = new Surface(texture);	// XXX API >= 14
    	nativeSetPreviewDisplay(mNativePtr, surface);
    }

    /**
     * set preview surface with Surface
     * @param Surface
     */
    public void setPreviewDisplay(Surface surface) {
    	nativeSetPreviewDisplay(mNativePtr, surface);
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

    // #nativeCreate and #nativeDestroy are not static methods.
    private final native long nativeCreate();
    private final native void nativeDestroy(long id_camera);

    private static final native int nativeConnect(long id_camera, int venderId, int productId, int fileDescriptor);
    private static final native int nativeRelease(long id_camera);

    private static final native int nativeStartPreview(long id_camera);
    private static final native int nativeStopPreview(long id_camera);
    private static final native int nativeSetPreviewDisplay(long id_camera, Surface surface);

//**********************************************************************
    /**
     * start movie capturing(this should call while previewing)
     * @param surface
     */
    public void startCapture(Surface surface) {
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
