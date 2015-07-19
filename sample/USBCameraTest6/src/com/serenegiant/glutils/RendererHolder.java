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
import android.util.Log;
import android.util.SparseArray;
import android.view.Surface;

/**
 * Hold shared texture that has camera frame and draw them to registered surface if needs<br>
 * Using RenderHandler is little bit slow and it is better to draw in this class directly.
 */
public class RendererHolder implements Runnable {
	private static final boolean DEBUG = true;
	private static final String TAG = "RendererHolder";

	private static final int WIDTH = 640;
	private static final int HEIGHT = 480;

	public interface RenderHolderCallback {
		public void onCreate(Surface surface);
		public void onDestroy();
	}

	public interface OnFrameAvailableCallback {
		public void onFrameAvailable();
	}

	private final Object mSync = new Object();
	private final RenderHolderCallback mCallback;
	private final SparseArray<RenderHandler> mClients = new SparseArray<RenderHandler>();
	private final SparseArray<OnFrameAvailableCallback> mOnFrameAvailables = new SparseArray<OnFrameAvailableCallback>();
	private volatile boolean isRunning;
	private volatile boolean requestDraw;
	private File mCaptureFile;
	private EGLBase mMasterEgl;
	private EGLBase.EglSurface mDummySurface;
	private int mTexId;
	private SurfaceTexture mMasterTexture;
	final float[] mTexMatrix = new float[16];
	private Surface mSurface;

	public RendererHolder(final RenderHolderCallback callback) {
		if (DEBUG) Log.v(TAG, "Constructor");
		mCallback = callback;
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

	public void addSurface(final int id, final Surface surface, final boolean isRecordable, final OnFrameAvailableCallback onFrameAvailableListener) {
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
				if (!mClients.valueAt(i).isValid()) {
					final int id = mClients.keyAt(i);
					if (DEBUG) Log.i(TAG, "checkSurface:found invalid surface:id=" + id);
					mClients.valueAt(i).release();
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
				} catch (final Exception e) {
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
			isRunning = true;
			mSync.notifyAll();
			while (isRunning) {
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
    	final ByteBuffer buf = ByteBuffer.allocateDirect(WIDTH * HEIGHT * 4);
    	EGLBase egl;
    	EGLBase.EglSurface captureSurface;
    	GLDrawer2D drawer;

    	@Override
		public void run() {
			if (DEBUG) Log.v(TAG, "captureTask start");
	    	buf.order(ByteOrder.LITTLE_ENDIAN);
			synchronized (mSync) {
				if (!isRunning) {
					try {
						mSync.wait();
					} catch (final InterruptedException e) {
					}
				}
			}
			init();
			File captureFile = null;
			if (DEBUG) Log.v(TAG, "captureTask loop");
			while (isRunning) {
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
						captureSurface.makeCurrent();
						drawer.draw(mTexId, mTexMatrix);
						captureSurface.swap();
				        buf.clear();
				        // XXX it is better to feed resolution as parameter so that user can change camera resolution.
				        // if the camera resolution is smaller than WIDTH x HEIGHT, app may crash...
				        GLES20.glReadPixels(0, 0, WIDTH, HEIGHT, GLES20.GL_RGBA, GLES20.GL_UNSIGNED_BYTE, buf);
						// if you save every frame as a Bitmap, app may crash by Out of Memory exception...
				        if (DEBUG) Log.v(TAG, "save pixels to png file:" + captureFile);
				        BufferedOutputStream os = null;
						try {
					        try {
					            os = new BufferedOutputStream(new FileOutputStream(captureFile));
					            final Bitmap bmp = Bitmap.createBitmap(WIDTH, HEIGHT, Bitmap.Config.ARGB_8888);
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
	    	egl = new EGLBase(mMasterEgl.getContext(), false, false);
	    	captureSurface = egl.createOffscreen(WIDTH,  HEIGHT);
	    	drawer = new GLDrawer2D();
	    	drawer.getMvpMatrxi()[5] *= -1.0f;	// flip up-side down
		}

		private final void release() {
			captureSurface.release();
			captureSurface = null;
			drawer.release();
			drawer = null;
			egl.release();
			egl = null;
		}
	};
}
