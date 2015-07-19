package com.serenegiant.encoder;
/*
 * UVCCamera
 * library and sample to access to UVC web camera on non-rooted Android device
 *
 * Copyright (c) 2014-2015 saki t_saki@serenegiant.com
 *
 * File name: MediaVideoEncoder.java
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

import java.io.IOException;

import android.media.MediaCodec;
import android.media.MediaCodecInfo;
import android.media.MediaCodecList;
import android.media.MediaFormat;
import android.opengl.EGLContext;
import android.util.Log;
import android.view.Surface;

import com.serenegiant.glutils.RenderHandler;

public class MediaVideoEncoder extends MediaEncoder {
	private static final boolean DEBUG = true;	// TODO set false on release
	private static final String TAG = "MediaVideoEncoder";

	private static final String MIME_TYPE = "video/avc";
	// parameters for recording
	// VIDEO_WITH and VIDEO_HEIGHT should be same as the camera preview size.
    private static final int VIDEO_WIDTH = 640;
    private static final int VIDEO_HEIGHT = 480;
    private static final int FRAME_RATE = 15;
    private static final float BPP = 0.50f;

    private RenderHandler mRenderHandler;
    private Surface mSurface;

	public MediaVideoEncoder(final MediaMuxerWrapper muxer, final MediaEncoderListener listener) {
		super(muxer, listener);
		if (DEBUG) Log.i(TAG, "MediaVideoEncoder: ");
		mRenderHandler = RenderHandler.createHandler(TAG);
	}

	public boolean frameAvailableSoon(final float[] tex_matrix) {
		boolean result;
		if (result = super.frameAvailableSoon())
			mRenderHandler.draw(tex_matrix);
		return result;
	}

	@Override
	public boolean frameAvailableSoon() {
		boolean result;
		if (result = super.frameAvailableSoon())
			mRenderHandler.draw(null);
		return result;
	}

	@Override
	protected void prepare() throws IOException {
		if (DEBUG) Log.i(TAG, "prepare: ");
        mTrackIndex = -1;
        mMuxerStarted = mIsEOS = false;

        final MediaCodecInfo videoCodecInfo = selectVideoCodec(MIME_TYPE);
        if (videoCodecInfo == null) {
            Log.e(TAG, "Unable to find an appropriate codec for " + MIME_TYPE);
            return;
        }
		if (DEBUG) Log.i(TAG, "selected codec: " + videoCodecInfo.getName());

        final MediaFormat format = MediaFormat.createVideoFormat(MIME_TYPE, VIDEO_WIDTH, VIDEO_HEIGHT);
        format.setInteger(MediaFormat.KEY_COLOR_FORMAT, MediaCodecInfo.CodecCapabilities.COLOR_FormatSurface);	// API >= 18
        format.setInteger(MediaFormat.KEY_BIT_RATE, calcBitRate());
        format.setInteger(MediaFormat.KEY_FRAME_RATE, FRAME_RATE);
        format.setInteger(MediaFormat.KEY_I_FRAME_INTERVAL, 10);
		if (DEBUG) Log.i(TAG, "format: " + format);

        mMediaCodec = MediaCodec.createEncoderByType(MIME_TYPE);
        mMediaCodec.configure(format, null, null, MediaCodec.CONFIGURE_FLAG_ENCODE);
        // get Surface for encoder input
        // this method only can call between #configure and #start
        mSurface = mMediaCodec.createInputSurface();	// API >= 18
        mMediaCodec.start();
        if (DEBUG) Log.i(TAG, "prepare finishing");
        if (mListener != null) {
        	try {
        		mListener.onPrepared(this);
        	} catch (final Exception e) {
        		Log.e(TAG, "prepare:", e);
        	}
        }
	}

	public void setEglContext(final EGLContext shared_context, final int tex_id) {
		mRenderHandler.setEglContext(shared_context, tex_id, mSurface, true);
	}

	@Override
    protected void release() {
		if (DEBUG) Log.i(TAG, "release:");
		if (mSurface != null) {
			mSurface.release();
			mSurface = null;
		}
		if (mRenderHandler != null) {
			mRenderHandler.release();
			mRenderHandler = null;
		}
		super.release();
	}

	private int calcBitRate() {
		final int bitrate = (int)(BPP * FRAME_RATE * VIDEO_WIDTH * VIDEO_HEIGHT);
		Log.i(TAG, String.format("bitrate=%5.2f[Mbps]", bitrate / 1024f / 1024f));
		return bitrate;
	}

    /**
     * select the first codec that match a specific MIME type
     * @param mimeType
     * @return null if no codec matched
     */
    @SuppressWarnings("deprecation")
    protected static final MediaCodecInfo selectVideoCodec(final String mimeType) {
    	if (DEBUG) Log.v(TAG, "selectVideoCodec:");

    	// get the list of available codecs
		final int numCodecs = MediaCodecList.getCodecCount();
        for (int i = 0; i < numCodecs; i++) {
        	final MediaCodecInfo codecInfo = MediaCodecList.getCodecInfoAt(i);

            if (!codecInfo.isEncoder()) {	// skipp decoder
                continue;
            }
            // select first codec that match a specific MIME type and color format
            final String[] types = codecInfo.getSupportedTypes();
            for (int j = 0; j < types.length; j++) {
                if (types[j].equalsIgnoreCase(mimeType)) {
                	if (DEBUG) Log.i(TAG, "codec:" + codecInfo.getName() + ",MIME=" + types[j]);
            		final int format = selectColorFormat(codecInfo, mimeType);
                	if (format > 0) {
                		return codecInfo;
                	}
                }
            }
        }
        return null;
    }

    /**
     * select color format available on specific codec and we can use.
     * @return 0 if no colorFormat is matched
     */
    protected static final int selectColorFormat(final MediaCodecInfo codecInfo, final String mimeType) {
		if (DEBUG) Log.i(TAG, "selectColorFormat: ");
    	int result = 0;
    	final MediaCodecInfo.CodecCapabilities caps;
    	try {
    		Thread.currentThread().setPriority(Thread.MAX_PRIORITY);
    		caps = codecInfo.getCapabilitiesForType(mimeType);
    	} finally {
    		Thread.currentThread().setPriority(Thread.NORM_PRIORITY);
    	}
        int colorFormat;
        for (int i = 0; i < caps.colorFormats.length; i++) {
        	colorFormat = caps.colorFormats[i];
            if (isRecognizedViewoFormat(colorFormat)) {
            	if (result == 0)
            		result = colorFormat;
                break;
            }
        }
        if (result == 0)
        	Log.e(TAG, "couldn't find a good color format for " + codecInfo.getName() + " / " + mimeType);
        return result;
    }

	/**
	 * color formats that we can use in this class
	 */
    protected static int[] recognizedFormats;
	static {
		recognizedFormats = new int[] {
//        	MediaCodecInfo.CodecCapabilities.COLOR_FormatYUV420Planar,
//        	MediaCodecInfo.CodecCapabilities.COLOR_FormatYUV420SemiPlanar,
//        	MediaCodecInfo.CodecCapabilities.COLOR_QCOM_FormatYUV420SemiPlanar,
        	MediaCodecInfo.CodecCapabilities.COLOR_FormatSurface,
		};
	}

    private static final boolean isRecognizedViewoFormat(final int colorFormat) {
		if (DEBUG) Log.i(TAG, "isRecognizedViewoFormat:colorFormat=" + colorFormat);
    	final int n = recognizedFormats != null ? recognizedFormats.length : 0;
    	for (int i = 0; i < n; i++) {
    		if (recognizedFormats[i] == colorFormat) {
    			return true;
    		}
    	}
    	return false;
    }

}
