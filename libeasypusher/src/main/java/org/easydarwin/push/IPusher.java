package org.easydarwin.push;

import android.content.Context;

/**
 * Created by john on 2017/5/6.
 */

public interface IPusher {

    public void stop() ;

    public  void initPush(String serverIP, String serverPort, String streamName, String key, Context context, InitCallback callback);
    public  void initPush(String url, String key, Context context, InitCallback callback, int pts);
    public  void initPush(String url, String key, Context context, InitCallback callback);

    public  void push(byte[] data, int offset, int length, long timestamp, int type);

    public  void push(byte[] data, long timestamp, int type);
}
