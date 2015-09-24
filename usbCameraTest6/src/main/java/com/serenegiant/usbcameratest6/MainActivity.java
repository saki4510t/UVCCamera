package com.serenegiant.usbcameratest6;
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

import java.io.File;

import android.app.Activity;
import android.graphics.SurfaceTexture;
import android.hardware.usb.UsbDevice;
import android.media.MediaScannerConnection;
import android.os.Bundle;
import android.os.Environment;
import android.util.Log;
import android.view.Surface;
import android.view.TextureView;
import android.view.View;
import android.view.View.OnClickListener;
import android.view.View.OnLongClickListener;
import android.widget.ImageButton;
import android.widget.Toast;
import android.widget.ToggleButton;

import com.serenegiant.encoder.MediaMuxerWrapper;

import com.serenegiant.usb.CameraDialog;
import com.serenegiant.usb.USBMonitor;
import com.serenegiant.usb.USBMonitor.OnDeviceConnectListener;
import com.serenegiant.usb.USBMonitor.UsbControlBlock;
import com.serenegiant.usb.UVCCamera;
import com.serenegiant.usb.UVCCameraHandler;
import com.serenegiant.widget.UVCCameraTextureView;

public final class MainActivity extends Activity implements CameraDialog.CameraDialogParent {
	private static final boolean DEBUG = true;	// TODO set false on release
	private static final String TAG = "MainActivity";

	/**
	 * for accessing USB
	 */
	private USBMonitor mUSBMonitor;
	/**
	 * Handler to execute camera releated methods sequentially on private thread
	 */
	private UVCCameraHandler mHandler;
	/**
	 * for camera preview display
	 */
	private UVCCameraTextureView mUVCCameraViewL;
	private UVCCameraTextureView mUVCCameraViewR;
	/**
	 * for open&start / stop&close camera preview
	 */
	private ToggleButton mCameraButton;
	/**
	 * button for start/stop recording
	 */
	private ImageButton mCaptureButton;

	@Override
	protected void onCreate(final Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		if (DEBUG) Log.v(TAG, "onCreate:");
		setContentView(R.layout.activity_main);
		mCameraButton = (ToggleButton)findViewById(R.id.camera_button);
		mCameraButton.setOnClickListener(mOnClickListener);
		mCaptureButton = (ImageButton)findViewById(R.id.capture_button);
		mCaptureButton.setOnClickListener(mOnClickListener);
		mCaptureButton.setVisibility(View.INVISIBLE);

		mUVCCameraViewL = (UVCCameraTextureView)findViewById(R.id.camera_view_L);
		mUVCCameraViewL.setAspectRatio(UVCCamera.DEFAULT_PREVIEW_WIDTH / (float)UVCCamera.DEFAULT_PREVIEW_HEIGHT);
		mUVCCameraViewL.setSurfaceTextureListener(mSurfaceTextureListener);
		mUVCCameraViewL.setOnLongClickListener(mOnLongClickListener);

		mUVCCameraViewR = (UVCCameraTextureView)findViewById(R.id.camera_view_R);
		mUVCCameraViewR.setAspectRatio(UVCCamera.DEFAULT_PREVIEW_WIDTH / (float)UVCCamera.DEFAULT_PREVIEW_HEIGHT);
		mUVCCameraViewR.setSurfaceTextureListener(mSurfaceTextureListener);
		mUVCCameraViewR.setOnLongClickListener(mOnLongClickListener);

		mUSBMonitor = new USBMonitor(this, mOnDeviceConnectListener);
		mHandler = UVCCameraHandler.createHandler(this);
	}

	@Override
	public void onResume() {
		super.onResume();
		if (DEBUG) Log.v(TAG, "onResume:");
		mUSBMonitor.register();
	}

	@Override
	public void onPause() {
		if (DEBUG) Log.v(TAG, "onPause:");
//		mHandler.stopRecording();
//		mHandler.stopPreview();
    	mHandler.close();	// #close include #stopRecording and #stopPreview
		mCameraButton.setChecked(false);
		mCaptureButton.setVisibility(View.INVISIBLE);
		mUSBMonitor.unregister();
		super.onPause();
	}

	@Override
	public void onDestroy() {
		if (DEBUG) Log.v(TAG, "onDestroy:");
        if (mHandler != null) {
	        mHandler.release();
	        mHandler = null;
        }
        if (mUSBMonitor != null) {
	        mUSBMonitor.destroy();
	        mUSBMonitor = null;
        }
        mUVCCameraViewL = null;
        mUVCCameraViewR = null;
        mCameraButton = null;
        mCaptureButton = null;
		super.onDestroy();
	}

	/**
	 * event handler when click camera / capture button
	 */
	private final OnClickListener mOnClickListener = new OnClickListener() {
		@Override
		public void onClick(final View view) {
			switch (view.getId()) {
			case R.id.camera_button:
				if (!mHandler.isOpened()) {
					CameraDialog.showDialog(MainActivity.this);
				} else {
					mHandler.close();
					mCaptureButton.setVisibility(View.INVISIBLE);
				}
				break;
			case R.id.capture_button:
				if (mHandler.isOpened()) {
					if (!mHandler.isRecording()) {
						mCaptureButton.setColorFilter(0xffff0000);	// turn red
						mHandler.startRecording();
					} else {
						mCaptureButton.setColorFilter(0);	// return to default color
						mHandler.stopRecording();
					}
				}
				break;
			}
		}
	};

	/**
	 * capture still image when you long click on preview image(not on buttons)
	 */
	private final OnLongClickListener mOnLongClickListener = new OnLongClickListener() {
		@Override
		public boolean onLongClick(final View view) {
			switch (view.getId()) {
			case R.id.camera_view_L:
			case R.id.camera_view_R:
				if (mHandler.isOpened()) {
					final File outputFile = MediaMuxerWrapper.getCaptureFile(Environment.DIRECTORY_DCIM, ".png");
					mHandler.captureStill(outputFile.toString());
					try {
						if (DEBUG) Log.i(TAG, "MediaScannerConnection#scanFile");
						MediaScannerConnection.scanFile(getApplicationContext(), new String[]{ outputFile.toString() }, null, null);
					} catch (final Exception e) {
						Log.e(TAG, "MediaScannerConnection#scanFile:", e);
					}
					return true;
				}
			}
			return false;
		}
	};

	private void startPreview() {
		mHandler.startPreview();
		runOnUiThread(new Runnable() {
			@Override
			public void run() {
				mCaptureButton.setVisibility(View.VISIBLE);
			}
		});
	}

	private final OnDeviceConnectListener mOnDeviceConnectListener = new OnDeviceConnectListener() {
		@Override
		public void onAttach(final UsbDevice device) {
			Toast.makeText(MainActivity.this, "USB_DEVICE_ATTACHED", Toast.LENGTH_SHORT).show();
		}

		@Override
		public void onConnect(final UsbDevice device, final UsbControlBlock ctrlBlock, final boolean createNew) {
			if (DEBUG) Log.v(TAG, "onConnect:");
			mHandler.open(ctrlBlock);
			startPreview();
		}

		@Override
		public void onDisconnect(final UsbDevice device, final UsbControlBlock ctrlBlock) {
			if (DEBUG) Log.v(TAG, "onDisconnect:");
			if (mHandler != null) {
				mHandler.close();
				runOnUiThread(new Runnable() {
					@Override
					public void run() {
						if (!isFinishing())
						try {
							mCaptureButton.setVisibility(View.INVISIBLE);
							mCameraButton.setChecked(false);
						} catch (final Exception e) {
						}
					}
				});
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
	@Override
	public USBMonitor getUSBMonitor() {
		return mUSBMonitor;
	}

	private final TextureView.SurfaceTextureListener mSurfaceTextureListener
		= new TextureView.SurfaceTextureListener() {

			@Override
			public void onSurfaceTextureAvailable(final SurfaceTexture surface, final int width, final int height) {
				final Surface _surface = new Surface(surface);
				mHandler.addSurface(surface.hashCode(), _surface, false, null);
			}

			@Override
			public void onSurfaceTextureSizeChanged(final SurfaceTexture surface, final int width, final int height) {
			}

			@Override
			public boolean onSurfaceTextureDestroyed(final SurfaceTexture surface) {
				mHandler.removeSurface(surface.hashCode());
				return true;
			}

			@Override
			public void onSurfaceTextureUpdated(final SurfaceTexture surface) {
				// TODO Auto-generated method stub

			}

	};
}
