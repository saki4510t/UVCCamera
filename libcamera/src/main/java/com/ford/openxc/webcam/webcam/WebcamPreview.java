package com.ford.openxc.webcam.webcam;

import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.ServiceConnection;
import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.Rect;
import android.os.IBinder;
import android.util.AttributeSet;
import android.view.SurfaceHolder;
import android.view.SurfaceView;

import com.socks.library.KLog;

public class WebcamPreview extends SurfaceView implements
        SurfaceHolder.Callback, Runnable {
    private final static String TAG = "WebcamPreview";

    private Rect mViewWindow;
    private volatile boolean mRunning = true;
    private final Object mServiceSyncToken = new Object();
    private WebcamService mWebcamService;
    private SurfaceHolder mHolderL;

    public WebcamPreview(Context context) {
        super(context);
        init();
    }

    public WebcamPreview(Context context, AttributeSet attrs) {
        super(context, attrs);
        init();
    }

    private void init() {
        KLog.w(TAG, "WebcamPreview constructed");
        setFocusable(true);

        mHolderL = getHolder();
        mHolderL.addCallback(this);

    }

    public void startPreview(String video) {
        KLog.w(TAG, "video: " + video);
//        stopPreview();

        mRunning = true;

        Intent intent = new Intent(getContext(), WebcamService.class);
        intent.putExtra("video", video);
        getContext().bindService(intent, mConnection, Context.BIND_AUTO_CREATE);
        (new Thread(this)).start();
    }

    public void stopPreview() {
        mRunning = false;

        if(mWebcamService != null) {
            KLog.w(TAG, "Unbinding from webcam manager");
            getContext().unbindService(mConnection);
            mWebcamService = null;
        }
    }

    @Override
    public void run() {
        while(mRunning) {
            synchronized(mServiceSyncToken) {
                if(mWebcamService == null) {
                    try {
                        mServiceSyncToken.wait();
                    } catch(InterruptedException e) {
                        break;
                    }
                }

                final Bitmap bitmap = mWebcamService.getFrame();
//                KLog.w(TAG, "frame: " + bitmap.getByteCount());

/*                new Thread(new Runnable() {
                    @Override
                    public void run() {
                        ByteArrayOutputStream baos = new ByteArrayOutputStream();
                        bitmap.compress(Bitmap.CompressFormat.PNG, 100, baos);
                        KLog.w(TAG, "frame: " + bitmap.getByteCount() + Arrays.toString(baos.toByteArray()));
                    }
                }).start();*/

                Canvas canvas = mHolderL.lockCanvas();
                if(canvas != null) {
                    drawOnCanvas(canvas, bitmap);
                    mHolderL.unlockCanvasAndPost(canvas);
                }

            }
        }
    }

    protected void drawOnCanvas(Canvas canvas, Bitmap videoBitmap) {
        canvas.drawBitmap(videoBitmap, null, mViewWindow, null);
    }

    protected Rect getViewingWindow() {
        return mViewWindow;
    }

    private volatile boolean isOpened;

    @Override
    public void surfaceCreated(SurfaceHolder holder) {
        KLog.w(TAG, "Surface created: isOpened: " + isOpened);

        synchronized (WebcamPreview.this) {
            if (isOpened) {
                startPreview(WebcamService.VIDEO);
            } else {
                startPreview("/dev/video6");
            }
            isOpened = true;
        }

    }

    @Override
    public void surfaceDestroyed(SurfaceHolder holder) {
        KLog.w(TAG, "Surface destroyed");

        stopPreview();
    }

    @Override
    public void surfaceChanged(SurfaceHolder holder, int format, int winWidth,
            int winHeight) {
        KLog.w("WebCam", "surfaceChanged");
        int width, height, dw, dh;
        if(winWidth * 3 / 4 <= winHeight) {
            dw = 0;
            dh = (winHeight - winWidth * 3 / 4) / 2;
            width = dw + winWidth - 1;
            height = dh + winWidth * 3 / 4 - 1;
        } else {
            dw = (winWidth - winHeight * 4 / 3) / 2;
            dh = 0;
            width = dw + winHeight * 4 / 3 - 1;
            height = dh + winHeight - 1;
        }
        mViewWindow = new Rect(dw, dh, width, height);
    }

    private ServiceConnection mConnection = new ServiceConnection() {
        public void onServiceConnected(ComponentName className,
                IBinder service) {
            KLog.w(TAG, "Bound to WebcamManager: class: " + className.getClassName());
            synchronized(mServiceSyncToken) {
                mWebcamService = ((WebcamService.WebcamBinder) service).getService();
                mServiceSyncToken.notify();
            }
        }

        public void onServiceDisconnected(ComponentName className) {
            KLog.w(TAG, "WebcamManager disconnected unexpectedly");
            synchronized(mServiceSyncToken) {
                mRunning = false;
                mWebcamService = null;
                mServiceSyncToken.notify();
            }
        }
    };
}
