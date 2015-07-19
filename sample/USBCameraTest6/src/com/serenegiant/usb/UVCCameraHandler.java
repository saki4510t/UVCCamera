package com.serenegiant.usb;
/*
 * UVCCamera
 * library and sample to access to UVC web camera on non-rooted Android device
 *
 * Copyright (c) 2014-2015 saki t_saki@serenegiant.com
 *
 * File name: UVCCameraHandler.java
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

import java.io.IOException;
import java.lang.ref.WeakReference;
import java.lang.reflect.Field;

import android.content.Context;
import android.media.AudioManager;
import android.media.MediaScannerConnection;
import android.media.SoundPool;
import android.os.Handler;
import android.os.Looper;
import android.os.Message;
import android.text.TextUtils;
import android.util.Log;
import android.view.Surface;

import com.serenegiant.encoder.MediaAudioEncoder;
import com.serenegiant.encoder.MediaEncoder;
import com.serenegiant.encoder.MediaMuxerWrapper;
import com.serenegiant.encoder.MediaSurfaceEncoder;
import com.serenegiant.glutils.RendererHolder;
import com.serenegiant.glutils.RendererHolder.OnFrameAvailableCallback;
import com.serenegiant.usb.USBMonitor.UsbControlBlock;
import com.serenegiant.usbcameratest6.R;

public final class UVCCameraHandler extends Handler {
	private static final boolean DEBUG = true;
	private static final String TAG = "UVCCameraHandler";

	private RendererHolder mRendererHolder;
	private final WeakReference<CameraThread> mWeakThread;

	public static UVCCameraHandler createHandler(final Context context) {
		if (DEBUG) Log.d(TAG, "createServer:");
		final CameraThread thread = new CameraThread(context);
		thread.start();
		return thread.getHandler();
	}

	private UVCCameraHandler(final CameraThread thread) {
		if (DEBUG) Log.d(TAG, "Constructor:");
		mWeakThread = new WeakReference<CameraThread>(thread);
		mRendererHolder = new RendererHolder(null);
	}

	@Override
	protected void finalize() throws Throwable {
		if (DEBUG) Log.i(TAG, "finalize:");
		release();
		super.finalize();
	}

	public void release() {
		if (DEBUG) Log.d(TAG, "release:");
		close();
		if (mRendererHolder != null) {
			mRendererHolder.release();
			mRendererHolder = null;
		}
	}

//********************************************************************************
//********************************************************************************
	public void open(final UsbControlBlock ctrlBlock) {
		if (DEBUG) Log.d(TAG, "open:");
		final CameraThread thread = mWeakThread.get();
		if (!thread.isOpened()) {
			sendMessage(obtainMessage(MSG_OPEN, ctrlBlock));
		} else {
			if (DEBUG) Log.d(TAG, "already connected, just call callback");
		}
	}

	public void close() {
		if (DEBUG) Log.d(TAG, "close:");
		stopRecording();
		final CameraThread thread = mWeakThread.get();
		if ((thread == null) || !thread.isOpened()) return;
		synchronized (thread.mSync) {
			stopPreview();
			// wait for actually preview stopped to avoid releasing Surface/SurfaceTexture
			// while preview is still running.
			// therefore this method will take a time to execute
			try {
				thread.mSync.wait();
			} catch (final InterruptedException e) {
			}
		}
		sendEmptyMessage(MSG_CLOSE);
	}

	public boolean isOpened() {
		final CameraThread thread = mWeakThread.get();
		return (thread != null) && thread.isOpened();
	}

	public void startPreview() {
		sendMessage(obtainMessage(MSG_PREVIEW_START, mRendererHolder.getSurface()));
	}

	public void stopPreview() {
		sendEmptyMessage(MSG_PREVIEW_STOP);
	}

	public boolean isRecording() {
		final CameraThread thread = mWeakThread.get();
		return (thread != null) && thread.isRecording();
	}

	public void addSurface(final int id, final Surface surface, final boolean isRecordable, final OnFrameAvailableCallback onFrameAvailableListener) {
		if (DEBUG) Log.d(TAG, "addSurface:id=" + id +",surface=" + surface);
		mRendererHolder.addSurface(id, surface, isRecordable, onFrameAvailableListener);
	}

	public void removeSurface(final int id) {
		if (DEBUG) Log.d(TAG, "removeSurface:id=" + id);
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
		mRendererHolder.captureStill(path);
		sendMessage(obtainMessage(MSG_CAPTURE_STILL, path));
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

	@Override
	public void handleMessage(final Message msg) {
		final CameraThread thread = mWeakThread.get();
		if (thread == null) return;
		switch (msg.what) {
		case MSG_OPEN:
			thread.handleOpen((UsbControlBlock)msg.obj);
			break;
		case MSG_CLOSE:
			thread.handleClose();
			break;
		case MSG_PREVIEW_START:
			thread.handleStartPreview((Surface)msg.obj);
			break;
		case MSG_PREVIEW_STOP:
			thread.handleStopPreview();
			break;
		case MSG_CAPTURE_STILL:
			thread.handleCaptureStill((String)msg.obj);
			break;
		case MSG_CAPTURE_START:
			thread.handleStartRecording();
			break;
		case MSG_CAPTURE_STOP:
			thread.handleStopRecording();
			break;
		case MSG_MEDIA_UPDATE:
			thread.handleUpdateMedia((String)msg.obj);
			break;
		case MSG_RELEASE:
			thread.handleRelease();
			break;
		default:
			throw new RuntimeException("unsupported message:what=" + msg.what);
		}
	}

	private static final class CameraThread extends Thread {
		private static final String TAG_THREAD = "CameraThread";
		private final Object mSync = new Object();
		private boolean mIsRecording;
	    private final WeakReference<Context> mWeakContext;
		private int mEncoderSurfaceId;
		/**
		 * shutter sound
		 */
		private SoundPool mSoundPool;
		private int mSoundId;
		private UVCCameraHandler mHandler;
		private UsbControlBlock mCtrlBlock;
		/**
		 * for accessing UVC camera
		 */
		private UVCCamera mUVCCamera;
		/**
		 * muxer for audio/video recording
		 */
		private MediaMuxerWrapper mMuxer;
		private MediaSurfaceEncoder mVideoEncoder;

		private CameraThread(final Context context) {
			super("CameraThread");
			if (DEBUG) Log.d(TAG_THREAD, "Constructor:");
			mWeakContext = new WeakReference<Context>(context);
			loadSutterSound(context);
		}

		@Override
		protected void finalize() throws Throwable {
			Log.i(TAG_THREAD, "CameraThread#finalize");
			super.finalize();
		}

		public UVCCameraHandler getHandler() {
			if (DEBUG) Log.d(TAG_THREAD, "getHandler:");
			synchronized (mSync) {
				if (mHandler == null)
				try {
					mSync.wait();
				} catch (final InterruptedException e) {
				}
			}
			return mHandler;
		}

		public boolean isOpened() {
			synchronized (mSync) {
				return mUVCCamera != null;
			}
		}

		public boolean isRecording() {
			synchronized (mSync) {
				return (mUVCCamera != null) && (mMuxer != null);
			}
		}

		public void handleOpen(final UsbControlBlock ctrlBlock) {
			if (DEBUG) Log.d(TAG_THREAD, "handleOpen:");
			handleClose();
			mCtrlBlock = ctrlBlock;
			synchronized (mSync) {
				mUVCCamera = new UVCCamera();
				mUVCCamera.open(ctrlBlock);
				if (DEBUG) Log.i(TAG, "supportedSize:" + mUVCCamera.getSupportedSize());
			}
		}

		public void handleClose() {
			if (DEBUG) Log.d(TAG_THREAD, "handleClose:");
			handleStopRecording();
			synchronized (mSync) {
				if (mUVCCamera != null) {
					mUVCCamera.stopPreview();
					mUVCCamera.destroy();
					mUVCCamera = null;
				}
			}
			if (mCtrlBlock != null) {
				mCtrlBlock.close();
				mCtrlBlock = null;
			}
		}

		public void handleStartPreview(final Surface surface) {
			if (DEBUG) Log.d(TAG_THREAD, "handleStartPreview:");
			if (mUVCCamera == null) return;
			try {
				mUVCCamera.setPreviewSize(UVCCamera.DEFAULT_PREVIEW_WIDTH, UVCCamera.DEFAULT_PREVIEW_HEIGHT, UVCCamera.FRAME_FORMAT_MJPEG);
			} catch (final IllegalArgumentException e) {
				try {
					// fallback to YUV mode
					mUVCCamera.setPreviewSize(UVCCamera.DEFAULT_PREVIEW_WIDTH, UVCCamera.DEFAULT_PREVIEW_HEIGHT, UVCCamera.DEFAULT_PREVIEW_MODE);
				} catch (final IllegalArgumentException e1) {
					synchronized (mSync) {
						mUVCCamera.destroy();
						mUVCCamera = null;
					}
				}
			}
			if (mUVCCamera == null) return;
//			mUVCCamera.setFrameCallback(mIFrameCallback, UVCCamera.PIXEL_FORMAT_YUV);
			mUVCCamera.setPreviewDisplay(surface);
			mUVCCamera.startPreview();
		}

		public void handleStopPreview() {
			if (DEBUG) Log.d(TAG_THREAD, "handleStopPreview:");
			if (mUVCCamera != null) {
				mUVCCamera.stopPreview();
			}
			synchronized (mSync) {
				mSync.notifyAll();
			}
		}

		public void handleCaptureStill(final String path) {
			if (DEBUG) Log.d(TAG_THREAD, "handleCaptureStill:");

			mSoundPool.play(mSoundId, 0.2f, 0.2f, 0, 0, 1.0f);	// play shutter sound
		}

		public void handleStartRecording() {
			if (DEBUG) Log.d(TAG_THREAD, "handleStartRecording:");
			try {
				if ((mUVCCamera == null) || (mMuxer != null)) return;
				mMuxer = new MediaMuxerWrapper(".mp4");	// if you record audio only, ".m4a" is also OK.
				new MediaSurfaceEncoder(mMuxer, mMediaEncoderListener);
				if (true) {
					// for audio capturing
					new MediaAudioEncoder(mMuxer, mMediaEncoderListener);
				}
				mMuxer.prepare();
				mMuxer.startRecording();
			} catch (final IOException e) {
				Log.e(TAG, "startCapture:", e);
			}
		}

		public void handleStopRecording() {
			if (DEBUG) Log.d(TAG_THREAD, "handleStopRecording:mMuxer=" + mMuxer);
			if (mMuxer != null) {
				mMuxer.stopRecording();
				mMuxer = null;
				// you should not wait here
			}
		}

		public void handleUpdateMedia(final String path) {
			if (DEBUG) Log.d(TAG_THREAD, "handleUpdateMedia:path=" + path);
			final Context context = mWeakContext.get();
			if (context != null) {
				try {
					if (DEBUG) Log.i(TAG, "MediaScannerConnection#scanFile");
					MediaScannerConnection.scanFile(context, new String[]{ path }, null, null);
				} catch (final Exception e) {
					Log.e(TAG, "handleUpdateMedia:", e);
				}
			} else {
				Log.w(TAG, "MainActivity already destroyed");
				// give up to add this movice to MediaStore now.
				// Seeing this movie on Gallery app etc. will take a lot of time.
				handleRelease();
			}
		}

		public void handleRelease() {
			if (DEBUG) Log.d(TAG_THREAD, "handleRelease:");
			handleClose();
			if (!mIsRecording)
				Looper.myLooper().quit();
		}

/*		// if you need frame data as ByteBuffer on Java side, you can use this callback method with UVCCamera#setFrameCallback
		private final IFrameCallback mIFrameCallback = new IFrameCallback() {
			@Override
			public void onFrame(final ByteBuffer frame) {
			}
		}; */

		private final OnFrameAvailableCallback mOnFrameAvailable = new OnFrameAvailableCallback() {
			@Override
			public void onFrameAvailable() {
//				if (DEBUG) Log.d(TAG_THREAD, "onFrameAvailable:");
				if (mVideoEncoder != null)
					mVideoEncoder.frameAvailableSoon();
			}
		};

		private final MediaEncoder.MediaEncoderListener mMediaEncoderListener = new MediaEncoder.MediaEncoderListener() {
			@Override
			public void onPrepared(final MediaEncoder encoder) {
				if (DEBUG) Log.d(TAG, "onPrepared:encoder=" + encoder);
				mIsRecording = true;
				if (encoder instanceof MediaSurfaceEncoder)
				try {
					mVideoEncoder = (MediaSurfaceEncoder)encoder;
					final Surface encoderSurface = mVideoEncoder.getInputSurface();
					mEncoderSurfaceId = encoderSurface.hashCode();
					mHandler.mRendererHolder.addSurface(mEncoderSurfaceId, encoderSurface, true, mOnFrameAvailable);
				} catch (final Exception e) {
					Log.e(TAG, "onPrepared:", e);
				}
			}

			@Override
			public void onStopped(final MediaEncoder encoder) {
				if (DEBUG) Log.v(TAG_THREAD, "onStopped:encoder=" + encoder);
				if ((encoder instanceof MediaSurfaceEncoder))
				try {
					mIsRecording = false;
					if (mEncoderSurfaceId > 0)
						mHandler.mRendererHolder.removeSurface(mEncoderSurfaceId);
					mEncoderSurfaceId = -1;
					mUVCCamera.stopCapture();
					mVideoEncoder = null;
					final String path = encoder.getOutputPath();
					if (!TextUtils.isEmpty(path)) {
						mHandler.sendMessageDelayed(mHandler.obtainMessage(MSG_MEDIA_UPDATE, path), 1000);
					}
				} catch (final Exception e) {
					Log.e(TAG, "onPrepared:", e);
				}
			}
		};

		/**
		 * prepare and load shutter sound for still image capturing
		 */
		@SuppressWarnings("deprecation")
		private void loadSutterSound(final Context context) {
			if (DEBUG) Log.d(TAG_THREAD, "loadSutterSound:");
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
	        // load shutter sound from resource
		    mSoundPool = new SoundPool(2, streamType, 0);
		    mSoundId = mSoundPool.load(context, R.raw.camera_click, 1);
		}

		@Override
		public void run() {
			if (DEBUG) Log.d(TAG_THREAD, "run:");
			Looper.prepare();
			synchronized (mSync) {
				mHandler = new UVCCameraHandler(this);
				mSync.notifyAll();
			}
			Looper.loop();
			synchronized (mSync) {
				mHandler = null;
				mSoundPool.release();
				mSoundPool = null;
				mSync.notifyAll();
			}
			if (DEBUG) Log.d(TAG_THREAD, "run:finished");
		}
	}

}
