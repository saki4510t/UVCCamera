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

package com.serenegiant.usbcameratest7;

import android.content.Intent;
import android.graphics.SurfaceTexture;
import android.hardware.usb.UsbDevice;
import android.os.Bundle;
import android.util.Log;
import android.view.Surface;
import android.view.View;
import android.view.View.OnClickListener;
import android.widget.ImageButton;
import android.widget.Toast;

import com.ford.openxc.webcam.webcam.WebcamPreview;
import com.serenegiant.common.BaseActivity;
import com.serenegiant.service.LogService;
import com.serenegiant.usb.CameraDialog;
import com.serenegiant.usb.USBMonitor;
import com.serenegiant.usb.USBMonitor.OnDeviceConnectListener;
import com.serenegiant.usb.USBMonitor.UsbControlBlock;
import com.serenegiant.usb.UVCCamera;
import com.serenegiant.usbcameracommon.UVCCameraHandler;
import com.serenegiant.widget.CameraViewInterface;
import com.serenegiant.widget.UVCCameraTextureView;

/**
 * Show side by side view from two camera.
 * You cane record video images from both camera, but secondarily started recording can not record
 * audio because of limitation of Android AudioRecord(only one instance of AudioRecord is available
 * on the device) now.
 */
public final class MainActivity extends BaseActivity implements CameraDialog.CameraDialogParent {
	private static final boolean DEBUG = false;    // FIXME set false when production
	private static final String TAG = "MainActivity";

	//带宽, 影响每包的最大数据量, 见libuvc/stream.c下函数uvc_stream_start_bandwidth
	// TODO: 17-9-9 下午4:01 yuyv设置为1, mjpeg设置为0.5 来自:= https://github.com/saki4510t/UVCCamera/issues/231#issuecomment-328016969
	private static final float[] BANDWIDTH_FACTORS = {0.5f, 1.0f};

	private static final int WIDTH = UVCCamera.DEFAULT_PREVIEW_WIDTH / 2;
	private static final int HEIGHT = UVCCamera.DEFAULT_PREVIEW_HEIGHT / 2;

	// for accessing USB and USB camera
	private USBMonitor mUSBMonitor;

	private WebcamPreview mWebcamPreview;

	private UVCCameraHandler mHandlerR;
	private CameraViewInterface mUVCCameraViewR;
	private ImageButton mCaptureButtonR;
	private Surface mRightPreviewSurface;

	private UVCCameraHandler mHandlerL;
	private CameraViewInterface mUVCCameraViewL;
	private ImageButton mCaptureButtonL;
	private Surface mLeftPreviewSurface;


	@Override
	protected void onCreate(final Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		setContentView(R.layout.activity_main);

		//记录日志
		startService(new Intent(this, LogService.class));

		mWebcamPreview = (WebcamPreview) findViewById(R.id.camera_view_preview);

		findViewById(R.id.RelativeLayout1).setOnClickListener(mOnClickListener);
		mUVCCameraViewL = (CameraViewInterface) findViewById(R.id.camera_view_L);
		mUVCCameraViewL.setAspectRatio(WIDTH / (float) HEIGHT);
		((UVCCameraTextureView) mUVCCameraViewL).setOnClickListener(mOnClickListener);
		mCaptureButtonL = (ImageButton) findViewById(R.id.capture_button_L);
		mCaptureButtonL.setOnClickListener(mOnClickListener);
		mCaptureButtonL.setVisibility(View.INVISIBLE);
		mHandlerL = UVCCameraHandler.createHandler(this, mUVCCameraViewL, WIDTH,
				HEIGHT, UVCCamera.FRAME_FORMAT_MJPEG, BANDWIDTH_FACTORS[0]);


		mUVCCameraViewR = (CameraViewInterface) findViewById(R.id.camera_view_R);
		mUVCCameraViewR.setAspectRatio(WIDTH / (float) HEIGHT);
		((UVCCameraTextureView) mUVCCameraViewR).setOnClickListener(mOnClickListener);
		mCaptureButtonR = (ImageButton) findViewById(R.id.capture_button_R);
		mCaptureButtonR.setOnClickListener(mOnClickListener);
		mCaptureButtonR.setVisibility(View.INVISIBLE);
		mHandlerR = UVCCameraHandler.createHandler(this, mUVCCameraViewR, WIDTH,
				HEIGHT, UVCCamera.FRAME_FORMAT_YUYV, BANDWIDTH_FACTORS[1]);

		mUSBMonitor = new USBMonitor(this, mOnDeviceConnectListener);
	}

	@Override
	protected void onStart() {
		super.onStart();
		mUSBMonitor.register();
		if (mUVCCameraViewR != null)
			mUVCCameraViewR.onResume();
		if (mUVCCameraViewL != null)
			mUVCCameraViewL.onResume();
	}

	@Override
	protected void onStop() {
		mHandlerR.close();
		if (mUVCCameraViewR != null)
			mUVCCameraViewR.onPause();
		mHandlerL.close();
		mCaptureButtonR.setVisibility(View.INVISIBLE);
		if (mUVCCameraViewL != null)
			mUVCCameraViewL.onPause();
		mCaptureButtonL.setVisibility(View.INVISIBLE);
		mUSBMonitor.unregister();
		super.onStop();
	}

	@Override
	protected void onDestroy() {
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

		stopService(new Intent(this, LogService.class));

		super.onDestroy();
	}

	private final OnClickListener mOnClickListener = new OnClickListener() {
		@Override
		public void onClick(final View view) {
			switch (view.getId()) {
				case R.id.camera_view_L:
					if (mHandlerL != null) {
						if (!mHandlerL.isOpened()) {
							CameraDialog.showDialog(MainActivity.this);
						} else {
							mHandlerL.close();
							setCameraButton();
						}
					}
					break;
				case R.id.capture_button_L:
					if (mHandlerL != null) {
						if (mHandlerL.isOpened()) {
							if (checkPermissionWriteExternalStorage() && checkPermissionAudio()) {
								if (!mHandlerL.isRecording()) {
									mCaptureButtonL.setColorFilter(0xffff0000);    // turn red
									mHandlerL.startRecording();
								} else {
									mCaptureButtonL.setColorFilter(0);    // return to default color
									mHandlerL.stopRecording();
								}
							}
						}
					}
					break;
				case R.id.camera_view_R:
					if (mHandlerR != null) {
						if (!mHandlerR.isOpened()) {
							CameraDialog.showDialog(MainActivity.this);
						} else {
							mHandlerR.close();
							setCameraButton();
						}
					}
					break;
				case R.id.capture_button_R:
					if (mHandlerR != null) {
						if (mHandlerR.isOpened()) {
							if (checkPermissionWriteExternalStorage() && checkPermissionAudio()) {
								if (!mHandlerR.isRecording()) {
									mCaptureButtonR.setColorFilter(0xffff0000);    // turn red
									mHandlerR.startRecording();
								} else {
									mCaptureButtonR.setColorFilter(0);    // return to default color
									mHandlerR.stopRecording();
								}
							}
						}
					}
					break;
			}
		}
	};

	private final OnDeviceConnectListener mOnDeviceConnectListener = new OnDeviceConnectListener() {
		@Override
		public void onAttach(final UsbDevice device) {
			if (DEBUG) Log.w(TAG, "onAttach:" + device);
			Toast.makeText(MainActivity.this, "USB_DEVICE_ATTACHED", Toast.LENGTH_SHORT).show();
		}

		@Override
		public void onConnect(final UsbDevice device, final UsbControlBlock ctrlBlock, final boolean createNew) {
			if (DEBUG) Log.w(TAG, "onConnect:" + device);
			if (!mHandlerL.isOpened()) {
				mHandlerL.open(ctrlBlock);
				final SurfaceTexture st = mUVCCameraViewL.getSurfaceTexture();
				mHandlerL.startPreview(new Surface(st));
				runOnUiThread(new Runnable() {
					@Override
					public void run() {
						mCaptureButtonL.setVisibility(View.VISIBLE);
					}
				});
			} else if (!mHandlerR.isOpened()) {
				mHandlerR.open(ctrlBlock);
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
			if (DEBUG) Log.w(TAG, "onDisconnect:" + device);
			if ((mHandlerL != null) && !mHandlerL.isEqual(device)) {
				queueEvent(new Runnable() {
					@Override
					public void run() {
						mHandlerL.close();
						if (mLeftPreviewSurface != null) {
							mLeftPreviewSurface.release();
							mLeftPreviewSurface = null;
						}
						setCameraButton();
					}
				}, 0);
			} else if ((mHandlerR != null) && !mHandlerR.isEqual(device)) {
				queueEvent(new Runnable() {
					@Override
					public void run() {
						mHandlerR.close();
						if (mRightPreviewSurface != null) {
							mRightPreviewSurface.release();
							mRightPreviewSurface = null;
						}
						setCameraButton();
					}
				}, 0);
			}
		}

		@Override
		public void onDettach(final UsbDevice device) {
			if (DEBUG) Log.w(TAG, "onDettach:" + device);
			Toast.makeText(MainActivity.this, "USB_DEVICE_DETACHED", Toast.LENGTH_SHORT).show();
		}

		@Override
		public void onCancel(final UsbDevice device) {
			if (DEBUG) Log.w(TAG, "onCancel:");
		}
	};

	/**
	 * to access from CameraDialog
	 *
	 * @return
	 */
	@Override
	public USBMonitor getUSBMonitor() {
		return mUSBMonitor;
	}

	@Override
	public void onDialogResult(boolean canceled) {
		if (canceled) {
			runOnUiThread(new Runnable() {
				@Override
				public void run() {
					setCameraButton();
				}
			}, 0);
		}
	}

	private void setCameraButton() {
		runOnUiThread(new Runnable() {
			@Override
			public void run() {
				if ((mHandlerL != null) && !mHandlerL.isOpened() && (mCaptureButtonL != null)) {
					mCaptureButtonL.setVisibility(View.INVISIBLE);
				}
				if ((mHandlerR != null) && !mHandlerR.isOpened() && (mCaptureButtonR != null)) {
					mCaptureButtonR.setVisibility(View.INVISIBLE);
				}
			}
		}, 0);
	}
}
