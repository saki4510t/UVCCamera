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

package com.serenegiant.serviceclient;

import java.lang.ref.WeakReference;

import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.ServiceConnection;
import android.hardware.usb.UsbDevice;
import android.os.Handler;
import android.os.IBinder;
import android.os.Looper;
import android.os.Message;
import android.os.RemoteException;
import android.util.Log;
import android.view.Surface;

import com.serenegiant.service.IUVCService;
import com.serenegiant.service.IUVCServiceCallback;

public class CameraClient implements ICameraClient {
	private static final boolean DEBUG = true;
	private static final String TAG = "CameraClient";

	protected final WeakReference<Context> mWeakContext;
	protected final WeakReference<CameraHandler> mWeakHandler;
	protected UsbDevice mUsbDevice;

	protected final Object mServiceSync = new Object();
	protected IUVCService mService;
	protected ICameraClientCallback mListener;

	public CameraClient(final Context context, final ICameraClientCallback listener) {
		if (DEBUG) Log.v(TAG, "Constructor:");
		mWeakContext = new WeakReference<Context>(context);
		mListener = listener;
		mWeakHandler = new WeakReference<CameraHandler>(CameraHandler.createHandler(this));
		doBindService();
	}

	@Override
	protected void finalize() throws Throwable {
		if (DEBUG) Log.v(TAG, "finalize");
		doUnBindService();
		super.finalize();
	}

	@Override
	public void select(final UsbDevice device) {
		if (DEBUG) Log.v(TAG, "select:device=" + (device != null ? device.getDeviceName() : null));
		mUsbDevice = device;
		final CameraHandler handler = mWeakHandler.get();
		handler.sendMessage(handler.obtainMessage(MSG_SELECT, device));
	}

	@Override
	public void release() {
		if (DEBUG) Log.v(TAG, "release:" + this);
		mUsbDevice = null;
		mWeakHandler.get().sendEmptyMessage(MSG_RELEASE);
	}

	@Override
	public UsbDevice getDevice() {
		return mUsbDevice;
	}

	@Override
	public void resize(final int width, final int height) {
		if (DEBUG) Log.v(TAG, String.format("resize(%d,%d)", width, height));
		final CameraHandler handler = mWeakHandler.get();
		handler.sendMessage(handler.obtainMessage(MSG_RESIZE, width, height));
	}
	
	@Override
	public void connect() {
		if (DEBUG) Log.v(TAG, "connect:");
		mWeakHandler.get().sendEmptyMessage(MSG_CONNECT);
	}

	@Override
	public void disconnect() {
		if (DEBUG) Log.v(TAG, "disconnect:" + this);
		mWeakHandler.get().sendEmptyMessage(MSG_DISCONNECT);
	}

	@Override
	public void addSurface(final Surface surface, final boolean isRecordable) {
		if (DEBUG) Log.v(TAG, "addSurface:surface=" + surface + ",hash=" + surface.hashCode());
		final CameraHandler handler = mWeakHandler.get();
		handler.sendMessage(handler.obtainMessage(MSG_ADD_SURFACE, isRecordable ? 1 : 0, 0, surface));
	}

	@Override
	public void removeSurface(final Surface surface) {
		if (DEBUG) Log.v(TAG, "removeSurface:surface=" + surface + ",hash=" + surface.hashCode());
		final CameraHandler handler = mWeakHandler.get();
		handler.sendMessage(handler.obtainMessage(MSG_REMOVE_SURFACE, surface));
	}

	@Override
	public boolean isRecording() {
		final CameraHandler handler = mWeakHandler.get();
		return (handler != null) && handler.isRecording();
	}

	@Override
	public void startRecording() {
		final CameraHandler handler = mWeakHandler.get();
		if ((handler != null) && !handler.isRecording()) {
			handler.sendEmptyMessage(MSG_START_RECORDING);
		}
	}

	@Override
	public void stopRecording() {
		final CameraHandler handler = mWeakHandler.get();
		if ((handler != null) && handler.isRecording()) {
			handler.sendEmptyMessage(MSG_STOP_RECORDING);
		}
	}

	@Override
	public void captureStill(final String path) {
		final CameraHandler handler = mWeakHandler.get();
		if (handler != null) {
			handler.sendMessage(handler.obtainMessage(MSG_CAPTURE_STILL, path));
		}
	}

	protected boolean doBindService() {
		if (DEBUG) Log.v(TAG, "doBindService:");
		synchronized (mServiceSync) {
			if (mService == null) {
				final Context context = mWeakContext.get();
				if (context != null) {
					final Intent intent = new Intent(IUVCService.class.getName());
					intent.setPackage("com.serenegiant.usbcameratest4");
					context.bindService(intent,
						mServiceConnection, Context.BIND_AUTO_CREATE);
				} else
					return true;
			}
		}
		return false;
	}

	protected void doUnBindService() {
		if (DEBUG) Log.v(TAG, "doUnBindService:");
		synchronized (mServiceSync) {
			if (mService != null) {
				final Context context = mWeakContext.get();
				if (context != null) {
					try {
						context.unbindService(mServiceConnection);
					} catch (final Exception e) {
						// ignore
					}
				}
				mService = null;
			}
		}
	}

	private final ServiceConnection mServiceConnection = new ServiceConnection() {
		@Override
		public void onServiceConnected(final ComponentName name, final IBinder service) {
			if (DEBUG) Log.v(TAG, "onServiceConnected:name=" + name);
			synchronized (mServiceSync) {
				mService = IUVCService.Stub.asInterface(service);
				mServiceSync.notifyAll();
			}
		}

		@Override
		public void onServiceDisconnected(final ComponentName name) {
			if (DEBUG) Log.v(TAG, "onServiceDisconnected:name=" + name);
			synchronized (mServiceSync) {
				mService = null;
				mServiceSync.notifyAll();
			}
		}
	};

	/**
	 * get reference to instance of IUVCService
	 * you should not call this from UI thread, this method block until the service is available
	 * @return
	 */
	private IUVCService getService() {
		synchronized (mServiceSync) {
			if (mService == null) {
				try {
					mServiceSync.wait();
				} catch (final InterruptedException e) {
					if (DEBUG) Log.e(TAG, "getService:", e);
				}
			}
		}
		return mService;
	}

	private static final int MSG_SELECT = 0;
	private static final int MSG_CONNECT = 1;
	private static final int MSG_DISCONNECT = 2;
	private static final int MSG_ADD_SURFACE = 3;
	private static final int MSG_REMOVE_SURFACE = 4;
	private static final int MSG_START_RECORDING = 6;
	private static final int MSG_STOP_RECORDING = 7;
	private static final int MSG_CAPTURE_STILL = 8;
	private static final int MSG_RESIZE = 9;
	private static final int MSG_RELEASE = 99;

	private static final class CameraHandler extends Handler {

		public static CameraHandler createHandler(final CameraClient parent) {
			final CameraTask runnable = new CameraTask(parent);
			new Thread(runnable).start();
			return runnable.getHandler();
		}

		private CameraTask mCameraTask;
		private CameraHandler(final CameraTask cameraTask) {
			mCameraTask = cameraTask;
		}

		public boolean isRecording() {
			final IUVCService service = mCameraTask.mParent.getService();
			if (service != null)
			try {
				return service.isRecording(mCameraTask.mServiceId);
			} catch (final RemoteException e) {
				if (DEBUG) Log.e(TAG, "isRecording:", e);
			}
			return false;
		}

		@Override
		public void handleMessage(final Message msg) {
			switch (msg.what) {
			case MSG_SELECT:
				mCameraTask.handleSelect((UsbDevice)msg.obj);
				break;
			case MSG_CONNECT:
				mCameraTask.handleConnect();
				break;
			case MSG_DISCONNECT:
				mCameraTask.handleDisconnect();
				break;
			case MSG_ADD_SURFACE:
				mCameraTask.handleAddSurface((Surface)msg.obj, msg.arg1 != 0);
				break;
			case MSG_REMOVE_SURFACE:
				mCameraTask.handleRemoveSurface((Surface)msg.obj);
				break;
			case MSG_START_RECORDING:
				mCameraTask.handleStartRecording();
				break;
			case MSG_STOP_RECORDING:
				mCameraTask.handleStopRecording();
				break;
			case MSG_CAPTURE_STILL:
				mCameraTask.handleCaptureStill((String)msg.obj);
				break;
			case MSG_RESIZE:
				mCameraTask.handleResize(msg.arg1, msg.arg2);
				break;
			case MSG_RELEASE:
				mCameraTask.handleRelease();
				mCameraTask = null;
				Looper.myLooper().quit();
				break;
			default:
				throw new RuntimeException("unknown message:what=" + msg.what);
			}
		}

		private static final class CameraTask extends IUVCServiceCallback.Stub implements Runnable {
			private static final String TAG_CAMERA = "CameraClientThread";
			private final Object mSync = new Object();
			private CameraClient mParent;
			private CameraHandler mHandler;
			private boolean mIsConnected;
			private int mServiceId;

			private CameraTask(final CameraClient parent) {
				mParent = parent;
			}

			public CameraHandler getHandler() {
				synchronized (mSync) {
					if (mHandler == null)
					try {
						mSync.wait();
					} catch (final InterruptedException e) {
					}
				}
				return mHandler;
			}

			@Override
			public void run() {
				if (DEBUG) Log.v(TAG_CAMERA, "run:");
				Looper.prepare();
				synchronized (mSync) {
					mHandler = new CameraHandler(this);
					mSync.notifyAll();
				}
				Looper.loop();
				if (DEBUG) Log.v(TAG_CAMERA, "run:finishing");
				synchronized (mSync) {
					mHandler = null;
					mParent = null;
					mSync.notifyAll();
				}
			}

//================================================================================
// callbacks from service
			@Override
			public void onConnected() throws RemoteException {

				if (DEBUG) Log.v(TAG_CAMERA, "onConnected:");
				mIsConnected = true;
				if (mParent != null) {
					if (mParent.mListener != null) {
						mParent.mListener.onConnect();
					}
				}
			}

			@Override
			public void onDisConnected() throws RemoteException {
				if (DEBUG) Log.v(TAG_CAMERA, "onDisConnected:");
				mIsConnected = false;
				if (mParent != null) {
					if (mParent.mListener != null) {
						mParent.mListener.onDisconnect();
					}
				}
			}

//================================================================================
			public void handleSelect(final UsbDevice device) {
				if (DEBUG) Log.v(TAG_CAMERA, "handleSelect:");
				final IUVCService service = mParent.getService();
				if (service != null) {
					try {
						mServiceId = service.select(device, this);
					} catch (final RemoteException e) {
						if (DEBUG) Log.e(TAG_CAMERA, "select:", e);
					}
				}
			}

			public void handleRelease() {
				if (DEBUG) Log.v(TAG_CAMERA, "handleRelease:");
				mIsConnected = false;
				mParent.doUnBindService();
			}

			public void handleConnect() {
				if (DEBUG) Log.v(TAG_CAMERA, "handleConnect:");
				final IUVCService service = mParent.getService();
				if (service != null)
				try {
					if (!mIsConnected/*!service.isConnected(mServiceId)*/) {
						service.connect(mServiceId);
					} else {
						onConnected();
					}
				} catch (final RemoteException e) {
					if (DEBUG) Log.e(TAG_CAMERA, "handleConnect:", e);
				}
			}

			public void handleDisconnect() {
				if (DEBUG) Log.v(TAG_CAMERA, "handleDisconnect:");
				final IUVCService service = mParent.getService();
				if (service != null)
				try {
					if (service.isConnected(mServiceId)) {
						service.disconnect(mServiceId);
					} else {
						onDisConnected();
					}
				} catch (final RemoteException e) {
					if (DEBUG) Log.e(TAG_CAMERA, "handleDisconnect:", e);
				}
				mIsConnected = false;
			}

			public void handleAddSurface(final Surface surface, final boolean isRecordable) {
				if (DEBUG) Log.v(TAG_CAMERA, "handleAddSurface:surface=" + surface + ",hash=" + surface.hashCode());
				final IUVCService service = mParent.getService();
				if (service != null)
				try {
					service.addSurface(mServiceId, surface.hashCode(), surface, isRecordable);
				} catch (final RemoteException e) {
					if (DEBUG) Log.e(TAG_CAMERA, "handleAddSurface:", e);
				}
			}

			public void handleRemoveSurface(final Surface surface) {
				if (DEBUG) Log.v(TAG_CAMERA, "handleRemoveSurface:surface=" + surface + ",hash=" + surface.hashCode());
				final IUVCService service = mParent.getService();
				if (service != null)
				try {
					service.removeSurface(mServiceId, surface.hashCode());
				} catch (final RemoteException e) {
					if (DEBUG) Log.e(TAG_CAMERA, "handleRemoveSurface:", e);
				}
			}

			public void handleStartRecording() {
				if (DEBUG) Log.v(TAG, "handleStartRecording:");
				final IUVCService service = mParent.getService();
				if (service != null)
				try {
					if (!service.isRecording(mServiceId)) {
						service.startRecording(mServiceId);
					}
				} catch (final RemoteException e) {
					if (DEBUG) Log.e(TAG_CAMERA, "handleStartRecording:", e);
				}
			}

			public void handleStopRecording() {
				if (DEBUG) Log.v(TAG, "handleStopRecording:");
				final IUVCService service = mParent.getService();
				if (service != null)
				try {
					if (service.isRecording(mServiceId)) {
						service.stopRecording(mServiceId);
					}
				} catch (final RemoteException e) {
					if (DEBUG) Log.e(TAG_CAMERA, "handleStopRecording:", e);
				}
			}

			public void handleCaptureStill(final String path) {
				if (DEBUG) Log.v(TAG, "handleCaptureStill:" + path);
				final IUVCService service = mParent.getService();
				if (service != null)
				try {
					service.captureStillImage(mServiceId, path);
				} catch (final RemoteException e) {
					if (DEBUG) Log.e(TAG_CAMERA, "handleCaptureStill:", e);
				}
			}
			
			public void handleResize(final int width, final int height) {
				if (DEBUG) Log.v(TAG, String.format("handleResize(%d,%d)", width, height));
				final IUVCService service = mParent.getService();
				if (service != null)
				try {
					service.resize(mServiceId, width, height);
				} catch (final RemoteException e) {
					if (DEBUG) Log.e(TAG_CAMERA, "handleResize:", e);
				}
			}
		}
	}

}
