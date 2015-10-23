package com.serenegiant.usb;

public interface IStatusCallback {
    void onStatus(int statusClass, int event, int selector, int statusAttribute, byte[] data);
}
