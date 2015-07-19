package com.serenegiant.encoder;
/*
 * UVCCamera
 * library and sample to access to UVC web camera on non-rooted Android device
 *
 * Copyright (c) 2014-2015 saki t_saki@serenegiant.com
 *
 * File name: MediaEncoder.java
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
import java.lang.ref.WeakReference;
import java.nio.ByteBuffer;

import android.media.MediaCodec;
import android.media.MediaFormat;
import android.os.Handler;
import android.os.Looper;
import android.os.Message;
import android.util.Log;

public abstract class MediaEncoder implements Runnable {
	private static final boolean DEBUG = true;	// TODO set false on release
	private static final String TAG = "MediaEncoder";

	protected static final int TIMEOUT_USEC = 10000;	// 10[msec]
	protected static final int MSG_FRAME_AVAILABLE = 1;
	protected static final int MSG_STOP_RECORDING = 9;

	public interface MediaEncoderListener {
		public void onPrepared(MediaEncoder encoder);
		public void onStopped(MediaEncoder encoder);
	}

	protected final Object mSync = new Object();
	/**
	 * Flag that indicate this encoder is capturing now.
	 */
    protected volatile boolean mIsCapturing;
    /**
     * Flag to request stop capturing
     */
    protected volatile boolean mRequestStop;
    /**
     * Flag that indicate encoder received EOS(End Of Stream)
     */
    protected boolean mIsEOS;
    /**
     * Flag the indicate the muxer is running
     */
    protected boolean mMuxerStarted;
    /**
     * Track Number
     */
    protected int mTrackIndex;
    /**
     * MediaCodec instance for encoding
     */
    protected MediaCodec mMediaCodec;				// API >= 16(Android4.1.2)
    /**
     * Weak refarence of MediaMuxerWarapper instance
     */
    protected final WeakReference<MediaMuxerWrapper> mWeakMuxer;
    /**
     * BufferInfo instance for dequeuing
     */
    private MediaCodec.BufferInfo mBufferInfo;		// API >= 16(Android4.1.2)
    /**
     * Handler of encoding thread
     */
    private EncoderHandler mHandler;
    protected final MediaEncoderListener mListener;

    public MediaEncoder(final MediaMuxerWrapper muxer, final MediaEncoderListener listener) {
    	if (listener == null) throw new NullPointerException("MediaEncoderListener is null");
    	if (muxer == null) throw new NullPointerException("MediaMuxerWrapper is null");
		mWeakMuxer = new WeakReference<MediaMuxerWrapper>(muxer);
		muxer.addEncoder(this);
		mListener = listener;
        synchronized (mSync) {
            // create BufferInfo here for effectiveness(to reduce GC)
            mBufferInfo = new MediaCodec.BufferInfo();
            // wait for Handler is ready
            new Thread(this, getClass().getSimpleName()).start();
            try {
            	mSync.wait();
            } catch (final InterruptedException e) {
            }
        }
	}

    public String getOutputPath() {
    	final MediaMuxerWrapper muxer = mWeakMuxer.get();
    	return muxer != null ? muxer.getOutputPath() : null;
    }

    /**
     * the method to indicate frame data is soon available or already available
     * @return return true if encoder is ready to encod.
     */
    public boolean frameAvailableSoon() {
//    	if (DEBUG) Log.v(TAG, "frameAvailableSoon");
        synchronized (mSync) {
            if (!mIsCapturing || mRequestStop) {
                return false;
            }
            mHandler.sendEmptyMessage(MSG_FRAME_AVAILABLE);
        }
        return true;
    }

    /**
     * Message loop for encoding thread
     * Prepare Looper/Handler and execute message loop and wait terminating.
     */
	@Override
	public void run() {
        // create Looper and Handler to access to this thread
    	Looper.prepare();
        synchronized (mSync) {
            mHandler = new EncoderHandler(this);
            mRequestStop = false;
            mSync.notify();
        }
        Looper.loop();

		if (DEBUG) Log.d(TAG, "Encoder thread exiting");
        synchronized (mSync) {
            mIsCapturing = false;
            mRequestStop = true;
            mHandler = null;
        }
	}

	/*
    * prepareing method for each sub class
    * this method should be implemented in sub class, so set this as abstract method
    * @throws IOException
    */
   /*package*/ abstract void prepare() throws IOException;

	/*package*/ void startRecording() {
   	if (DEBUG) Log.v(TAG, "startRecording");
		synchronized (mSync) {
			mIsCapturing = true;
			mRequestStop = false;
			mSync.notifyAll();
		}
	}

   /**
    * the method to request stop encoding
    */
	/*package*/ void stopRecording() {
		if (DEBUG) Log.v(TAG, "stopRecording");
		synchronized (mSync) {
			if (!mIsCapturing || mRequestStop) {
				return;
			}
			mRequestStop = true;	// for rejecting newer frame
			mSync.notifyAll();
            mHandler.removeMessages(MSG_FRAME_AVAILABLE);
	        // request endoder handler to stop encoding
	        mHandler.sendEmptyMessage(MSG_STOP_RECORDING);
	        // We can not know when the encoding and writing finish.
	        // so we return immediately after request to avoid delay of caller thread
		}
	}

//********************************************************************************
//********************************************************************************
    /**
     * Method to request stop recording
     * this method is called from message hander of EncoderHandler
     */
    private final void handleStopRecording() {
		if (DEBUG) Log.d(TAG, "handleStopRecording");
		// process all available output data
    	drain();
    	// request stop recording
        signalEndOfInputStream();
        // process output data again for EOS signale
        drain();
        // release all related objects
        release();
    }

    /**
     * Release all releated objects
     */
    protected void release() {
		if (DEBUG) Log.d(TAG, "release:");
		try {
			mListener.onStopped(this);
		} catch (final Exception e) {
			Log.e(TAG, "failed onStopped", e);
		}
		mIsCapturing = false;
        if (mMediaCodec != null) {
			try {
	            mMediaCodec.stop();
	            mMediaCodec.release();
	            mMediaCodec = null;
			} catch (final Exception e) {
				Log.e(TAG, "failed releasing MediaCodec", e);
			}
        }
        if (mMuxerStarted) {
       		final MediaMuxerWrapper muxer = mWeakMuxer.get();
       		if (muxer != null) {
       			try {
           			muxer.stop();
    			} catch (final Exception e) {
    				Log.e(TAG, "failed stopping muxer", e);
    			}
       		}
        }
        mBufferInfo = null;
    }

    protected void signalEndOfInputStream() {
		if (DEBUG) Log.d(TAG, "sending EOS to encoder");
        // signalEndOfInputStream is only avairable for video encoding with surface
        // and equivalent sending a empty buffer with BUFFER_FLAG_END_OF_STREAM flag.
//		mMediaCodec.signalEndOfInputStream();	// API >= 18
        encode(null, 0, getPTSUs());
	}

    /**
     * Method to set byte array to the MediaCodec encoder
     * @param buffer
     * @param length　length of byte array, zero means EOS.
     * @param presentationTimeUs
     */
    protected void encode(final byte[] buffer, final int length, final long presentationTimeUs) {
    	if (!mIsCapturing) return;
    	int ix = 0, sz;
        final ByteBuffer[] inputBuffers = mMediaCodec.getInputBuffers();
        while (mIsCapturing && ix < length) {
	        final int inputBufferIndex = mMediaCodec.dequeueInputBuffer(TIMEOUT_USEC);
	        if (inputBufferIndex >= 0) {
	            final ByteBuffer inputBuffer = inputBuffers[inputBufferIndex];
	            inputBuffer.clear();
	            sz = inputBuffer.remaining();
	            sz = (ix + sz < length) ? sz : length - ix;
	            if (sz > 0 && (buffer != null)) {
	            	inputBuffer.put(buffer, ix, sz);
	            }
	            ix += sz;
//	            if (DEBUG) Log.v(TAG, "encode:queueInputBuffer");
	            if (length <= 0) {
	            	// send EOS
	            	mIsEOS = true;
	            	if (DEBUG) Log.i(TAG, "send BUFFER_FLAG_END_OF_STREAM");
	            	mMediaCodec.queueInputBuffer(inputBufferIndex, 0, 0,
	            		presentationTimeUs, MediaCodec.BUFFER_FLAG_END_OF_STREAM);
		            break;
	            } else {
	            	mMediaCodec.queueInputBuffer(inputBufferIndex, 0, sz,
	            		presentationTimeUs, 0);
	            }
	        } else if (inputBufferIndex == MediaCodec.INFO_TRY_AGAIN_LATER) {
	        	// wait for MediaCodec encoder is ready to encode
	        	// nothing to do here because MediaCodec#dequeueInputBuffer(TIMEOUT_USEC)
	        	// will wait for maximum TIMEOUT_USEC(10msec) on each call
	        }
        }
    }

    /**
     * drain encoded data and write them to muxer
     */
    protected void drain() {
    	if (mMediaCodec == null) return;
        ByteBuffer[] encoderOutputBuffers = mMediaCodec.getOutputBuffers();
        int encoderStatus, count = 0;
        final MediaMuxerWrapper muxer = mWeakMuxer.get();
        if (muxer == null) {
//        	throw new NullPointerException("muxer is unexpectedly null");
        	Log.w(TAG, "muxer is unexpectedly null");
        	return;
        }
LOOP:	while (mIsCapturing) {
			// get encoded data with maximum timeout duration of TIMEOUT_USEC(=10[msec])
            encoderStatus = mMediaCodec.dequeueOutputBuffer(mBufferInfo, TIMEOUT_USEC);
            if (encoderStatus == MediaCodec.INFO_TRY_AGAIN_LATER) {
                // wait 5 counts(=TIMEOUT_USEC x 5 = 50msec) until data/EOS come
                if (!mIsEOS) {
                	if (++count > 5)
                		break LOOP;		// out of while
                }
            } else if (encoderStatus == MediaCodec.INFO_OUTPUT_BUFFERS_CHANGED) {
            	if (DEBUG) Log.v(TAG, "INFO_OUTPUT_BUFFERS_CHANGED");
                // this shoud not come when encoding
                encoderOutputBuffers = mMediaCodec.getOutputBuffers();
            } else if (encoderStatus == MediaCodec.INFO_OUTPUT_FORMAT_CHANGED) {
            	if (DEBUG) Log.v(TAG, "INFO_OUTPUT_FORMAT_CHANGED");
            	// this status indicate the output format of codec is changed
                // this should come only once before actual encoded data
            	// but this status never come on Android4.3 or less
            	// and in that case, you should treat when MediaCodec.BUFFER_FLAG_CODEC_CONFIG come.
                if (mMuxerStarted) {	// second time request is error
                    throw new RuntimeException("format changed twice");
                }
				// get output format from codec and pass them to muxer
				// getOutputFormat should be called after INFO_OUTPUT_FORMAT_CHANGED otherwise crash.
                final MediaFormat format = mMediaCodec.getOutputFormat(); // API >= 16
               	mTrackIndex = muxer.addTrack(format);
               	mMuxerStarted = true;
               	if (!muxer.start()) {
               		// we should wait until muxer is ready
               		synchronized (muxer) {
	               		while (!muxer.isStarted())
						try {
							muxer.wait(100);
						} catch (final InterruptedException e) {
							break LOOP;
						}
               		}
               	}
            } else if (encoderStatus < 0) {
            	// unexpected status
            	if (DEBUG) Log.w(TAG, "drain:unexpected result from encoder#dequeueOutputBuffer: " + encoderStatus);
            } else {
                final ByteBuffer encodedData = encoderOutputBuffers[encoderStatus];
                if (encodedData == null) {
                	// this never should come...may be a MediaCodec internal error
                    throw new RuntimeException("encoderOutputBuffer " + encoderStatus + " was null");
                }
                if ((mBufferInfo.flags & MediaCodec.BUFFER_FLAG_CODEC_CONFIG) != 0) {
                	// You shoud set output format to muxer here when you target Android4.3 or less
                	// but MediaCodec#getOutputFormat can not call here(because INFO_OUTPUT_FORMAT_CHANGED don't come yet)
                	// therefor we should expand and prepare output format from buffer data.
                	// This sample is for API>=18(>=Android 4.3), just ignore this flag here
					if (DEBUG) Log.d(TAG, "drain:BUFFER_FLAG_CODEC_CONFIG");
					mBufferInfo.size = 0;
                }

                if (mBufferInfo.size != 0) {
                	// encoded data is ready, clear waiting counter
            		count = 0;
                    if (!mMuxerStarted) {
                    	// muxer is not ready...this will prrograming failure.
                        throw new RuntimeException("drain:muxer hasn't started");
                    }
                    // write encoded data to muxer(need to adjust presentationTimeUs.
                   	mBufferInfo.presentationTimeUs = getPTSUs();
                   	muxer.writeSampleData(mTrackIndex, encodedData, mBufferInfo);
					prevOutputPTSUs = mBufferInfo.presentationTimeUs;
                }
                // return buffer to encoder
                mMediaCodec.releaseOutputBuffer(encoderStatus, false);
                if ((mBufferInfo.flags & MediaCodec.BUFFER_FLAG_END_OF_STREAM) != 0) {
                	// when EOS come.
               		mMuxerStarted = mIsCapturing = false;
                    break;      // out of while
                }
            }
        }
    }

    /**
     * previous presentationTimeUs for writing
     */
	private long prevOutputPTSUs = 0;
	/**
	 * get next encoding presentationTimeUs
	 * @return
	 */
    protected long getPTSUs() {
		long result = System.nanoTime() / 1000L;
		// presentationTimeUs should be monotonic
		// otherwise muxer fail to write
		if (result < prevOutputPTSUs)
			result = (prevOutputPTSUs - result) + result;
		return result;
    }

    /**
     * Handler class to handle the asynchronous request to encoder thread
     */
    private static final class EncoderHandler extends Handler {
        private final WeakReference<MediaEncoder> mWeakEncoder;

        public EncoderHandler(final MediaEncoder encoder) {
            mWeakEncoder = new WeakReference<MediaEncoder>(encoder);
        }

        /**
         * message handler
         */
        @Override
        public void handleMessage(final Message inputMessage) {
            final int what = inputMessage.what;
            final MediaEncoder encoder = mWeakEncoder.get();
            if (encoder == null) {
                Log.w(TAG, "EncoderHandler#handleMessage: encoder is null");
                return;
            }
            switch (what) {
                case MSG_FRAME_AVAILABLE:
               		encoder.drain();
                    break;
                case MSG_STOP_RECORDING:
                    encoder.handleStopRecording();
                    Looper.myLooper().quit();
                    break;
                default:
                    throw new RuntimeException("unknown message what=" + what);
            }
        }
    }

}
