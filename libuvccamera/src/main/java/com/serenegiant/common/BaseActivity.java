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

package com.serenegiant.common;

import android.Manifest;
import android.annotation.SuppressLint;
import android.content.pm.PackageManager;
import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.support.annotation.NonNull;
import android.support.annotation.StringRes;
import android.support.v7.app.AppCompatActivity;
import android.util.Log;
import android.widget.Toast;

import com.serenegiant.dialog.MessageDialogFragmentV4;
import com.serenegiant.utils.BuildCheck;
import com.serenegiant.utils.HandlerThreadHandler;
import com.serenegiant.utils.PermissionCheck;

/**
 * Created by saki on 2016/11/18.
 *
 */
public class BaseActivity extends AppCompatActivity
	implements MessageDialogFragmentV4.MessageDialogListener {

	private static boolean DEBUG = false;	// FIXME 実働時はfalseにセットすること
	private static final String TAG = BaseActivity.class.getSimpleName();

	/** UI操作のためのHandler */
	private final Handler mUIHandler = new Handler(Looper.getMainLooper());
	private final Thread mUiThread = mUIHandler.getLooper().getThread();
	/** ワーカースレッド上で処理するためのHandler */
	private Handler mWorkerHandler;
	private long mWorkerThreadID = -1;

	@Override
	protected void onCreate(final Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		// ワーカースレッドを生成
		if (mWorkerHandler == null) {
			mWorkerHandler = HandlerThreadHandler.createHandler(TAG);
			mWorkerThreadID = mWorkerHandler.getLooper().getThread().getId();
		}
	}

	@Override
	protected void onPause() {
		clearToast();
		super.onPause();
	}

	@Override
	protected synchronized void onDestroy() {
		// ワーカースレッドを破棄
		if (mWorkerHandler != null) {
			try {
				mWorkerHandler.getLooper().quit();
			} catch (final Exception e) {
				//
			}
			mWorkerHandler = null;
		}
		super.onDestroy();
	}

//================================================================================
	/**
	 * UIスレッドでRunnableを実行するためのヘルパーメソッド
	 * @param task
	 * @param duration
	 */
	public final void runOnUiThread(final Runnable task, final long duration) {
		if (task == null) return;
		mUIHandler.removeCallbacks(task);
		if ((duration > 0) || Thread.currentThread() != mUiThread) {
			mUIHandler.postDelayed(task, duration);
		} else {
			try {
				task.run();
			} catch (final Exception e) {
				Log.w(TAG, e);
			}
		}
	}

	/**
	 * UIスレッド上で指定したRunnableが実行待ちしていれば実行待ちを解除する
	 * @param task
	 */
	public final void removeFromUiThread(final Runnable task) {
		if (task == null) return;
		mUIHandler.removeCallbacks(task);
	}

	/**
	 * ワーカースレッド上で指定したRunnableを実行する
	 * 未実行の同じRunnableがあればキャンセルされる(後から指定した方のみ実行される)
	 * @param task
	 * @param delayMillis
	 */
	protected final synchronized void queueEvent(final Runnable task, final long delayMillis) {
		if ((task == null) || (mWorkerHandler == null)) return;
		try {
			mWorkerHandler.removeCallbacks(task);
			if (delayMillis > 0) {
				mWorkerHandler.postDelayed(task, delayMillis);
			} else if (mWorkerThreadID == Thread.currentThread().getId()) {
				task.run();
			} else {
				mWorkerHandler.post(task);
			}
		} catch (final Exception e) {
			// ignore
		}
	}

	/**
	 * 指定したRunnableをワーカースレッド上で実行予定であればキャンセルする
	 * @param task
	 */
	protected final synchronized void removeEvent(final Runnable task) {
		if (task == null) return;
		try {
			mWorkerHandler.removeCallbacks(task);
		} catch (final Exception e) {
			// ignore
		}
	}

//================================================================================
	private Toast mToast;
	/**
	 * Toastでメッセージを表示
	 * @param msg
	 */
	protected void showToast(@StringRes final int msg, final Object... args) {
		removeFromUiThread(mShowToastTask);
		mShowToastTask = new ShowToastTask(msg, args);
		runOnUiThread(mShowToastTask, 0);
	}

	/**
	 * Toastが表示されていればキャンセルする
	 */
	protected void clearToast() {
		removeFromUiThread(mShowToastTask);
		mShowToastTask = null;
		try {
			if (mToast != null) {
				mToast.cancel();
				mToast = null;
			}
		} catch (final Exception e) {
			// ignore
		}
	}

	private ShowToastTask mShowToastTask;
	private final class ShowToastTask implements Runnable {
		final int msg;
		final Object args;
		private ShowToastTask(@StringRes final int msg, final Object... args) {
			this.msg = msg;
			this.args = args;
		}

		@Override
		public void run() {
			try {
				if (mToast != null) {
					mToast.cancel();
					mToast = null;
				}
				final String _msg = (args != null) ? getString(msg, args) : getString(msg);
				mToast = Toast.makeText(BaseActivity.this, _msg, Toast.LENGTH_SHORT);
				mToast.show();
			} catch (final Exception e) {
				// ignore
			}
		}
	}

//================================================================================
	/**
	 * MessageDialogFragmentメッセージダイアログからのコールバックリスナー
	 * @param dialog
	 * @param requestCode
	 * @param permissions
	 * @param result
	 */
	@SuppressLint("NewApi")
	@Override
	public void onMessageDialogResult(final MessageDialogFragmentV4 dialog, final int requestCode, final String[] permissions, final boolean result) {
		if (result) {
			// メッセージダイアログでOKを押された時はパーミッション要求する
			if (BuildCheck.isMarshmallow()) {
				requestPermissions(permissions, requestCode);
				return;
			}
		}
		// メッセージダイアログでキャンセルされた時とAndroid6でない時は自前でチェックして#checkPermissionResultを呼び出す
		for (final String permission: permissions) {
			checkPermissionResult(requestCode, permission, PermissionCheck.hasPermission(this, permission));
		}
	}

	/**
	 * パーミッション要求結果を受け取るためのメソッド
	 * @param requestCode
	 * @param permissions
	 * @param grantResults
	 */
	@Override
	public void onRequestPermissionsResult(final int requestCode, @NonNull final String[] permissions, @NonNull final int[] grantResults) {
		super.onRequestPermissionsResult(requestCode, permissions, grantResults);	// 何もしてないけど一応呼んどく
		final int n = Math.min(permissions.length, grantResults.length);
		for (int i = 0; i < n; i++) {
			checkPermissionResult(requestCode, permissions[i], grantResults[i] == PackageManager.PERMISSION_GRANTED);
		}
	}

	/**
	 * パーミッション要求の結果をチェック
	 * ここではパーミッションを取得できなかった時にToastでメッセージ表示するだけ
	 * @param requestCode
	 * @param permission
	 * @param result
	 */
	protected void checkPermissionResult(final int requestCode, final String permission, final boolean result) {
		// パーミッションがないときにはメッセージを表示する
		if (!result && (permission != null)) {
			if (Manifest.permission.RECORD_AUDIO.equals(permission)) {
				showToast(R.string.permission_audio);
			}
			if (Manifest.permission.WRITE_EXTERNAL_STORAGE.equals(permission)) {
				showToast(R.string.permission_ext_storage);
			}
			if (Manifest.permission.INTERNET.equals(permission)) {
				showToast(R.string.permission_network);
			}
		}
	}

	// 動的パーミッション要求時の要求コード
	protected static final int REQUEST_PERMISSION_WRITE_EXTERNAL_STORAGE = 0x12345;
	protected static final int REQUEST_PERMISSION_AUDIO_RECORDING = 0x234567;
	protected static final int REQUEST_PERMISSION_NETWORK = 0x345678;
	protected static final int REQUEST_PERMISSION_CAMERA = 0x537642;

	/**
	 * 外部ストレージへの書き込みパーミッションが有るかどうかをチェック
	 * なければ説明ダイアログを表示する
	 * @return true 外部ストレージへの書き込みパーミッションが有る
	 */
	protected boolean checkPermissionWriteExternalStorage() {
		if (!PermissionCheck.hasWriteExternalStorage(this)) {
			MessageDialogFragmentV4.showDialog(this, REQUEST_PERMISSION_WRITE_EXTERNAL_STORAGE,
				R.string.permission_title, R.string.permission_ext_storage_request,
				new String[]{Manifest.permission.WRITE_EXTERNAL_STORAGE});
			return false;
		}
		return true;
	}

	/**
	 * 録音のパーミッションが有るかどうかをチェック
	 * なければ説明ダイアログを表示する
	 * @return true 録音のパーミッションが有る
	 */
	protected boolean checkPermissionAudio() {
		if (!PermissionCheck.hasAudio(this)) {
			MessageDialogFragmentV4.showDialog(this, REQUEST_PERMISSION_AUDIO_RECORDING,
				R.string.permission_title, R.string.permission_audio_recording_request,
				new String[]{Manifest.permission.RECORD_AUDIO});
			return false;
		}
		return true;
	}

	/**
	 * ネットワークアクセスのパーミッションが有るかどうかをチェック
	 * なければ説明ダイアログを表示する
	 * @return true ネットワークアクセスのパーミッションが有る
	 */
	protected boolean checkPermissionNetwork() {
		if (!PermissionCheck.hasNetwork(this)) {
			MessageDialogFragmentV4.showDialog(this, REQUEST_PERMISSION_NETWORK,
				R.string.permission_title, R.string.permission_network_request,
				new String[]{Manifest.permission.INTERNET});
			return false;
		}
		return true;
	}

	/**
	 * カメラアクセスのパーミッションがあるかどうかをチェック
	 * なければ説明ダイアログを表示する
	 * @return true カメラアクセスのパーミッションが有る
	 */
	protected boolean checkPermissionCamera() {
		if (!PermissionCheck.hasCamera(this)) {
			MessageDialogFragmentV4.showDialog(this, REQUEST_PERMISSION_CAMERA,
				R.string.permission_title, R.string.permission_camera_request,
				new String[]{Manifest.permission.CAMERA});
			return false;
		}
		return true;
	}

}
