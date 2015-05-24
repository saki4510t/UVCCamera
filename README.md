UVCCamera
=========

library and sample to access to UVC web camera on non-rooted Android device

Copyright (c) 2014-2015 saki t_saki@serenegiant.com

 Licensed under the Apache License, Version 2.0 (the "License");
 you may not use this file except in compliance with the License.
 You may obtain a copy of the License at

     http://www.apache.org/licenses/LICENSE-2.0

 Unless required by applicable law or agreed to in writing, software
 distributed under the License is distributed on an "AS IS" BASIS,
 WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 See the License for the specific language governing permissions and
 limitations under the License.

All files in the folder are under this Apache License, Version 2.0.
Files in the jni/libjpeg, jni/libusb and jin/libuvc folders may have a different license,
see the respective files.

How to compile library  
=========
If you want to compile this library(on command line),
 1. move to libUVCCamera directory
 2. run ndk-build clean
 3. run ndk-build

(tested NDK-r9d/NDK-r10c 64bit on OSX)  
The libraries are saved into libs/`architecture name` directory, for example libs/armeabi-v7a
Only library for armeabi-v7a architecture is compiled with the default setting.
If you want to compile for other architecture, you need change APP_ABI flag in the
library/jni/Application.mk file.

After compiling, copy or move libuvc.so and libUVCCamera.so into your project's 
libs/`architecture name` folder.

Or  
You can use `libUVCCamera` as a library project. Please import all files under `libUVCCamera`
directory into your IDE.  

All files and directories under `library` directory is deprecated.

libusb and libjpeg are embedded　into libuvc.so with current setting. You can also compile to
separate shared libraries but some compile/link error may occur(we have not confirmed well yet). 

How to use
=========
See sample project and/or our web site(but sorry web site is Japanese only).
These sample projects are Eclipse project. Please import using Eclipse.
This library works on at least Android 3.1 or later(API >= 12), but Android 4.0(API >= 14)
or later is better. USB host function must be required.
If you want to try on Android 3.1, you will need some modification(need to remove 
setPreviewTexture method in UVCCamera.java etc.), but we have not confirm whether the sample
project run on Android 3.1 yet.
Some sample projects need API>=18 though.

###2014/07/25
Add some modification to the library and new sample project named "USBCameraTest2".
This new sample project demonstrate how to capture movie using frame data from
UVC camera with MediaCodec and MediaMuxer.
New sample requires at least Android 4.3(API>=18).
This limitation does not come from the library itself but from the limitation of 
MediaMuxer and MediaCodec#createInputSurface.

###2014/09/01
Add new sample project named `USBCameraTest3`
This new sample project demonstrate how to capture audio and movie simultaneously
using frame data from UVC camera and internal mic with MediaCodec and MediaMuxer.
This new sample includes still image capturing as png file.(you can easily change to
save as jpeg) This sample also requires at least Android 4.3(API>=18).
This limitation does not come from the library itself but from the limitation of 
MediaMuxer and MediaCodec#createInputSurface.

###2014/11/16
Add new sample project named `USBCameraTest4`
This new sample project mainly demonstrate how to use offscreen rendering
and record movie without any display.
The communication with camera execute as Service and continue working
even if you stop app. If you stop camera communication, click "stop service" button.

###2014/12/17
Add bulk transfer mode and update sample projects.

###2015/01/12
Add wiki page, [HowTo](https://github.com/saki4510t/UVCCamera/wiki/howto "HowTo")

###2015/01/22
Add method to adjust preview resolution and frame data mode.

###2015/02/12
Add IFrameCallback interface to get frame data as ByteArray
and new sample project `USBCameraTest5` to demonstrate how to use the callback method.

###2015/02/18
Add `libUVCCamera` as a library project(source code is almost same as previous release except Android.mk).
All files and directories under `library` directory is deprecated.
