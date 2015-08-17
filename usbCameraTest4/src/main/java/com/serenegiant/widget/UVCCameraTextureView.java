package com.serenegiant.widget;
/*
 * UVCCamera
 * library and sample to access to UVC web camera on non-rooted Android device
 *
 * Copyright (c) 2014-2015 saki t_saki@serenegiant.com
 *
 * File name: UVCCameraTextureView.java
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

import android.content.Context;
import android.graphics.SurfaceTexture;
import android.util.AttributeSet;
import android.util.Log;
import android.view.Surface;
import android.view.TextureView;

/**
 * change the view size with keeping the specified aspect ratio.
 * if you set this view with in a FrameLayout and set property "android:layout_gravity="center",
 * you can show this view in the center of screen and keep the aspect ratio of content
 * XXX it is better that can set the aspect raton a a xml property
 */
public class UVCCameraTextureView extends TextureView	// API >= 14
	implements TextureView.SurfaceTextureListener, CameraViewInterface {

	private static final boolean DEBUG = false;	// TODO set false on production
	private static final String TAG = "UVCCameraTextureView";

    private double mRequestedAspect = -1.0;		// initially use default window size

    private boolean mHasSurface;
	private Surface mPreviewSurface;
	private Callback mCallback;

	public UVCCameraTextureView(final Context context) {
		this(context, null, 0);
	}

	public UVCCameraTextureView(final Context context, final AttributeSet attrs) {
		this(context, attrs, 0);
	}

	public UVCCameraTextureView(final Context context, final AttributeSet attrs, final int defStyle) {
		super(context, attrs, defStyle);
		if (DEBUG) Log.v(TAG, "Constructor:");
		setSurfaceTextureListener(this);
	}

	@Override
	public void onSurfaceTextureAvailable(final SurfaceTexture surface, final int width, final int height) {
		if (DEBUG) Log.v(TAG, "onSurfaceTextureAvailable:");
		mHasSurface = true;
		if (mCallback != null) {
			mCallback.onSurfaceCreated(getSurface());
		}
	}

	@Override
	public void onSurfaceTextureSizeChanged(final SurfaceTexture surface, final int width, final int height) {
		if (DEBUG) Log.v(TAG, "onSurfaceTextureSizeChanged:");
		if (mCallback != null) {
			mCallback.onSurfaceChanged(getSurface(), width, height);
		}
	}

	@Override
	public boolean onSurfaceTextureDestroyed(final SurfaceTexture surface) {
		if (DEBUG) Log.v(TAG, "onSurfaceTextureDestroyed:");
		mHasSurface = false;
		if (mCallback != null) {
			mCallback.onSurfaceDestroy(getSurface());
		}
		if (mPreviewSurface != null) {
			mPreviewSurface.release();
			mPreviewSurface = null;
		}
		return true;
	}

	@Override
	public void onSurfaceTextureUpdated(final SurfaceTexture surface) {
	}

	@Override
	public void onResume() {
		if (DEBUG) Log.v(TAG, "onResume:");
	}

	@Override
	public void onPause() {
		if (DEBUG) Log.v(TAG, "onPause:");
		if (mPreviewSurface != null) {
			mPreviewSurface.release();
			mPreviewSurface = null;
		}
	}

	@Override
    public void setAspectRatio(final double aspectRatio) {
		if (DEBUG) Log.v(TAG, "setAspectRatio:");
        if (aspectRatio < 0) {
            throw new IllegalArgumentException();
        }
        if (mRequestedAspect != aspectRatio) {
            mRequestedAspect = aspectRatio;
            requestLayout();
        }
    }

    @Override
    protected void onMeasure(int widthMeasureSpec, int heightMeasureSpec) {
		if (DEBUG) Log.v(TAG, "onMeasure:");
		if (mRequestedAspect > 0) {
			int initialWidth = MeasureSpec.getSize(widthMeasureSpec);
			int initialHeight = MeasureSpec.getSize(heightMeasureSpec);

			final int horizPadding = getPaddingLeft() + getPaddingRight();
			final int vertPadding = getPaddingTop() + getPaddingBottom();
			initialWidth -= horizPadding;
			initialHeight -= vertPadding;

			final double viewAspectRatio = (double)initialWidth / initialHeight;
			final double aspectDiff = mRequestedAspect / viewAspectRatio - 1;

			if (Math.abs(aspectDiff) > 0.01) {
				if (aspectDiff > 0) {
					initialHeight = (int) (initialWidth / mRequestedAspect);
				} else {
					initialWidth = (int) (initialHeight * mRequestedAspect);
				}
				initialWidth += horizPadding;
				initialHeight += vertPadding;
				widthMeasureSpec = MeasureSpec.makeMeasureSpec(initialWidth, MeasureSpec.EXACTLY);
				heightMeasureSpec = MeasureSpec.makeMeasureSpec(initialHeight, MeasureSpec.EXACTLY);
			}
		}

        super.onMeasure(widthMeasureSpec, heightMeasureSpec);
    }

//--------------------------------------------------------------------------------
	@Override
	public boolean hasSurface() {
		return mHasSurface;
	}

	@Override
	public Surface getSurface() {
		if (DEBUG) Log.v(TAG, "getSurface:hasSurface=" + mHasSurface);
		if (mPreviewSurface == null) {
			final SurfaceTexture st = getSurfaceTexture();
			if (st != null)
				mPreviewSurface = new Surface(st);
		}
		return mPreviewSurface;
	}

	@Override
	public void setCallback(final Callback callback) {
		mCallback = callback;
	}
}
