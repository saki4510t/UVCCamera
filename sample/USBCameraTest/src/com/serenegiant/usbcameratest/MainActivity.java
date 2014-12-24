package com.serenegiant.usbcameratest;
/*
 * UVCCamera
 * library and sample to access to UVC web camera on non-rooted Android device
 *
 * Copyright (c) 2014 saki t_saki@serenegiant.com
 *
 * File name: MainActivity.java
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

import java.util.concurrent.LinkedBlockingQueue;
import java.util.concurrent.ThreadPoolExecutor;
import java.util.concurrent.TimeUnit;

import android.app.Activity;
import android.graphics.SurfaceTexture;
import android.hardware.usb.UsbDevice;
import android.os.Bundle;
import android.view.Surface;
import android.view.View;
import android.view.View.OnClickListener;
import android.widget.ImageButton;
import android.widget.Toast;

import com.serenegiant.usb.USBMonitor;
import com.serenegiant.usb.USBMonitor.OnDeviceConnectListener;
import com.serenegiant.usb.USBMonitor.UsbControlBlock;
import com.serenegiant.usb.UVCCamera;
import com.serenegiant.widget.UVCCameraTextureView;

public class MainActivity extends Activity {

    // for thread pool
    private static final int CORE_POOL_SIZE = 1;		// initial/minimum threads
    private static final int MAX_POOL_SIZE = 4;			// maximum threads
    private static final int KEEP_ALIVE_TIME = 10;		// time periods while keep the idle thread
    protected static final ThreadPoolExecutor EXECUTER
		= new ThreadPoolExecutor(CORE_POOL_SIZE, MAX_POOL_SIZE, KEEP_ALIVE_TIME,
			TimeUnit.SECONDS, new LinkedBlockingQueue<Runnable>());

    // for accessing USB and USB camera
    private USBMonitor mUSBMonitor;
	private UVCCamera mUVCCamera;
	private UVCCameraTextureView mUVCCameraView;
	// for open&start / stop&close camera preview
	private ImageButton mCameraButton;
	private Surface mPreviewSurface;

	@Override
	protected void onCreate(final Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		setContentView(R.layout.activity_main);
		mCameraButton = (ImageButton)findViewById(R.id.camera_button);
		mCameraButton.setOnClickListener(mOnClickListener);

		mUVCCameraView = (UVCCameraTextureView)findViewById(R.id.UVCCameraTextureView1);
		mUVCCameraView.setAspectRatio(640 / 480.f);

		mUSBMonitor = new USBMonitor(this, mOnDeviceConnectListener);
	}

	@Override
	public void onResume() {
		super.onResume();
		mUSBMonitor.register();
		if (mUVCCamera != null)
			mUVCCamera.startPreview();
	}

	@Override
	public void onPause() {
		if (mUVCCamera != null)
			mUVCCamera.stopPreview();
		mUSBMonitor.unregister();
		super.onPause();
	}

	@Override
	public void onDestroy() {
		if (mUVCCamera != null) {
			mUVCCamera.destroy();
			mUVCCamera = null;
		}
		if (mUSBMonitor != null) {
			mUSBMonitor.destroy();
			mUSBMonitor = null;
		}
		mUVCCameraView = null;
		mCameraButton = null;
		super.onDestroy();
	}

	private final OnClickListener mOnClickListener = new OnClickListener() {
		@Override
		public void onClick(final View view) {
			if (mUVCCamera == null)
				CameraDialog.showDialog(MainActivity.this);
			else {
				mUVCCamera.destroy();
				mUVCCamera = null;
			}
		}
	};

	private final OnDeviceConnectListener mOnDeviceConnectListener = new OnDeviceConnectListener() {
		@Override
		public void onAttach(final UsbDevice device) {
			Toast.makeText(MainActivity.this, "USB_DEVICE_ATTACHED", Toast.LENGTH_SHORT).show();
		}

		@Override
		public void onConnect(final UsbDevice device, final UsbControlBlock ctrlBlock, final boolean createNew) {
			if (mUVCCamera != null)
				mUVCCamera.destroy();
			mUVCCamera = new UVCCamera();
			EXECUTER.execute(new Runnable() {
				@Override
				public void run() {
					mUVCCamera.open(ctrlBlock);
//					mUVCCamera.setPreviewTexture(mUVCCameraView.getSurfaceTexture());
					if (mPreviewSurface != null) {
						mPreviewSurface.release();
						mPreviewSurface = null;
					}
					final SurfaceTexture st = mUVCCameraView.getSurfaceTexture();
					if (st != null)
						mPreviewSurface = new Surface(st);
					mUVCCamera.setPreviewDisplay(mPreviewSurface);
					mUVCCamera.startPreview();
				}
			});
		}

		@Override
		public void onDisconnect(final UsbDevice device, final UsbControlBlock ctrlBlock) {
			// XXX you should check whether the comming device equal to camera device that currently using
			if (mUVCCamera != null) {
				mUVCCamera.close();
				if (mPreviewSurface != null) {
					mPreviewSurface.release();
					mPreviewSurface = null;
				}
			}
		}

		@Override
		public void onDettach(final UsbDevice device) {
			Toast.makeText(MainActivity.this, "USB_DEVICE_DETACHED", Toast.LENGTH_SHORT).show();
		}

		@Override
		public void onCancel() {
		}
	};

	/**
	 * to access from CameraDialog
	 * @return
	 */
	public USBMonitor getUSBMonitor() {
		return mUSBMonitor;
	}

}
