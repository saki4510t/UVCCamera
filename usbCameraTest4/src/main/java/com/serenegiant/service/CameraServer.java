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

package com.serenegiant.service;

import android.content.Context;
import android.media.AudioManager;
import android.media.MediaScannerConnection;
import android.media.SoundPool;
import android.os.Handler;
import android.os.IBinder;
import android.os.Looper;
import android.os.Message;
import android.os.RemoteCallbackList;
import android.os.RemoteException;
import android.text.TextUtils;
import android.view.Surface;

import com.serenegiant.encoder.MediaEncoder;
import com.serenegiant.encoder.MediaMuxerWrapper;
import com.serenegiant.encoder.MediaSurfaceEncoder;
import com.serenegiant.glutils.RenderHolderCallback;
import com.serenegiant.glutils.RendererHolder;
import com.serenegiant.usb.IFrameCallback;
import com.serenegiant.usb.Size;
import com.serenegiant.usb.USBMonitor.UsbControlBlock;
import com.serenegiant.usb.UVCCamera;
import com.serenegiant.usbcameratest4.R;
import com.serenegiant.utils.Constants;
import com.socks.library.KLog;

import java.io.IOException;
import java.lang.ref.WeakReference;
import java.lang.reflect.Field;
import java.nio.ByteBuffer;

public final class CameraServer extends Handler {
	private static final boolean DEBUG = true;
	private static final String TAG = "CameraServer";

	private int mFrameWidth = Constants.PIC_WIDTH_DEFAULT / 2;
	private int mFrameHeight = Constants.PIC_HEIGHT_DEFAULT / 2;

    private static class CallbackCookie {
		boolean isConnected;
	}

    private final RemoteCallbackList<IUVCServiceCallback> mCallbacks
		= new RemoteCallbackList<IUVCServiceCallback>();
    private int mRegisteredCallbackCount;

	private RendererHolder mRendererHolder;
	private final WeakReference<CameraThread> mWeakThread;

	public static CameraServer createServer(final Context context, final UsbControlBlock ctrlBlock, final int vid, final int pid) {
		if (DEBUG) KLog.w(TAG, "createServer:");
		final CameraThread thread = new CameraThread(context, ctrlBlock);
		thread.start();
		return thread.getHandler();
	}

	private CameraServer(final CameraThread thread) {
		if (DEBUG) KLog.w(TAG, "Constructor:");
		mWeakThread = new WeakReference<CameraThread>(thread);
		mRegisteredCallbackCount = 0;
		mRendererHolder = new RendererHolder(mFrameWidth, mFrameHeight, mRenderHolderCallback);
	}

	@Override
	protected void finalize() throws Throwable {
		if (DEBUG) KLog.w(TAG, "finalize:");
		release();
		super.finalize();
	}

	public void registerCallback(final IUVCServiceCallback callback) {
		if (DEBUG) KLog.w(TAG, "registerCallback:");
		mCallbacks.register(callback, new CallbackCookie());
		mRegisteredCallbackCount++;
	}

	public boolean unregisterCallback(final IUVCServiceCallback callback) {
		if (DEBUG) KLog.w(TAG, "unregisterCallback:");
		mCallbacks.unregister(callback);
		mRegisteredCallbackCount--;
		if (mRegisteredCallbackCount < 0) mRegisteredCallbackCount = 0;
		return mRegisteredCallbackCount == 0;
	}

	public void release() {
		if (DEBUG) KLog.w(TAG, "release:");
		disconnect();
		mCallbacks.kill();
		if (mRendererHolder != null) {
			mRendererHolder.release();
			mRendererHolder = null;
		}
	}

//********************************************************************************
//********************************************************************************
	public void resize(final int width, final int height) {
		if (DEBUG) KLog.w(TAG, String.format("resize(%d,%d)", width, height));
		if (!isRecording()) {
			mFrameWidth = width;
			mFrameHeight = height;
			if (mRendererHolder != null) {
				mRendererHolder.resize(width, height);
			}
		}
	}
	
	public void connect() {
		if (DEBUG) KLog.w(TAG, "connect:");
		final CameraThread thread = mWeakThread.get();
		if (!thread.isCameraOpened()) {
			sendMessage(obtainMessage(MSG_OPEN));
			sendMessage(obtainMessage(MSG_PREVIEW_START, mFrameWidth, mFrameHeight, mRendererHolder.getSurface()));
		} else {
			if (DEBUG) KLog.w(TAG, "already connected, just call callback");
			processOnCameraStart();
		}
	}

	public void connectSlave() {
		if (DEBUG) KLog.w(TAG, "connectSlave:");
		final CameraThread thread = mWeakThread.get();
		if (thread.isCameraOpened()) {
			processOnCameraStart();
		}
	}

	public void disconnect() {
		if (DEBUG) KLog.w(TAG, "disconnect:");
		stopRecording();
		final CameraThread thread = mWeakThread.get();
		if (thread == null) return;
		synchronized (thread.mSync) {
			sendEmptyMessage(MSG_PREVIEW_STOP);
			sendEmptyMessage(MSG_CLOSE);
			// wait for actually preview stopped to avoid releasing Surface/SurfaceTexture
			// while preview is still running.
			// therefore this method will take a time to execute
			try {
				thread.mSync.wait();
			} catch (final InterruptedException e) {
			}
		}
	}

	public boolean isConnected() {
		final CameraThread thread = mWeakThread.get();
		return (thread != null) && thread.isCameraOpened();
	}

	public boolean isRecording() {
		final CameraThread thread = mWeakThread.get();
		return (thread != null) && thread.isRecording();
	}

	public void addSurface(final int id, final Surface surface, final boolean isRecordable, final IUVCServiceOnFrameAvailable onFrameAvailableListener) {
		if (DEBUG) KLog.w(TAG, "addSurface:id=" + id +",surface=" + surface);
		if (mRendererHolder != null)
			mRendererHolder.addSurface(id, surface, isRecordable);
	}

	public void removeSurface(final int id) {
		if (DEBUG) KLog.w(TAG, "removeSurface:id=" + id);
		if (mRendererHolder != null)
			mRendererHolder.removeSurface(id);
	}

	public void startRecording() {
		if (!isRecording())
			sendEmptyMessage(MSG_CAPTURE_START);
	}

	public void stopRecording() {
		if (isRecording())
			sendEmptyMessage(MSG_CAPTURE_STOP);
	}

	public void captureStill(final String path) {
		if (mRendererHolder != null) {
			mRendererHolder.captureStill(path);
			sendMessage(obtainMessage(MSG_CAPTURE_STILL, path));
		}
	}

	public void startPush() {
		sendEmptyMessage(MSG_PUSH_START);
	}

	public void stopPush() {
		sendEmptyMessage(MSG_PUSH_STOP);
	}

//********************************************************************************
	private void processOnCameraStart() {
		if (DEBUG) KLog.w(TAG, "processOnCameraStart:");
		try {
			final int n = mCallbacks.beginBroadcast();
			for (int i = 0; i < n; i++) {
				if (!((CallbackCookie)mCallbacks.getBroadcastCookie(i)).isConnected)
				try {
					mCallbacks.getBroadcastItem(i).onConnected();
					((CallbackCookie)mCallbacks.getBroadcastCookie(i)).isConnected = true;
				} catch (final Exception e) {
					KLog.e(TAG, "failed to call IOverlayCallback#onFrameAvailable");
				}
			}
			mCallbacks.finishBroadcast();
		} catch (final Exception e) {
			KLog.w(TAG, e);
		}
	}

	private void processOnCameraStop() {
		if (DEBUG) KLog.w(TAG, "processOnCameraStop:");
		final int n = mCallbacks.beginBroadcast();
		for (int i = 0; i < n; i++) {
			if (((CallbackCookie)mCallbacks.getBroadcastCookie(i)).isConnected)
			try {
				mCallbacks.getBroadcastItem(i).onDisConnected();
				((CallbackCookie)mCallbacks.getBroadcastCookie(i)).isConnected = false;
			} catch (final Exception e) {
				KLog.e(TAG, "failed to call IOverlayCallback#onDisConnected");
			}
		}
		mCallbacks.finishBroadcast();
	}

//**********************************************************************
	private static final int MSG_OPEN = 0;
	private static final int MSG_CLOSE = 1;
	private static final int MSG_PREVIEW_START = 2;
	private static final int MSG_PREVIEW_STOP = 3;
	private static final int MSG_CAPTURE_STILL = 4;
	private static final int MSG_CAPTURE_START = 5;
	private static final int MSG_CAPTURE_STOP = 6;
	private static final int MSG_MEDIA_UPDATE = 7;
	private static final int MSG_RELEASE = 9;
	private static final int MSG_PUSH_START = 10;
	private static final int MSG_PUSH_STOP = 11;

	@Override
	public void handleMessage(final Message msg) {
		final CameraThread thread = mWeakThread.get();
		if (thread == null) return;
		switch (msg.what) {
			case MSG_OPEN:
				thread.handleOpen();
				break;
			case MSG_CLOSE:
				thread.handleClose();
				break;
			case MSG_PREVIEW_START:
				thread.handleStartPreview(msg.arg1, msg.arg2, (Surface) msg.obj);
				break;
			case MSG_PREVIEW_STOP:
				thread.handleStopPreview();
				break;
			case MSG_CAPTURE_STILL:
				thread.handleCaptureStill((String) msg.obj);
				break;
			case MSG_CAPTURE_START:
				thread.handleStartRecording();
				break;
			case MSG_CAPTURE_STOP:
				//停止推流
				thread.handleStopPush();
				thread.handleStopRecording();
				break;
			case MSG_MEDIA_UPDATE:
				thread.handleUpdateMedia((String) msg.obj);
				break;
			case MSG_RELEASE:
				thread.handleRelease();
				break;
			case MSG_PUSH_START:
				thread.handleStartPush();
				break;
			case MSG_PUSH_STOP:
				thread.handleStopPush();
				break;
			default:
				throw new RuntimeException("unsupported message:what=" + msg.what);
		}
	}

	private final RenderHolderCallback mRenderHolderCallback
		= new RenderHolderCallback() {
		@Override
		public void onCreate(final Surface surface) {
		}

		@Override
		public void onFrameAvailable() {
			final CameraThread thread = mWeakThread.get();
			if ((thread != null) && (thread.mVideoEncoder != null)) {
				try {
					thread.mVideoEncoder.frameAvailableSoon();
				} catch (final Exception e) {
					//
				}
			}
		}

		@Override
		public void onDestroy() {
		}
	};

	private static final class CameraThread extends Thread {
		private static final String TAG_THREAD = "CameraThread";
		private final Object mSync = new Object();
		private boolean mIsRecording;
	    private final WeakReference<Context> mWeakContext;
		private int mEncoderSurfaceId;
		private int mFrameWidth, mFrameHeight;

		/**
		 * shutter sound
		 */
		private SoundPool mSoundPool;
		private int mSoundId;
		private CameraServer mHandler;
		private UsbControlBlock mCtrlBlock;
		/**
		 * for accessing UVC camera
		 */
		private volatile UVCCamera mUVCCamera;
		/**
		 * muxer for audio/video recording
		 */
		private MediaMuxerWrapper mMuxer;
		private MediaSurfaceEncoder mVideoEncoder;

		private CameraThread(final Context context, final UsbControlBlock ctrlBlock) {
			super("CameraThread");
			if (DEBUG) KLog.w(TAG_THREAD, "Constructor:");
			mWeakContext = new WeakReference<Context>(context);
			mCtrlBlock = ctrlBlock;
			loadShutterSound(context);
		}

		@Override
		protected void finalize() throws Throwable {
			KLog.w(TAG_THREAD, "CameraThread#finalize");
			super.finalize();
		}

		public CameraServer getHandler() {
			if (DEBUG) KLog.w(TAG_THREAD, "getHandler:");
			synchronized (mSync) {
				if (mHandler == null)
				try {
					mSync.wait();
				} catch (final InterruptedException e) {
				}
			}
			return mHandler;
		}

		public boolean isCameraOpened() {
			return mUVCCamera != null;
		}

		public boolean isRecording() {
			return (mUVCCamera != null) && (mMuxer != null);
		}

		public void handleOpen() {
			if (DEBUG) KLog.w(TAG_THREAD, "handleOpen:");
			handleClose();
			synchronized (mSync) {
				mUVCCamera = new UVCCamera();
				mUVCCamera.open(mCtrlBlock);
				if (DEBUG) KLog.w(TAG, "supportedSize:" + mUVCCamera.getSupportedSize());
			}
			mHandler.processOnCameraStart();
		}

		public void handleClose() {
			if (DEBUG) KLog.w(TAG_THREAD, "handleClose:");
			handleStopRecording();
			boolean closed = false;
			synchronized (mSync) {
				if (mUVCCamera != null) {
					mUVCCamera.stopPreview();
					mUVCCamera.destroy();
					mUVCCamera = null;
					closed = true;
				}
				mSync.notifyAll();
			}
			if (closed)
				mHandler.processOnCameraStop();
			if (DEBUG) KLog.w(TAG_THREAD, "handleClose:finished");
		}

		public void handleStartPreview(final int width, final int height, final Surface surface) {
			if (DEBUG) KLog.w(TAG_THREAD, "handleStartPreview:");
			synchronized (mSync) {
				if (mUVCCamera == null) return;
				try {
					mUVCCamera.setPreviewSize(width, height, UVCCamera.FRAME_FORMAT_MJPEG);
				} catch (final IllegalArgumentException e) {
					try {
						// fallback to YUV mode
						mUVCCamera.setPreviewSize(width, height, UVCCamera.DEFAULT_PREVIEW_MODE);
					} catch (final IllegalArgumentException e1) {
						mUVCCamera.destroy();
						mUVCCamera = null;
					}
				}
				if (mUVCCamera == null) return;

//				mUVCCamera.setFrameCallback(mIFrameCallback, UVCCamera.PIXEL_FORMAT_YUV);
				mFrameWidth = width;
				mFrameHeight = height;
				mUVCCamera.setPreviewDisplay(surface);
				mUVCCamera.startPreview();
			}
		}

		public void handleStopPreview() {
			if (DEBUG) KLog.w(TAG_THREAD, "handleStopPreview:");
			synchronized (mSync) {
				if (mUVCCamera != null) {
					mUVCCamera.stopPreview();
				}
			}
		}

		private void handleResize(final int width, final int height, final Surface surface) {
			synchronized (mSync) {
				if (mUVCCamera != null) {
					final Size sz = mUVCCamera.getPreviewSize();
					if ((sz != null) && ((width != sz.width) || (height != sz.height))) {
						mUVCCamera.stopPreview();
						try {
							mUVCCamera.setPreviewSize(width, height);
						} catch (final IllegalArgumentException e) {
							try {
								mUVCCamera.setPreviewSize(sz.width, sz.height);
							} catch (final IllegalArgumentException e1) {
								// unexpectedly #setPreviewSize failed
								mUVCCamera.destroy();
								mUVCCamera = null;
							}
						}
						if (mUVCCamera == null) return;
						mFrameWidth = width;
						mFrameHeight = height;
						mUVCCamera.setPreviewDisplay(surface);
						mUVCCamera.startPreview();
					}
				}
			}
		}
		
		public void handleCaptureStill(final String path) {
			if (DEBUG) KLog.w(TAG_THREAD, "handleCaptureStill:");

			mSoundPool.play(mSoundId, 0.2f, 0.2f, 0, 0, 1.0f);	// play shutter sound
		}

		public void handleStartRecording() {
			if (DEBUG) KLog.w(TAG_THREAD, "handleStartRecording:");
			try {
				if ((mUVCCamera == null) || (mMuxer != null)) return;
				mMuxer = new MediaMuxerWrapper(mWeakContext.get(), ".mp4");    // if you record audio only, ".m4a" is also OK.

				new MediaSurfaceEncoder(mMuxer, mFrameWidth, mFrameHeight, mMediaEncoderListener);
				if (true) {
					//TODO for audio capturing
//					new MediaAudioEncoder(mMuxer, mMediaEncoderListener);
				}
				mMuxer.prepare();
				mMuxer.startRecording();
			} catch (final IOException e) {
				KLog.e(TAG, "startCapture:", e);
			}
		}

		private void handleStartPush() {
			KLog.w(TAG_THREAD, "handleStartPush:mMuxer=" + mMuxer);

//			initEasyPusher();
		}

		private void handleStopPush() {
			KLog.w(TAG_THREAD, "handleStopPush:mMuxer=" + mMuxer);
			if (mMuxer != null) {
				synchronized (mSync) {
					/*if (isPushing) {
						isPushing = false;
					}*/
				}
			}
		}

		public void handleStopRecording() {
			if (DEBUG) KLog.w(TAG_THREAD, "handleStopRecording:mMuxer=" + mMuxer);
			if (mMuxer != null) {
				synchronized (mSync) {
					if (mUVCCamera != null) {
						mUVCCamera.stopCapture();
					}
				}
				mMuxer.stopRecording();
				mMuxer = null;
				// you should not wait here
			}
		}

		public void handleUpdateMedia(final String path) {
			if (DEBUG) KLog.w(TAG_THREAD, "handleUpdateMedia:path=" + path);
			final Context context = mWeakContext.get();
			if (context != null) {
				try {
					if (DEBUG) KLog.w(TAG, "MediaScannerConnection#scanFile");
					MediaScannerConnection.scanFile(context, new String[]{ path }, null, null);
				} catch (final Exception e) {
					KLog.e(TAG, "handleUpdateMedia:", e);
				}
			} else {
				KLog.w(TAG, "MainActivity already destroyed");
				// give up to add this movice to MediaStore now.
				// Seeing this movie on Gallery app etc. will take a lot of time.
				handleRelease();
			}
		}

		public void handleRelease() {
			if (DEBUG) KLog.w(TAG_THREAD, "handleRelease:");
			handleClose();
			if (mCtrlBlock != null) {
				mCtrlBlock.close();
				mCtrlBlock = null;
			}
			if (!mIsRecording)
				Looper.myLooper().quit();
		}

		// if you need frame data as ByteBuffer on Java side, you can use this callback method with UVCCamera#setFrameCallback
		private final IFrameCallback mIFrameCallback = new IFrameCallback() {
			@Override
			public void onFrame(final ByteBuffer frame) {
				/*final byte[] buf = new byte[frame.remaining()];
				frame.get(buf);
				KLog.w(TAG, "onFrame: " + Arrays.toString(buf));*/
			}
		};

		private final IUVCServiceOnFrameAvailable mOnFrameAvailable = new IUVCServiceOnFrameAvailable() {
			@Override
			public IBinder asBinder() {
				if (DEBUG) KLog.w(TAG_THREAD, "asBinder:");
				return null;
			}
			@Override
			public void onFrameAvailable() throws RemoteException {
//				if (DEBUG) KLog.w(TAG_THREAD, "onFrameAvailable:");
				if (mVideoEncoder != null)
					mVideoEncoder.frameAvailableSoon();
			}
		};

		private final MediaEncoder.MediaEncoderListener mMediaEncoderListener = new MediaEncoder.MediaEncoderListener() {
			@Override
			public void onPrepared(final MediaEncoder encoder) {
				if (DEBUG) KLog.w(TAG, "onPrepared:encoder=" + encoder);
				mIsRecording = true;
				if (encoder instanceof MediaSurfaceEncoder)
				try {
					mVideoEncoder = (MediaSurfaceEncoder)encoder;
					final Surface encoderSurface = mVideoEncoder.getInputSurface();
					mEncoderSurfaceId = encoderSurface.hashCode();
					mHandler.mRendererHolder.addSurface(mEncoderSurfaceId, encoderSurface, true);
				} catch (final Exception e) {
					KLog.e(TAG, "onPrepared:", e);
				}
			}

			@Override
			public void onStopped(final MediaEncoder encoder) {
				if (DEBUG) KLog.w(TAG_THREAD, "onStopped:encoder=" + encoder);
				if ((encoder instanceof MediaSurfaceEncoder))
				try {
					mIsRecording = false;
					if (mEncoderSurfaceId > 0) {
						try {
							mHandler.mRendererHolder.removeSurface(mEncoderSurfaceId);
						} catch (final Exception e) {
							KLog.w(TAG, e);
						}
					}
					mEncoderSurfaceId = -1;
					synchronized (mSync) {
						if (mUVCCamera != null) {
							mUVCCamera.stopCapture();
						}
					}
					mVideoEncoder = null;
					final String path = encoder.getOutputPath();
					if (!TextUtils.isEmpty(path)) {
						mHandler.sendMessageDelayed(mHandler.obtainMessage(MSG_MEDIA_UPDATE, path), 1000);
					}
				} catch (final Exception e) {
					KLog.e(TAG, "onPrepared:", e);
				}
			}

		};

		/**
		 * prepare and load shutter sound for still image capturing
		 */
		@SuppressWarnings("deprecation")
		private void loadShutterSound(final Context context) {
			if (DEBUG) KLog.w(TAG_THREAD, "loadShutterSound:");
	    	// get system stream type using refrection
	        int streamType;
	        try {
	            final Class<?> audioSystemClass = Class.forName("android.media.AudioSystem");
	            final Field sseField = audioSystemClass.getDeclaredField("STREAM_SYSTEM_ENFORCED");
	            streamType = sseField.getInt(null);
	        } catch (final Exception e) {
	        	streamType = AudioManager.STREAM_SYSTEM;	// set appropriate according to your app policy
	        }
	        if (mSoundPool != null) {
	        	try {
	        		mSoundPool.release();
	        	} catch (final Exception e) {
	        	}
	        	mSoundPool = null;
	        }
	        // load sutter sound from resource
		    mSoundPool = new SoundPool(2, streamType, 0);
		    mSoundId = mSoundPool.load(context, R.raw.camera_click, 1);
		}

		@Override
		public void run() {
			if (DEBUG) KLog.w(TAG_THREAD, "run:");
			Looper.prepare();
			synchronized (mSync) {
				mHandler = new CameraServer(this);
				mSync.notifyAll();
			}
			Looper.loop();
			synchronized (mSync) {
				mHandler = null;
				mSoundPool.release();
				mSoundPool = null;
				mSync.notifyAll();
			}
			if (DEBUG) KLog.w(TAG_THREAD, "run:finished");
		}
	}

}
