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

package com.serenegiant.usbcameratest5;

import android.graphics.SurfaceTexture;
import android.hardware.usb.UsbDevice;
import android.os.Bundle;
import android.util.Log;
import android.view.Surface;
import android.view.View;
import android.view.View.OnClickListener;
import android.view.View.OnLongClickListener;
import android.widget.CompoundButton;
import android.widget.ImageButton;
import android.widget.Toast;
import android.widget.ToggleButton;

import com.serenegiant.common.BaseActivity;

import com.serenegiant.usb.CameraDialog;
import com.serenegiant.usb.USBMonitor;
import com.serenegiant.usb.USBMonitor.OnDeviceConnectListener;
import com.serenegiant.usb.USBMonitor.UsbControlBlock;
import com.serenegiant.usb.UVCCamera;
import com.serenegiant.usbcameracommon.UVCCameraHandler;
import com.serenegiant.widget.CameraViewInterface;

public final class MainActivity extends BaseActivity implements CameraDialog.CameraDialogParent {
	private static final boolean DEBUG = true;	// TODO set false on release
	private static final String TAG = "MainActivity";

    /**
     * preview resolution(width)
     * if your camera does not support specific resolution and mode,
     * {@link UVCCamera#setPreviewSize(int, int, int)} throw exception
     */
    private static final int PREVIEW_WIDTH = 640;
    /**
     * preview resolution(height)
     * if your camera does not support specific resolution and mode,
     * {@link UVCCamera#setPreviewSize(int, int, int)} throw exception
     */
    private static final int PREVIEW_HEIGHT = 480;
    /**
     * preview mode
     * if your camera does not support specific resolution and mode,
     * {@link UVCCamera#setPreviewSize(int, int, int)} throw exception
     * 0:YUYV, other:MJPEG
     */
    private static final int PREVIEW_MODE = UVCCamera.FRAME_FORMAT_MJPEG;

	/**
	 * for accessing USB
	 */
	private USBMonitor mUSBMonitor;
	/**
	 * Handler to execute camera releated methods sequentially on private thread
	 */
	private UVCCameraHandler mCameraHandler;
	/**
	 * for camera preview display
	 */
	private CameraViewInterface mUVCCameraView;
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
		mCameraButton.setOnCheckedChangeListener(mOnCheckedChangeListener);
		mCaptureButton = (ImageButton)findViewById(R.id.capture_button);
		mCaptureButton.setOnClickListener(mOnClickListener);
		mCaptureButton.setVisibility(View.INVISIBLE);
		final View view = findViewById(R.id.camera_view);
		view.setOnLongClickListener(mOnLongClickListener);
		mUVCCameraView = (CameraViewInterface)view;
		mUVCCameraView.setAspectRatio(PREVIEW_WIDTH / (float)PREVIEW_HEIGHT);

		mUSBMonitor = new USBMonitor(this, mOnDeviceConnectListener);
		mCameraHandler = UVCCameraHandler.createHandler(this, mUVCCameraView,
			2, PREVIEW_WIDTH, PREVIEW_HEIGHT, PREVIEW_MODE);
	}

	@Override
	protected void onStart() {
		super.onStart();
		if (DEBUG) Log.v(TAG, "onStart:");
		mUSBMonitor.register();
		if (mUVCCameraView != null)
			mUVCCameraView.onResume();
	}

	@Override
	protected void onStop() {
		if (DEBUG) Log.v(TAG, "onStop:");
		queueEvent(new Runnable() {
			@Override
			public void run() {
				mCameraHandler.close();
			}
		}, 0);
		if (mUVCCameraView != null)
			mUVCCameraView.onPause();
		setCameraButton(false);
		mCaptureButton.setVisibility(View.INVISIBLE);
		mUSBMonitor.unregister();
		super.onStop();
	}

	@Override
	public void onDestroy() {
		if (DEBUG) Log.v(TAG, "onDestroy:");
		if (mCameraHandler != null) {
			mCameraHandler.release();
			mCameraHandler = null;
		}
		if (mUSBMonitor != null) {
			mUSBMonitor.destroy();
			mUSBMonitor = null;
		}
		mUVCCameraView = null;
		mCameraButton = null;
		mCaptureButton = null;
		super.onDestroy();
	}

	protected void checkPermissionResult(final int requestCode, final String permission, final boolean result) {
		super.checkPermissionResult(requestCode, permission, result);
		if (!result && (permission != null)) {
			setCameraButton(false);
		}
	}

	/**
	 * event handler when click camera / capture button
	 */
	private final OnClickListener mOnClickListener = new OnClickListener() {
		@Override
		public void onClick(final View view) {
			switch (view.getId()) {
			case R.id.capture_button:
				if (mCameraHandler.isOpened()) {
					if (checkPermissionWriteExternalStorage() && checkPermissionAudio()) {
						if (!mCameraHandler.isRecording()) {
							mCaptureButton.setColorFilter(0xffff0000);	// turn red
							mCameraHandler.startRecording();
						} else {
							mCaptureButton.setColorFilter(0);	// return to default color
							mCameraHandler.stopRecording();
						}
					}
				}
				break;
			}
		}
	};

	private final CompoundButton.OnCheckedChangeListener mOnCheckedChangeListener
		= new CompoundButton.OnCheckedChangeListener() {
		@Override
		public void onCheckedChanged(final CompoundButton compoundButton, final boolean isChecked) {
			switch (compoundButton.getId()) {
			case R.id.camera_button:
				if (isChecked && !mCameraHandler.isOpened()) {
					CameraDialog.showDialog(MainActivity.this);
				} else {
					mCameraHandler.close();
					mCaptureButton.setVisibility(View.INVISIBLE);
					setCameraButton(false);
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
			case R.id.camera_view:
				if (mCameraHandler.isOpened()) {
					if (checkPermissionWriteExternalStorage()) {
						mCameraHandler.captureStill();
					}
					return true;
				}
			}
			return false;
		}
	};

	private void setCameraButton(final boolean isOn) {
		runOnUiThread(new Runnable() {
			@Override
			public void run() {
				if (mCameraButton != null) {
					try {
						mCameraButton.setOnCheckedChangeListener(null);
						mCameraButton.setChecked(isOn);
					} finally {
						mCameraButton.setOnCheckedChangeListener(mOnCheckedChangeListener);
					}
				}
				if (!isOn && (mCaptureButton != null)) {
					mCaptureButton.setVisibility(View.INVISIBLE);
				}
			}
		}, 0);
	}

	private Surface mSurface;
	private void startPreview() {
		final SurfaceTexture st = mUVCCameraView.getSurfaceTexture();
		if (mSurface != null) {
			mSurface.release();
		}
		mSurface = new Surface(st);
		mCameraHandler.startPreview(mSurface);
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
			mCameraHandler.open(ctrlBlock);
			startPreview();
		}

		@Override
		public void onDisconnect(final UsbDevice device, final UsbControlBlock ctrlBlock) {
			if (DEBUG) Log.v(TAG, "onDisconnect:");
			if (mCameraHandler != null) {
				mCameraHandler.close();
				setCameraButton(false);
			}
		}
		@Override
		public void onDettach(final UsbDevice device) {
			Toast.makeText(MainActivity.this, "USB_DEVICE_DETACHED", Toast.LENGTH_SHORT).show();
		}

		@Override
		public void onCancel(final UsbDevice device) {
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

	@Override
	public void onDialogResult(boolean canceled) {
		if (canceled) {
			setCameraButton(false);
		}
	}

//	/**
//	 * Handler class to execute camera releated methods sequentially on private thread
//	 */
//	private static final class AbstractUVCCameraHandler extends Handler {
//		private static final int MSG_OPEN = 0;
//		private static final int MSG_CLOSE = 1;
//		private static final int MSG_PREVIEW_START = 2;
//		private static final int MSG_PREVIEW_STOP = 3;
//		private static final int MSG_CAPTURE_STILL = 4;
//		private static final int MSG_CAPTURE_START = 5;
//		private static final int MSG_CAPTURE_STOP = 6;
//		private static final int MSG_MEDIA_UPDATE = 7;
//		private static final int MSG_RELEASE = 9;
//
//		private final WeakReference<CameraThread> mWeakThread;
//
//		public static final AbstractUVCCameraHandler createHandler(final MainActivity parent, final CameraViewInterface cameraView) {
//			final CameraThread thread = new CameraThread(parent, cameraView);
//			thread.start();
//			return thread.getHandler();
//		}
//
//		private AbstractUVCCameraHandler(final CameraThread thread) {
//			mWeakThread = new WeakReference<CameraThread>(thread);
//		}
//
//		public boolean isOpened() {
//			final CameraThread thread = mWeakThread.get();
//			return thread != null && thread.isOpened();
//		}
//
//		public boolean isRecording() {
//			final CameraThread thread = mWeakThread.get();
//			return thread != null && thread.isRecording();
//		}
//
//		public void open(final UsbControlBlock ctrlBlock) {
//			sendMessage(obtainMessage(MSG_OPEN, ctrlBlock));
//		}
//
//		public void close() {
//			stopPreview();
//			sendEmptyMessage(MSG_CLOSE);
//		}
//
//		public void startPreview(final Surface sureface) {
//			if (sureface != null)
//				sendMessage(obtainMessage(MSG_PREVIEW_START, sureface));
//		}
//
//		public void stopPreview() {
//			stopRecording();
//			final CameraThread thread = mWeakThread.get();
//			if (thread == null) return;
//			synchronized (thread.mSync) {
//				sendEmptyMessage(MSG_PREVIEW_STOP);
//				// wait for actually preview stopped to avoid releasing Surface/SurfaceTexture
//				// while preview is still running.
//				// therefore this method will take a time to execute
//				try {
//					thread.mSync.wait();
//				} catch (final InterruptedException e) {
//				}
//			}
//		}
//
//		public void captureStill() {
//			sendEmptyMessage(MSG_CAPTURE_STILL);
//		}
//
//		public void startRecording() {
//			sendEmptyMessage(MSG_CAPTURE_START);
//		}
//
//		public void stopRecording() {
//			sendEmptyMessage(MSG_CAPTURE_STOP);
//		}
//
//		@Override
//		public void handleMessage(final Message msg) {
//			final CameraThread thread = mWeakThread.get();
//			if (thread == null) return;
//			switch (msg.what) {
//			case MSG_OPEN:
//				thread.handleOpen((UsbControlBlock)msg.obj);
//				break;
//			case MSG_CLOSE:
//				thread.handleClose();
//				break;
//			case MSG_PREVIEW_START:
//				thread.handleStartPreview((Surface)msg.obj);
//				break;
//			case MSG_PREVIEW_STOP:
//				thread.handleStopPreview();
//				break;
//			case MSG_CAPTURE_STILL:
//				thread.handleCaptureStill();
//				break;
//			case MSG_CAPTURE_START:
//				thread.handleStartRecording();
//				break;
//			case MSG_CAPTURE_STOP:
//				thread.handleStopRecording();
//				break;
//			case MSG_MEDIA_UPDATE:
//				thread.handleUpdateMedia((String)msg.obj);
//				break;
//			case MSG_RELEASE:
//				thread.handleRelease();
//				break;
//			default:
//				throw new RuntimeException("unsupported message:what=" + msg.what);
//			}
//		}
//
//
//		private static final class CameraThread extends Thread {
//			private static final String TAG_THREAD = "CameraThread";
//			private final Object mSync = new Object();
//			private final WeakReference<MainActivity> mWeakParent;
//			private final WeakReference<CameraViewInterface> mWeakCameraView;
//			private boolean mIsRecording;
//			/**
//			 * shutter sound
//			 */
//			private SoundPool mSoundPool;
//			private int mSoundId;
//			private AbstractUVCCameraHandler mHandler;
//			/**
//			 * for accessing UVC camera
//			 */
//			private UVCCamera mUVCCamera;
//			/**
//			 * muxer for audio/video recording
//			 */
//			private MediaMuxerWrapper mMuxer;
//			/**
//			 * for video recording
//			 */
//			private MediaVideoBufferEncoder mVideoEncoder;
//
//			private CameraThread(final MainActivity parent, final CameraViewInterface cameraView) {
//				super("CameraThread");
//				mWeakParent = new WeakReference<MainActivity>(parent);
//				mWeakCameraView = new WeakReference<CameraViewInterface>(cameraView);
//				loadShutterSound(parent);
//			}
//
//			@Override
//			protected void finalize() throws Throwable {
//				Log.i(TAG, "CameraThread#finalize");
//				super.finalize();
//			}
//
//			public AbstractUVCCameraHandler getHandler() {
//				if (DEBUG) Log.v(TAG_THREAD, "getHandler:");
//				synchronized (mSync) {
//					if (mHandler == null)
//					try {
//						mSync.wait();
//					} catch (final InterruptedException e) {
//					}
//				}
//				return mHandler;
//			}
//
//			public boolean isOpened() {
//				return mUVCCamera != null;
//			}
//
//			public boolean isRecording() {
//				return (mUVCCamera != null) && (mMuxer != null);
//			}
//
//			public void handleOpen(final UsbControlBlock ctrlBlock) {
//				if (DEBUG) Log.v(TAG_THREAD, "handleOpen:");
//				handleClose();
//				mUVCCamera = new UVCCamera();
//				mUVCCamera.open(ctrlBlock);
//				if (DEBUG) Log.i(TAG, "supportedSize:" + mUVCCamera.getSupportedSize());
//			}
//
//			public void handleClose() {
//				if (DEBUG) Log.v(TAG_THREAD, "handleClose:");
//				handleStopRecording();
//				if (mUVCCamera != null) {
//					mUVCCamera.stopPreview();
//					mUVCCamera.destroy();
//					mUVCCamera = null;
//				}
//			}
//
//			public void handleStartPreview(final Surface surface) {
//				if (DEBUG) Log.v(TAG_THREAD, "handleStartPreview:");
//				if (mUVCCamera == null) return;
//				try {
//					mUVCCamera.setPreviewSize(PREVIEW_WIDTH, PREVIEW_HEIGHT, PREVIEW_MODE);
//				} catch (final IllegalArgumentException e) {
//					try {
//						// fallback to YUV mode
//						mUVCCamera.setPreviewSize(PREVIEW_WIDTH, PREVIEW_HEIGHT, UVCCamera.DEFAULT_PREVIEW_MODE);
//					} catch (final IllegalArgumentException e1) {
//						handleClose();
//					}
//				}
//				if (mUVCCamera != null) {
//					mUVCCamera.setPreviewDisplay(surface);
//					mUVCCamera.startPreview();
//				}
//			}
//
//			public void handleStopPreview() {
//				if (DEBUG) Log.v(TAG_THREAD, "handleStopPreview:");
//				if (mUVCCamera != null) {
//					mUVCCamera.stopPreview();
//				}
//				synchronized (mSync) {
//					mSync.notifyAll();
//				}
//			}
//
//			public void handleCaptureStill() {
//				if (DEBUG) Log.v(TAG_THREAD, "handleCaptureStill:");
//				final MainActivity parent = mWeakParent.get();
//				if (parent == null) return;
//				mSoundPool.play(mSoundId, 0.2f, 0.2f, 0, 0, 1.0f);	// play shutter sound
//				final Bitmap bitmap = mWeakCameraView.get().captureStillImage();
//				try {
//					// get buffered output stream for saving a captured still image as a file on external storage.
//					// the file name is came from current time.
//					// You should use extension name as same as CompressFormat when calling Bitmap#compress.
//					final File outputFile = MediaMuxerWrapper.getCaptureFile(Environment.DIRECTORY_DCIM, ".png");
//					final BufferedOutputStream os = new BufferedOutputStream(new FileOutputStream(outputFile));
//					try {
//						try {
//							bitmap.compress(CompressFormat.PNG, 100, os);
//							os.flush();
//							mHandler.sendMessage(mHandler.obtainMessage(MSG_MEDIA_UPDATE, outputFile.getPath()));
//						} catch (final IOException e) {
//						}
//					} finally {
//						os.close();
//					}
//				} catch (final FileNotFoundException e) {
//				} catch (final IOException e) {
//				}
//			}
//
//			public void handleStartRecording() {
//				if (DEBUG) Log.v(TAG_THREAD, "handleStartRecording:");
//				try {
//					if ((mUVCCamera == null) || (mMuxer != null)) return;
//					mMuxer = new MediaMuxerWrapper(".mp4");	// if you record audio only, ".m4a" is also OK.
//					// for video capturing using MediaVideoEncoder
//					mVideoEncoder = new MediaVideoBufferEncoder(mMuxer, mMediaEncoderListener);
//					if (true) {
//						// for audio capturing
//						new MediaAudioEncoder(mMuxer, mMediaEncoderListener);
//					}
//					mMuxer.prepare();
//					mMuxer.startRecording();
//					mUVCCamera.setFrameCallback(mIFrameCallback, UVCCamera.PIXEL_FORMAT_NV21);
//				} catch (final IOException e) {
//					Log.e(TAG, "startCapture:", e);
//				}
//			}
//
//			public void handleStopRecording() {
//				if (DEBUG) Log.v(TAG_THREAD, "handleStopRecording:mMuxer=" + mMuxer);
//				mVideoEncoder = null;
//				if (mMuxer != null) {
//					mMuxer.stopRecording();
//					mMuxer = null;
//					// you should not wait here
//				}
//				if (mUVCCamera != null)
//					mUVCCamera.setFrameCallback(null, 0);
//			}
//
//			public void handleUpdateMedia(final String path) {
//				if (DEBUG) Log.v(TAG_THREAD, "handleUpdateMedia:path=" + path);
//				final MainActivity parent = mWeakParent.get();
//				if (parent != null && parent.getApplicationContext() != null) {
//					try {
//						if (DEBUG) Log.i(TAG, "MediaScannerConnection#scanFile");
//						MediaScannerConnection.scanFile(parent.getApplicationContext(), new String[]{ path }, null, null);
//					} catch (final Exception e) {
//						Log.e(TAG, "handleUpdateMedia:", e);
//					}
//					if (parent.isDestroyed())
//						handleRelease();
//				} else {
//					Log.w(TAG, "MainActivity already destroyed");
//					// give up to add this movice to MediaStore now.
//					// Seeing this movie on Gallery app etc. will take a lot of time.
//					handleRelease();
//				}
//			}
//
//			public void handleRelease() {
//				if (DEBUG) Log.v(TAG_THREAD, "handleRelease:");
// 				handleClose();
//				if (!mIsRecording)
//					Looper.myLooper().quit();
//			}
//
//			private final IFrameCallback mIFrameCallback = new IFrameCallback() {
//				@Override
//				public void onFrame(final ByteBuffer frame) {
//					if (mVideoEncoder != null) {
//						mVideoEncoder.frameAvailableSoon();
//						mVideoEncoder.encode(frame);
//					}
//				}
//			};
//
//			private final MediaEncoder.MediaEncoderListener mMediaEncoderListener = new MediaEncoder.MediaEncoderListener() {
//				@Override
//				public void onPrepared(final MediaEncoder encoder) {
//					if (DEBUG) Log.v(TAG, "onPrepared:encoder=" + encoder);
//					mIsRecording = true;
//				}
//
//				@Override
//				public void onStopped(final MediaEncoder encoder) {
//					if (DEBUG) Log.v(TAG_THREAD, "onStopped:encoder=" + encoder);
//					if (encoder instanceof MediaVideoEncoder)
//					try {
//						mIsRecording = false;
//						final MainActivity parent = mWeakParent.get();
//						final String path = encoder.getOutputPath();
//						if (!TextUtils.isEmpty(path)) {
//							mHandler.sendMessageDelayed(mHandler.obtainMessage(MSG_MEDIA_UPDATE, path), 1000);
//						} else {
//							if (parent == null || parent.isDestroyed()) {
//								handleRelease();
//							}
//						}
//					} catch (final Exception e) {
//						Log.e(TAG, "onPrepared:", e);
//					}
//				}
//			};
//
//			/**
//			 * prepare and load shutter sound for still image capturing
//			 */
//			@SuppressWarnings("deprecation")
//			private void loadShutterSound(final Context context) {
//		    	// get system stream type using refrection
//		        int streamType;
//		        try {
//		            final Class<?> audioSystemClass = Class.forName("android.media.AudioSystem");
//		            final Field sseField = audioSystemClass.getDeclaredField("STREAM_SYSTEM_ENFORCED");
//		            streamType = sseField.getInt(null);
//		        } catch (final Exception e) {
//		        	streamType = AudioManager.STREAM_SYSTEM;	// set appropriate according to your app policy
//		        }
//		        if (mSoundPool != null) {
//		        	try {
//		        		mSoundPool.release();
//		        	} catch (final Exception e) {
//		        	}
//		        	mSoundPool = null;
//		        }
//		        // load sutter sound from resource
//			    mSoundPool = new SoundPool(2, streamType, 0);
//			    mSoundId = mSoundPool.load(context, R.raw.camera_click, 1);
//			}
//
//			@Override
//			public void run() {
//				Looper.prepare();
//				synchronized (mSync) {
//					mHandler = new AbstractUVCCameraHandler(this);
//					mSync.notifyAll();
//				}
//				Looper.loop();
//				synchronized (mSync) {
//					mHandler = null;
//					mSoundPool.release();
//					mSoundPool = null;
//					mSync.notifyAll();
//				}
//			}
//		}
//	}
}
