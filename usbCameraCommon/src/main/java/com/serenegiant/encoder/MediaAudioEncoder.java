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

package com.serenegiant.encoder;

import android.content.Context;
import android.media.AudioFormat;
import android.media.AudioRecord;
import android.media.MediaCodec;
import android.media.MediaCodecInfo;
import android.media.MediaCodecList;
import android.media.MediaFormat;
import android.media.MediaRecorder;
import android.util.Log;

import com.serenegiant.utils.Constants;

import org.easydarwin.push.EasyPusher;
import org.easydarwin.push.InitCallback;

import java.io.IOException;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;

public class MediaAudioEncoder extends MediaEncoder implements IAudioEncoder {
	private static final String TAG = "MediaAudioEncoder";

	private static final String MIME_TYPE = "audio/mp4a-latm";	//aac
//    private static final int SAMPLE_RATE = 44100;	// 44.1[KHz] is only setting guaranteed to be available on all devices.
    private static final int SAMPLE_RATE = 8000;	//采样率
//    private static final int BIT_RATE = 64000;
    private static final int BIT_RATE = 8000;		//码率
    private static final int CHANNEL_COUNT = 1;		//声道数， 1:单声道, 2:双声道
    private static final int AUDIO_FORMAT = AudioFormat.ENCODING_PCM_16BIT;		//该系统只支持16big

	private static final int SAMPLES_PER_FRAME = 1024;	// AAC, bytes/frame/channel, 不可改变
	private static final int FRAMES_PER_BUFFER = 25; 	// AAC, frame/buffer/sec

    private AudioThread mAudioThread = null;
	private PushThread mPushThread = null;

	private EasyPusher easyPusher;

	protected MediaCodec.BufferInfo mBufferInfo = new MediaCodec.BufferInfo();
	protected ByteBuffer[] mBuffers = null;


	public MediaAudioEncoder(final MediaMuxerWrapper muxer, final MediaEncoderListener listener) {
		super(muxer, listener);
	}

	/**
	 * 初始化easypusher
	 */
	private void initEasyPusher(Context activity, String videoIp, int tcpPort, int cameraId) {
		easyPusher = new EasyPusher();
		String id = "107700000088_" + cameraId;
		Log.w(TAG, "推流: url: " + String.format("rtsp://%s:%s/%s.sdp", videoIp, tcpPort, id));
		easyPusher.initPush(videoIp, tcpPort + "", String.format("%s.sdp", id), Constants.KEY_EASYPUSHER,
				activity, new InitCallback() {
					@Override
					public void onCallback(int code) {
						Log.w(TAG, "推流onCallback: " + code);
						switch (code) {
							case EasyPusher.OnInitPusherCallback.CODE.EASY_ACTIVATE_INVALID_KEY:
								Log.w(TAG, "推流无效Key");
								break;
							case EasyPusher.OnInitPusherCallback.CODE.EASY_ACTIVATE_SUCCESS:
								Log.w(TAG, "推流激活成功");
								break;
							case EasyPusher.OnInitPusherCallback.CODE.EASY_PUSH_STATE_CONNECTING:
								Log.w(TAG, "推流连接中");
								break;
							case EasyPusher.OnInitPusherCallback.CODE.EASY_PUSH_STATE_CONNECTED:
								Log.w(TAG, "推流连接成功");
								break;
							case EasyPusher.OnInitPusherCallback.CODE.EASY_PUSH_STATE_CONNECT_FAILED:
								Log.w(TAG, "推流连接失败");
								break;
							case EasyPusher.OnInitPusherCallback.CODE.EASY_PUSH_STATE_CONNECT_ABORT:
								Log.w(TAG, "推流连接异常中断");
								break;
							case EasyPusher.OnInitPusherCallback.CODE.EASY_PUSH_STATE_PUSHING:
								Log.w(TAG, "推流推流中");
									/*if (!isPushing) {
										isPushing = true;
									}*/
								break;
							case EasyPusher.OnInitPusherCallback.CODE.EASY_PUSH_STATE_DISCONNECTED:
								Log.w(TAG, "推流断开连接");
								break;
							case EasyPusher.OnInitPusherCallback.CODE.EASY_ACTIVATE_PLATFORM_ERR:
								Log.w(TAG, "推流平台不匹配");
								break;
							case EasyPusher.OnInitPusherCallback.CODE.EASY_ACTIVATE_COMPANY_ID_LEN_ERR:
								Log.w(TAG, "推流断授权使用商不匹配");
								break;
							case EasyPusher.OnInitPusherCallback.CODE.EASY_ACTIVATE_PROCESS_NAME_LEN_ERR:
								Log.w(TAG, "推流进程名称长度不匹配");
								break;
						}
					}
				});
	}

	@Override
	protected void prepare(Context activity) throws IOException {
		Log.w(TAG, "prepare:");

		initEasyPusher(activity, Constants.PUSHER_ADDR, Constants.PUSHER_PORT, 11);

        mTrackIndex = -1;
        mMuxerStarted = mIsEOS = false;
        // prepare MediaCodec for AAC encoding of audio data from inernal mic.
        final MediaCodecInfo audioCodecInfo = selectAudioCodec(MIME_TYPE);
        if (audioCodecInfo == null) {
            Log.e(TAG, "Unable to find an appropriate codec for " + MIME_TYPE);
            return;
        }
		Log.w(TAG, "selected codec: " + audioCodecInfo.getName());

        final MediaFormat audioFormat = MediaFormat.createAudioFormat(MIME_TYPE, SAMPLE_RATE, CHANNEL_COUNT);
		audioFormat.setInteger(MediaFormat.KEY_AAC_PROFILE, MediaCodecInfo.CodecProfileLevel.AACObjectLC);
		//单声道
		audioFormat.setInteger(MediaFormat.KEY_CHANNEL_MASK, AudioFormat.CHANNEL_IN_MONO);
		audioFormat.setInteger(MediaFormat.KEY_CHANNEL_COUNT, CHANNEL_COUNT);

		audioFormat.setInteger(MediaFormat.KEY_BIT_RATE, BIT_RATE);
//		audioFormat.setLong(MediaFormat.KEY_MAX_INPUT_SIZE, inputFile.length());
//      audioFormat.setLong(MediaFormat.KEY_DURATION, (long)durationInMs );
		Log.w(TAG, "音频格式: " + audioFormat);
        mMediaCodec = MediaCodec.createEncoderByType(MIME_TYPE);
        mMediaCodec.configure(audioFormat, null, null, MediaCodec.CONFIGURE_FLAG_ENCODE);
        mMediaCodec.start();
        Log.w(TAG, "prepare finishing");
        if (mListener != null) {
        	try {
        		mListener.onPrepared(this);
        	} catch (final Exception e) {
        		Log.e(TAG, "prepare:", e);
        	}
        }
	}

    @Override
	protected void startRecording() {
		super.startRecording();
		// create and execute audio capturing thread using internal mic
		/*if (mAudioThread == null) {
	        mAudioThread = new AudioThread();
			mAudioThread.start();
		}*/

		if (mPushThread == null) {
			mPushThread = new PushThread();
			mPushThread.start();
		}
	}

	@Override
    protected void release() {
		if (easyPusher != null) {
			easyPusher.stop();
			easyPusher = null;
		}

		if (mAudioThread != null) {
			mAudioThread.interrupt();
			mAudioThread = null;
		}

		if (mPushThread != null) {
			mPushThread.interrupt();
			mPushThread = null;
		}
		super.release();
    }

	private static final int[] AUDIO_SOURCES = new int[] {
		MediaRecorder.AudioSource.DEFAULT,
		MediaRecorder.AudioSource.MIC,
		MediaRecorder.AudioSource.CAMCORDER,
	};

	private class PushThread extends Thread {
		@Override
		public void run() {
			int index = 0;
			AudioRecord audioRecord = getAudioRecord();

			if (audioRecord == null) {
				return;
			}

			try {
				audioRecord.startRecording();

				if (android.os.Build.VERSION.SDK_INT >= android.os.Build.VERSION_CODES.LOLLIPOP) {
				} else {
					mBuffers = mMediaCodec.getOutputBuffers();
				}
				ByteBuffer mBuffer = ByteBuffer.allocate(10240);
				do {
					index = mMediaCodec.dequeueOutputBuffer(mBufferInfo, TIMEOUT_USEC);
					if (index >= 0) {
						if (mBufferInfo.flags == MediaCodec.BUFFER_FLAG_CODEC_CONFIG) {
							continue;
						}
						mBuffer.clear();
						ByteBuffer outputBuffer = null;
						if (android.os.Build.VERSION.SDK_INT >= android.os.Build.VERSION_CODES.LOLLIPOP) {
							outputBuffer = mMediaCodec.getOutputBuffer(index);
						} else {
							outputBuffer = mBuffers[index];
						}

						outputBuffer.get(mBuffer.array(), 7, mBufferInfo.size);
						outputBuffer.clear();
						mBuffer.position(7 + mBufferInfo.size);
						addADTStoPacket(mBuffer.array(), mBufferInfo.size + 7);
						mBuffer.flip();
						if (easyPusher != null) {
							easyPusher.push(mBuffer.array(), 0, mBufferInfo.size + 7,
									mBufferInfo.presentationTimeUs / 1000, 0);
						}
						Log.w(TAG, String.format("push audio stamp:%d", mBufferInfo.presentationTimeUs / 1000));

						encode(mBuffer, index, getPTSUs());
						frameAvailableSoon();

						mMediaCodec.releaseOutputBuffer(index, false);
					} else if (index == MediaCodec.INFO_OUTPUT_BUFFERS_CHANGED) {
						mBuffers = mMediaCodec.getOutputBuffers();
					} else if (index == MediaCodec.INFO_OUTPUT_FORMAT_CHANGED) {
						synchronized (MediaAudioEncoder.this) {
							Log.w(TAG, "output format changed...");
						/*newFormat = mMediaCodec.getOutputFormat();
						if (muxer != null) {
							muxer.addTrack(newFormat, false);
						}*/
						}
					} else if (index == MediaCodec.INFO_TRY_AGAIN_LATER) {
//                    Log.v(TAG, "No buffer available...");
					} else {
						Log.e(TAG, "Message: " + index);
					}
				} while (mPushThread != null);
			} catch (final Exception e) {
				Log.e(TAG, "PushThread#run", e);
			} finally {
				audioRecord.release();
			}

		}
	}


	/**
	 * Thread to capture audio data from internal mic as uncompressed 16bit PCM data
	 * and write them to the MediaCodec encoder
	 */
    private class AudioThread extends Thread {
    	@Override
    	public void run() {
			android.os.Process.setThreadPriority(android.os.Process.THREAD_PRIORITY_AUDIO); // THREAD_PRIORITY_URGENT_AUDIO
			int cnt = 0;
			final ByteBuffer buf = ByteBuffer.allocateDirect(SAMPLES_PER_FRAME).order(ByteOrder.nativeOrder());

			AudioRecord audioRecord = getAudioRecord();

			if (audioRecord != null) {
				try {
					if (mIsCapturing) {
						Log.w(TAG, "AudioThread:start audio recording");
						int readBytes;
						audioRecord.startRecording();
						try {
							for ( ; mIsCapturing && !mRequestStop && !mIsEOS ; ) {
								// read audio data from internal mic
								buf.clear();
								try {
									readBytes = audioRecord.read(buf, SAMPLES_PER_FRAME);
								} catch (final Exception e) {
									break;
								}
								if (readBytes > 0) {
									// set audio data to encoder
									buf.position(readBytes);
									buf.flip();
									encode(buf, readBytes, getPTSUs());
									frameAvailableSoon();
									cnt++;
								}
							}
							if (cnt > 0) {
								frameAvailableSoon();
							}
						} finally {
							audioRecord.stop();
						}
					}
				} catch (final Exception e) {
					Log.e(TAG, "AudioThread#run", e);
				} finally {
					audioRecord.release();
				}
			}
			if (cnt == 0) {
				for (int i = 0; mIsCapturing && (i < 5); i++) {
					buf.position(SAMPLES_PER_FRAME);
					buf.flip();
					try {
						encode(buf, SAMPLES_PER_FRAME, getPTSUs());
						frameAvailableSoon();
					} catch (final Exception e) {
						break;
					}
					synchronized(this) {
						try {
							wait(50);
						} catch (final InterruptedException e) {
						}
					}
				}
			}
			Log.w(TAG, "AudioThread:finished");
    	}
    }

	private AudioRecord getAudioRecord() {
		final int min_buffer_size = AudioRecord.getMinBufferSize(
				SAMPLE_RATE, AudioFormat.CHANNEL_IN_MONO, AUDIO_FORMAT);
		int buffer_size = SAMPLES_PER_FRAME * FRAMES_PER_BUFFER;
		if (buffer_size < min_buffer_size) {
			buffer_size = ((min_buffer_size / SAMPLES_PER_FRAME) + 1) * SAMPLES_PER_FRAME * 2;
		}

		AudioRecord audioRecord = null;
		for (final int src: AUDIO_SOURCES) {
			try {
				Log.w(TAG, "音/视频: micId: " + src);
				audioRecord = new AudioRecord(src,
						SAMPLE_RATE, AudioFormat.CHANNEL_IN_MONO, AUDIO_FORMAT, buffer_size);
				if (audioRecord != null) {
					if (audioRecord.getState() != AudioRecord.STATE_INITIALIZED) {
						audioRecord.release();
						audioRecord = null;
					}
				}
			} catch (final Exception e) {
				audioRecord = null;
			}
			if (audioRecord != null) {
				//找到麦克后退出
				return audioRecord;
			}
		}

		return null;
	}

	/**
	 * Add ADTS header at the beginning of each and every AAC packet.
	 * This is needed as MediaCodec encoder generates a packet of raw
	 * AAC data.
	 * <p>
	 * Note the packetLen must count in the ADTS header itself.
	 **/
	private void addADTStoPacket(byte[] packet, int packetLen) {
		int profile = 1;  // AACObjectLC
		//39=MediaCodecInfo.CodecProfileLevel.AACObjectELD;
		int freqIdx = 11;  //8000Hz
		int chanCfg = 1;  //单声道

		// fill in ADTS data
		packet[0] = (byte) 0xFF;
		packet[1] = (byte) 0xF1;
		packet[2] = (byte) (((profile - 1) << 6) + (freqIdx << 2) + (chanCfg >> 2));
		packet[3] = (byte) (((chanCfg & 3) << 6) + (packetLen >> 11));
		packet[4] = (byte) ((packetLen & 0x7FF) >> 3);
		packet[5] = (byte) (((packetLen & 7) << 5) + 0x1F);
		packet[6] = (byte) 0xFC;
	}


    /**
     * select the first codec that match a specific MIME type
     * @param mimeType
     * @return
     */
    private static final MediaCodecInfo selectAudioCodec(final String mimeType) {
    	Log.w(TAG, "selectAudioCodec:");

    	MediaCodecInfo result = null;
    	// get the list of available codecs
        final int numCodecs = MediaCodecList.getCodecCount();
LOOP:	for (int i = 0; i < numCodecs; i++) {
        	final MediaCodecInfo codecInfo = MediaCodecList.getCodecInfoAt(i);
            if (!codecInfo.isEncoder()) {	// skipp decoder
                continue;
            }
            final String[] types = codecInfo.getSupportedTypes();
            for (int j = 0; j < types.length; j++) {
            	Log.w(TAG, "supportedType:" + codecInfo.getName() + ",MIME=" + types[j]);
                if (types[j].equalsIgnoreCase(mimeType)) {
                	if (result == null) {
                		result = codecInfo;
               			break LOOP;
                	}
                }
            }
        }
   		return result;
    }

}
