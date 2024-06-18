# SpoutCapture
An Openframeworks screen capture application for Microsoft Windows.

- Captures the visible desktop at high speed using the [Desktop Duplication API](https://docs.microsoft.com/en-us/windows/win32/direct3ddxgi/desktop-dup-api).
- Captures the region of the desktop under the application window.
- Captures individual windows using [GDI](https://docs.microsoft.com/en-us/windows/win32/gdi/windows-gdi).

Two Spout senders are created, one for the entire desktop and one for the selected region or 
application window. A region of interest is always part of the “visible” desktop, so can be obscured by other windows.
Window capture is independent and the selected application can be obscured without affecting the capture.
The capture continues if SpoutCapture is minimized. Download the package from releases.

The project depends on :  
* ofxWinMenu - https://github.com/leadedge/ofxWinMenu  
* Spout 2.007 - https://github.com/leadedge/Spout2/

The project files are for Visual Studio 2022

This program uses various techniques that may be useful for reference.

- low level mouse hook
- transparent window
- menu for an OpenFrameworks application with ofxWinMenu
- openframeworks screen text with truetype font
- add a custom icon to the task bar
- GDI window capture
- desktop duplication screen capture
- use DirectX within and OpenGL application
- draw part of a texture to an fbo
- show tompost function
- resources for an about dialog and version number
- hyperlink within the about box
- send DirectX texture and readback to OpenGL
- create two senders in the same application



