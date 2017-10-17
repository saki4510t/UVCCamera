package com.ford.openxc.webcam.webcam;

import android.graphics.Bitmap;

public interface IWebcam {
    public Bitmap getFrame();
    public void stop();
    public boolean isAttached();
}
