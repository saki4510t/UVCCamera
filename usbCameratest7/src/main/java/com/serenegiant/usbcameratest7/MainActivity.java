package com.serenegiant.usbcameratest7;
/*
 * UVCCamera
 * library and sample to access to UVC web camera on non-rooted Android device
 *
 * Copyright (c) 2014-2015 saki t_saki@serenegiant.com
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
 * Files in the jni/libjpeg, jni/libusb, jin/libuvc, jni/rapidjson folder may have a different license, see the respective files.
*/

import java.util.concurrent.LinkedBlockingQueue;
import java.util.concurrent.ThreadPoolExecutor;
import java.util.concurrent.TimeUnit;

import android.app.Activity;
import android.graphics.SurfaceTexture;
import android.hardware.usb.UsbDevice;
import android.os.Bundle;
import android.util.Log;
import android.view.Surface;
import android.view.View;
import android.view.View.OnClickListener;
import android.widget.Toast;

import com.serenegiant.usb.CameraDialog;
import com.serenegiant.usb.USBMonitor;
import com.serenegiant.usb.USBMonitor.OnDeviceConnectListener;
import com.serenegiant.usb.USBMonitor.UsbControlBlock;
import com.serenegiant.usb.UVCCamera;
import com.serenegiant.widget.UVCCameraTextureView;

public final class MainActivity extends Activity implements CameraDialog.CameraDialogParent {
	private static final boolean DEBUG = false;	// FIXME set false when production
	private static final String TAG = "MainActivity";

    // for thread pool
    private static final int CORE_POOL_SIZE = 1;		// initial/minimum threads
    private static final int MAX_POOL_SIZE = 4;			// maximum threads
    private static final int KEEP_ALIVE_TIME = 10;		// time periods while keep the idle thread
    protected static final ThreadPoolExecutor EXECUTER
		= new ThreadPoolExecutor(CORE_POOL_SIZE, MAX_POOL_SIZE, KEEP_ALIVE_TIME,
			TimeUnit.SECONDS, new LinkedBlockingQueue<Runnable>());

    // for accessing USB and USB camera
    private USBMonitor mUSBMonitor;
	private UVCCamera mLeftCamera;
	private UVCCamera mRightCamera;
	private UVCCameraTextureView mUVCCameraViewR;
	private UVCCameraTextureView mUVCCameraViewL;
	private Surface mLeftPreviewSurface;
	private Surface mRightPreviewSurface;

	@Override
	protected void onCreate(final Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		setContentView(R.layout.activity_main);

		findViewById(R.id.RelativeLayout1).setOnClickListener(mOnClickListener);
		mUVCCameraViewL = (UVCCameraTextureView)findViewById(R.id.camera_view_L);
		mUVCCameraViewL.setAspectRatio(UVCCamera.DEFAULT_PREVIEW_WIDTH / (float)UVCCamera.DEFAULT_PREVIEW_HEIGHT);
		mUVCCameraViewL.setOnClickListener(mOnClickListener);

		mUVCCameraViewR = (UVCCameraTextureView)findViewById(R.id.camera_view_R);
		mUVCCameraViewR.setAspectRatio(UVCCamera.DEFAULT_PREVIEW_WIDTH / (float)UVCCamera.DEFAULT_PREVIEW_HEIGHT);
		mUVCCameraViewR.setOnClickListener(mOnClickListener);

		mUSBMonitor = new USBMonitor(this, mOnDeviceConnectListener);
	}

	@Override
	public void onResume() {
		super.onResume();
		mUSBMonitor.register();
		if (mLeftCamera != null)
			mLeftCamera.startPreview();
		if (mRightCamera != null)
			mRightCamera.startPreview();
	}

	@Override
	public void onPause() {
		if (mLeftCamera != null)
			mLeftCamera.stopPreview();
		if (mRightCamera != null)
			mRightCamera.stopPreview();
		mUSBMonitor.unregister();
		super.onPause();
	}

	@Override
	public void onDestroy() {
		if (mLeftCamera != null) {
			mLeftCamera.destroy();
			mLeftCamera = null;
		}
		if (mRightCamera != null) {
			mRightCamera.destroy();
			mRightCamera = null;
		}
		if (mUSBMonitor != null) {
			mUSBMonitor.destroy();
			mUSBMonitor = null;
		}
		mUVCCameraViewR = null;
		mUVCCameraViewL = null;
		super.onDestroy();
	}

	private final OnClickListener mOnClickListener = new OnClickListener() {
		@Override
		public void onClick(final View view) {
			switch (view.getId()) {
			case R.id.camera_view_L:
				if (mLeftCamera == null)
					CameraDialog.showDialog(MainActivity.this);
				else {
					mLeftCamera.destroy();
					mLeftCamera = null;
				}
				break;
			case R.id.camera_view_R:
				if (mRightCamera == null)
					CameraDialog.showDialog(MainActivity.this);
				else {
					mRightCamera.destroy();
					mRightCamera = null;
				}
				break;
			}
		}
	};

	private final OnDeviceConnectListener mOnDeviceConnectListener = new OnDeviceConnectListener() {
		@Override
		public void onAttach(final UsbDevice device) {
			if (DEBUG) Log.v(TAG, "onAttach:" + device);
			Toast.makeText(MainActivity.this, "USB_DEVICE_ATTACHED", Toast.LENGTH_SHORT).show();
		}

		@Override
		public void onConnect(final UsbDevice device, final UsbControlBlock ctrlBlock, final boolean createNew) {
			if (mLeftCamera != null && mRightCamera != null) return;
			if (DEBUG) Log.v(TAG, "onConnect:" + device);
			final UVCCamera camera = new  UVCCamera();
			EXECUTER.execute(new Runnable() {
				@Override
				public void run() {
					camera.open(ctrlBlock);
//					camera.setPreviewTexture(mRightCameraView.getSurfaceTexture());
					try {
						camera.setPreviewSize(UVCCamera.DEFAULT_PREVIEW_WIDTH, UVCCamera.DEFAULT_PREVIEW_HEIGHT, UVCCamera.FRAME_FORMAT_MJPEG, 0.5f);
					} catch (final IllegalArgumentException e) {
						// fallback to YUV mode
						try {
							camera.setPreviewSize(UVCCamera.DEFAULT_PREVIEW_WIDTH, UVCCamera.DEFAULT_PREVIEW_HEIGHT, UVCCamera.DEFAULT_PREVIEW_MODE, 0.5f);
						} catch (final IllegalArgumentException e1) {
							camera.destroy();
							return;
						}
					}
					if (mLeftCamera == null) {
						mLeftCamera = camera;
						if (mLeftPreviewSurface != null) {
							mLeftPreviewSurface.release();
							mLeftPreviewSurface = null;
						}
						final SurfaceTexture st = mUVCCameraViewL.getSurfaceTexture();
						if (st != null)
							mLeftPreviewSurface = new Surface(st);
						camera.setPreviewDisplay(mLeftPreviewSurface);
//						camera.setFrameCallback(mIFrameCallback, UVCCamera.PIXEL_FORMAT_NV21);
						camera.startPreview();
					} else if (mRightCamera == null) {
						mRightCamera = camera;
						if (mRightPreviewSurface != null) {
							mRightPreviewSurface.release();
							mRightPreviewSurface = null;
						}
						final SurfaceTexture st = mUVCCameraViewR.getSurfaceTexture();
						if (st != null)
							mRightPreviewSurface = new Surface(st);
						camera.setPreviewDisplay(mRightPreviewSurface);
//						camera.setFrameCallback(mIFrameCallback, UVCCamera.PIXEL_FORMAT_NV21);
						camera.startPreview();
					}
				}
			});
		}

		@Override
		public void onDisconnect(final UsbDevice device, final UsbControlBlock ctrlBlock) {
			if (DEBUG) Log.v(TAG, "onDisconnect:" + device);
			if (mLeftCamera != null && device.equals(mLeftCamera.getDevice())) {
				mLeftCamera.close();
				if (mLeftPreviewSurface != null) {
					mLeftPreviewSurface.release();
					mLeftPreviewSurface = null;
				}
				mLeftCamera.destroy();
				mLeftCamera = null;
			} else if (mRightCamera != null && device.equals(mRightCamera.getDevice())) {
				mRightCamera.close();
				if (mLeftPreviewSurface != null) {
					mLeftPreviewSurface.release();
					mLeftPreviewSurface = null;
				}
				mRightCamera.destroy();
				mRightCamera = null;
			}
		}

		@Override
		public void onDettach(final UsbDevice device) {
			if (DEBUG) Log.v(TAG, "onDettach:" + device);
			Toast.makeText(MainActivity.this, "USB_DEVICE_DETACHED", Toast.LENGTH_SHORT).show();
		}

		@Override
		public void onCancel() {
			if (DEBUG) Log.v(TAG, "onCancel:");
		}
	};

	/**
	 * to access from CameraDialog
	 * @return
	 */
	@Override
	public USBMonitor getUSBMonitor() {
		return mUSBMonitor;
	}

/*
	// if you need frame data as byte array on Java side, you can use this callback method with UVCCamera#setFrameCallback
	private final IFrameCallback mIFrameCallback = new IFrameCallback() {
		@Override
		public void onFrame(final ByteBuffer frame) {
		}
	}; */
}
