package com.serenegiant.glutils;
/*
 * UVCCamera
 * library and sample to access to UVC web camera on non-rooted Android device
 * 
 * Copyright (c) 2014 saki t_saki@serenegiant.com
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
 * Files in the jni/libjpeg, jni/libusb and jin/libuvc folder may have a different license, see the respective files.
*/

import android.graphics.SurfaceTexture;
import android.opengl.EGLContext;
import android.os.Handler;
import android.os.Looper;
import android.os.Message;
import android.util.Log;
import android.view.Surface;

/**
 * Helper class to draw texture to whole view on private thread
 */
public final class RenderHandler extends Handler {
	private static final boolean DEBUG = true;	// TODO set false on release
	private static final String TAG = "RenderHandler";

    private static final int MSG_RENDER_SET_GLCONTEXT = 1;
    private static final int MSG_RENDER_DRAW = 2;
    private static final int MSG_RENDER_QUIT = 9;

	private int mTexId = -1;
	private final RenderThread mThread;

	public static RenderHandler createHandler() {
		return createHandler(null);
	}

	public static final RenderHandler createHandler(String name) {
		final RenderThread thread = new RenderThread(name);
		thread.start();
		return thread.getHandler();
	}

	public final void setEglContext(EGLContext shared_context, int tex_id, Surface surface) {
		if (DEBUG) Log.i(TAG, "RenderHandler:setEglContext:surface=" + surface);
		mTexId = tex_id;
		sendMessage(obtainMessage(MSG_RENDER_SET_GLCONTEXT, new ContextParams(shared_context, surface)));
	}

	public final void setEglContext(EGLContext shared_context, int tex_id, SurfaceTexture surface) {
		if (DEBUG) Log.i(TAG, "RenderHandler:setEglContext:surface=" + surface);
		mTexId = tex_id;
		sendMessage(obtainMessage(MSG_RENDER_SET_GLCONTEXT, new ContextParams(shared_context, surface)));
	}

	public final void draw() {
		removeMessages(MSG_RENDER_DRAW);
		sendMessage(obtainMessage(MSG_RENDER_DRAW, mTexId, 0, null));
	}

	public final void draw(int tex_id) {
		removeMessages(MSG_RENDER_DRAW);
		sendMessage(obtainMessage(MSG_RENDER_DRAW, tex_id, 0, null));
	}

	public final void draw(final float[] tex_matrix) {
		removeMessages(MSG_RENDER_DRAW);
		sendMessage(obtainMessage(MSG_RENDER_DRAW, mTexId, 0, tex_matrix));
	}
	
	public final void draw(int tex_id, final float[] tex_matrix) {
		removeMessages(MSG_RENDER_DRAW);
		sendMessage(obtainMessage(MSG_RENDER_DRAW, tex_id, 0, tex_matrix));
	}

	public final void release() {
		if (DEBUG) Log.i(TAG, "release:");
		sendEmptyMessage(MSG_RENDER_QUIT);
	}

	@Override
	public final void handleMessage(Message msg) {
		switch (msg.what) {
		case MSG_RENDER_SET_GLCONTEXT:
			final ContextParams params = (ContextParams)msg.obj;
			mThread.setEglContext(params.shared_context, params.surface);
			break;
		case MSG_RENDER_DRAW:
			mThread.draw(msg.arg1, (float[])msg.obj);
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
	private RenderHandler(RenderThread thread) {
		if (DEBUG) Log.i(TAG, "RenderHandler:");
		mThread = thread;
	}

	private static final class ContextParams {
    	final EGLContext shared_context;
    	final Object surface;
    	public ContextParams(EGLContext shared_context, Object surface) {
    		this.shared_context = shared_context;
    		this.surface = surface;
    	}
    }

	/**
	 * Thread to execute render methods
	 * You can also use HandlerThread insted of this and create Handler from its Looper.  
	 */
    private static final class RenderThread extends Thread {
    	private final Object mSync = new Object();
    	private RenderHandler mHandler;
    	private EGLBase mEgl;
    	private EGLBase.EglSurface mInputSurface;
    	private GLDrawer2D mDrawer;

    	public RenderThread(String name) {
    		super(name);
    	}

    	public final RenderHandler getHandler() {
            synchronized (mSync) {
                // create rendering thread
            	try {
            		mSync.wait();
            	} catch (InterruptedException e) {
                }
            }
            return mHandler;
    	}

    	@Override
    	public final void run() {
            Log.d(TAG, getName() + " started");
            Looper.prepare();
            synchronized (mSync) {
                mHandler = new RenderHandler(this);
                mSync.notify();
            }
            Looper.loop();

            Log.d(TAG, getName() + " finishing");
            release();
            synchronized (mSync) {
                mHandler = null;
                mSync.notify();
            }
    	}

    	private final void release() {
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

    	private final void setEglContext(EGLContext shard_context, Object surface) {
    		if (DEBUG) Log.i(TAG, "RenderThread#setEglContext:");
    		release();
    		mEgl = new EGLBase(shard_context, false);
			if (surface instanceof Surface)
	    		mInputSurface = mEgl.createFromSurface((Surface)surface);
			else if (surface instanceof SurfaceTexture)
	    		mInputSurface = mEgl.createFromSurface((SurfaceTexture)surface);
			else
				throw new IllegalArgumentException("Invalid surface object:" + surface);
    		mInputSurface.makeCurrent();
    		mDrawer = new GLDrawer2D();
    	}
 
    	private final void draw(int tex_id, final float[] tex_matrix) {
//    		if (DEBUG) Log.i(TAG, "RenderThread#draw:tex_id=" + tex_id);
    		if (tex_id >= 0) {
	    		mInputSurface.makeCurrent();
	    		mDrawer.draw(tex_id, tex_matrix);
	    		mInputSurface.swap();
    		}
    	}
    }

}
