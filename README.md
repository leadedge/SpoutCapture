# SpoutCap
A minimal command line interface version of SpoutCapture.

### comand line branch
This project is a branch of "SpoutCapture" but is not equivalent. SpoutCapture has many more features, but the command line interface cannot be used because it disables the mouse hook. Download the entire repository when this branch is active and find "SpoutCap.exe" in the "bin" folder.


Captures a window with the nominated title.

The application can be started without a command line but will do nothing. To capture a specific window use a batch file or type from a command console window started in the same folder as the executable. "aa-dos.bat" is provided to do that.

SpoutCap "Title of window to capture"

The application will start minimised to the taskbar. It can be restored to exit or just close from the taskbar.

"aa-sender.bat" is an example that captures "Spout Demo Sender".

Multiple windows acn be captured by starting more instances with the same method. Senders will be created as :

"WindowSender_1", "WindowSender_2", etc.

Each can be closed independently.

The project depends on :  
* ofxWinMenu - https://github.com/leadedge/ofxWinMenu  
* Spout 2.007 - https://github.com/leadedge/Spout2/

The project files are for Visual Studio 2022
