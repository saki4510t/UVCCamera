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

package com.serenegiant.usbcameratest0;

import android.hardware.usb.UsbDevice;
import android.media.MediaCodec;
import android.media.MediaFormat;
import android.os.Bundle;
import android.util.Log;
import android.view.Surface;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.view.View;
import android.view.View.OnClickListener;
import android.widget.Button;
import android.widget.ImageButton;
import android.widget.Toast;

import com.serenegiant.common.BaseActivity;
import com.serenegiant.usb.CameraDialog;
import com.serenegiant.usb.IFrameCallback;
import com.serenegiant.usb.USBMonitor;
import com.serenegiant.usb.USBMonitor.OnDeviceConnectListener;
import com.serenegiant.usb.USBMonitor.UsbControlBlock;
import com.serenegiant.usb.UVCCamera;

import java.io.BufferedInputStream;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.IOException;
import java.io.InputStream;
import java.nio.ByteBuffer;

public class MainActivity extends BaseActivity implements CameraDialog.CameraDialogParent {
	private static final boolean DEBUG = true;	// TODO set false when production
	private static final String TAG = "MainActivity";

    private final Object mSync = new Object();
    // for accessing USB and USB camera
    private USBMonitor mUSBMonitor;
	private UVCCamera mUVCCamera;
	private SurfaceView mUVCCameraView;
	// for open&start / stop&close camera preview
	private ImageButton mCameraButton;
	private Surface mPreviewSurface;
	private boolean isActive, isPreview;



	private String h264Path = "/mnt/sdcard/720pq.h264";
	private File h264File = new File(h264Path);
	private InputStream is = null;
	private FileInputStream fs = null;

	private SurfaceView mSurfaceView;
	private Button mReadButton;
	private MediaCodec mCodec;

	Thread readFileThread;
	boolean isInit = false;

	// Video Constants
	private final static String MIME_TYPE = "video/avc"; // H.264 Advanced Video
	private final static int VIDEO_WIDTH = 848;
	private final static int VIDEO_HEIGHT = 480;
	private final static int TIME_INTERNAL = 30;
	private final static int HEAD_OFFSET = 0;

	@Override
	protected void onCreate(final Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		setContentView(R.layout.activity_main);
		mCameraButton = (ImageButton)findViewById(R.id.camera_button);
		mCameraButton.setOnClickListener(mOnClickListener);

		mUVCCameraView = (SurfaceView)findViewById(R.id.camera_surface_view);
		mUVCCameraView.getHolder().addCallback(mSurfaceViewCallback);

		mUSBMonitor = new USBMonitor(this, mOnDeviceConnectListener);

		mSurfaceView = (SurfaceView) findViewById(R.id.surfaceView1);
		mReadButton = (Button) findViewById(R.id.btn_readfile);

		initH264();
	}

	private void initH264() {
		mReadButton.setOnClickListener(new View.OnClickListener() {
			@Override
			public void onClick(View v) {
				if (h264File.exists()) {
					if (!isInit) {
						initDecoder();
						isInit = true;
					}

					readFileThread = new Thread(readFile);
					readFileThread.start();
				} else {
					Toast.makeText(getApplicationContext(),
							"H264 file not found", Toast.LENGTH_SHORT).show();
				}
			}
		});
	}

	public void initDecoder() {
		try {
			mCodec = MediaCodec.createDecoderByType(MIME_TYPE);
		} catch (IOException e) {
			e.printStackTrace();
		}
		MediaFormat mediaFormat = MediaFormat.createVideoFormat(MIME_TYPE,
				VIDEO_WIDTH, VIDEO_HEIGHT);
		mCodec.configure(mediaFormat, mSurfaceView.getHolder().getSurface(),
				null, 0);
		mCodec.start();
	}

	int mCount = 0;

	public boolean onFrame(byte[] buf, int offset, int length) {
		Log.e("Media", "onFrame start " + buf.length + " offset = " + offset + " length = " + length);
		Log.e("Media", "onFrame Thread:" + Thread.currentThread().getId());
		// Get input buffer index
		ByteBuffer[] inputBuffers = mCodec.getInputBuffers();
		int inputBufferIndex = mCodec.dequeueInputBuffer(100);

		Log.e("Media", "onFrame index:" + inputBufferIndex);
		if (inputBufferIndex >= 0) {
			ByteBuffer inputBuffer = inputBuffers[inputBufferIndex];
			inputBuffer.clear();
			inputBuffer.put(buf, offset, length);
			mCodec.queueInputBuffer(inputBufferIndex, 0, length, mCount
					* TIME_INTERNAL, 0);
			mCount++;
		} else {
			return false;
		}

		// Get output buffer index
		MediaCodec.BufferInfo bufferInfo = new MediaCodec.BufferInfo();
		int outputBufferIndex = mCodec.dequeueOutputBuffer(bufferInfo, 100);
		while (outputBufferIndex >= 0) {
			mCodec.releaseOutputBuffer(outputBufferIndex, true);
			outputBufferIndex = mCodec.dequeueOutputBuffer(bufferInfo, 0);
		}
		Log.e("Media", "onFrame end");
		return true;
	}

	/**
	 * Find H264 frame head
	 *
	 * @param buffer
	 * @param len
	 * @return the offset of frame head, return 0 if can not find one
	 */
	static int findHead(byte[] buffer, int len) {
		int i;
		for (i = HEAD_OFFSET; i < len; i++) {
			if (checkHead(buffer, i))
				break;
		}
		if (i == len)
			return 0;
		if (i == HEAD_OFFSET)
			return 0;
		return i;
	}

	/**
	 * Check if is H264 frame head
	 *
	 * @param buffer
	 * @param offset
	 * @return whether the src buffer is frame head
	 */
	static boolean checkHead(byte[] buffer, int offset) {
		// 00 00 00 01
		if (buffer[offset] == 0 && buffer[offset + 1] == 0
				&& buffer[offset + 2] == 0 && buffer[3] == 1)
			return true;
		// 00 00 01
		if (buffer[offset] == 0 && buffer[offset + 1] == 0
				&& buffer[offset + 2] == 1)
			return true;
		return false;
	}

	Runnable readFile = new Runnable() {

		@Override
		public void run() {
			int h264Read = 0;
			int frameOffset = 0;
			byte[] buffer = new byte[100000];
			byte[] framebuffer = new byte[200000];
			boolean readFlag = true;
			try {
				fs = new FileInputStream(h264File);
				is = new BufferedInputStream(fs);
			} catch (FileNotFoundException e) {
				e.printStackTrace();
			}
			while (!Thread.interrupted() && readFlag) {
				try {
					int length = is.available();
					if (length > 0) {
						// Read file and fill buffer
						int count = is.read(buffer);
						Log.i("count", "" + count);
						h264Read += count;
						Log.d("Read", "count:" + count + " h264Read:"
								+ h264Read);
						// Fill frameBuffer
						if (frameOffset + count < 200000) {
							System.arraycopy(buffer, 0, framebuffer,
									frameOffset, count);
							frameOffset += count;
						} else {
							frameOffset = 0;
							System.arraycopy(buffer, 0, framebuffer,
									frameOffset, count);
							frameOffset += count;
						}

						// Find H264 head
						int offset = findHead(framebuffer, frameOffset);
						Log.i("find head", " Head:" + offset);
						while (offset > 0) {
							if (checkHead(framebuffer, 0)) {
								// Fill decoder
								boolean flag = onFrame(framebuffer, 0, offset);
								if (flag) {
									byte[] temp = framebuffer;
									framebuffer = new byte[200000];
									System.arraycopy(temp, offset, framebuffer,
											0, frameOffset - offset);
									frameOffset -= offset;
									Log.e("Check", "is Head:" + offset);
									// Continue finding head
									offset = findHead(framebuffer, frameOffset);
								}
							} else {

								offset = 0;
							}

						}
						Log.d("loop", "end loop");
					} else {
						h264Read = 0;
						frameOffset = 0;
						readFlag = false;
						// Start a new thread
						readFileThread = new Thread(readFile);
						readFileThread.start();
					}

				} catch (Exception e) {
					e.printStackTrace();
				}

				try {
					Thread.sleep(TIME_INTERNAL);
				} catch (InterruptedException e) {

				}
			}
		}
	};

	@Override
	protected void onStart() {
		super.onStart();
		if (DEBUG) Log.v(TAG, "onStart:");
		synchronized (mSync) {
			if (mUSBMonitor != null) {
				mUSBMonitor.register();
			}
		}
	}

	@Override
	protected void onStop() {
		if (DEBUG) Log.v(TAG, "onStop:");
		synchronized (mSync) {
			if (mUSBMonitor != null) {
				mUSBMonitor.unregister();
			}
		}
		super.onStop();
	}

	@Override
	protected void onDestroy() {
		if (DEBUG) Log.v(TAG, "onDestroy:");
		synchronized (mSync) {
			isActive = isPreview = false;
			if (mUVCCamera != null) {
				mUVCCamera.destroy();
				mUVCCamera = null;
			}
			if (mUSBMonitor != null) {
				mUSBMonitor.destroy();
				mUSBMonitor = null;
			}
		}
		mUVCCameraView = null;
		mCameraButton = null;
		super.onDestroy();
		readFileThread.interrupt();
	}

	private final OnClickListener mOnClickListener = new OnClickListener() {
		@Override
		public void onClick(final View view) {
			if (!isInit) {
				initDecoder();
				isInit = true;
			}

			if (mUVCCamera == null) {
				// XXX calling CameraDialog.showDialog is necessary at only first time(only when app has no permission).
				CameraDialog.showDialog(MainActivity.this);
			} else {
				synchronized (mSync) {
					mUVCCamera.destroy();
					mUVCCamera = null;
					isActive = isPreview = false;
				}
			}
		}
	};

	private final OnDeviceConnectListener mOnDeviceConnectListener = new OnDeviceConnectListener() {
		@Override
		public void onAttach(final UsbDevice device) {
			if (DEBUG) Log.v(TAG, "onAttach:");
			Toast.makeText(MainActivity.this, "USB_DEVICE_ATTACHED", Toast.LENGTH_SHORT).show();
		}

		@Override
		public void onConnect(final UsbDevice device, final UsbControlBlock ctrlBlock, final boolean createNew) {
			if (DEBUG) Log.v(TAG, "onConnect:");
			synchronized (mSync) {
				if (mUVCCamera != null) {
					mUVCCamera.destroy();
				}
				isActive = isPreview = false;
			}
			queueEvent(new Runnable() {
				@Override
				public void run() {
					synchronized (mSync) {
						final UVCCamera camera = new UVCCamera();
						camera.open(ctrlBlock);
						if (DEBUG) Log.i(TAG, "supportedSize:" + camera.getSupportedSize());
						try {
							camera.setPreviewSize(UVCCamera.DEFAULT_PREVIEW_WIDTH, UVCCamera.DEFAULT_PREVIEW_HEIGHT, UVCCamera.FRAME_FORMAT_H264);
						} catch (final IllegalArgumentException e) {
							try {
								// fallback to YUV mode
								camera.setPreviewSize(UVCCamera.DEFAULT_PREVIEW_WIDTH, UVCCamera.DEFAULT_PREVIEW_HEIGHT, UVCCamera.DEFAULT_PREVIEW_MODE);
							} catch (final IllegalArgumentException e1) {
								camera.destroy();
								return;
							}
						}
						mPreviewSurface = mUVCCameraView.getHolder().getSurface();
						if (mPreviewSurface != null) {
							isActive = true;
							camera.setPreviewDisplay(mPreviewSurface);
							camera.setFrameCallback(mIFrameCallback, UVCCamera.PIXEL_FORMAT_RAW);
							camera.startPreview();
							isPreview = true;
						}
						synchronized (mSync) {
							mUVCCamera = camera;
						}
					}
				}
			}, 0);
		}

		@Override
		public void onDisconnect(final UsbDevice device, final UsbControlBlock ctrlBlock) {
			if (DEBUG) Log.v(TAG, "onDisconnect:");
			// XXX you should check whether the comming device equal to camera device that currently using
			queueEvent(new Runnable() {
				@Override
				public void run() {
					synchronized (mSync) {
						if (mUVCCamera != null) {
							mUVCCamera.close();
							if (mPreviewSurface != null) {
								mPreviewSurface.release();
								mPreviewSurface = null;
							}
							isActive = isPreview = false;
						}
					}
				}
			}, 0);
		}

		@Override
		public void onDettach(final UsbDevice device) {
			if (DEBUG) Log.v(TAG, "onDettach:");
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
			runOnUiThread(new Runnable() {
				@Override
				public void run() {
					// FIXME
				}
			}, 0);
		}
	}

	private final SurfaceHolder.Callback mSurfaceViewCallback = new SurfaceHolder.Callback() {
		@Override
		public void surfaceCreated(final SurfaceHolder holder) {
			if (DEBUG) Log.v(TAG, "surfaceCreated:");
		}

		@Override
		public void surfaceChanged(final SurfaceHolder holder, final int format, final int width, final int height) {
			if ((width == 0) || (height == 0)) return;
			if (DEBUG) Log.v(TAG, "surfaceChanged:");
			mPreviewSurface = holder.getSurface();
			synchronized (mSync) {
				if (isActive && !isPreview && (mUVCCamera != null)) {
					mUVCCamera.setPreviewDisplay(mPreviewSurface);
					mUVCCamera.startPreview();
					isPreview = true;
				}
			}
		}

		@Override
		public void surfaceDestroyed(final SurfaceHolder holder) {
			if (DEBUG) Log.v(TAG, "surfaceDestroyed:");
			synchronized (mSync) {
				if (mUVCCamera != null) {
					mUVCCamera.stopPreview();
				}
				isPreview = false;
			}
			mPreviewSurface = null;
		}
	};

	private final IFrameCallback mIFrameCallback = new IFrameCallback() {
		@Override
		public void onFrame(final ByteBuffer frame) {
			int length = frame.limit();
			byte[] buffer = new byte[length];
			frame.get(buffer, 0, length);
			MainActivity.this.onFrame(buffer, 0, length);
		}
	};
}
