package com.ford.openxc.webcam.app;
/*
******************* Copyright (c) ***********************\
**
**         (c) Copyright 2017, 蒋朋, china
**                  All Rights Reserved
**
**                       _oo0oo_
**                      o8888888o
**                      88" . "88
**                      (| -_- |)
**                      0\  =  /0
**                    ___/`---'\___
**                  .' \\|     |// '.
**                 / \\|||  :  |||// \
**                / _||||| -:- |||||- \
**               |   | \\\  -  /// |   |
**               | \_|  ''\---/''  |_/ |
**               \  .-\__  '-'  ___/-. /
**             ___'. .'  /--.--\  `. .'___
**          ."" '<  `.___\_<|>_/___.' >' "".
**         | | :  `- \`.;`\ _ /`;.`/ - ` : | |
**         \  \ `_.   \_ __\ /__ _/   .-` /  /
**     =====`-.____`.___ \_____/___.-`___.-'=====
**                       `=---='
**
**
**     ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
**
**               佛祖保佑         永无BUG
**
**
**                   南无本师释迦牟尼佛
**

**-----------------------版本信息------------------------
** 版    本: V0.1
**
******************** End of Head **********************\
*/


import android.app.Application;

import com.ford.openxc.webcam.BuildConfig;
import com.socks.library.KLog;

public class WebCamApp extends Application {
    @Override
    public void onCreate() {
        super.onCreate();

        KLog.init(BuildConfig.DEBUG);
    }
}
