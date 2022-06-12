#pragma once

#include "ofMain.h"
#include "resource.h"
#include "..\apps\SpoutGL\SpoutSender.h"

class ofApp : public ofBaseApp {

public:

	void setup();
	void update();
	void exit();
	void draw();

	// Window sender
	SpoutSender windowSender;
	unsigned int windowWidth = 0;
	unsigned int windowHeight = 0;
	unsigned char * windowBuffer = nullptr;

	// GDI capture
	HWND windowHwnd = NULL; // Window to capture
	HDC hWindowDC;
	HDC hWindowMemDC;
	HBITMAP hWindowBitmap;
	HBITMAP hWindowOld;
	bool capture_window(HWND hwnd);

	// Flags
	bool bInitialized = false;
	bool bWindow = false;

	// Screen draw font
	ofTrueTypeFont myFont;

	// For command line args to set the capture window
	// e.g. "SpoutCap" "Window Title"
	LPSTR lpCmdLine = nullptr;
};
