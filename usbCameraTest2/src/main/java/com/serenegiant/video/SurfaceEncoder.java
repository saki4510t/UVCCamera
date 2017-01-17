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

package com.serenegiant.video;

import java.io.IOException;

import android.media.MediaCodec;
import android.media.MediaCodecInfo;
import android.media.MediaFormat;
import android.media.MediaMuxer;
import android.util.Log;
import android.view.Surface;

public class SurfaceEncoder extends Encoder {
	private static final boolean DEBUG = true;	// set false when releasing
	private static final String TAG = "SurfaceEncoder";

	private static final String MIME_TYPE = "video/avc";
	private static final int IFRAME_INTERVAL = 10;
	private static final int FRAME_WIDTH = 640;
	private static final int FRAME_HEIGHT = 480;
	private static final int CAPTURE_FPS = 15;
	private static final int BIT_RATE = 1000000;

	protected Surface mInputSurface;

	public SurfaceEncoder(final String filePath) {
		super();
		setOutputFile(filePath);
	}

	/**
	* Returns the encoder's input surface.
	*/
	public Surface getInputSurface() {
		return mInputSurface;
	}

	@Override
	public void prepare() throws IOException {
		if (DEBUG) Log.i(TAG, "prepare:");
		mTrackIndex = -1;
		mMuxerStarted = false;
		mIsCapturing = true;
		mIsEOS = false;

		final MediaCodecInfo codecInfo = selectCodec(MIME_TYPE);
		if (codecInfo == null) {
			Log.e(TAG, "Unable to find an appropriate codec for " + MIME_TYPE);
			return;
		}
		if (DEBUG) Log.i(TAG, "selected codec: " + codecInfo.getName());

		mBufferInfo = new MediaCodec.BufferInfo();
		final MediaFormat format = MediaFormat.createVideoFormat(MIME_TYPE, FRAME_WIDTH, FRAME_HEIGHT);

		// set configulation, invalid configulation crash app
		format.setInteger(MediaFormat.KEY_COLOR_FORMAT,
			MediaCodecInfo.CodecCapabilities.COLOR_FormatSurface);	// API >= 18
		format.setInteger(MediaFormat.KEY_BIT_RATE, BIT_RATE);
		format.setInteger(MediaFormat.KEY_FRAME_RATE, CAPTURE_FPS);
		format.setInteger(MediaFormat.KEY_I_FRAME_INTERVAL, IFRAME_INTERVAL);
		if (DEBUG) Log.i(TAG, "format: " + format);

		// create a MediaCodec encoder with specific configuration
		mMediaCodec = MediaCodec.createEncoderByType(MIME_TYPE);
		mMediaCodec.configure(format, null, null, MediaCodec.CONFIGURE_FLAG_ENCODE);
		// get Surface for input to encoder
		mInputSurface = mMediaCodec.createInputSurface();	// API >= 18
		mMediaCodec.start();

		// create MediaMuxer. You should never call #start here
		if (DEBUG) Log.i(TAG, "output will go to " + mOutputPath);
		mMuxer = new MediaMuxer(mOutputPath, MediaMuxer.OutputFormat.MUXER_OUTPUT_MPEG_4);

		if (mEncodeListener != null) {
			try {
				mEncodeListener.onPreapared(this);
			} catch (final Exception e) {
				Log.w(TAG, e);
			}
		}
	}

	/**
	* Releases encoder resources.
	*/
	@Override
	protected void release() {
		if (DEBUG) Log.i(TAG, "release:");
		super.release();
		if (mInputSurface != null) {
			mInputSurface.release();
			mInputSurface = null;
		}
	}

}
