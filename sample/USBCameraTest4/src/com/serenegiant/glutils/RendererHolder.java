package com.serenegiant.glutils;
/*
 * UVCCamera
 * library and sample to access to UVC web camera on non-rooted Android device
 * 
 * Copyright (c) 2014 saki t_saki@serenegiant.com
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
 * Files in the jni/libjpeg, jni/libusb and jin/libuvc folder may have a different license, see the respective files.
*/

import com.serenegiant.service.IUVCServiceOnFrameAvailable;

import android.graphics.SurfaceTexture;
import android.graphics.SurfaceTexture.OnFrameAvailableListener;
import android.opengl.EGL14;
import android.opengl.GLES20;
import android.os.RemoteException;
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
	private EGLBase mMasterEgl;
	private EGLBase.EglSurface mDummySurface;
	private int mTexId;
	private SurfaceTexture mMasterTexture;
	final float[] mTexMatrix = new float[16];
	private Surface mSurface;

	public RendererHolder(RenderHolderCallback callback) {
		if (DEBUG) Log.v(TAG, "Constructor");
		mCallback = callback;
		final Thread thread = new Thread(this, TAG);
		thread.start();
		synchronized (mSync) {
			if (!isRunning) {
				try {
					mSync.wait();
				} catch (InterruptedException e) {
				}
			}
		}
	}

	public Surface getSurface() {
		if (DEBUG) Log.v(TAG, "getSurface:surface=" + mSurface);
		return mSurface;
	}

	public void addSurface(int id, Surface surface, boolean isRecordable, IUVCServiceOnFrameAvailable onFrameAvailableListener) {
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

	public void removeSurface(int id) {
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
		public void onFrameAvailable(SurfaceTexture surfaceTexture) {
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
//			mDummySurface.makeCurrent();
			mMasterTexture.updateTexImage();
			mMasterTexture.getTransformMatrix(mTexMatrix);
		} catch (Exception e) {
			Log.e(TAG, "draw:thread id =" + Thread.currentThread().getId(), e);
			return;
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
				} catch (RemoteException e) {
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
					} catch (InterruptedException e) {
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

}
