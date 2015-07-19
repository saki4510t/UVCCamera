package com.serenegiant.glutils;
/*
 * UVCCamera
 * library and sample to access to UVC web camera on non-rooted Android device
 *
 * Copyright (c) 2014-2015 saki t_saki@serenegiant.com
 *
 * File name: RenderHandler.java
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

import android.graphics.SurfaceTexture;
import android.opengl.EGLContext;
import android.text.TextUtils;
import android.util.Log;
import android.view.Surface;
import android.view.SurfaceHolder;

/**
 * Helper class to draw texture to whole view on private thread
 */
public final class RenderHandler implements Runnable {
	private static final boolean DEBUG = true;	// TODO set false on release
	private static final String TAG = "RenderHandler";

	private final Object mSync = new Object();
    private EGLContext mShard_context;
    private boolean mIsRecordable;
    private Object mSurface;
	private int mTexId = -1;
	private float[] mTexMatrix;

	private boolean mRequestSetEglContext;
	private boolean mRequestRelease;
	private int mRequestDraw;

	public static final RenderHandler createHandler(final String name) {
		if (DEBUG) Log.v(TAG, "createHandler:");
		final RenderHandler handler = new RenderHandler();
		synchronized (handler.mSync) {
			new Thread(handler, !TextUtils.isEmpty(name) ? name : TAG).start();
			try {
				handler.mSync.wait();
			} catch (final InterruptedException e) {
			}
		}
		return handler;
	}

	public final void setEglContext(final EGLContext shared_context, final int tex_id, final Object surface, final boolean isRecordable) {
		if (DEBUG) Log.i(TAG, "setEglContext:");
		if (!(surface instanceof Surface) && !(surface instanceof SurfaceTexture) && !(surface instanceof SurfaceHolder))
			throw new RuntimeException("unsupported window type:" + surface);
		synchronized (mSync) {
			if (mRequestRelease) return;
			mShard_context = shared_context;
			mTexId = tex_id;
			mSurface = surface;
			mIsRecordable = isRecordable;
			mRequestSetEglContext = true;
			mSync.notifyAll();
			try {
				mSync.wait();
			} catch (final InterruptedException e) {
			}
		}
	}

	public final void draw() {
		draw(mTexId, mTexMatrix);
	}

	public final void draw(final int tex_id) {
		draw(tex_id, mTexMatrix);
	}

	public final void draw(final float[] tex_matrix) {
		draw(mTexId, tex_matrix);
	}

	public final void draw(final int tex_id, final float[] tex_matrix) {
		synchronized (mSync) {
			if (mRequestRelease) return;
			mTexId = tex_id;
			mTexMatrix = tex_matrix;
			mRequestDraw++;
			mSync.notifyAll();
			try {
				mSync.wait();
			} catch (final InterruptedException e) {
			}
		}
	}

	public boolean isValid() {
		synchronized (mSync) {
			return mSurface != null
				? (mSurface instanceof Surface ? ((Surface)mSurface).isValid() : true)
				: false;
		}
	}

	public final void release() {
		if (DEBUG) Log.i(TAG, "release:");
		synchronized (mSync) {
			if (mRequestRelease) return;
			mRequestRelease = true;
			mSync.notifyAll();
			try {
				mSync.wait();
			} catch (final InterruptedException e) {
			}
		}
	}

//********************************************************************************
//********************************************************************************
	private EGLBase mEgl;
	private EGLBase.EglSurface mInputSurface;
	private GLDrawer2D mDrawer;

	@Override
	public final void run() {
		if (DEBUG) Log.i(TAG, "RenderHandler thread started:");
		synchronized (mSync) {
			mRequestSetEglContext = mRequestRelease = false;
			mRequestDraw = 0;
			mSync.notifyAll();
		}
        boolean isRunning = true;
        boolean localRequestDraw;
        while (isRunning) {
        	synchronized (mSync) {
        		if (mRequestRelease) break;
	        	if (mRequestSetEglContext) {
	        		mRequestSetEglContext = false;
	        		internalPrepare();
	        	}
	        	localRequestDraw = mRequestDraw > 0;
	        	if (localRequestDraw)
	        		mRequestDraw--;
        	}
        	if (localRequestDraw) {
        		if ((mEgl != null) && mTexId >= 0) {
            		mInputSurface.makeCurrent();
            		mDrawer.draw(mTexId, mTexMatrix);
            		mInputSurface.swap();
        		}
        		synchronized (mSync) {
        			mSync.notifyAll();
        		}
        	} else {
        		synchronized(mSync) {
        			try {
						mSync.wait();
					} catch (final InterruptedException e) {
						break;
					}
        		}
        	}
        }
        synchronized (mSync) {
        	isRunning = false;
        	mRequestRelease = true;
            internalRelease();
            mSync.notifyAll();
        }
		if (DEBUG) Log.i(TAG, "RenderHandler thread finished:");
	}

	private final void internalPrepare() {
		if (DEBUG) Log.i(TAG, "internalPrepare:");
		internalRelease();
		mEgl = new EGLBase(mShard_context, false, mIsRecordable);

		if (mSurface instanceof Surface)
    		mInputSurface = mEgl.createFromSurface(mSurface);
		else if (mSurface instanceof SurfaceTexture)
    		mInputSurface = mEgl.createFromSurface(mSurface);
		else
			throw new IllegalArgumentException("Invalid surface object:" + mSurface);

		mInputSurface.makeCurrent();
		mDrawer = new GLDrawer2D();
		mSurface = null;
		mSync.notifyAll();
	}

	private final void internalRelease() {
		if (DEBUG) Log.i(TAG, "internalRelease:");
		if (mInputSurface != null) {
			mInputSurface.release();
			mInputSurface = null;
		}
		if (mDrawer != null) {
			mDrawer.release();
			mDrawer = null;
		}
		if (mEgl != null) {
			mEgl.release();
			mEgl = null;
		}
	}

}
