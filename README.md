# QtNdiMonitorCapture

**Shameless plug:** Seriously, one way you can really help out this project is to subscribe to NightVsKnight's [YouTube](https://www.youtube.com/channel/UCn8Ds6jeUzjxCPkMApg_koA) and/or [Twitch](https://www.twitch.tv/nightvsknight) channels. I will be showing off this project there from time to time, and getting new subscribers gives me a little morale boost to help me continue this project.

Qt6 based app that simulates [`NDI Tools`](https://ndi.tv/tools/)' `Studio Monitor` and `Screen Capture`.

* NDI's `Studio Monitor` is a NDI Receiver + Viewer.
* NDI's `Screen Capture` is a Monitor Capture + NDI Sender.

**`QtNdiMonitorCapture` does them both, so it can actually monitor its own output! :)**  
(`Studio Monitor` can also send `NDI video output`, but even if you were able to have it view its own output then you would probably just be viewing a black screen.)  
**NOTE:** *It is interesting to run Monitor mode to view its own Monitor Capture screen in full-screen and see the result of the lossy data transfer. If P216 pixel format was used to capture->transmit->receive->render then I suspect that would be much less lossy (I wonder how close to lossless).*

### Limitations
* Currently only compiles and runs on Windows.  
  Sorry! Feel free to help compile and test and implement on other platforms!
* Missing features (not exhaustive):
  * `Studio Monitor`
    * Recording: Very low priority to me.
    * KVM: I tried to get this to work but the NDI KVM API keeps crashing whenever it is called. :/  
      Another problem is that this requires NDI Advanced SDK, which may have an incompatible license.
    * Settings ...
      * Application ...
      * Audio ...
      * Video ...
      * Overlay ...
      * Output ...
      * PTZ Settings ...: Very low priority to me.
  * `Screen Capture`
    * **Audio**: Again, sorry, but this initial `**Screen** Capture` proof of concept was focused literally on the screen/video only;  
      Audio is [sort of] simpler to capture & send than video, but the UX is more complicated to select which of the multiple audio input and output devices to capture from.  
      I will get to this eventually, but it is lower priority proof of concept for now.
    * Frame Rate ...
    * Capture Settings ...
    * Audio Source ...
    * Webcam Video Source ... : Very low priority to me.
    * Webcam Audio Source ... : Very low priority to me.
    * Enable KVM Control: I tried to get this to work but the NDI KVM API keeps crashing whenever it is called. :/

Feel free to help add missing features or support for other platforms.  
See [#TODO](#TODO).

## Usage
Launch the app and right click on the view window to get a context menu showing all of the currently availble NDI Sources and select an NDI Source to view.

`Monitor Capture Start` will show a `System Tray Icon` that you can use to later stop the capture.

The app will not close while a capture is running.

To close the app while a capture is running either:
1. first stop the capture and then close the app.
   -or-
2. `Right-click` on the main window or the system tray icon and select `Exit`.

### Command line options
Launch in `Monitor` mode (the default):
```
QtNdiMonitorCapture.exe
-or-
QtNdiMonitorCapture.exe --mode=monitor
```
Launch in `Capture` mode:
```
QtNdiMonitorCapture.exe --mode=capture
```
More are planned.

## Build
Requirements:
* Currently only compiles and runs on Windows
* Visual Studio 2019 Community installed
* Qt6 installed with the following components:
  * MSVC 2019 64-bit
  * Qt Multimedia
  * Qt Creator 9.0.2
  * Qt Installer Framework 4.5
* NDI Advanced SDK to be installed.  
  I could have just required the non-Advanced SDK, but I hope to one day add KVM support, which requires the Advanced SDK.

### Steps:

#### Qt Creator 9.0.2 IDE
1. Launch Qt Creator
2. Open the QtNdiMonitorCapture.pro file
3. Build and Run

#### PowerShell 7.3.3 Command-Line
1. `$env:Qt6_DIR = 'C:\Qt\6.4.2\msvc2019_64\'`
2. `CI\windows-build.ps1 "CONFIG+=package"`

## TODO
Please help to whittle down this list!:
* Check for memory leaks and fix any found, especially when:
  1. Stopping and starting Capture.
  2. Stopping and starting when switching between different NDI Sources.
* Add support for any platform that Qt6 supports, especially:
  1. Raspberry Pi OS
  2. Jetson Nano Linux4Tegra
  3. Linux
  4. MacOS
* Confirm and/or improve hardware acceleration options.
* Confirm the app compiles and creates a distribution for all supported platforms.
* Confirm the app launches on all supported platforms.
* Confirm `Monitor` mode works on all supported platforms.
* **Implement `Capture` mode for all supported platforms.**
* Confirm multiple monitors works on all supported platforms.
* Find a way to capture from more than one monitor...or all! :)
* Add command-line to list NDI Sources and exit.
* Add command-line to start viewing a specific NDI Source.
* Add command-line to start capturing a specific Monitor (or Monitors).
* Successfully port from QMake to CMake.
* Add all of the [Limitations](#Limitations) of missing NDI `Studio Monitor` and `Screen Capture` features, including `NDI|HX3` support.
  `NDI|HX` requires NDI Advanced SDK which may have an incompatible license. :/
* Add support for sending to and receiving from NDI Groups.
* Improve `Capture` performance, including adding `Game Capture` option [similar to OBS].
* Improve `Monitor` performance.
* Audit the code for all uses of pass-by-pointer (`*`) vs pass-by-reference (`&`) and decide the best one to use for each situation.  
  For performance tuning, this should usually be the fastest one.  
  I have worked in the Managed world too long and was a little lazy writing the code. :/  
  Some Qt classes I even pass without either `*` or `&`, and I suspect that results in a copy operation that probably isn't very efficient. :)

NOTES:
* `Capture` does try to use a double-buffering technique, but there is still some noticable sheering. :/
* `Monitor` code uses 100% Qt6 classes and **should** be very portable to any platform that Qt6 supports.  
  It would be exceptionally nice to implement a cross-platform P216 pixel format! :)
* `Capture` code is [currently] Windows specific and could use a cross-platform abstraction, probably using OBS Studio as inspiration.
  https://github.com/obsproject/obs-studio/blob/master/libobs/obs-video.c

## References

Monitor Capture is based off of WinRT sample code from:
* https://github.com/microsoft/Windows.UI.Composition-Win32-Samples/tree/master/cpp/ScreenCaptureforHWND
* https://github.com/peilinok/Windows.UI.Composition-Win32-Samples/tree/master/cpp/ScreenCaptureforHWND
* https://github.com/robmikh/Win32CaptureSample
I've made a few tweaks of my own, but you can still compare them with:


OBS Studio and OBS-NDI:
* https://github.com/obsproject/obs-studio
  * https://github.com/obsproject/obs-studio/tree/master/plugins/win-capture
  * https://github.com/obsproject/obs-studio/blob/master/libobs-winrt/winrt-capture.h
  * https://github.com/obsproject/obs-studio/blob/master/libobs-winrt/winrt-capture.cpp
  * Display Capture:
    * https://github.com/obsproject/obs-studio/blob/master/plugins/win-capture/dc-capture.h
    * https://github.com/obsproject/obs-studio/blob/master/plugins/win-capture/dc-capture.c
  * Game Capture:
    * https://github.com/obsproject/obs-studio/blob/master/plugins/win-capture/game-capture.c
  * Monitor Capture:
    * https://github.com/obsproject/obs-studio/blob/master/plugins/win-capture/monitor-capture.c
  * Window Capture:
    * https://github.com/obsproject/obs-studio/blob/master/plugins/win-capture/window-capture.c

* https://github.com/Palakis/obs-ndi

Safe Queue:
* https://stackoverflow.com/a/16075550/252308

Other Inspirations:
* https://git.lpgs.io/GPG/gptv-ndi-recorder
* https://orfast.com
* https://dicaffeine.com
* https://github.com/melnijir/Dicaffeine
* https://github.com/melnijir/libyuri_ndi_module/tree/master/src/modules/ndi
* https://github.com/jleben/ndi-tools
* https://github.com/smilingthax/xndiview
* https://github.com/Gargaj/Bonzomatic
