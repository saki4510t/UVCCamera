package com.serenegiant.video;
/*
 * UVCCamera
 * library and sample to access to UVC web camera on non-rooted Android device
 * 
 * Copyright (c) 2014 saki t_saki@serenegiant.com
 * 
 * File name: Encoder.java
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
import java.lang.ref.WeakReference;

import android.media.MediaCodec;
import android.media.MediaCodecInfo;
import android.media.MediaCodecList;
import android.os.Handler;
import android.os.Looper;
import android.os.Message;
import android.util.Log;

public abstract class Encoder {
	private static final String TAG = "Encoder";

    protected EncodeListener mEncodeListener;   
    protected volatile boolean mMuxerStarted;
	protected volatile boolean mIsCapturing;
    protected MediaCodec.BufferInfo mBufferInfo;	// API >= 16(Android4.1.2)
    protected MediaCodec mCodec;					// API >= 16(Android4.1.2)
	protected String mOutputPath;
 
    private static final int MSG_FRAME_AVAILABLE = 1;
    private static final int MSG_SEND_EOS = 2;
    private static final int MSG_STOP_RECORDING = 3;

    private EncoderThread mEncoderThread;
    
    public interface EncodeListener {
    	/**
    	 * callback after finishing initialization of encoder
    	 * @param encoder
    	 */
    	public void onPreapared(Encoder encoder);
    	/**
    	 * callback before releasing encoder
    	 * @param encoder
    	 */
    	public void onRelease(Encoder encoder);
    }

//********************************************************************************
    public Encoder() {
    	// create and start encoder thread
    	mEncoderThread = new EncoderThread(this);
    	mEncoderThread.start();
    }
    
//********************************************************************************
    public boolean isCapturing() {
    	return mIsCapturing;
    }
    
    /**
     * request stop encoding
     */
    public void stopRecording() {
    	final EncoderHandler handler = mEncoderThread.getHandler();
    	// remove drain request in the queue.
    	handler.removeMessages(MSG_FRAME_AVAILABLE);
    	// request drain onece to signalEndOfInputStream
        handler.sendMessage(handler.obtainMessage(MSG_FRAME_AVAILABLE));
        // request sending EOS
        handler.sendMessage(handler.obtainMessage(MSG_SEND_EOS));
        // request stop and release encoder
        handler.sendMessage(handler.obtainMessage(MSG_STOP_RECORDING));
    }

    /**
     * notify to frame data will arrive soon or already arrived.
     * (request to process frame data)
     */
    public void frameAvailable() {
    	final EncoderHandler handler = mEncoderThread.getHandler();
    	handler.sendMessage(handler.obtainMessage(MSG_FRAME_AVAILABLE));
    }

//********************************************************************************
    private static final class EncoderThread extends Thread {
    	private final Object mReadySync = new Object();
        private final WeakReference<Encoder> mWeakEncoder;
    	private EncoderHandler mHandler;
        private boolean mReady;
        
        public EncoderThread(Encoder encoder) {
        	mWeakEncoder = new 	WeakReference<Encoder>(encoder);
        }
        @Override
        public void run() {
            // Prepare Looper and create Hander to access to this thread
            Looper.prepare();
            synchronized (mReadySync) {
                mHandler = new EncoderHandler(mWeakEncoder.get());
                mReady = true;
                mReadySync.notify();
            }
            Looper.loop();

            synchronized (mReadySync) {
                mReady = false;
                mHandler = null;
            }
        }

        public EncoderHandler getHandler() {
            synchronized (mReadySync) {
                while (!mReady) {
                    try {
                        mReadySync.wait();
                    } catch (InterruptedException e) {
                        // ignore
                    }
                }
            }
        	return mHandler;
        }
    }

    /**
     * Message handler class of asynchronusly message processing for encoder thread
     * all messages are processed on encoder thread
     */
    private static final class EncoderHandler extends Handler {
        private final WeakReference<Encoder> mWeakEncoder;

        public EncoderHandler(Encoder encoder) {
            mWeakEncoder = new WeakReference<Encoder>(encoder);
        }

        @Override
        public void handleMessage(Message inputMessage) {

            final Encoder encoder = mWeakEncoder.get();
            if (encoder == null) {
                Log.w(TAG, "EncoderHandler.handleMessage: encoder is null");
                return;
            }

            switch (inputMessage.what) {
                case MSG_FRAME_AVAILABLE:
                    encoder.drain();
                    break;
                case MSG_SEND_EOS:
                	encoder.signalEndOfInputStream();
                	break;
                case MSG_STOP_RECORDING:
                    encoder.drain();
                    encoder.release();
                    Looper.myLooper().quit();
                    break;
                default:
                    throw new RuntimeException("Unhandled msg what=" + inputMessage.what);
            }
        }
    }

//********************************************************************************
    public void setOutputFile(String filePath) {
		mOutputPath = filePath;
	}
	
	public abstract void prepare()  throws IOException;
	protected abstract void release();
	protected abstract void drain();
	protected abstract void signalEndOfInputStream();

	public void setEncodeListener(EncodeListener listener) {
		mEncodeListener = listener;
	}

    /**
     * select primary codec for encoding from the available list which MIME is specific type
     * return null if nothing is available
     * @param mimeType
     */
    public static final MediaCodecInfo selectCodec(String mimeType) {
    	MediaCodecInfo result = null;

    	// get avcodec list
        final int numCodecs = MediaCodecList.getCodecCount();
LOOP:	for (int i = 0; i < numCodecs; i++) {
        	final MediaCodecInfo codecInfo = MediaCodecList.getCodecInfoAt(i);

            if (!codecInfo.isEncoder()) {	// skip decoder
                continue;
            }

            // select encoder that MIME is equal to the specific type
            final String[] types = codecInfo.getSupportedTypes();
            for (int j = 0; j < types.length; j++) {
                if (types[j].equalsIgnoreCase(mimeType)) {
               		result = codecInfo;
               		break LOOP;
                }
            }
        }
        return result;
    }

}
