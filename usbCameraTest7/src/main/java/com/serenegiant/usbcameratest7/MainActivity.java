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

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.concurrent.LinkedBlockingQueue;
import java.util.concurrent.ThreadPoolExecutor;
import java.util.concurrent.TimeUnit;

import android.app.Activity;
import android.graphics.SurfaceTexture;
import android.hardware.usb.UsbDevice;
import android.os.Bundle;
import android.os.Handler;
import android.os.HandlerThread;
import android.util.Log;
import android.view.Surface;
import android.view.View;
import android.view.View.OnClickListener;
import android.widget.Toast;

import com.serenegiant.usb.CameraDialog;
import com.serenegiant.usb.DeviceFilter;
import com.serenegiant.usb.USBMonitor;
import com.serenegiant.usb.USBMonitor.OnDeviceConnectListener;
import com.serenegiant.usb.USBMonitor.UsbControlBlock;
import com.serenegiant.usb.UVCCamera;
import com.serenegiant.widget.UVCCameraTextureView;

public final class MainActivity extends Activity implements CameraDialog.CameraDialogParent {
	private static final boolean DEBUG = true;	// FIXME set false when production
	private static final String TAG = "MainActivity";

    // for thread pool
    private static final int CORE_POOL_SIZE = 1;		// initial/minimum threads
    private static final int MAX_POOL_SIZE = 4;			// maximum threads
    private static final int KEEP_ALIVE_TIME = 10;		// time periods while keep the idle thread

	private final Object mSync = new Object();
	private final Map<UsbDevice, CameraRec> mCameras = new HashMap<UsbDevice, CameraRec>();
    // for accessing USB and USB camera
    private USBMonitor mUSBMonitor;
	private UVCCameraTextureView mUVCCameraViewR;
	private UVCCameraTextureView mUVCCameraViewL;
	private Handler mAsyncHandler;

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

		final HandlerThread thread = new HandlerThread(TAG);
		thread.start();
		mAsyncHandler = new Handler(thread.getLooper());
		mUSBMonitor = new USBMonitor(this, mOnDeviceConnectListener);
		final List<DeviceFilter> filters = DeviceFilter.getDeviceFilters(this, R.xml.device_filter);
		mUSBMonitor.setDeviceFilter(filters);
	}

	@Override
	public void onResume() {
		super.onResume();
		if (DEBUG) Log.v(TAG, "onResume:");
		mUSBMonitor.register();
		synchronized (mSync) {
			for (final CameraRec rec: mCameras.values()) {
				rec.startPreview();
			}
		}
		mAsyncHandler.removeCallbacks(tryOpenTask);
		mAsyncHandler.postDelayed(tryOpenTask, 1000);
	}

	@Override
	public void onPause() {
		if (DEBUG) Log.v(TAG, "onPause:" + mCameras.size());
		mAsyncHandler.removeCallbacks(tryOpenTask);
		if (isFinishing()) {
			final List<CameraRec> list = new ArrayList<CameraRec>();
			synchronized (mSync) {
				list.addAll(mCameras.values());
				mCameras.clear();
			}
			synchronized (mSync) {
				mAsyncHandler.post(new Runnable() {
					@Override
					public void run() {
						if (DEBUG) Log.v(TAG, "onPause:" + list.size());
						for (final CameraRec rec: list) {
							rec.release();
						}
						synchronized (mSync) {
							mSync.notifyAll();
						}
					}
				});
				try {
					mSync.wait();
				} catch (InterruptedException e) {
				}
			}
		}
		mUSBMonitor.unregister();
		super.onPause();
		if (DEBUG) Log.v(TAG, "onPause:finished");
	}

	@Override
	public void onDestroy() {
		if (DEBUG) Log.v(TAG, "onDestroy:");
		if (mAsyncHandler != null) {
			mAsyncHandler.removeCallbacks(tryOpenTask);
		}
		if (mUSBMonitor != null) {
			mUSBMonitor.destroy();
			mUSBMonitor = null;
		}
		mUVCCameraViewR = null;
		mUVCCameraViewL = null;
		try {
			if (mAsyncHandler != null) {
				mAsyncHandler.getLooper().quitSafely();
			}
		} catch (final Exception e) {
		}
		mAsyncHandler = null;
		super.onDestroy();
	}

	private final OnClickListener mOnClickListener = new OnClickListener() {
		@Override
		public void onClick(final View view) {
			switch (view.getId()) {
			case R.id.camera_view_L:
			{
				final CameraRec rec = (CameraRec)mUVCCameraViewL.getTag();
				if (rec == null) {
					CameraDialog.showDialog(MainActivity.this);
				} else {
					rec.release();
				}
				break;
			}
			case R.id.camera_view_R:
			{
				final CameraRec rec = (CameraRec)mUVCCameraViewR.getTag();
				if (rec == null) {
					CameraDialog.showDialog(MainActivity.this);
				} else {
					rec.release();
				}
				break;
			}
			}
		}
	};

	private final OnDeviceConnectListener mOnDeviceConnectListener = new OnDeviceConnectListener() {
		@Override
		public void onAttach(final UsbDevice device) {
			if (DEBUG) Log.v(TAG, "onAttach:" + (device != null ? device.getDeviceName() : "null"));
			mAsyncHandler.removeCallbacks(tryOpenTask);
			mAsyncHandler.postDelayed(tryOpenTask, 1000);
		}

		@Override
		public void onConnect(final UsbDevice device, final UsbControlBlock ctrlBlock, final boolean createNew) {
			if (DEBUG) Log.v(TAG, "onConnect:" + (device != null ? device.getDeviceName() : "null"));
			openCamera(device, ctrlBlock);
		}

		@Override
		public void onDisconnect(final UsbDevice device, final UsbControlBlock ctrlBlock) {
			if (DEBUG) Log.v(TAG, "onDisconnect:" + (device != null ? device.getDeviceName() : "null"));
			mAsyncHandler.post(new Runnable() {
				@Override
				public void run() {
					synchronized (mSync) {
						final CameraRec rec = mCameras.remove(device);
						if (rec != null) {
							rec.release();
						}
					}
				}
			});
		}

		@Override
		public void onDettach(final UsbDevice device) {
			if (DEBUG) Log.v(TAG, "onDettach:" + (device != null ? device.getDeviceName() : "null"));
			mAsyncHandler.post(new Runnable() {
				@Override
				public void run() {
					synchronized (mSync) {
						final CameraRec rec = mCameras.remove(device);
						if (rec != null) {
							rec.release();
						}
					}
				}
			});
		}

		@Override
		public void onCancel() {
			if (DEBUG) Log.v(TAG, "onCancel:");
			synchronized (mSync) {
				mSync.notifyAll();
			}
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

	private boolean tryOpen(final UsbDevice device) {
		if (DEBUG) Log.v(TAG, "tryOpen:" + (device != null ? device.getDeviceName() : ""));
		if (device == null) return false;
		boolean retry = true;
		CameraRec rec = null;
		synchronized (mSync) {
			rec = mCameras.containsKey(device) ? mCameras.get(device) : null;
		}
		if (rec == null) {
			UVCCameraTextureView view = null;
			if ((mUVCCameraViewR != null) && (mUVCCameraViewR.getTag() == null)) {
				view = mUVCCameraViewR;
			} else if ((mUVCCameraViewL != null) && (mUVCCameraViewL.getTag() == null)) {
				view = mUVCCameraViewL;
			}
			if (view != null) {
				rec = new CameraRec(view);
				synchronized (mSync) {
					mCameras.put(device, rec);
					mUSBMonitor.requestPermission(device);
					try {
						mSync.wait();
						if (rec.mCamera == null) {
							if (DEBUG) Log.w(TAG, "failed to start camera");
							mCameras.remove(device);
						}
					} catch (final InterruptedException e) {
						retry = false;
					}
				}
			} else {
				if (DEBUG) Log.w(TAG, "there may be more than three connected camera.");
				retry = false;
			}
		} else {
			if (DEBUG) Log.v(TAG, "will be already connected.");
			retry = false;
		}
		return retry;
	}

	private static final float[] BANDWIDTH_FACTORS = { 0.67f, 0.67f };
	private void openCamera(final UsbDevice device, final UsbControlBlock ctrlBlock) {
		if (DEBUG) Log.v(TAG, "openCamera:" + (device != null ? device.getDeviceName() : "null"));
		final UVCCamera camera = new  UVCCamera();
		synchronized (mSync) {
			final CameraRec rec = mCameras.containsKey(device) ? mCameras.get(device) : null;
			if (rec != null) {
				final int open_camera_nums = mCameras.size() - 1; // (mLeftCamera != null ? 1 : 0) + (mRightCamera != null ? 1 : 0);
				camera.open(ctrlBlock);
				try {
					camera.setPreviewSize(UVCCamera.DEFAULT_PREVIEW_WIDTH, UVCCamera.DEFAULT_PREVIEW_HEIGHT, UVCCamera.FRAME_FORMAT_MJPEG, BANDWIDTH_FACTORS[open_camera_nums]);
				} catch (final IllegalArgumentException e) {
					// fallback to YUV mode
					try {
						camera.setPreviewSize(UVCCamera.DEFAULT_PREVIEW_WIDTH, UVCCamera.DEFAULT_PREVIEW_HEIGHT, UVCCamera.DEFAULT_PREVIEW_MODE, BANDWIDTH_FACTORS[open_camera_nums]);
					} catch (final IllegalArgumentException e1) {
						camera.destroy();
						return;
					}
				}
				rec.setCamera(camera);
			} else {
				if (DEBUG) Log.v(TAG, "CameraRec is null");
			}
			mSync.notifyAll();
		}
		if (DEBUG) Log.v(TAG, "openCamera:finished");
	}

	private static class CameraRec {
		private UVCCamera mCamera;
		private final UVCCameraTextureView mCameraView;
		private Surface mSurface;
		public CameraRec(final UVCCameraTextureView view) {
			mCameraView = view;
			view.setTag(this);
		}

		public void release() {
			if (DEBUG) Log.v(TAG, "release:");
			mCameraView.setTag(null);
			if (mCamera != null) {
				mCamera.destroy();
				mCamera = null;
			}
			if (mSurface != null) {
				mSurface.release();
				mSurface = null;
			}
			if (DEBUG) Log.v(TAG, "release:finished");
		}

		public void setCamera(final UVCCamera camera) {
			if (DEBUG) Log.v(TAG, "setCamera:");
			mCamera = camera;
			if (mSurface != null) {
				mSurface.release();
				mSurface = null;
			}
			final SurfaceTexture st = mCameraView.getSurfaceTexture();
			if (st != null)
				mSurface = new Surface(st);
			camera.setPreviewDisplay(mSurface);
			camera.startPreview();
		}

		public void startPreview() {
			if (mCamera != null) {
				mCamera.startPreview();
			}
		}

		public void stopPreview() {
			if (mCamera != null) {
				mCamera.stopPreview();
			}
		}
	}

	private final Runnable tryOpenTask = new Runnable() {
		@Override
		public void run() {
			if (DEBUG) Log.v(TAG, "tryOpenTask#run:");
			final List<UsbDevice> list = mUSBMonitor.getDeviceList();
			int retry = list.size();
			for (final UsbDevice device: list) {
				if (mUSBMonitor.hasPermission(device)) {
					if (!tryOpen(device)) {
						retry--;
					}
				} else {
					retry--;
				}
			}
			if ((retry > 0) && !isFinishing()) {
				mAsyncHandler.postDelayed(this, 1000);
			}
			if (DEBUG) Log.v(TAG, "tryOpenTask#run:finished");
		}
	};
}
