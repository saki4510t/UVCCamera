package com.serenegiant.usbcameratest3;
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

import java.io.BufferedOutputStream;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;
import java.lang.reflect.Field;
import java.util.concurrent.LinkedBlockingQueue;
import java.util.concurrent.ThreadPoolExecutor;
import java.util.concurrent.TimeUnit;

import com.serenegiant.encoder.MediaAudioEncoder;
import com.serenegiant.encoder.MediaEncoder;
import com.serenegiant.encoder.MediaMuxerWrapper;
import com.serenegiant.encoder.MediaVideoEncoder;
import com.serenegiant.usb.USBMonitor;
import com.serenegiant.usb.UVCCamera;
import com.serenegiant.usb.USBMonitor.OnDeviceConnectListener;
import com.serenegiant.usb.USBMonitor.UsbControlBlock;
import com.serenegiant.usbcameratest.R;
import com.serenegiant.widget.CameraViewInterface;

import android.app.Activity;
import android.graphics.Bitmap;
import android.graphics.Bitmap.CompressFormat;
import android.graphics.SurfaceTexture;
import android.hardware.usb.UsbDevice;
import android.media.AudioManager;
import android.media.SoundPool;
import android.os.Bundle;
import android.os.Environment;
import android.util.Log;
import android.view.Surface;
import android.view.View;
import android.view.View.OnClickListener;
import android.view.View.OnLongClickListener;
import android.widget.ImageButton;
import android.widget.Toast;
import android.widget.ToggleButton;

public class MainActivity extends Activity {
	private static final boolean DEBUG = true;	// TODO set false on release
	private static final String TAG = "MainActivity";

	// for thread pool
	private static final int CORE_POOL_SIZE = 1;		// initial/minimum threads
	private static final int MAX_POOL_SIZE = 4;			// maximum threads
	private static final int KEEP_ALIVE_TIME = 10;		// time periods while keep the idle thread
	protected static final ThreadPoolExecutor sEXECUTER
		= new ThreadPoolExecutor(CORE_POOL_SIZE, MAX_POOL_SIZE, KEEP_ALIVE_TIME,
			TimeUnit.SECONDS, new LinkedBlockingQueue<Runnable>());

	// for accessing USB and USB camera
	private USBMonitor mUSBMonitor;
	private UVCCamera mUVCCamera;
	/**
	 * for camera preview display
	 */
	private CameraViewInterface mUVCCameraView;
	private Surface mPreviewSurface;
	/**
	 * for open&start / stop&close camera preview
	 */
	private ToggleButton mCameraButton;
	/**
	 * button for start/stop recording
	 */
	private ImageButton mCaptureButton;
	/**
	 * muxer for audio/video recording
	 */
	private MediaMuxerWrapper mMuxer;
	/**
	 * shutter sound
	 */
	private SoundPool mSoundPool;
	private int mSoundId;

	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		if (DEBUG) Log.v(TAG, "onCreate:");
		setContentView(R.layout.activity_main);
		mCameraButton = (ToggleButton)findViewById(R.id.camera_button);
		mCameraButton.setOnClickListener(mOnClickListener);
		mCaptureButton = (ImageButton)findViewById(R.id.capture_button);
		mCaptureButton.setOnClickListener(mOnClickListener);
		mCaptureButton.setVisibility(View.INVISIBLE);
		final View view = findViewById(R.id.camera_view);
		view.setOnLongClickListener(mOnLongClickListener);
		mUVCCameraView = (CameraViewInterface)view;
		mUVCCameraView.setAspectRatio(640 / 480.f);

		mUSBMonitor = new USBMonitor(this, mOnDeviceConnectListener);
		loadSutterSound();
	}

	@Override
	public void onResume() {
		super.onResume();
		if (DEBUG) Log.v(TAG, "onResume:");
		mUSBMonitor.register();
		if (mUVCCameraView != null)
			mUVCCameraView.onResume();
		if (mUVCCamera != null)
			mUVCCamera.startPreview();
	}

	@Override
	public void onPause() {
		if (DEBUG) Log.v(TAG, "onPause:");
		if (mUVCCamera != null) {
			mUVCCamera.stopPreview();
		}
		if (mUVCCameraView != null)
			mUVCCameraView.onPause();
		mCameraButton.setChecked(false);
		mCaptureButton.setVisibility(View.INVISIBLE);
		mUSBMonitor.unregister();
		super.onPause();
	}

	@Override
	public void onDestroy() {
		if (DEBUG) Log.v(TAG, "onDestroy:");
        if (mSoundPool != null) {
        	mSoundPool.release();
        	mSoundPool = null;
        }
		if (mUVCCamera != null) {
			mUVCCamera.destroy();
		}
		super.onDestroy();
	}

	/**
	 * event handler when click camera / capture button
	 */
	private final OnClickListener mOnClickListener = new OnClickListener() {
		@Override
		public void onClick(View view) {
			switch (view.getId()) {
			case R.id.camera_button:
				if (mUVCCamera == null) {
					CameraDialog.showDialog(MainActivity.this);
				} else {
					mUVCCamera.destroy();
					mUVCCamera = null;
					mCaptureButton.setVisibility(View.INVISIBLE);
				}
				break;
			case R.id.capture_button:
				if (mMuxer == null) {
					startRecording();
				} else {
					stopRecording();
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
		public boolean onLongClick(View view) {
			switch (view.getId()) {
			case R.id.camera_view:
				if (mUVCCamera != null) {
					captureStillImage();
				}
				return true;
			}
			return false;
		}
	};

	private final OnDeviceConnectListener mOnDeviceConnectListener = new OnDeviceConnectListener() {
		@Override
		public void onAttach(UsbDevice device) {
			Toast.makeText(MainActivity.this, "USB_DEVICE_ATTACHED", Toast.LENGTH_SHORT).show();
		}

		@Override
		public void onConnect(UsbDevice device, final UsbControlBlock ctrlBlock, boolean createNew) {
			if (DEBUG) Log.v(TAG, "onConnect:");
			if (mUVCCamera != null)
				mUVCCamera.destroy();
			mUVCCamera = new UVCCamera();
			sEXECUTER.execute(new Runnable() {
				@Override
				public void run() {
					mUVCCamera.open(ctrlBlock);
					if (mPreviewSurface != null) {
						mPreviewSurface.release();
						mPreviewSurface = null;
					}
					final SurfaceTexture st = mUVCCameraView.getSurfaceTexture(); 
					if (st != null)
						mPreviewSurface = new Surface(st);
					else {
						Log.w(TAG, "SurfaceTexture is null");
					}
					mUVCCamera.setPreviewDisplay(mPreviewSurface);
					mUVCCamera.startPreview();
					runOnUiThread(new Runnable() {
						@Override
						public void run() {
							mCaptureButton.setVisibility(View.VISIBLE);
						}
					});
				}
			});
		}

		@Override
		public void onDisconnect(UsbDevice device, UsbControlBlock ctrlBlock) {
			if (DEBUG) Log.v(TAG, "onDisconnect:");
			// XXX you should check whether the comming device equal to camera device that currently using
			if (mUVCCamera != null) {
				mUVCCamera.close();
				if (mPreviewSurface != null) {
					mPreviewSurface.release();
					mPreviewSurface = null;
				}
			}
			runOnUiThread(new Runnable() {
				@Override
				public void run() {
					mCaptureButton.setVisibility(View.INVISIBLE);
				}
			});
		}
		@Override
		public void onDettach(UsbDevice device) {
			Toast.makeText(MainActivity.this, "USB_DEVICE_DETACHED", Toast.LENGTH_SHORT).show();
		}
	};

	/**
	 * to access from CameraDialog
	 * @return
	 */
	public USBMonitor getUSBMonitor() {
		return mUSBMonitor;
	}

	/**
	 * prepare and load shutter sound for still image capturing
	 */
	private void loadSutterSound() {
    	// get system strean type using refrection
        int streamType;
        try {
            final Class<?> audioSystemClass = Class.forName("android.media.AudioSystem");
            final Field sseField = audioSystemClass.getDeclaredField("STREAM_SYSTEM_ENFORCED");
            streamType = sseField.getInt(null);
        } catch (Exception e) {
        	streamType = AudioManager.STREAM_SYSTEM;	// set appropriate according to your app policy
        }
        if (mSoundPool != null) {
        	try {
        		mSoundPool.release();
        	} catch (Exception e) {
        	}
        	mSoundPool = null;
        }
        // load sutter sound from resource
	    mSoundPool = new SoundPool(2, streamType, 0);
	    mSoundId = mSoundPool.load(this, R.raw.camera_click, 1);
	}

	/**
	 * capture still image from current preview display
	 */
	private void captureStillImage() {
		// capturing still image is heavy work on UI thread therefor you should use worker thread
		sEXECUTER.execute(new Runnable() {
			@Override
			public void run() {
				if (DEBUG) Log.v(TAG, "captureStillImage:");
				mSoundPool.play(mSoundId, 0.2f, 0.2f, 0, 0, 1.0f);	// play shutter sound
				final Bitmap bitmap = mUVCCameraView.captureStillImage();
				try {
					// get buffered output stream for saving a captured still image as a file on external storage.
					// the file name is came from current time.
					// You should use extension name as same as CompressFormat when calling Bitmap#compress.
					final BufferedOutputStream os = new BufferedOutputStream(new FileOutputStream(
							MediaMuxerWrapper.getCaptureFile(Environment.DIRECTORY_DCIM, ".png")));
					try {
						try {
							bitmap.compress(CompressFormat.PNG, 100, os);
							os.flush();
						} catch (IOException e) {
						}
					} finally {
						os.close();
					}
				} catch (FileNotFoundException e) {
				} catch (IOException e) {
				}
			}
		});
	}

	/**
	 * start resorcing
	 * This is a sample project and call this on UI thread to avoid being complicated
	 * but basically this should be called on private thread because prepareing
	 * of encoder is heavy work
	 */
	private void startRecording() {
		if (DEBUG) Log.v(TAG, "startRecording:");
		try {
			mCaptureButton.setColorFilter(0xffff0000);	// turn red
			mMuxer = new MediaMuxerWrapper(".mp4");	// if you record audio only, ".m4a" is also OK.
			if (true) {
				// for video capturing
				new MediaVideoEncoder(mMuxer, mMediaEncoderListener);
			}
			if (true) {
				// for audio capturing
				new MediaAudioEncoder(mMuxer, mMediaEncoderListener);
			}
			mMuxer.prepare();
			mMuxer.startRecording();
		} catch (IOException e) {
			mCaptureButton.setColorFilter(0);
			Log.e(TAG, "startCapture:", e);
		}
	}

	/**
	 * request stop recording
	 */
	private void stopRecording() {
		if (DEBUG) Log.v(TAG, "stopRecording:mMuxer=" + mMuxer);
		mCaptureButton.setColorFilter(0);	// return to default color
		if (mMuxer != null) {
			mMuxer.stopRecording();
			mMuxer = null;
			// you should not wait here
		}
	}

	/**
	 * callback methods from encoder
	 */
	private final MediaEncoder.MediaEncoderListener mMediaEncoderListener = new MediaEncoder.MediaEncoderListener() {
		@Override
		public void onPrepared(MediaEncoder encoder) {
			if (DEBUG) Log.v(TAG, "onPrepared:encoder=" + encoder);
			if (encoder instanceof MediaVideoEncoder)
				mUVCCameraView.setVideoEncoder((MediaVideoEncoder)encoder);
		}

		@Override
		public void onStopped(MediaEncoder encoder) {
			if (DEBUG) Log.v(TAG, "onStopped:encoder=" + encoder);
			if (encoder instanceof MediaVideoEncoder)
				mUVCCameraView.setVideoEncoder(null);
		}
	};

}
