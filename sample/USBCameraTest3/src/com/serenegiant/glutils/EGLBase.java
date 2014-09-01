package com.serenegiant.glutils;
/*
 * UVCCamera
 * library and sample to access to UVC web camera on non-rooted Android device
 * 
 * Copyright (c) 2014 saki t_saki@serenegiant.com
 * 
 * File name: EGLBase.java
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
import android.opengl.EGL14;
import android.opengl.EGLConfig;
import android.opengl.EGLContext;
import android.opengl.EGLDisplay;
import android.opengl.EGLSurface;
import android.util.Log;
import android.view.Surface;

public class EGLBase {
	private static final boolean DEBUG = true;	// TODO set false on release
	private static final String TAG = "EGLBase";

    private static final int EGL_RECORDABLE_ANDROID = 0x3142;

    private EGLConfig mEglConfig = null;
	private EGLContext mEglContext = EGL14.EGL_NO_CONTEXT;
	private EGLDisplay mEglDisplay = EGL14.EGL_NO_DISPLAY;

	public static class EglSurface {
		private final EGLBase mEgl;
		private EGLSurface mEglSurface = EGL14.EGL_NO_SURFACE;

		EglSurface(EGLBase egl, Object surface) {
			if (DEBUG) Log.i(TAG, "EglSurface:surface=" + surface);
			mEgl = egl;
			mEglSurface = mEgl.createWindowSurface(surface);
		}

		public void makeCurrent() {
			mEgl.makeCurrent(mEglSurface);
		}

		public void swap() {
			mEgl.swap(mEglSurface);
		}

		public void release() {
			if (DEBUG) Log.i(TAG, "EglSurface:release:");
			mEgl.destroyWindowSurface(mEglSurface);
	        mEglSurface = EGL14.EGL_NO_SURFACE;
		}

		public EGLContext getContext() {
			return mEgl.getContext();
		}
	}

	public EGLBase(EGLContext shared_context, boolean with_depth_buffer) {
		if (DEBUG) Log.i(TAG, "EGLBase:");
		init(shared_context, with_depth_buffer);
	}

    public void release() {
		if (DEBUG) Log.v(TAG, "release:");
        if (mEglDisplay != EGL14.EGL_NO_DISPLAY) {
	    	destroyContext();
	        EGL14.eglTerminate(mEglDisplay);
	        EGL14.eglReleaseThread();
        }
        mEglDisplay = EGL14.EGL_NO_DISPLAY;
        mEglContext = EGL14.EGL_NO_CONTEXT;
    }

	public EglSurface createFromSurface(Surface surface) {
		if (DEBUG) Log.i(TAG, "createFromSurface:surface=" + surface);
		final EglSurface eglSurface = new EglSurface(this, surface);
		return eglSurface;
	}

	public EglSurface createFromSurface(SurfaceTexture surface) {
		if (DEBUG) Log.i(TAG, "createFromSurface:surface=" + surface);
		final EglSurface eglSurface = new EglSurface(this, surface);
		return eglSurface;
	}

	public EGLContext getContext() {
		if (DEBUG) Log.v(TAG, "getContext:" + mEglContext);
		return mEglContext;
	}

	private void init(EGLContext shared_context, boolean with_depth_buffer) {
		if (DEBUG) Log.v(TAG, "init:");
        if (mEglDisplay != EGL14.EGL_NO_DISPLAY) {
            throw new RuntimeException("EGL already set up");
        }

        mEglDisplay = EGL14.eglGetDisplay(EGL14.EGL_DEFAULT_DISPLAY);
        if (mEglDisplay == EGL14.EGL_NO_DISPLAY) {
            throw new RuntimeException("eglGetDisplay failed");
        }

		final int[] version = new int[2];
        if (!EGL14.eglInitialize(mEglDisplay, version, 0, version, 1)) {
        	mEglDisplay = null;
            throw new RuntimeException("eglInitialize failed");
        }

		shared_context = shared_context != null ? shared_context : EGL14.EGL_NO_CONTEXT;
        if (mEglContext == EGL14.EGL_NO_CONTEXT) {
            mEglConfig = getConfig(with_depth_buffer);
            if (mEglConfig == null) {
                throw new RuntimeException("chooseConfig failed");
            }
            // create EGL rendering context
	        mEglContext = createContext(shared_context);
        }
        // confirm whether the EGL rendering context is successfully created
        final int[] values = new int[1];
        EGL14.eglQueryContext(mEglDisplay, mEglContext, EGL14.EGL_CONTEXT_CLIENT_VERSION, values, 0);
        if (DEBUG) Log.d(TAG, "EGLContext created, client version " + values[0]);
	}

	/**
	 * change context to draw this window surface
	 * @return
	 */
	private boolean makeCurrent(EGLSurface surface) {
//		if (DEBUG) Log.v(TAG, "makeCurrent:");
        if (mEglDisplay == null) {
            if (DEBUG) Log.d(TAG, "makeCurrent:eglDisplay not initialized");
        }
        if (surface == null || surface == EGL14.EGL_NO_SURFACE) {
            int error = EGL14.eglGetError();
            if (error == EGL14.EGL_BAD_NATIVE_WINDOW) {
                Log.e(TAG, "makeCurrent:returned EGL_BAD_NATIVE_WINDOW.");
            }
            return false;
        }
        // attach EGL renderring context to specific EGL window surface
        if (!EGL14.eglMakeCurrent(mEglDisplay, surface, surface, mEglContext)) {
            Log.w(TAG, "eglMakeCurrent:" + EGL14.eglGetError());
            return false;
        }
        return true;
	}

	private int swap(EGLSurface surface) {
//		if (DEBUG) Log.v(TAG, "swap:");
        if (!EGL14.eglSwapBuffers(mEglDisplay, surface)) {
        	final int err = EGL14.eglGetError();
        	if (DEBUG) Log.w(TAG, "swap:err=" + err);
            return err;
        }
        return EGL14.EGL_SUCCESS;
    }

    private EGLContext createContext(EGLContext shared_context) {
		if (DEBUG) Log.v(TAG, "createContext:");

        final int[] attrib_list = {
        	EGL14.EGL_CONTEXT_CLIENT_VERSION, 2,
        	EGL14.EGL_NONE
        };
        final EGLContext context = EGL14.eglCreateContext(mEglDisplay, mEglConfig, shared_context, attrib_list, 0);
        checkEglError("eglCreateContext");
        return context;
    }

    private void destroyContext() {
		if (DEBUG) Log.v(TAG, "destroyContext:");

        if (!EGL14.eglDestroyContext(mEglDisplay, mEglContext)) {
            Log.e("DefaultContextFactory", "display:" + mEglDisplay + " context: " + mEglContext);
            Log.e(TAG, "eglDestroyContex:" + EGL14.eglGetError());
        }
        mEglContext = EGL14.EGL_NO_CONTEXT;
    }

    private EGLSurface createWindowSurface(Object nativeWindow) {
		if (DEBUG) Log.v(TAG, "createWindowSurface:nativeWindow=" + nativeWindow);

        final int[] surfaceAttribs = {
                EGL14.EGL_NONE
        };
		EGLSurface result = null;
		try {
			result = EGL14.eglCreateWindowSurface(mEglDisplay, mEglConfig, nativeWindow, surfaceAttribs, 0);
		} catch (IllegalArgumentException e) {
			Log.e(TAG, "eglCreateWindowSurface", e);
		}
		return result;
	}

	private void destroyWindowSurface(EGLSurface surface) {
		if (DEBUG) Log.v(TAG, "destroySurface:");

        if (surface != EGL14.EGL_NO_SURFACE) {
        	EGL14.eglMakeCurrent(mEglDisplay,
        		EGL14.EGL_NO_SURFACE, EGL14.EGL_NO_SURFACE, EGL14.EGL_NO_CONTEXT);
        	EGL14.eglDestroySurface(mEglDisplay, surface);
        }
        surface = EGL14.EGL_NO_SURFACE;
	}
	
    private void checkEglError(String msg) {
        int error;
        if ((error = EGL14.eglGetError()) != EGL14.EGL_SUCCESS) {
            throw new RuntimeException(msg + ": EGL error: 0x" + Integer.toHexString(error));
        }
    }

    private EGLConfig getConfig(boolean with_depth_buffer) {
        final int[] attribList = {
                EGL14.EGL_RENDERABLE_TYPE, EGL14.EGL_OPENGL_ES2_BIT,
                EGL14.EGL_RED_SIZE, 8,
                EGL14.EGL_GREEN_SIZE, 8,
                EGL14.EGL_BLUE_SIZE, 8,
                EGL14.EGL_ALPHA_SIZE, 8,
                //EGL14.EGL_STENCIL_SIZE, 8,
                EGL_RECORDABLE_ANDROID, 1,	// this flag need to recording of MediaCodec
				with_depth_buffer ? EGL14.EGL_DEPTH_SIZE : EGL14.EGL_NONE,
				with_depth_buffer ? 16 : 0,
                EGL14.EGL_NONE
        };
        final EGLConfig[] configs = new EGLConfig[1];
        final int[] numConfigs = new int[1];
        if (!EGL14.eglChooseConfig(mEglDisplay, attribList, 0, configs, 0, configs.length, numConfigs, 0)) {
        	// XXX it will be better to fallback to RGB565
            Log.w(TAG, "unable to find RGBA8888 / " + " EGLConfig");
            return null;
        }
        return configs[0];
    }
}
