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
import android.opengl.GLES20;
import android.os.Handler;
import android.os.Looper;
import android.os.Message;
import android.util.Log;
import android.view.Surface;
import android.view.SurfaceHolder;

/**
 * Draw shared texture on specific whole Surface using OpenGL|ES
 */
public final class RenderHandler extends Handler {
	private static final boolean DEBUG = false;	// TODO set false on release
	private static final String TAG = "RenderHandler";

    private static final int MSG_RENDER_SET_GLCONTEXT = 1;
    private static final int MSG_RENDER_DRAW = 2;
    private static final int MSG_CHECK_VALID = 3;
    private static final int MSG_RENDER_QUIT = 9;

	private int mTexId = -1;
	private final RenderThread mThread;

	public static RenderHandler createHandler() {
		if (DEBUG) Log.v(TAG, "createHandler:");
		return createHandler("RenderThread");
	}

	public static final RenderHandler createHandler(final String name) {
		if (DEBUG) Log.v(TAG, "createHandler:name=" + name);
		final RenderThread thread = new RenderThread(name);
		thread.start();
		return thread.getHandler();
	}

	public final void setEglContext(final EGLContext shared_context, final int tex_id, final Object surface, final boolean isRecordable) {
		if (DEBUG) Log.i(TAG, "RenderHandler:setEglContext:");
		if (!(surface instanceof Surface) && !(surface instanceof SurfaceTexture) && !(surface instanceof SurfaceHolder))
			throw new RuntimeException("unsupported window type:" + surface);
		mTexId = tex_id;
		sendMessage(obtainMessage(MSG_RENDER_SET_GLCONTEXT, isRecordable ? 1 : 0, 0, new ContextParams(shared_context, surface)));
	}

	public final void draw() {
		sendMessage(obtainMessage(MSG_RENDER_DRAW, mTexId, 0, null));
	}

	public final void draw(final int tex_id) {
		sendMessage(obtainMessage(MSG_RENDER_DRAW, tex_id, 0, null));
	}

	public final void draw(final float[] tex_matrix) {
		sendMessage(obtainMessage(MSG_RENDER_DRAW, mTexId, 0, tex_matrix));
	}

	public final void draw(final int tex_id, final float[] tex_matrix) {
		sendMessage(obtainMessage(MSG_RENDER_DRAW, tex_id, 0, tex_matrix));
	}

	public boolean isValid() {
		synchronized (mThread.mSync) {
			sendEmptyMessage(MSG_CHECK_VALID);
			try {
				mThread.mSync.wait();
			} catch (final InterruptedException e) {
			}
			return mThread.mSurface != null ? mThread.mSurface.isValid() : false;
		}
	}

	public final void release() {
		if (DEBUG) Log.i(TAG, "release:");
		removeMessages(MSG_RENDER_SET_GLCONTEXT);
		removeMessages(MSG_RENDER_DRAW);
		sendEmptyMessage(MSG_RENDER_QUIT);
	}

	@Override
	public final void handleMessage(final Message msg) {
		switch (msg.what) {
		case MSG_RENDER_SET_GLCONTEXT:
			final ContextParams params = (ContextParams)msg.obj;
			mThread.handleSetEglContext(params.shared_context, params.surface, msg.arg1 != 0);
			break;
		case MSG_RENDER_DRAW:
			mThread.handleDraw(msg.arg1, (float[])msg.obj);
			break;
		case MSG_CHECK_VALID:
			synchronized (mThread.mSync) {
				mThread.mSync.notify();
			}
			break;
		case MSG_RENDER_QUIT:
			Looper.myLooper().quit();
			break;
		default:
			super.handleMessage(msg);
		}
	}

//********************************************************************************
//********************************************************************************
	private RenderHandler(final RenderThread thread) {
		if (DEBUG) Log.i(TAG, "RenderHandler:");
		mThread = thread;
	}

	private static final class ContextParams {
    	final EGLContext shared_context;
    	final Object surface;
    	public ContextParams(final EGLContext shared_context, final Object surface) {
    		this.shared_context = shared_context;
    		this.surface = surface;
    	}
    }

	/**
	 * Thread to execute render methods
	 * You can also use HandlerThread insted of this and create Handler from its Looper.
	 */
    private static final class RenderThread extends Thread {
		private static final String TAG_THREAD = "RenderThread";
    	private final Object mSync = new Object();
    	private RenderHandler mHandler;
    	private EGLBase mEgl;
    	private EGLBase.EglSurface mTargetSurface;
    	private Surface mSurface;
    	private GLDrawer2D mDrawer;

    	public RenderThread(final String name) {
    		super(name);
    	}

    	public final RenderHandler getHandler() {
            synchronized (mSync) {
                // create rendering thread
            	try {
            		mSync.wait();
            	} catch (final InterruptedException e) {
                }
            }
            return mHandler;
    	}

    	/**
    	 * Set shared context and Surface
    	 * @param shard_context
    	 * @param surface
    	 */
    	public final void handleSetEglContext(final EGLContext shard_context, final Object surface, final boolean isRecordable) {
    		if (DEBUG) Log.i(TAG_THREAD, "setEglContext:");
    		release();
    		synchronized (mSync) {
    			mSurface = surface instanceof Surface ? (Surface)surface
    				: (surface instanceof SurfaceTexture ? new Surface((SurfaceTexture)surface) : null);
    		}
    		mEgl = new EGLBase(shard_context, false, isRecordable);
   			mTargetSurface = mEgl.createFromSurface(surface);
    		mDrawer = new GLDrawer2D();
    	}

    	/**
    	 * drawing
    	 * @param tex_id
    	 * @param tex_matrix
    	 */
    	public void handleDraw(final int tex_id, final float[] tex_matrix) {
//    		if (DEBUG) Log.i(TAG_THREAD, "draw");
    		if (tex_id >= 0) {
	    		mTargetSurface.makeCurrent();
	    		mDrawer.draw(tex_id, tex_matrix);
	    		mTargetSurface.swap();
    		}
    	}

    	@Override
    	public final void run() {
            if (DEBUG) Log.v(TAG_THREAD, "started");
            Looper.prepare();
            synchronized (mSync) {
                mHandler = new RenderHandler(this);
                mSync.notify();
            }
            Looper.loop();
            if (DEBUG) Log.v(TAG_THREAD, "finishing");
            release();
            synchronized (mSync) {
                mHandler = null;
            }
            if (DEBUG) Log.v(TAG_THREAD, "finished");
    	}

    	private final void release() {
    		if (DEBUG) Log.v(TAG_THREAD, "release:");
    		if (mDrawer != null) {
    			mDrawer.release();
    			mDrawer = null;
    		}
    		synchronized (mSync) {
    			mSurface = null;
    		}
    		if (mTargetSurface != null) {
    			clear();
    			mTargetSurface.release();
    			mTargetSurface = null;
    		}
    		if (mEgl != null) {
    			mEgl.release();
    			mEgl = null;
    		}
    	}

    	/**
    	 * Fill black on specific Surface
    	 */
    	private final void clear() {
    		if (DEBUG) Log.v(TAG_THREAD, "clear:");
    		mTargetSurface.makeCurrent();
			GLES20.glClearColor(0, 0, 0, 1);
			GLES20.glClear(GLES20.GL_COLOR_BUFFER_BIT);
			mTargetSurface.swap();
    	}
    }

}
