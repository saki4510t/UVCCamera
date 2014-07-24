package com.serenegiant.video;
/*
 * UVCCamera
 * library and sample to access to UVC web camera on non-rooted Android device
 * 
 * Copyright (c) 2014 saki t_saki@serenegiant.com
 * 
 * File name: SurfaceEncoder.java
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

import java.io.IOException;
import java.nio.ByteBuffer;

import android.media.MediaCodec;
import android.media.MediaCodecInfo;
import android.media.MediaFormat;
import android.media.MediaMuxer;
import android.util.Log;
import android.view.Surface;

public class SurfaceEncoder extends Encoder {
	private static final boolean DEBUG = false;	// set false when releasing
	private static final String TAG = "SurfaceEncoder";

	private static final String MIME_TYPE = "video/avc";
	private static final int IFRAME_INTERVAL = 10;
	private static final int TIMEOUT_USEC = 10000;	// 10ミリ秒
	private static final int FRAME_WIDTH = 640;
	private static final int FRAME_HEIGHT = 480;
	private static final int CAPTURE_FPS = 15;
	private static final int BIT_RATE = 1000000;

	protected Surface mInputSurface;
	protected MediaMuxer mMuxer;				// API >= 18
	protected int mTrackIndex;
	private boolean mIsEOS;

	public SurfaceEncoder(String filePath) {
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
		mCodec = MediaCodec.createEncoderByType(MIME_TYPE);
		mCodec.configure(format, null, null, MediaCodec.CONFIGURE_FLAG_ENCODE);
		// get Surface for input to encoder
		mInputSurface = mCodec.createInputSurface();	// API >= 18
		mCodec.start();

		// create MediaMuxer. You should never call #start here
		if (DEBUG) Log.i(TAG, "output will go to " + mOutputPath);
		mMuxer = new MediaMuxer(mOutputPath, MediaMuxer.OutputFormat.MUXER_OUTPUT_MPEG_4);

		if (mEncodeListener != null) {
			try {
				mEncodeListener.onPreapared(this);
			} catch (Exception e) {
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
		if (mEncodeListener != null) {
			try {
				mEncodeListener.onRelease(this);
			} catch (Exception e) {
				if (DEBUG) Log.w(TAG, e);
			}
		}
		if (mInputSurface != null) {
			mInputSurface.release();
			mInputSurface = null;
		}
		if (mCodec != null) {
			mCodec.stop();
			mCodec.release();
			mCodec = null;
		}
		if (mMuxer != null) {
			if (mMuxerStarted)
				mMuxer.stop();
			mMuxer.release();
			mMuxer = null;
		}
	}

	@Override
	protected void signalEndOfInputStream() {
		if (DEBUG) Log.i(TAG, "signalEndOfInputStream:");
		mIsEOS = true;
		mCodec.signalEndOfInputStream();	// API >= 18
	}

	@Override
	protected void drain() {
		if (DEBUG) Log.i(TAG, "drain:");
		ByteBuffer[] encoderOutputBuffers = mCodec.getOutputBuffers();
		int encoderStatus, count = 0;
LOOP:	while (mIsCapturing) {
			encoderStatus = mCodec.dequeueOutputBuffer(mBufferInfo, TIMEOUT_USEC);	// wait maximum TIMEOUT_USEC(10 msec)
			if (encoderStatus == MediaCodec.INFO_TRY_AGAIN_LATER) {
				if (DEBUG) Log.i(TAG, "INFO_TRY_AGAIN_LATER:count=" + count);
				// when no output available
				if (!mIsEOS) {
					if (++count > 5)	// TIMEOUT_USEC x 5 = 50msec
						break LOOP;
				}
			} else if (encoderStatus == MediaCodec.INFO_OUTPUT_BUFFERS_CHANGED) {
				// not expected for an encoder
				encoderOutputBuffers = mCodec.getOutputBuffers();
				if (DEBUG) Log.i(TAG, "INFO_OUTPUT_BUFFERS_CHANGED");
			} else if (encoderStatus == MediaCodec.INFO_OUTPUT_FORMAT_CHANGED) {
				if (DEBUG) Log.i(TAG, "INFO_OUTPUT_FORMAT_CHANGED");
				// this should come only once before receiving output buffers
				if (mMuxerStarted) {
					throw new RuntimeException("format changed twice");
				}
				final MediaFormat newFormat = mCodec.getOutputFormat();	// API >= 16
				mTrackIndex = mMuxer.addTrack(newFormat);
				mMuxer.start();
				mMuxerStarted = true;
			} else if (encoderStatus < 0) {
				// skip when unexpected result came from #dequeueOutputBuffer
				if (DEBUG) Log.i(TAG, "unexpected result came from #dequeueOutputBuffer");
			} else {
				if (DEBUG) Log.i(TAG, "encoderStatus=" + encoderStatus);
				final ByteBuffer encodedData = encoderOutputBuffers[encoderStatus];
				if (encodedData == null) {
					throw new RuntimeException("encoderOutputBuffer[" + encoderStatus + "] was null");
				}

				if ((mBufferInfo.flags & MediaCodec.BUFFER_FLAG_CODEC_CONFIG) != 0) {
					// the configulation is already set to muxer when INFO_OUTPUT_FORMAT_CHANGED come.
					// ignore this.
					mBufferInfo.size = 0;
				}

				if (mBufferInfo.size > 0) {
					count = 0;
					if (!mMuxerStarted) {
						throw new RuntimeException("muxer hasn't started");
					}

					// adjust the ByteBuffer values to match BufferInfo(this is not necessary for current implementation of MediaMuxer)
					encodedData.position(mBufferInfo.offset);
					encodedData.limit(mBufferInfo.offset + mBufferInfo.size);

					mMuxer.writeSampleData(mTrackIndex, encodedData, mBufferInfo);
				}

				mCodec.releaseOutputBuffer(encoderStatus, false);

				if ((mBufferInfo.flags & MediaCodec.BUFFER_FLAG_END_OF_STREAM) != 0) {
					// when EOS came
					if (!mIsEOS) {
						if (DEBUG) Log.w(TAG, "reached end of stream unexpectedly");
					}
					mIsCapturing = false;
					break LOOP;
				}
			}
		}
	}
}
