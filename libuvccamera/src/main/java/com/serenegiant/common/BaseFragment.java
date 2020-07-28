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
import android.app.Fragment;
import android.content.pm.PackageManager;
import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.support.annotation.NonNull;
import android.support.annotation.StringRes;
import android.util.Log;
import android.widget.Toast;

import com.serenegiant.dialog.MessageDialogFragment;
import com.serenegiant.utils.BuildCheck;
import com.serenegiant.utils.HandlerThreadHandler;
import com.serenegiant.utils.PermissionCheck;

/**
 * Created by saki on 2016/11/19.
 *
 */
public class BaseFragment extends Fragment
	implements MessageDialogFragment.MessageDialogListener {

	private static boolean DEBUG = false;	// FIXME 在生产期间设置为false
	private static final String TAG = BaseFragment.class.getSimpleName();

	/** UI操作的处理程序 */
	private final Handler mUIHandler = new Handler(Looper.getMainLooper());
	private final Thread mUiThread = mUIHandler.getLooper().getThread();
	/** 在工作线程上处理的处理程序 */
	private Handler mWorkerHandler;
	private long mWorkerThreadID = -1;

	public BaseFragment() {
		super();
	}

	@Override
	public void onCreate(final Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		// 生成工作者线程
		if (mWorkerHandler == null) {
			mWorkerHandler = HandlerThreadHandler.createHandler(TAG);
			mWorkerThreadID = mWorkerHandler.getLooper().getThread().getId();
		}
	}

	@Override
	public void onPause() {
		clearToast();
		super.onPause();
	}

	@Override
	public synchronized void onDestroy() {
		// 销毁工作线程
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
	 * 在UI线程上运行Runnable的辅助方法
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
	 * 如果UI线程上指定的Runnable正在等待执行，请释放执行等待
	 * @param task
	 */
	public final void removeFromUiThread(final Runnable task) {
		if (task == null) return;
		mUIHandler.removeCallbacks(task);
	}

	/**
	 * 在工作线程上执行指定的Runnable
	 * 如果没有相同的Runnable尚未执行，它将被取消（仅执行稍后指定的那个）。
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
	 * 如果要在工作线程上执行，请取消指定的Runnable
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
	 * 查看带有Toast的消息
	 * @param msg
	 */
	protected void showToast(@StringRes final int msg, final Object... args) {
		removeFromUiThread(mShowToastTask);
		mShowToastTask = new ShowToastTask(msg, args);
		runOnUiThread(mShowToastTask, 0);
	}

	/**
	 * 如果显示Toast，则取消
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
				if (args != null) {
					final String _msg = getString(msg, args);
					mToast = Toast.makeText(getActivity(), _msg, Toast.LENGTH_SHORT);
				} else {
					mToast = Toast.makeText(getActivity(), msg, Toast.LENGTH_SHORT);
				}
				mToast.show();
			} catch (final Exception e) {
				// ignore
			}
		}
	}

//================================================================================
	/**
	 * MessageDialogFragment消息对话框中的回调侦听器
	 * @param dialog
	 * @param requestCode
	 * @param permissions
	 * @param result
	 */
	@SuppressLint("NewApi")
	@Override
	public void onMessageDialogResult(final MessageDialogFragment dialog, final int requestCode, final String[] permissions, final boolean result) {
		if (result) {
			// 在消息对话框中按“确定”时，请求权限
			if (BuildCheck.isMarshmallow()) {
				requestPermissions(permissions, requestCode);
				return;
			}
		}
		// 如果在消息对话框中取消了它，并且不是Android 6，则请自己检查并调用#checkPermissionResult
		for (final String permission: permissions) {
			checkPermissionResult(requestCode, permission, PermissionCheck.hasPermission(getActivity(), permission));
		}
	}

	/**
	 * 接收许可请求结果的方法
	 * @param requestCode
	 * @param permissions
	 * @param grantResults
	 */
	@Override
	public void onRequestPermissionsResult(final int requestCode, @NonNull final String[] permissions, @NonNull final int[] grantResults) {
		super.onRequestPermissionsResult(requestCode, permissions, grantResults);	// 我什么都没做，只是打电话给我
		final int n = Math.min(permissions.length, grantResults.length);
		for (int i = 0; i < n; i++) {
			checkPermissionResult(requestCode, permissions[i], grantResults[i] == PackageManager.PERMISSION_GRANTED);
		}
	}

	/**
	 * 检查许可请求的结果
	 * 在这里，当无法获得许可时，在Toast中显示一条消息
	 * @param requestCode
	 * @param permission
	 * @param result
	 */
	protected void checkPermissionResult(final int requestCode, final String permission, final boolean result) {
		// 没有权限时显示一条消息
		if (!result && (permission != null)) {
			if (Manifest.permission.RECORD_AUDIO.equals(permission)) {
				showToast(com.serenegiant.common.R.string.permission_audio);
			}
			if (Manifest.permission.WRITE_EXTERNAL_STORAGE.equals(permission)) {
				showToast(com.serenegiant.common.R.string.permission_ext_storage);
			}
			if (Manifest.permission.INTERNET.equals(permission)) {
				showToast(com.serenegiant.common.R.string.permission_network);
			}
		}
	}

	// 动态权限请求的请求代码
	protected static final int REQUEST_PERMISSION_WRITE_EXTERNAL_STORAGE = 0x12345;
	protected static final int REQUEST_PERMISSION_AUDIO_RECORDING = 0x234567;
	protected static final int REQUEST_PERMISSION_NETWORK = 0x345678;
	protected static final int REQUEST_PERMISSION_CAMERA = 0x537642;

	/**
	 * 检查您是否具有对外部存储的写入权限
	 * 如果没有，显示说明对话框
	 * @return true 拥有对外部存储的写入权限
	 */
	protected boolean checkPermissionWriteExternalStorage() {
		if (!PermissionCheck.hasWriteExternalStorage(getActivity())) {
			MessageDialogFragment.showDialog(this, REQUEST_PERMISSION_WRITE_EXTERNAL_STORAGE,
				com.serenegiant.common.R.string.permission_title, com.serenegiant.common.R.string.permission_ext_storage_request,
				new String[]{Manifest.permission.WRITE_EXTERNAL_STORAGE});
			return false;
		}
		return true;
	}

	/**
	 * 检查您是否有录音权限
	 * 如果没有，显示说明对话框
	 * @return true 有录音权限
	 */
	protected boolean checkPermissionAudio() {
		if (!PermissionCheck.hasAudio(getActivity())) {
			MessageDialogFragment.showDialog(this, REQUEST_PERMISSION_AUDIO_RECORDING,
				com.serenegiant.common.R.string.permission_title, com.serenegiant.common.R.string.permission_audio_recording_request,
				new String[]{Manifest.permission.RECORD_AUDIO});
			return false;
		}
		return true;
	}

	/**
	 * 检查您是否具有网络访问权限
	 * 如果没有，显示说明对话框
	 * @return true 具有网络访问权限
	 */
	protected boolean checkPermissionNetwork() {
		if (!PermissionCheck.hasNetwork(getActivity())) {
			MessageDialogFragment.showDialog(this, REQUEST_PERMISSION_NETWORK,
				com.serenegiant.common.R.string.permission_title, com.serenegiant.common.R.string.permission_network_request,
				new String[]{Manifest.permission.INTERNET});
			return false;
		}
		return true;
	}

	/**
	 * 检查您是否具有摄像头访问权限
	 * 如果没有，显示说明对话框
	 * @return true 具有摄像头访问权限
	 */
	protected boolean checkPermissionCamera() {
		if (!PermissionCheck.hasCamera(getActivity())) {
			MessageDialogFragment.showDialog(this, REQUEST_PERMISSION_CAMERA,
				com.serenegiant.common.R.string.permission_title, com.serenegiant.common.R.string.permission_camera_request,
				new String[]{Manifest.permission.CAMERA});
			return false;
		}
		return true;
	}
}
