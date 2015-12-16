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
import android.widget.ImageButton;
import android.widget.Toast;

import com.serenegiant.usb.CameraDialog;
import com.serenegiant.usb.USBMonitor;
import com.serenegiant.usb.USBMonitor.OnDeviceConnectListener;
import com.serenegiant.usb.USBMonitor.UsbControlBlock;
import com.serenegiant.usb.UVCCamera;
import com.serenegiant.widget.CameraViewInterface;
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

	private CameraHandler mHandlerR;
	private CameraViewInterface mUVCCameraViewR;
	private ImageButton mCaptureButtonR;
	private Surface mRightPreviewSurface;

	private CameraHandler mHandlerL;
	private CameraViewInterface mUVCCameraViewL;
	private ImageButton mCaptureButtonL;
	private Surface mLeftPreviewSurface;


	@Override
	protected void onCreate(final Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		setContentView(R.layout.activity_main);

		findViewById(R.id.RelativeLayout1).setOnClickListener(mOnClickListener);
		mUVCCameraViewL = (CameraViewInterface)findViewById(R.id.camera_view_L);
		mUVCCameraViewL.setAspectRatio(UVCCamera.DEFAULT_PREVIEW_WIDTH / (float)UVCCamera.DEFAULT_PREVIEW_HEIGHT);
		((UVCCameraTextureView)mUVCCameraViewL).setOnClickListener(mOnClickListener);
		mCaptureButtonL = (ImageButton)findViewById(R.id.capture_button_L);
		mCaptureButtonL.setOnClickListener(mOnClickListener);
		mCaptureButtonL.setVisibility(View.INVISIBLE);
		mHandlerL = CameraHandler.createHandler(this, mUVCCameraViewL);

		mUVCCameraViewR = (CameraViewInterface)findViewById(R.id.camera_view_R);
		mUVCCameraViewR.setAspectRatio(UVCCamera.DEFAULT_PREVIEW_WIDTH / (float)UVCCamera.DEFAULT_PREVIEW_HEIGHT);
		((UVCCameraTextureView)mUVCCameraViewR).setOnClickListener(mOnClickListener);
		mCaptureButtonR = (ImageButton)findViewById(R.id.capture_button_R);
		mCaptureButtonR.setOnClickListener(mOnClickListener);
		mCaptureButtonR.setVisibility(View.INVISIBLE);
		mHandlerR = CameraHandler.createHandler(this, mUVCCameraViewR);

		mUSBMonitor = new USBMonitor(this, mOnDeviceConnectListener);
	}

	@Override
	public void onResume() {
		super.onResume();
		mUSBMonitor.register();
		if (mUVCCameraViewR != null)
			mUVCCameraViewR.onResume();
		if (mUVCCameraViewL != null)
			mUVCCameraViewL.onResume();
	}

	@Override
	public void onPause() {
		mHandlerR.closeCamera();
		if (mUVCCameraViewR != null)
			mUVCCameraViewR.onPause();
		mHandlerL.closeCamera();
		mCaptureButtonR.setVisibility(View.INVISIBLE);
		if (mUVCCameraViewL != null)
			mUVCCameraViewL.onPause();
		mCaptureButtonL.setVisibility(View.INVISIBLE);
		mUSBMonitor.unregister();
		super.onPause();
	}

	@Override
	public void onDestroy() {
		if (mHandlerR != null) {
			mHandlerR = null;
  		}
		if (mHandlerL != null) {
			mHandlerL = null;
  		}
		if (mUSBMonitor != null) {
			mUSBMonitor.destroy();
			mUSBMonitor = null;
		}
		mUVCCameraViewR = null;
		mCaptureButtonR = null;
		mUVCCameraViewL = null;
		mCaptureButtonL = null;
		super.onDestroy();
	}

	private final OnClickListener mOnClickListener = new OnClickListener() {
		@Override
		public void onClick(final View view) {
			switch (view.getId()) {
			case R.id.camera_view_L:
				if (!mHandlerL.isCameraOpened()) {
					CameraDialog.showDialog(MainActivity.this);
				} else {
					mHandlerL.closeCamera();
					mCaptureButtonL.setVisibility(View.INVISIBLE);
				}
				break;
			case R.id.capture_button_L:
				if (mHandlerL.isCameraOpened()) {
					if (!mHandlerL.isRecording()) {
						mCaptureButtonL.setColorFilter(0xffff0000);	// turn red
						mHandlerL.startRecording();
					} else {
						mCaptureButtonL.setColorFilter(0);	// return to default color
						mHandlerL.stopRecording();
					}
				}
				break;
			case R.id.camera_view_R:
				if (!mHandlerR.isCameraOpened()) {
					CameraDialog.showDialog(MainActivity.this);
				} else {
					mHandlerR.closeCamera();
					mCaptureButtonR.setVisibility(View.INVISIBLE);
				}
				break;
			case R.id.capture_button_R:
				if (mHandlerR.isCameraOpened()) {
					if (!mHandlerR.isRecording()) {
						mCaptureButtonR.setColorFilter(0xffff0000);	// turn red
						mHandlerR.startRecording();
					} else {
						mCaptureButtonR.setColorFilter(0);	// return to default color
						mHandlerR.stopRecording();
					}
				}
				break;
			}
		}
	};

	private static final float[] BANDWIDTH_FACTORS = { 0.67f, 0.67f };
	private final OnDeviceConnectListener mOnDeviceConnectListener = new OnDeviceConnectListener() {
		@Override
		public void onAttach(final UsbDevice device) {
			if (DEBUG) Log.v(TAG, "onAttach:" + device);
			Toast.makeText(MainActivity.this, "USB_DEVICE_ATTACHED", Toast.LENGTH_SHORT).show();
		}

		@Override
		public void onConnect(final UsbDevice device, final UsbControlBlock ctrlBlock, final boolean createNew) {
			if (DEBUG) Log.v(TAG, "onConnect:" + device);
			if (!mHandlerL.isCameraOpened()) {
				mHandlerL.openCamera(ctrlBlock);
				final SurfaceTexture st = mUVCCameraViewL.getSurfaceTexture();
				mHandlerL.startPreview(new Surface(st));
				runOnUiThread(new Runnable() {
					@Override
					public void run() {
						mCaptureButtonL.setVisibility(View.VISIBLE);
					}
				});
			} else if (!mHandlerR.isCameraOpened()) {
				mHandlerR.openCamera(ctrlBlock);
				final SurfaceTexture st = mUVCCameraViewR.getSurfaceTexture();
				mHandlerR.startPreview(new Surface(st));
				runOnUiThread(new Runnable() {
					@Override
					public void run() {
						mCaptureButtonR.setVisibility(View.VISIBLE);
					}
				});
			}
		}

		@Override
		public void onDisconnect(final UsbDevice device, final UsbControlBlock ctrlBlock) {
			if (DEBUG) Log.v(TAG, "onDisconnect:" + device);
			if ((mHandlerL != null) && !mHandlerL.isEqual(device)) {
				mHandlerL.closeCamera();
				if (mLeftPreviewSurface != null) {
					mLeftPreviewSurface.release();
					mLeftPreviewSurface = null;
				}
				runOnUiThread(new Runnable() {
					@Override
					public void run() {
						mCaptureButtonL.setVisibility(View.INVISIBLE);
					}
				});
			} else if ((mHandlerR != null) && !mHandlerR.isEqual(device)) {
				mHandlerR.closeCamera();
				if (mRightPreviewSurface != null) {
					mRightPreviewSurface.release();
					mRightPreviewSurface = null;
				}
				runOnUiThread(new Runnable() {
					@Override
					public void run() {
						mCaptureButtonR.setVisibility(View.INVISIBLE);
					}
				});
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
