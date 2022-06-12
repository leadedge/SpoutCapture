//
//	SpoutCap
//
//	Command line version of "SpoutCapture"
//
// The application can be started without a command line but will do nothing.
// To capture a specific window use a batch file or type from a command console window
// started in the same folder as the executable. "aa-dos.bat" is provided to do that.
//
// SpoutCap "Title of window to capture"
//
// The application will start minimised to the taskbar.
// It can be restored to exit or just close from the taskbar.
//
// "aa-sender.bat" is an example that captures "Spout Demo Sender".
//
// Multiple windows can be captured by starting more instances with the same method.
// Senders will be created as :
//
// "WindowSender_1", "WindowSender_2", etc.
//
// Each can be closed independently.
//
//	SpoutCap is Licensed with the LGPL3 license.
//	Copyright(C) 2022. Lynn Jarvis.
//
//	https://spout.zeal.co/
//
// -----------------------------------------------------------------------------------------
// This program is free software : you can redistribute it and/or modify it under the terms
// of the GNU Lesser General Public License as published by the Free Software Foundation, 
// either version 3 of the License, or (at your option) any later version.
// This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
// without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
// See the GNU Lesser General Public License for more details.
// You will receive a copy of the GNU Lesser General Public License along with this program.
// If not, see http http://www.gnu.org/licenses/. 
// -----------------------------------------------------------------------------------------
//
//	11.05.22	- Initial release 1.000
//				  VS2022 /MD Win32
//

#include "ofApp.h"

//--------------------------------------------------------------
void ofApp::setup() {

	// Set the caption
	ofSetWindowTitle("SpoutCap");

	// Set a custom icon so the minimised window can be recognised on the taskbar
	SetClassLongPtr(ofGetWin32Window(), GCLP_HICON, (LONG_PTR)LoadIconA(GetModuleHandle(NULL), MAKEINTRESOURCEA(IDI_ICON1)));

	// Load a font rather than the default
	if (!myFont.load("fonts/DejaVuSansCondensed-Bold.ttf", 14, true, true))
		printf("ofApp error - Font not loaded\n");

	// Centre on the screen
	ofSetWindowPosition((ofGetScreenWidth() - ofGetWidth()) / 2, (ofGetScreenHeight() - ofGetHeight()) / 2);
	
	//
	// Command line argument
	//
	// Specify a sender to receive from.
	// Start the application from a DOS prompt. For example :
	//
	// "SpoutCap" "Window Title"
	//
	// The application window will minimize and capture the window with that title if it exists. 
	// The application will stay minimised and the Spout output will start.
	//
	if (lpCmdLine && lpCmdLine[0]) {

		// Remove double quotes
		std::string name = lpCmdLine;
		name.erase(std::remove(name.begin(), name.end(), '\"'), name.end());

		// Find the requested window
		windowHwnd = FindWindowA(NULL, name.c_str());
		if (windowHwnd != 0) {
			RECT rect;
			GetClientRect(windowHwnd, &rect);
			windowWidth = rect.right - rect.left;
			windowHeight = rect.bottom - rect.top;
			if (windowBuffer) free((void*)windowBuffer);
			windowBuffer = (unsigned char*)malloc(windowWidth*windowHeight * 4 * sizeof(unsigned char));
			windowSender.CreateSender("WindowSender", windowWidth, windowHeight);
			windowSender.SetActiveSender("WindowSender");
			bInitialized = true;
		}

		// Minimize the window
		// Can be made visible to exit or closed from taskbar
		ShowWindow(ofGetWin32Window(), SW_MINIMIZE);

	}

}

//--------------------------------------------------------------
void ofApp::exit() {

	windowSender.ReleaseSender();

}

//--------------------------------------------------------------
void ofApp::update() {

	//
	// GDI capture
	//
	if(windowHwnd) {
		// Check the size which the user might have changed
		RECT rect;
		GetClientRect(windowHwnd, &rect);
		unsigned int width = rect.right - rect.left;
		unsigned int height = rect.bottom - rect.top;
		if (width != windowWidth || height != windowHeight) {
			// Update globals and buffers
			windowWidth = width;
			windowHeight = height;
			if (windowBuffer) free((void*)windowBuffer);
			windowBuffer = (unsigned char*)malloc(windowWidth*windowHeight * 4 * sizeof(unsigned char));
			windowSender.UpdateSender("WindowSender", windowWidth, windowHeight);
		}
		// Try a capture
		if (capture_window(windowHwnd)) {
			// Send GDI pixels
			windowSender.SendImage(windowBuffer, windowWidth, windowHeight, GL_BGRA_EXT);
		}
	}

}

//--------------------------------------------------------------
void ofApp::draw() {

	// Instruct user
	ofSetColor(255, 192, 0);
	myFont.drawString("Start the application from a command window prompt", 50, ofGetHeight() / 2);
	ofSetColor(255);

}


bool ofApp::capture_window(HWND hwnd)
{
	// Closed window or self
	if (!IsWindow(hwnd) || hwnd == ofGetWin32Window()) {
		return false;
	}

	hWindowDC = GetDC(hwnd);
	hWindowMemDC = CreateCompatibleDC(hWindowDC);
	hWindowBitmap = CreateCompatibleBitmap(hWindowDC, windowWidth, windowHeight);
	if (hWindowBitmap) {
		hWindowOld = (HBITMAP)SelectObject(hWindowMemDC, hWindowBitmap);
		BitBlt(hWindowMemDC, 0, 0, windowWidth, windowHeight, hWindowDC, 0, 0, SRCCOPY | CAPTUREBLT);
		SelectObject(hWindowMemDC, hWindowOld);
		// Get the pixel data
		GetBitmapBits(hWindowBitmap, windowWidth*windowHeight * 4, windowBuffer);
		DeleteObject(hWindowBitmap);
		DeleteDC(hWindowMemDC);
		ReleaseDC(NULL, hWindowDC);
		return true;
	}

	// hWindowBitmap failed
	DeleteDC(hWindowMemDC);
	ReleaseDC(NULL, hWindowDC);

	return false;

}

