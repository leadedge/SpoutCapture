//
//	SpoutCapture
//
//	Capture the visible desktop
//	Send the desktop and the part beneath the app window
//
//	SpoutCapture is Licensed with the LGPL3 license.
//	Copyright(C) 2019-2024. Lynn Jarvis.
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
//	13.10.19	- Initial release 1.000
//				  VS2017 /MD Win32
//	29.09.21	- Update for 2.007 SpoutGL
//				  Version 1.001
//	30.09.21	- Add GDI window capture and other improvements
//				  Version 2.000
//	04.11.21	- Test for duplication failure and retry
//				  Version 2.001
//	04.05.22	- SetClassLong -> SetClassLongPtr
//	12.06.22	- Update for VS2022
//				  VS2022 /MD x64
//				  Version 2.002
//	22.04.24	- Update for Openframeworks 12 and latest Spout files
//				  Replace About dialog with SpoutMessageBox
//				  Change from SendImage to SendTexture for window capture
//				  Change shared texture to non-shared texture for duplication
//				  Remove unused dlls from bin folder
//	24.04.24	- Use SendImage if iconic, SendTexture if not
//				- SpoutLogs for errors instead of printf
//				  VS2022 /MT x64
//				  Version 2.003
//

#include "ofApp.h"

// Mouse hook used to detect RH button
static HHOOK g_hMouseHook = NULL;
static LRESULT CALLBACK LowLevelMouseProc(int nCode, WPARAM wParam, LPARAM lParam);
static bool bRHdown = false;
static int xCoord = 0;
static int yCoord = 0;
static HWND windowHwnd = NULL; // Window to capture
static HWND g_hWnd = NULL; // Application window

//--------------------------------------------------------------
void ofApp::setup() {

	// For debugging
	// OpenSpoutConsole(); // empty console
	// EnableSpoutLog(); // Error messages
	// Enable a log file in ..\AppData\Roaming\Spout
	// EnableSpoutLogFile("SpoutCapture");

	// Get instance for dialogs
	g_hInstance = GetModuleHandle(NULL);

	// Window handle used for the menu
	g_hWnd =  ofGetWin32Window();

	// Set a custom window icon
	SetClassLongPtr(g_hWnd, GCLP_HICON, (LONG_PTR)LoadIconA(GetModuleHandle(NULL), MAKEINTRESOURCEA(IDI_ICON1)));

	// Set the caption
	ofSetWindowTitle("SpoutCapture");

	// Resize to the menu
	int width = ofGetWidth();
	int height = ofGetHeight() + GetSystemMetrics(SM_CYMENU);
	ofSetWindowShape(width, height);

	// Centre on the screen
	ofSetWindowPosition((ofGetScreenWidth() - width) / 2, (ofGetScreenHeight() - height) / 2);

	// Disable escape key exit
	ofSetEscapeQuitsApp(false);

	//
	// Create a menu using ofxWinMenu
	// https://github.com/leadedge/ofxWinMenu
	//

	// A new menu object with a pointer to this class
	menu = new ofxWinMenu(this, g_hWnd);

	// Register an ofApp function that is called when a menu item is selected.
	// The function can be called anything but must exist. 
	// See "appMenuFunction".
	menu->CreateMenuFunction(&ofApp::appMenuFunction);

	// Create a menu
	HMENU hMenu = menu->CreateWindowMenu();

	//
	// File popup
	//
	HMENU hPopup = menu->AddPopupMenu(hMenu, "File");
	menu->AddPopupItem(hPopup, "Exit", false, false);

	//
	// Window popup
	//
	hPopup = menu->AddPopupMenu(hMenu, "Capture");
	// Show the entire desktop
	menu->AddPopupItem(hPopup, "Desktop", true); // Checked and auto-check
	menu->AddPopupItem(hPopup, "Region", false); // Not checked and auto-check
	menu->AddPopupItem(hPopup, "Window", false); // Not checked and auto-check
	menu->AddPopupSeparator(hPopup);
	menu->AddPopupItem(hPopup, "Show fps", false); // Not checked and auto-check
	menu->AddPopupItem(hPopup, "Show on top", false); // Not checked and auto-check
	bDesktop = true;
	bRegion = false;
	bWindow = false;
	bTopmost = false;

	//
	// Help popup
	//
	hPopup = menu->AddPopupMenu(hMenu, "Help");
	menu->AddPopupItem(hPopup, "Documentation", false, false); // No auto check
	menu->AddPopupItem(hPopup, "About", false, false); // No auto check

	// Set the menu to the window
	menu->SetWindowMenu();

	// Initialize for DirectX11 now which creates a Direct3D 11 device.
	desktopSender.SetDX9(false); // Set this here because DX9 won't work
	if (!desktopSender.spout.OpenDirectX11()) {
		MessageBoxA(NULL, "OpenDX11 failed", "Info", MB_OK);
		return;
	}

	// Get the Spout DX11 device to create the resources
	// The device may have been created as Direct3D 11.0
	// and not 11.1, but it still seems to work OK
	g_d3dDevice = desktopSender.spout.GetDX11Device();

	// Setup Desktop duplication which establishes monitorWidth and monitorHeight
	if (!setupDesktopDuplication()) {
		MessageBoxA(NULL, "Desktop duplication interface creation failed", "Error", MB_OK);
		return;
	}

	// Create a DX11 texture to receive the duplication texture
	// Use DXGI_FORMAT_B8G8R8A8_UNORM because that is the same format as
	// the Spout shared texture and is also the format of the desktop image
	// no matter what the current display mode is. 
	desktopSender.spout.spoutdx.CreateDX11Texture(g_d3dDevice,
		monitorWidth, monitorHeight, DXGI_FORMAT_B8G8R8A8_UNORM, &g_pDeskTexture);

	// Allocate a readback OpenGL texture for the desktop
	desktopTexture.allocate(monitorWidth, monitorHeight, GL_RGBA);

	// Set the size of the OF window for the part of the desktop under the window
	windowWidth = (unsigned int)ofGetWidth();
	windowHeight = (unsigned int)ofGetHeight();

	// Get the starting top, left position
	RECT rect{};
	GetClientRect(g_hWnd, &rect);
	positionLeft = rect.left + GetSystemMetrics(SM_CYFRAME)*2;
	positionTop = rect.top + GetSystemMetrics(SM_CYMENU) + GetSystemMetrics(SM_CYCAPTION) + GetSystemMetrics(SM_CYFRAME)*2;

	// Fbo for crop of the desktop texture to the window size
	windowFbo.allocate(windowWidth, windowHeight, GL_RGBA);

	// Window sending buffer
	windowBuffer = (unsigned char *)malloc(windowWidth*windowHeight * 4 * sizeof(unsigned char));
	
	// Window drawing texture
	windowTexture.allocate(windowWidth, windowHeight, GL_RGBA);

	// Window GDI capture
	windowHwnd = NULL; // selected window

	// Load a font rather than the default
	if (!myFont.load("fonts/DejaVuSansCondensed-Bold.ttf", 14, true, true))
		SpoutLogWarning("ofApp error - Font not loaded");

}

bool ofApp::setupDesktopDuplication() {

	LONG width = 0;
	LONG height = 0;
	IDXGIFactory1* factory = NULL;
	IDXGIOutput1* output1 = NULL;
	IDXGIAdapter1* adapter = NULL;
	HRESULT hr = NULL;

	if (!g_d3dDevice) {
		SpoutLogError("setupDesktopDuplication : no device");
		return false;
	}

	g_d3dDevice->GetImmediateContext(&g_d3dDeviceContext);

	CreateDXGIFactory1(__uuidof(IDXGIFactory1), reinterpret_cast<void**>(&factory));

	for (int i = 0; (factory->EnumAdapters1(i, &adapter) != DXGI_ERROR_NOT_FOUND); ++i) {
		IDXGIOutput* output;
		for (int j = 0; (adapter->EnumOutputs(j, &output) != DXGI_ERROR_NOT_FOUND); j++) {

			DXGI_OUTPUT_DESC outputDesc;
			output->GetDesc(&outputDesc);

			MONITORINFOEX monitorInfo;
			monitorInfo.cbSize = sizeof(MONITORINFOEX);
			GetMonitorInfo(outputDesc.Monitor, &monitorInfo);

			LONG w = 0;
			LONG h = 0;
			w = monitorInfo.rcMonitor.right - monitorInfo.rcMonitor.left;
			h = monitorInfo.rcMonitor.bottom - monitorInfo.rcMonitor.top;

			if (monitorInfo.dwFlags == MONITORINFOF_PRIMARY) {
				width = monitorInfo.rcMonitor.right - monitorInfo.rcMonitor.left;
				height = monitorInfo.rcMonitor.bottom - monitorInfo.rcMonitor.top;
				// A process can have only one desktop duplication interface on a single desktop output;
				// however, that process can have a desktop duplication interface for each output 
				// that is part of the desktop.
				output1 = reinterpret_cast<IDXGIOutput1*>(output);
				hr = output1->DuplicateOutput(g_d3dDevice, &g_deskDupl);
				/// https://msdn.microsoft.com/en-gb/library/windows/desktop/hh404600(v=vs.85).aspx
				if (FAILED(hr)) {
					SpoutLogError("setupDesktopDuplication : DuplicateOutput failed");
					output->Release();
					adapter->Release();
					factory->Release();
					return false;
				}
			}
			output->Release();
		}
		adapter->Release();
	}
	factory->Release();

	monitorWidth = width;
	monitorHeight = height;

	return true;
}


//
// Desktop duplication method to capture the entire desktop
//
// https://msdn.microsoft.com/en-us/library/windows/desktop/hh404487(v=vs.85).aspx
//
//
bool ofApp::capture_desktop() {

	HRESULT hr = S_OK;
	IDXGIResource* DesktopResource = NULL;
	DXGI_OUTDUPL_FRAME_INFO FrameInfo;

	// Allow for UAC disabling desktop duplication
	if (g_deskDupl == NULL) {
		if (!setupDesktopDuplication()) {
			return false;
		}
		return true; // skip this frame
	}

	// Get new frame
	hr = g_deskDupl->AcquireNextFrame(500, &FrameInfo, &DesktopResource);
	if (FAILED(hr)) {
		if ((hr != DXGI_ERROR_ACCESS_LOST) && (hr != DXGI_ERROR_WAIT_TIMEOUT)) {
			SpoutLogError("Failed to acquire next frame in DUPLICATIONMANAGER");
		}
		else if(hr == DXGI_ERROR_ACCESS_LOST) {
			// DXGI_ERROR_ACCESS_LOST if the desktop duplication interface is invalid.
			// The desktop duplication interface typically becomes invalid when a
			// different type of image is displayed on the desktop.
			// Examples of this situation are:
			//		Desktop switch
			//		Mode change
			//		Switch from DWM on, DWM off, or other full - screen application
			// In this situation, the application must release the IDXGIOutputDuplication interface
			// and create a new IDXGIOutputDuplication for the new content.
			g_deskDupl->Release();
			g_deskDupl = NULL;
		}
		return false;
	}

	// Query Interface for the texture from the desktop resource
	// The format of the desktop image is always DXGI_FORMAT_B8G8R8A8_UNORM
	// no matter what the current display mode is.
	hr = DesktopResource->QueryInterface(__uuidof(ID3D11Texture2D),
		reinterpret_cast<void **>(&g_pDeskTexture));

	// Done with the Desktop resource
	DesktopResource->Release();
	DesktopResource = NULL;

	if (hr == S_OK) {
		// Send the DX11 texture from the desktop resource and at the same
		// time read back the linked OpenGL texture for the window sender
		desktopSender.spout.WriteTextureReadback(&g_pDeskTexture,
			desktopTexture.getTextureData().textureID,
			desktopTexture.getTextureData().textureTarget,
			monitorWidth, monitorHeight, false);
	}

	// Release the frame for the next round
	g_deskDupl->ReleaseFrame();

	return true;

}

bool ofApp::capture_window(HWND hwnd)
{
	// Closed window or self
	if (hwnd == NULL || hwnd == ofGetWin32Window() || hwnd == GetConsoleWindow() || !IsWindow(hwnd)) {
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


//--------------------------------------------------------------
void ofApp::exit() {

	desktopSender.ReleaseSender();
	windowSender.ReleaseSender();
	if (g_pDeskTexture)
		g_pDeskTexture->Release();
	if(g_hMouseHook)
		UnhookWindowsHookEx(g_hMouseHook);

}

//--------------------------------------------------------------
void ofApp::update() {

	if (!bInitialized) {
		// Create the window sender first so that the desktop sender is set as active
	    windowSender.CreateSender("WindowSender", ofGetWidth(), ofGetHeight());
		// This sets up the interop device and object for WriteTextureReadBack to use
		desktopSender.CreateSender("DesktopSender", ofGetScreenWidth(), ofGetScreenHeight());
		bInitialized = true;
	}

	// Always capture using the desktop duplication method.
	// The DirectX desktop texture is sent by capture_desktop().
	// A readback texture allows the desktop to be drawn
	// and a portion of it drawn to fbo for region capture.
	capture_desktop();

	if (bRegion) {

		//
		// Region - using desktop duplication capture
		//
		if (bResized) {
			// Resize the window sender fbo
			windowFbo.allocate(windowWidth, windowHeight, GL_RGBA);
			bResized = false;
		}

		// We have the client rectangle dimensions, get the top, left position
		if (!IsIconic(g_hWnd)) {
			RECT rect{};
			GetWindowRect(g_hWnd, &rect);
			positionLeft = rect.left + GetSystemMetrics(SM_CYFRAME) * 2;
			positionTop = rect.top + GetSystemMetrics(SM_CYMENU) + GetSystemMetrics(SM_CYCAPTION) + GetSystemMetrics(SM_CYFRAME) * 2;
		}

		// Draw a portion of the readback texture to the window sender fbo.
		// Sender width and height mirror the ofApp window.
		ofSetColor(255);
		windowFbo.bind();

		desktopTexture.drawSubsection(0, 0,
			(float)windowWidth, // size to draw and crop
			(float)windowHeight,
			(float)positionLeft, // position to crop at
			(float)positionTop);

		// Send the window fbo
		// Invert because DirectX textures have origin reverse in y compared to OpenGL
		windowSender.SendFbo(windowFbo.getId(), windowWidth, windowHeight, true);

		windowFbo.unbind();

	}
	else if (bWindow) {

		//
		// Window selection
		//

		if (bRHdown) {

			// The user has selected a new window
			// Get the window handle from the mouse hook click position
			POINT p;
			p.x = xCoord;
			p.y = yCoord;
			HWND hwnd = WindowFromPoint(p);

			// Set global selected window handle
			// If the window closes it is tested by IsWindow in capture_window
			windowHwnd = hwnd;

			char str[256]{};
			GetWindowTextA(hwnd, str, 256);

			RECT rect{};
			GetClientRect(hwnd, &rect);
			// printf("Window = %s (0X%7.7X) %dx%d\n", str, PtrToUint(hwnd), rect.right - rect.left, rect.bottom - rect.top);

			// Look for Class to avoid console
			GetClassNameA(hwnd, str, 256);
			if (strcmp(str, "ConsoleWindowClass") != 0) {
				unsigned int width = rect.right - rect.left;
				unsigned int height = rect.bottom - rect.top;
				if (width != windowWidth || height != windowHeight) {
					windowWidth = width;
					windowHeight = height;
					windowTexture.allocate(windowWidth, windowHeight, GL_RGBA);
					if (windowBuffer) free((void *)windowBuffer);
					windowBuffer = (unsigned char *)malloc(windowWidth*windowHeight * 4 * sizeof(unsigned char));
					if (bInitialized)
						windowSender.UpdateSender("WindowSender", windowWidth, windowHeight);
				}
			}

			// Re-set focus
			SetFocus(ofGetWin32Window());

			// Enable the menu item
			menu->SetPopupItem("Window", true);

			// Reset mouse hook flag
			bRHdown = false;

			// Release the mouse hook when it's no longer needed.
			// This will stop the mouse from lagging and affecting the whole system.
			if (g_hMouseHook) UnhookWindowsHookEx(g_hMouseHook);
			g_hMouseHook = NULL;

		}

		//
		// GDI capture
		//
		if (capture_window(windowHwnd)) {

			// find the current selected window position and size
			RECT rect{};
			GetClientRect(windowHwnd, &rect);
			// Check the size and position which the user might have changed
			unsigned int width = rect.right - rect.left;
			unsigned int height = rect.bottom - rect.top;
			if (width != windowWidth || height != windowHeight) {
				// Update globals and buffers
				windowWidth = width;
				windowHeight = height;
				windowTexture.allocate(windowWidth, windowHeight, GL_RGBA);
				if (windowBuffer) free((void *)windowBuffer);
				windowBuffer = (unsigned char *)malloc(windowWidth*windowHeight * 4 * sizeof(unsigned char));
			}
			else {
				// Send window texture, loaded from GDI pixels
				if (bInitialized && windowTexture.isAllocated()) {
					if (!IsIconic(g_hWnd)) {
						// 3 msec higher speed than SendImage compensates for loadData to texture in Draw()
						// If not iconic, capture time is approximately the same (14-16 msec full screen window)
						windowSender.SendTexture(windowTexture.getTextureData().textureID,
							windowTexture.getTextureData().textureTarget, windowWidth, windowHeight, GL_BGRA_EXT);
					}
					else {
						windowSender.SendImage(windowBuffer, windowWidth, windowHeight);
					}
					// Can limit fps here to hold a more constant rate
					// However, 60 fps is typically achieved
					// windowSender.HoldFps(30);
				}
			}
		}
	}

}

//--------------------------------------------------------------
void ofApp::draw() {

	// do not process if Iconic
	if (IsIconic(g_hWnd))
		return;

	if (bDesktop) {
		//
		// Desktop mode
		//
		// Draw the entire desktop.
		ofBackground(0, 0, 0, 255);
		// Draw the desktop readback texture
		desktopTexture.draw(0, 0, ofGetWidth(), ofGetHeight());
	}
	else if (bWindow) {
		//
		// GDI capture
		//
		ofBackground(128); // Grey for no capture

		if (windowHwnd && windowBuffer) {
			windowTexture.loadData((const unsigned char *)windowBuffer, windowWidth, windowHeight, GL_BGRA_EXT);
			windowTexture.draw(0, 0, ofGetWidth(), ofGetHeight());
		}
		else {
			// No window captured - instruct user
			ofSetColor(255, 0, 0);
			myFont.drawString("Click MIDDLE mouse button on the window to capture", 50, ofGetHeight() / 2);
			ofSetColor(255);
			// Set mouse hook here and release after button press is processed
			if (!g_hMouseHook) g_hMouseHook = SetWindowsHookEx(WH_MOUSE_LL, LowLevelMouseProc, NULL, 0);
		}
	}
	else if(bRegion) {
		// Red background becomes transparent
		// draw is not necessary
		ofBackground(255, 0, 0, 255);
	}

	// Capture frame rate display
	if (bShowfps) {
		char tmp[64];
		ofSetColor(255, 255, 0);
		sprintf_s(tmp, 64, "Fps - %d", (int)roundf(ofGetFrameRate()));
		myFont.drawString(tmp, ofGetWidth() - 110, 30);
		ofSetColor(255);
	}

}

//--------------------------------------------------------------
void ofApp::windowResized(int w, int h) {

	// Do not process before Setup
	if (!g_d3dDevice)
		return;

	// do not process if Iconic
	if (IsIconic(g_hWnd))
		return;

	if (w > 0 && h > 0) {
		if (w != (int)windowSender.GetWidth() || h != (int)windowSender.GetHeight()) {
			// Update the sender dimensions
			// The changes will take effect in Draw()
			windowWidth = (unsigned int)w;
			windowHeight = (unsigned int)h;
			bResized = true;
		}
	}

}

//--------------------------------------------------------------
//
// Menu function callback
//
// This function is called by ofxWinMenu when an item is selected.
// The the title and state can be checked for required action.
// 
void ofApp::appMenuFunction(string title, bool bChecked) {

	//
	// File menu
	//

	if (title == "Exit") {
		ofExit(); // Quit the application
	}

	//
	// Capture menu
	//

	if (title == "Desktop") {
		bDesktop = true;
		bRegion = false;
		bWindow = false;
		menu->SetPopupItem("Region", false);
		menu->SetPopupItem("Window", false);
		// Set desktop sender active
		desktopSender.SetActiveSender("DesktopSender");

		// Disable layered style
		HWND hwnd = ofGetWin32Window();
		LONG_PTR dwStyle = GetWindowLongPtrA(hwnd, GWL_EXSTYLE);
		SetWindowLongPtrA(hwnd, GWL_EXSTYLE, dwStyle ^= WS_EX_LAYERED);

	}

	if (title == "Window") {

		// Disable layered style
		HWND hwnd = ofGetWin32Window();
		LONG_PTR dwStyle = GetWindowLongPtrA(hwnd, GWL_EXSTYLE);
		SetWindowLongPtrA(hwnd, GWL_EXSTYLE, dwStyle ^= WS_EX_LAYERED);

		bWindow = true;
		bDesktop = false;
		bRegion = false;
		menu->SetPopupItem("Desktop", false);
		menu->SetPopupItem("Region", false);
		menu->SetPopupItem("Window", false);

		// Always select a new window to capture
		windowHwnd = nullptr;

		// clear buffer to grey
		if (windowBuffer) {
			memset((void *)windowBuffer, 128, windowWidth*windowHeight * 4);
			// Send a grey image frame to signal to overwrite the previous one
			if (bInitialized)
				windowSender.SendImage(windowBuffer, windowWidth, windowHeight, GL_BGRA_EXT);
		}
		// Set window sender active
		if (bInitialized) desktopSender.SetActiveSender("WindowSender");
	}

	if (title == "Region") {
		bRegion = true;
		bDesktop = false;
		bWindow = false;
		menu->SetPopupItem("Desktop", false);
		menu->SetPopupItem("Window", false);

		//
		// Create a transparent window
		// https://stackoverflow.com/questions/3970066/creating-a-transparent-window-in-c-win32
		//
		// Set layered style
		HWND hwnd = ofGetWin32Window();
		LONG_PTR dwStyle = GetWindowLongPtrA(hwnd, GWL_EXSTYLE);
		SetWindowLongPtrA(hwnd, GWL_EXSTYLE, dwStyle | WS_EX_LAYERED);

		// Make red pixels transparent:
		SetLayeredWindowAttributes(hwnd, RGB(255, 0, 0), 0, LWA_COLORKEY);

		// Set window sender active
		desktopSender.SetActiveSender("WindowSender");
	}

	if (title == "Show fps") {
		bShowfps = bChecked;
	}

	if (title == "Show on top") {
		bTopmost = bChecked;
		doTopmost(bTopmost);
	}


	//
	// Help menu
	//
	if (title == "Documentation") {
		char path[MAX_PATH];
		HMODULE hModule = GetModuleHandle(NULL);
		GetModuleFileNameA(hModule, path, MAX_PATH);
		PathRemoveFileSpecA(path);
		strcat_s(path, MAX_PATH, "\\SpoutCapture.pdf");
		ShellExecuteA(g_hWnd, "open", path, NULL, NULL, SW_SHOWNORMAL);
	}

	if (title == "About") {

		char about[1024]{};
		char tmp[MAX_PATH]{};
		DWORD dwSize, dummy;

		sprintf_s(about, 256, "          SpoutCapture - Version ");
		// Get product version number
		if (GetModuleFileNameA(NULL, tmp, MAX_PATH)) {
			dwSize = GetFileVersionInfoSizeA(tmp, &dummy);
			if (dwSize > 0) {
				vector<BYTE> data(dwSize);
				if (GetFileVersionInfoA(tmp, NULL, dwSize, &data[0])) {
					LPVOID pvProductVersion = NULL;
					unsigned int iProductVersionLen = 0;
					if (VerQueryValueA(&data[0], ("\\StringFileInfo\\080904E4\\ProductVersion"), &pvProductVersion, &iProductVersionLen)) {
						sprintf_s(tmp, MAX_PATH, "%s\n", (char*)pvProductVersion);
						strcat_s(about, 1024, tmp);
					}
				}
			}
		}

		strcat_s(about, 1024, "\n");
		strcat_s(about, 1024, "                  <a href=\"http://spout.zeal.co\">http://spout.zeal.co</a>\n");
		strcat_s(about, 1024, "\n");

		strcat_s(about, 1024, "          High speed capture of the desktop\n");
		strcat_s(about, 1024, "          or the part under the app window.\n");
		strcat_s(about, 1024, "          Or capture any other window\n");
		strcat_s(about, 1024, "          with middle mouse button click.\n");
		// Show fps
		sprintf_s(tmp, MAX_PATH, "          (Current capture frame rate - %d)\n\n", (int)roundf(ofGetFrameRate()));
		strcat_s(about, 1024, tmp);
		strcat_s(about, 1024, "          If you find SpoutCapture useful\n");
		strcat_s(about, 1024, "          please donate to the Spout project\n\n");

		SpoutMessageBoxIcon(LoadIconA(GetModuleHandle(NULL), MAKEINTRESOURCEA(IDI_ICON1)));
		SpoutMessageBox(NULL, about, "SpoutCapture", MB_OK | MB_USERICON);

	}

} // end appMenuFunction


void ofApp::doTopmost(bool bTop)
{
	if (bTop) {
		// get the current top window for return
		g_hwndForeground = GetForegroundWindow();
		// Set this window topmost
		SetWindowPos(g_hWnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
		ShowWindow(g_hWnd, SW_SHOW);
	}
	else {
		SetWindowPos(g_hWnd, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
		ShowWindow(g_hWnd, SW_SHOW);
		// Reset the window that was topmost before
		if (GetWindowLongPtrA(g_hwndForeground, GWL_EXSTYLE) & WS_EX_TOPMOST)
			SetWindowPos(g_hwndForeground, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
		else
			SetWindowPos(g_hwndForeground, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
	}
}


//
// Callback-Function for mouse hook
//
static LRESULT CALLBACK LowLevelMouseProc(int nCode, WPARAM wParam, LPARAM lParam)
{

	// < 0 doesn't concern us
	// If nCode is less than zero, the hook procedure must return the value returned by CallNextHookEx
	if (nCode < 0) return CallNextHookEx(g_hMouseHook, nCode, wParam, lParam);

	MSLLHOOKSTRUCT * pMouseStruct = (MSLLHOOKSTRUCT *)lParam; // WH_MOUSE_LL struct
	bool bProcessed = false;

	// If nCode is greater than or equal to zero, and the hook procedure did not process the message, 
	// it is highly recommended that you call CallNextHookEx and return the value it returns; 
	// otherwise, other applications that have installed WH_MOUSE_LL hooks will not receive
	// hook notifications and may behave incorrectly as a result. 
	//
	// If the hook procedure processed the message, it may return a nonzero value
	// to prevent the system from passing the message to the rest of the hook chain
	// or the target window procedure. 
	//
	if (nCode == HC_ACTION) {
		// Look for middle button down
		bRHdown = false;
		if (wParam == WM_MBUTTONDOWN) {
			// Only act for a new window selection
			if (!windowHwnd) {
				xCoord = pMouseStruct->pt.x;
				yCoord = pMouseStruct->pt.y;
				bRHdown = true;
			}
			bProcessed = true;
		}
	}

	if (bProcessed)
		return 1L;
	else
		return CallNextHookEx(g_hMouseHook, nCode, wParam, lParam);
}

// ... the end

