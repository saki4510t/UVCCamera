UVCCamera
=========

library and sample to access to UVC web camera on non-rooted Android device

Copyright (c) 2014 saki t_saki@serenegiant.com

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

=========
How to compile library (tested NDK-r9d/NDK-r10c 64bit on OSX)
If you want to compile this library,
1. move to library/jni directory
2. run ndk-build clean
3. run ndk-build
The libraries are saved into libs/"architecture name" directory, for example libs/armeabi-v7a
Only library for armeabi-v7a architecture is compiled with the default setting.
If you want to compile for other architecture, you need change APP_ABI flag in the
library/jni/Application.mk file.

After compiling, copy or move libuvc.so and libUVCCamera.so into your project's 
libs/"architecture name" folder.

libusb and libjpeg are embedded　into libuvc.so with current setting. You can also compile to
separate shared libraries but some compile/link error may occur(we have not confirmed well yet). 

=========
How to use
See sample project and/or our web site(but sorry web site is Japanese only).
These sample projects are Eclipse project. Please import using Eclipse.
This library works on at least Android 3.1 or later(API >= 12), but Android 4.0(API >= 14)
or later is better. USB host function must be required.
If you want to try on Android 3.1, you will need some modification(need to remove 
setPreviewTexture method in UVCCamera.java etc.), but we have not confirm whether the sample
project run on Android 3.1 yet.
Some sample projects need API>=18 though.

2014/07/25
Add some modification to the library and new sample project named "USBCameraTest2".
This new sample project demonstrate how to capture movie using frame data from
UVC camera with MediaCodec and MediaMuxer.
New sample requires at least Android 4.3(API>=18).
This limitation does not come from the library itself but from the limitation of 
MediaMuxer and MediaCodec#createInputSurface.

2014/09/01
Add new sample project named "USBCameraTest3"
This new sample project demonstrate how to capture audio and movie simultaneously
using frame data from UVC camera and internal mic with MediaCodec and MediaMuxer.
This new sample includes still image capturing as png file.(you can easily change to
save as jpeg) This sample also requires at least Android 4.3(API>=18).
This limitation does not come from the library itself but from the limitation of 
MediaMuxer and MediaCodec#createInputSurface.

2014/11/16
Add new sample project named "USBCameraTest4"
This new sample project mainly demonstrate how to use offscreen rendering
and record movie without any display.
The communication with camera execute as Service and continue working
even if you stop app. If you stop camera communication, click "stop service" button.

2014/12/17
Add bulk transfer mode and update sample projects.
