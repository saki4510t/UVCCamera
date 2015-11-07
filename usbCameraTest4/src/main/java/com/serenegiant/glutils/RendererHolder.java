package com.serenegiant.glutils;
/*
 * UVCCamera
 * library and sample to access to UVC web camera on non-rooted Android device
 *
 * Copyright (c) 2014-2015 saki t_saki@serenegiant.com
 *
 * File name: RendererHolder.java
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

import java.io.BufferedOutputStream;
import java.io.File;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;

import android.graphics.Bitmap;
import android.graphics.SurfaceTexture;
import android.graphics.SurfaceTexture.OnFrameAvailableListener;
import android.opengl.EGL14;
import android.opengl.GLES20;
import android.os.RemoteException;
import android.util.Log;
import android.util.SparseArray;
import android.view.Surface;

import com.serenegiant.service.IUVCServiceOnFrameAvailable;

/**
 * Hold shared texture that has camera frame and draw them to registered surface if needs<br>
 * Using RenderHandler is little bit slow and it is better to draw in this class directly.
 */
public class RendererHolder implements Runnable {
	private static final boolean DEBUG = true;
	private static final String TAG = "RendererHolder";

	private static final int DEFAULT_WIDTH = 640;
	private static final int DEFAULT_HEIGHT = 480;

	public interface RenderHolderCallback {
		public void onCreate(Surface surface);
		public void onDestroy();
	}

	private final Object mSync = new Object();
	private final RenderHolderCallback mCallback;
	private final SparseArray<RenderHandler> mClients = new SparseArray<RenderHandler>();
	private final SparseArray<IUVCServiceOnFrameAvailable> mOnFrameAvailables = new SparseArray<IUVCServiceOnFrameAvailable>();
	private volatile boolean isRunning;
	private volatile boolean requestDraw;
	private volatile boolean requestResize;
	private File mCaptureFile;
	private EGLBase mMasterEgl;
	private EGLBase.EglSurface mDummySurface;
	private int mTexId;
	private SurfaceTexture mMasterTexture;
	final float[] mTexMatrix = new float[16];
	private Surface mSurface;
	private int mFrameWidth, mFrameHeight;
	private int mRequestWidth, mRequestHeight;

	public RendererHolder(final RenderHolderCallback callback) {
		this(DEFAULT_WIDTH, DEFAULT_HEIGHT, callback);
	}
	
	public RendererHolder(final int width, final int height, final RenderHolderCallback callback) {
		if (DEBUG) Log.v(TAG, "Constructor");
		mCallback = callback;
		mFrameWidth = width;
		mFrameHeight = height;
		final Thread thread = new Thread(this, TAG);
		thread.start();
		new Thread(mCaptureTask, "CaptureTask").start();
		synchronized (mSync) {
			if (!isRunning) {
				try {
					mSync.wait();
				} catch (final InterruptedException e) {
				}
			}
		}
	}

	public Surface getSurface() {
		if (DEBUG) Log.v(TAG, "getSurface:surface=" + mSurface);
		return mSurface;
	}

	public void addSurface(final int id, final Surface surface, final boolean isRecordable, final IUVCServiceOnFrameAvailable onFrameAvailableListener) {
		if (DEBUG) Log.v(TAG, "addSurface:id=" + id + ",surface=" + surface);
		checkSurface();
		synchronized (mSync) {
			RenderHandler handler = mClients.get(id);
			if (handler == null) {
				handler = RenderHandler.createHandler();
				mClients.append(id, handler);
				if (onFrameAvailableListener != null)
					mOnFrameAvailables.append(id, onFrameAvailableListener);
				handler.setEglContext(mMasterEgl.getContext(), mTexId, surface, true);
				requestDraw = false;
				if (DEBUG) Log.v(TAG, "success to add surface:id=" + id);
			} else {
				Log.w(TAG, "specific surface id already exist");
			}
			mSync.notifyAll();
		}
	}

	public void removeSurface(final int id) {
		if (DEBUG) Log.v(TAG, "removeSurface:id=" + id);
		RenderHandler handler = null;
		synchronized (mSync) {
			mOnFrameAvailables.remove(id);
			handler = mClients.get(id);
			if (handler != null) {
				requestDraw = false;
				mClients.remove(id);
				handler.release();
				handler = null;
				if (DEBUG) Log.v(TAG, "success to remove surface:id=" + id);
			} else {
				Log.w(TAG, "specific surface id not found");
			}
			mSync.notifyAll();
		}
		checkSurface();
	}

	public void removeAll() {
		if (DEBUG) Log.v(TAG, "removeAll:");
		requestDraw = false;
		synchronized (mSync) {
			final int n = mClients.size();
			for (int i = 0; i < n; i++) {
				mClients.valueAt(i).release();
			}
			mClients.clear();
			mOnFrameAvailables.clear();
			mSync.notifyAll();
		}
	}

	public void resize(final int width, final int height) {
		synchronized (mSync) {
			mRequestWidth = width;
			mRequestHeight = height;
			requestResize = true;
			mSync.notifyAll();
		}
	}
	
	public void captureStill(final String path) {
		if (DEBUG) Log.v(TAG, "captureStill:" + path);
		final File file = new File(path);
		if (DEBUG) Log.v(TAG, "captureStill:canWrite");
		synchronized (mSync) {
			mCaptureFile = file;
			mSync.notifyAll();
		}
	}

	public void release() {
		if (DEBUG) Log.v(TAG, "release:");
		removeAll();
		synchronized (mSync) {
			isRunning = false;
			mSync.notifyAll();
		}
	}

	private final OnFrameAvailableListener mOnFrameAvailableListener = new OnFrameAvailableListener() {
		@Override
		public void onFrameAvailable(final SurfaceTexture surfaceTexture) {
			synchronized (mSync) {
				requestDraw = isRunning;
				mSync.notifyAll();
			}
		}
	};

	private void checkSurface() {
		if (DEBUG) Log.v(TAG, "checkSurface");
		synchronized (mSync) {
			final int n = mClients.size();
			for (int i = 0; i < n; i++) {
				final RenderHandler rh = mClients.valueAt(i);
				if (rh == null || !rh.isValid()) {
					final int id = mClients.keyAt(i);
					if (DEBUG) Log.i(TAG, "checkSurface:found invalid surface:id=" + id);
					if (rh != null)
						rh.release();
					mClients.remove(id);
					mOnFrameAvailables.remove(id);
				}
			}
			mSync.notifyAll();
		}
	}

	private void draw() {
		try {
			mDummySurface.makeCurrent();
			mMasterTexture.updateTexImage();
			mMasterTexture.getTransformMatrix(mTexMatrix);
		} catch (final Exception e) {
			Log.e(TAG, "draw:thread id =" + Thread.currentThread().getId(), e);
			return;
		}
		synchronized (mCaptureTask) {
			mCaptureTask.notify();
		}
		synchronized (mSync) {
			final int n = mClients.size();
			for (int i = 0; i < n; i++) {
				mClients.valueAt(i).draw(mTexId, mTexMatrix);
			}
			final int m = mOnFrameAvailables.size();
			for (int i = 0; i < m; i++) {
				try {
					mOnFrameAvailables.valueAt(i).onFrameAvailable();
				} catch (final RemoteException e) {
				}
			}
		}
		GLES20.glClear(GLES20.GL_COLOR_BUFFER_BIT);
		GLES20.glFlush();
	}

	@Override
	public void run() {
		if (DEBUG) Log.v(TAG, "start:threadid=" + Thread.currentThread().getId());
		mMasterEgl = new EGLBase(EGL14.EGL_NO_CONTEXT, false, false);
    	mDummySurface = mMasterEgl.createOffscreen(2, 2);
		mDummySurface.makeCurrent();
		mTexId = GLDrawer2D.initTex();
		mMasterTexture = new SurfaceTexture(mTexId);
		mSurface = new Surface(mMasterTexture);
		mMasterTexture.setOnFrameAvailableListener(mOnFrameAvailableListener);
		if (mCallback != null) {
			mCallback.onCreate(mSurface);
		}
		synchronized (mSync) {
			mMasterTexture.setDefaultBufferSize(mFrameWidth, mFrameHeight);
			isRunning = true;
			mSync.notifyAll();
			for ( ; isRunning ; ) {
				if (requestResize) {
					requestResize = false;
					if ((mRequestWidth > 0) && (mRequestHeight > 0)
						&& ((mFrameWidth != mRequestWidth) || (mFrameHeight != mRequestHeight))) {
						mFrameWidth = mRequestWidth;
						mFrameHeight = mRequestHeight;
						mMasterTexture.setDefaultBufferSize(mFrameWidth, mFrameHeight);
					}
				}
				if (requestDraw) {
					requestDraw = false;
					draw();
				} else {
					try {
						mSync.wait();
					} catch (final InterruptedException e) {
						break;
					}
				}
			}
		}
		if (DEBUG) Log.v(TAG, "finishing");
		if (mCallback != null) {
			mCallback.onDestroy();
		}
		release();
		mSurface = null;
		mMasterTexture.release();
		mMasterTexture = null;
		GLDrawer2D.deleteTex(mTexId);
		mDummySurface.release();
		mDummySurface = null;
		mMasterEgl.release();
		mMasterEgl = null;
		if (DEBUG) Log.v(TAG, "finished");
	}

	private final Runnable mCaptureTask = new Runnable() {
    	EGLBase egl;
    	EGLBase.EglSurface captureSurface;
    	GLDrawer2D drawer;

    	@Override
		public void run() {
			if (DEBUG) Log.v(TAG, "captureTask:start");
			synchronized (mSync) {
				if (!isRunning) {
					try {
						mSync.wait();
					} catch (final InterruptedException e) {
					}
				}
			}
			init();
			int width = mFrameWidth;
			int height = mFrameHeight;
			ByteBuffer buf = ByteBuffer.allocateDirect(width * height * 4);
			buf.order(ByteOrder.LITTLE_ENDIAN);
			File captureFile = null;
			if (DEBUG) Log.v(TAG, "captureTask:loop");
			for ( ; isRunning ; ) {
				if (captureFile == null)
				synchronized (mSync) {
					if (mCaptureFile == null) {
						try {
							mSync.wait();
						} catch (final InterruptedException e) {
							break;
						}
					}
					if (mCaptureFile != null) {
						captureFile = mCaptureFile;
						mCaptureFile = null;
					}
				} else {
					synchronized (mCaptureTask) {
						try {
							mCaptureTask.wait();
						} catch (final InterruptedException e) {
							break;
						}
					}
					if (isRunning && (captureFile != null)) {
						if ((width != mFrameWidth) || (height != mFrameHeight)) {
							width = mFrameWidth;
							height = mFrameHeight;
							if (DEBUG) Log.v(TAG, String.format("resize capture size(%d,%d)", width, height));
							buf = ByteBuffer.allocateDirect(width * height * 4);
							buf.order(ByteOrder.LITTLE_ENDIAN);
							if (captureSurface != null) {
								captureSurface.release();
								captureSurface = null;
							}
							captureSurface = egl.createOffscreen(width, height);
						}
						captureSurface.makeCurrent();
						drawer.draw(mTexId, mTexMatrix);
						captureSurface.swap();
				        buf.clear();
						GLES20.glReadPixels(0, 0, width, height, GLES20.GL_RGBA, GLES20.GL_UNSIGNED_BYTE, buf);
						// if you save every frame as a Bitmap, app may crash by Out of Memory exception...
				        if (DEBUG) Log.v(TAG, String.format("save pixels(%dx%d) to png file:", width, height) + captureFile);
				        BufferedOutputStream os = null;
						try {
					        try {
					            os = new BufferedOutputStream(new FileOutputStream(captureFile));
								final Bitmap bmp = Bitmap.createBitmap(width, height, Bitmap.Config.ARGB_8888);
						        buf.clear();
					            bmp.copyPixelsFromBuffer(buf);
					            bmp.compress(Bitmap.CompressFormat.PNG, 90, os);
					            bmp.recycle();
					        } finally {
					            if (os != null) os.close();
					        }
						} catch (final FileNotFoundException e) {
						} catch (final IOException e) {
						}
					}
					captureFile = null;
				}
			}	// wnd of while (isRunning)
			// release resources
			if (DEBUG) Log.v(TAG, "captureTask finishing");
			release();
			if (DEBUG) Log.v(TAG, "captureTask finished");
		}

		private final void init() {
			if (DEBUG) Log.v(TAG, "captureTask:init");
	    	egl = new EGLBase(mMasterEgl.getContext(), false, false);
			captureSurface = egl.createOffscreen(mFrameWidth, mFrameHeight);
	    	drawer = new GLDrawer2D();
	    	drawer.getMvpMatrxi()[5] *= -1.0f;	// flip up-side down
		}

		private final void release() {
			if (DEBUG) Log.v(TAG, "captureTask:release");
			captureSurface.release();
			captureSurface = null;
			drawer.release();
			drawer = null;
			egl.release();
			egl = null;
		}
	};
}
