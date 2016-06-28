UVCCamera
=========

Library and sample code to access UVC web cameras on non-rooted Android devices

Copyright (c) 2014-2016 saki t_saki@serenegiant.com

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

How to build this project
=========================

**If you want to use Android Studio (NDK support in Android Studio is a work in progress but it does work):**

1. Set the environment variable `ANDROID_NDK_HOME` with the path to your Android NDK installation to avoid having to edit local.properties later!
2. Clone this repository
3. Start Android Studio
4. Select `Open an existing Android Studio project` and open the cloned repository folder
5. If the alert `Unregistered VCS root detected` appears, select "Add root" if you want Android Studio to manage git operations - otherwise click "Ignore"
6. If you see Gradle sync fail with `Error:(37, 0) Cannot get property 'absolutePath' on null object`, you either did not set ANDROID_NDK_HOME above or it's somehow incorrect. Either correct the variable and restart Android Studio or do the following:
Edit `local.properties` and add `ndk.dir` key to the end of the file:  

    ```
    sdk.dir={path to your Android SDK}
    ndk.dir={path to your Android NDK}
    ```
Then synchronize the project  

7. Execute "Build -> Make Project" from the toolbar
8. To build the sample APKs, execute "Build -> Build APK" from the toolbar
Note: In Windows *if the build is successful* a "Build APK" alert box will direct you select "Show in Explorer". If that folder view doesn't work properly, just look for the APKs in their respective folders (e.g., {UVCCamera}/usbCameraTest0/build/outputs/apk/)

Note: If you want to use the built-in VCS in Android Studio, use `Check out project from Version Control` from this repo. After cloning, Android Studio will ask you open the project but don't open it now - instead open the project using `Open an existing Android Studio project`. Other procedures are the same as above.

**If you want to use the Gradle build system to build the entire project, including the NDK parts:**

1. On some systems, you may need add `JAVA_HOME` environment variable that points to the JDK directory
2. Clone this repository
3. Enter the repository directory
4. Build library with all sample projects using `gradle build`
It will takes several minutes to build. Now you can see apks in each `{sample project}/build/outputs/apks` directory  
Or if you want to install and try all sample projects on your device, run `gradle installDebug`  

**If you still need to use Eclipse or if you don't want to use Gradle, you can build using `ndk-build`:**

1. Clone this repository
2. Change directory into the `{UVCCamera}/libuvccamera/build/src/main/jni` directory
3. Run `ndk-build`
4. The shared libraries are found in the `{UVCCamera}/libuvccamera/build/src/main/libs` directory. Copy them into your project with directories
5. Copy the files under `{UVCCamera}/libuvccamera/build/src/main/java` into your project source directory

How to use the library and examples
===================================

* Please see the sample projects and [the Wiki](https://github.com/saki4510t/UVCCamera/wiki/howto) for examples and information
* These sample projects are IntelliJ projects, as is the library
* This library works on at least Android 3.1 or later (API >= 12), but Android 4.0 or later (API>=14) is better. USB host function is required
* If you want to try on Android 3.1, you will need some modification (need to remove setPreviewTexture method in UVCCamera.java etc.), but we have not confirmed whether the sample project runs on Android 3.1
* Some sample projects need Android 4.3 or better (API>=18)

**Included sample projects:**

- **usbCameraTest0** - This project demonstrates how to start/stop previewing using a SurfaceView.

- **usbCameraTest1** - This project demonstrates how to start/stop previewing using a customized TextureView.

- **usbCameraTest2** - This project demonstrates how to record video from a UVC camera (without audio) as an .MP4 file using the MediaCodec encoder. This sample requires API>=18 because MediaMuxer is only supported in more recent APIs.

- **usbCameraTest3** - This project demonstrates how to record video from a UVC camera with audio (from internal mic) as a .MP4 file. This also shows several ways to capture still images. This sample may most useful as base project of your customized app.

- **usbCameraTest4** - This project demonstrates a way to access UVC camera and save video images using a background service. This is one of the most complex samples because this requires IPC using AIDL.

- **usbCameraTest5** - This project is almost the same as usbCameraTest3 but saves video images using the IFrameCallback interface instead of using input Surface from MediaCodec encoder. In most cases, you should not use IFrameCallback to save images because IFrameCallback is much slower than using Surface, but IFrameCallback will be useful if you want to get video frame data and process them yourself or pass them to another external library as a byte buffer.

- **usbCameraTest6** - This project demonstrates how to split video images to multiple Surfaces. You can see video images in a side-by-side view on this app. This sample also show how to use EGL to render images. If you want to show video images after adding visual effect/filter effects, this sample may help you.

- **usbCameraTest7** - This project demonstrates how to use two cameras and show video images from each camera side-by side. This is still experimental and may have some issues.

Change history
==============

###2014/07/25
Add some modification to the library and new sample project named "usbCameraTest2".
This new sample project demonstrate how to capture movie using frame data from
UVC camera with MediaCodec and MediaMuxer.
New sample requires at least Android 4.3 (API>=18).
This limitation does not come from the library itself but from the limitation of
MediaMuxer and MediaCodec#createInputSurface.

###2014/09/01
Add new sample project named `usbCameraTest3`
This new sample project demonstrate how to capture audio and movie simultaneously
using frame data from UVC camera and internal mic with MediaCodec and MediaMuxer.
This new sample includes still image capturing as png file.(you can easily change to
save as jpeg) This sample also requires at least Android 4.3 (API>=18).
This limitation does not come from the library itself but from the limitation of
MediaMuxer and MediaCodec#createInputSurface.

###2014/11/16
Add new sample project named `usbCameraTest4`
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
and new sample project `usbCameraTest5` to demonstrate how to use the callback method.

###2015/02/18
Add `libUVCCamera` as a library project (source code is almost same as previous release except Android.mk).
All files and directories under `library` directory is deprecated.

###2015/05/25
libraryProject branch merged to master.

###2015/05/30
Fixed the issue that DeviceFilter class could not work well when providing venderID, productID etc.

###2015/06/03
Add new sample project named `usbCameraTest6`
This new sample project mainly demonstrate how to show video images on two TextureView simultaneously, side by side.

###2015/06/10
Fixed the issue of pixel format is wrong when NV21 mode on calling IFrameCallback#onFrame (U and V plane was swapped) and added YUV420SP mode.

###2015/06/11
Improve the issue of `usbCameraTest4` that fails to connect/disconnect.

###2015/07/19
Add new methods to get/set camera features like brightness, contrast etc.  
Add new method to get supported resolution from camera as json format.  

###2015/08/17
Add new sample project `usbCameraTest7` to demonstrate how to use two camera at the same time.  

###2015/09/20
Fixed the issue that building native libraries fail on Windows.

###2015/10/30
Merge pull request (add status and button callback). Thanks Alexey Pelykh.

###2015/12/16
Add feature so that user can request fps range from Java code when negotiating with camera. Actual resulted fps depends on each UVC camera. Currently there is no way to get resulted fps (will add in the future).
