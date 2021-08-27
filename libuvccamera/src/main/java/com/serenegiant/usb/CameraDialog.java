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

package com.serenegiant.usb;

import java.util.ArrayList;
import java.util.List;

import android.app.Activity;
import android.app.AlertDialog;
import android.app.Dialog;
import android.app.DialogFragment;
import android.content.Context;
import android.content.DialogInterface;
import android.hardware.usb.UsbDevice;
import android.os.Bundle;
import android.view.LayoutInflater;
import android.view.View;
import android.view.View.OnClickListener;
import android.view.ViewGroup;
import android.widget.BaseAdapter;
import android.widget.Button;
import android.widget.CheckedTextView;
import android.widget.Spinner;

import com.serenegiant.uvccamera.R;

public class CameraDialog extends DialogFragment {
	private static final String TAG = CameraDialog.class.getSimpleName();

	public interface CameraDialogParent {
		public USBMonitor getUSBMonitor();
		public void onDialogResult(boolean canceled);
	}
	
	/**
	 * Helper method
	 * @param parent FragmentActivity
	 * @return
	 */
	public static CameraDialog showDialog(final Activity parent/* add parameters here if you need */) {
		CameraDialog dialog = newInstance(/* add parameters here if you need */);
		try {
			dialog.show(parent.getFragmentManager(), TAG);
		} catch (final IllegalStateException e) {
			dialog = null;
		}
    	return dialog;
	}

	public static CameraDialog newInstance(/* add parameters here if you need */) {
		final CameraDialog dialog = new CameraDialog();
		final Bundle args = new Bundle();
		// add parameters here if you need
		dialog.setArguments(args);
		return dialog;
	}

	protected USBMonitor mUSBMonitor;
	private Spinner mSpinner;
	private DeviceListAdapter mDeviceListAdapter;

	public CameraDialog(/* no arguments */) {
		// Fragment need default constructor
	}

	@SuppressWarnings("deprecation")
	@Override
	public void onAttach(final Activity activity) {
		super.onAttach(activity);
       if (mUSBMonitor == null)
        try {
    		mUSBMonitor = ((CameraDialogParent)activity).getUSBMonitor();
        } catch (final ClassCastException e) {
    	} catch (final NullPointerException e) {
        }
		if (mUSBMonitor == null) {
        	throw new ClassCastException(activity.toString() + " must implement CameraDialogParent#getUSBController");
		}
	}

	@Override
    public void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		if (savedInstanceState == null)
			savedInstanceState = getArguments();
	}

	@Override
	public void onSaveInstanceState(final Bundle saveInstanceState) {
		final Bundle args = getArguments();
		if (args != null)
			saveInstanceState.putAll(args);
		super.onSaveInstanceState(saveInstanceState);
	}

	@Override
    public Dialog onCreateDialog(final Bundle savedInstanceState) {
		final AlertDialog.Builder builder = new AlertDialog.Builder(getActivity());
		builder.setView(initView());
    	builder.setTitle(R.string.select);
	    builder.setPositiveButton(android.R.string.ok, mOnDialogClickListener);
	    builder.setNegativeButton(android.R.string.cancel , mOnDialogClickListener);
	    builder.setNeutralButton(R.string.refresh, null);
	    final Dialog dialog = builder.create();
	    dialog.setCancelable(true);
	    dialog.setCanceledOnTouchOutside(true);
        return dialog;
	}

	/**
	 * create view that this fragment shows
	 * @return
	 */
	private final View initView() {
		final View rootView = getActivity().getLayoutInflater().inflate(R.layout.dialog_camera, null);
		mSpinner = (Spinner)rootView.findViewById(R.id.spinner1);
		final View empty = rootView.findViewById(android.R.id.empty);
		mSpinner.setEmptyView(empty);
		return rootView;
	}


	@Override
	public void onResume() {
		super.onResume();
		updateDevices();
	    final Button button = (Button)getDialog().findViewById(android.R.id.button3);
	    if (button != null) {
	    	button.setOnClickListener(mOnClickListener);
	    }
	}

	private final OnClickListener mOnClickListener = new OnClickListener() {
		@Override
		public void onClick(final View v) {
			switch (v.getId()) {
			case android.R.id.button3:
				updateDevices();
				break;
			}
		}
	};

	private final DialogInterface.OnClickListener mOnDialogClickListener = new DialogInterface.OnClickListener() {
		@Override
		public void onClick(final DialogInterface dialog, final int which) {
			switch (which) {
			case DialogInterface.BUTTON_POSITIVE:
				final Object item = mSpinner.getSelectedItem();
				if (item instanceof UsbDevice) {
					mUSBMonitor.requestPermission((UsbDevice)item);
					((CameraDialogParent)getActivity()).onDialogResult(false);
				}
				break;
			case DialogInterface.BUTTON_NEGATIVE:
				((CameraDialogParent)getActivity()).onDialogResult(true);
				break;
			}
		}
	};

	@Override
	public void onCancel(final DialogInterface dialog) {
		((CameraDialogParent)getActivity()).onDialogResult(true);
		super.onCancel(dialog);
	}

	public void updateDevices() {
//		mUSBMonitor.dumpDevices();
		final List<DeviceFilter> filter = DeviceFilter.getDeviceFilters(getActivity(), R.xml.device_filter);
		mDeviceListAdapter = new DeviceListAdapter(getActivity(), mUSBMonitor.getDeviceList(filter.get(0)));
		mSpinner.setAdapter(mDeviceListAdapter);
	}

	private static final class DeviceListAdapter extends BaseAdapter {

		private final LayoutInflater mInflater;
		private final List<UsbDevice> mList;

		public DeviceListAdapter(final Context context, final List<UsbDevice>list) {
			mInflater = LayoutInflater.from(context);
			mList = list != null ? list : new ArrayList<UsbDevice>();
		}

		@Override
		public int getCount() {
			return mList.size();
		}

		@Override
		public UsbDevice getItem(final int position) {
			if ((position >= 0) && (position < mList.size()))
				return mList.get(position);
			else
				return null;
		}

		@Override
		public long getItemId(final int position) {
			return position;
		}

		@Override
		public View getView(final int position, View convertView, final ViewGroup parent) {
			if (convertView == null) {
				convertView = mInflater.inflate(R.layout.listitem_device, parent, false);
			}
			if (convertView instanceof CheckedTextView) {
				final UsbDevice device = getItem(position);
				((CheckedTextView)convertView).setText(
					String.format("UVC Camera:(%x:%x:%s)", device.getVendorId(), device.getProductId(), device.getDeviceName()));
			}
			return convertView;
		}
	}
}
