#include "ofMain.h"
#include "ofApp.h"

//--------------------------------------------------------------
// to change options for console window (Visual Studio)
//
// Properties > Linker > System > Subsystem
//    for console : Windows (/SUBSYSTEM:CONSOLE)
//
//    for Window : Windows (/SUBSYSTEM:WINDOWS)
//
// Click APPLY and OK. Then make changes to Main as below
//--------------------------------------------------------------

// for default console
//========================================================================
// int main() {
//
// for window without console
//========================================================================
int WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ int nShowCmd) {
	ofSetupOpenGL(640, 360, OF_WINDOW);			// <-------- setup the GL context
	// this kicks off the running of my app
	// can be OF_WINDOW or OF_FULLSCREEN
	// pass in width and height too:
	// ofRunApp(new ofApp());
	// ofRunApp( new ofApp());

	// Allow for app lpCmdLine
	ofApp* app = new ofApp();
	app->lpCmdLine = lpCmdLine;
	ofRunApp(app); // start the app

}
