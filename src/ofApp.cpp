//
//	SpoutCapture
//
//	Capture the visible desktop
//	Send the desktop and the part beneath the app window
//
//	SpoutCapture is Licensed with the LGPL3 license.
//	Copyright(C) 2019. Lynn Jarvis.
//	http://spout.zeal.co/
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
//	13/10/19	- Initial release 1.000
//				  VS2017 /MD Win32

#include "ofApp.h"

INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);

//--------------------------------------------------------------
void ofApp::setup() {

	// Enable a log file in ..\AppData\Roaming\Spout
	EnableSpoutLogFile("SpoutCapture");

	// Create a transparent window
	// https://stackoverflow.com/questions/3970066/creating-a-transparent-window-in-c-win32
	HWND hwnd = ofGetWin32Window();
	// Set layered style
	SetWindowLong(hwnd, GWL_EXSTYLE, GetWindowLong(hwnd, GWL_EXSTYLE) | WS_EX_LAYERED);
	// Make red pixels transparent:
	SetLayeredWindowAttributes(hwnd, RGB(255, 0, 0), 0, LWA_COLORKEY);

	// Get instance for dialogs
	g_hInstance = GetModuleHandle(NULL);

	// Window handle used for the menu
	g_hWnd = WindowFromDC(wglGetCurrentDC());

	// Resize to the menu
	int windowWidth = ofGetWidth();
	int windowHeight = ofGetHeight() + GetSystemMetrics(SM_CYMENU);
	ofSetWindowShape(windowWidth, windowHeight);

	// Centre on the screen
	ofSetWindowPosition((ofGetScreenWidth() - windowWidth) / 2, (ofGetScreenHeight() - windowHeight) / 2);

	// Set a custom window icon
	hwnd = WindowFromDC(wglGetCurrentDC());
	SetClassLong(hwnd, GCLP_HICON, (LONG)LoadIconA(GetModuleHandle(NULL), MAKEINTRESOURCEA(IDI_ICON1)));

	// Set the caption
	ofSetWindowTitle("SpoutCapture");

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
	// File popup menu
	//
	HMENU hPopup = menu->AddPopupMenu(hMenu, "File");
	menu->AddPopupItem(hPopup, "Exit", false, false);

	//
	// Window popup
	//
	hPopup = menu->AddPopupMenu(hMenu, "Window");
	// Show the entire desktop
	bDesktop = false;
	menu->AddPopupItem(hPopup, "Show desktop", false); // Checked and auto-check
	// Topmost
	bTopmost = false;
	menu->AddPopupItem(hPopup, "Show on top", false); // Not checked and auto-check

	//
	// Help popup
	//
	hPopup = menu->AddPopupMenu(hMenu, "Help");
	menu->AddPopupItem(hPopup, "Documentation", false, false); // No auto check
	menu->AddPopupItem(hPopup, "About", false, false); // No auto check

	// Set the menu to the window
	menu->SetWindowMenu();

	// Initialize for DirectX11 now which creates a Direct3D 11 device.
	desktopSender.spout.SetDX9(false); // Set this here because DX9 won't work
	if (!desktopSender.spout.interop.OpenDirectX11()) {
		MessageBoxA(NULL, "OpenDX11 failed", "Info", MB_OK);
		return;
	}

	// Get the Spout DX11 device to create the resources
	// The device may have been created as Direct3D 11.0
	// and not 11.1, but it still seems to work OK
	g_d3dDevice = desktopSender.spout.interop.g_pd3dDevice;

	// Setup Desktop duplication which establishes monitorWidth and monitorHeight
	if (!setupDesktopDuplication()) {
		MessageBoxA(NULL, "Desktop duplication interface creation failed", "Error", MB_OK);
		return;
	}

	// Create a DX11 texture to receive the duplication texture
	// Use DXGI_FORMAT_B8G8R8A8_UNORM because that is the same format as
	// the Spout shared texture and is also the format of the desktop image
	// no matter what the current display mode is. 
	// Use a Spout function - the texture is shareable but that has no effect
	desktopSender.spout.interop.spoutdx.CreateSharedDX11Texture(g_d3dDevice,
		monitorWidth, monitorHeight,
		DXGI_FORMAT_B8G8R8A8_UNORM, &g_pDeskTexture, g_hSharehandle);

	// Allocate a readback OpenGL texture for the desktop
	desktopTexture.allocate(monitorWidth, monitorHeight, GL_RGBA);

	// Create a desktop sender so that receivers can access the entire desktop
	desktopSender.SetupSender("SpoutDesktop", monitorWidth, monitorHeight);

	// Create a sender the size of the OF window
	// to send the part of the desktop under the window
	windowWidth = (unsigned int)ofGetWidth();
	windowHeight = (unsigned int)ofGetHeight();
	windowSender.SetupSender("SpoutWindow", windowWidth, windowHeight);

	// Fbo for crop of the desktop texture to the window size
	windowFbo.allocate(windowWidth, windowHeight, GL_RGBA);

}

bool ofApp::setupDesktopDuplication() {

	LONG width = 0;
	LONG height = 0;
	IDXGIFactory1* factory = NULL;
	IDXGIOutput1* output1 = NULL;
	IDXGIAdapter1* adapter = NULL;
	HRESULT hr = NULL;

	if (!g_d3dDevice)
		return false;

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

	if (g_deskDupl == NULL)
		return false;

	// Get new frame
	hr = g_deskDupl->AcquireNextFrame(500, &FrameInfo, &DesktopResource);
	if (FAILED(hr)) {
		if ((hr != DXGI_ERROR_ACCESS_LOST) && (hr != DXGI_ERROR_WAIT_TIMEOUT)) {
			printf("Failed to acquire next frame in DUPLICATIONMANAGER\n");
		}
		return false;
	}

	// Query Interface for the texture from the desktop resource
	hr = DesktopResource->QueryInterface(__uuidof(ID3D11Texture2D),
		reinterpret_cast<void **>(&g_pDeskTexture));

	// Done with the Desktop resource
	DesktopResource->Release();
	DesktopResource = NULL;

	// Send the DX11 texture from the desktop resource
	// to the Spout desktop sender and at the same time
	// read back the linked OpenGL texture for the window sender
	desktopSender.spout.interop.WriteTextureReadback(&g_pDeskTexture,
		desktopTexture.getTextureData().textureID,
		desktopTexture.getTextureData().textureTarget,
		monitorWidth, monitorHeight);

	// Release the frame for the next round
	g_deskDupl->ReleaseFrame();

	return true;

}

//--------------------------------------------------------------
void ofApp::exit() {

	if (g_pDeskTexture) 
		g_pDeskTexture->Release();
	desktopSender.ReleaseSender();
	windowSender.ReleaseSender();

}

//--------------------------------------------------------------
void ofApp::update() {

	// Capture the desktop
	capture_desktop();
}

//--------------------------------------------------------------
void ofApp::draw() {

	if (bDesktop) {
		
		//
		// Desktop mode - show the entire desktop
		// The desktop texture is already transferred
		// to the desktop sender by capture_desktop
		//

		// Black background is opaque
		ofBackground(0, 0, 0, 255);
		// Draw the desktop texture
		desktopTexture.draw(0, 0, ofGetWidth(), ofGetHeight());

	}
	else if (!IsIconic(g_hWnd))	{ // do not process if Iconic

		//
		// Window mode
		//

		if (bResized) {
			// Resize the window sender and sending fbo
			windowSender.UpdateSender("SpoutWindow", windowWidth, windowHeight);
			windowFbo.allocate(windowWidth, windowHeight, GL_RGBA);
			bResized = false;
		}

		// Red background becomes transparent
		ofBackground(255, 0, 0, 255);

		// Draw a portion of the total screen capture
		// to the window sender OpenGL texture via FBO
		// Sender width and height mirror the ofApp window
		ofSetColor(255);
		windowFbo.bind();
		desktopTexture.drawSubsection(0, 0,
			(float)ofGetWidth(), // size to draw and crop
			(float)ofGetHeight(),
			(float)ofGetWindowPositionX(), // position to crop at
			(float)ofGetWindowPositionY());
		windowFbo.unbind();

		// Send the window fbo texture
		windowSender.SendTextureData(windowFbo.getTexture().getTextureData().textureID,
			windowFbo.getTexture().getTextureData().textureTarget);

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
	// Window menu
	//

	if (title == "Show desktop") {

		bDesktop = bChecked;
		HWND hwnd = ofGetWin32Window();
		if (bDesktop) {
			DWORD dwStyle = GetWindowLong(hwnd, GWL_EXSTYLE);
			dwStyle ^= WS_EX_LAYERED;
			SetWindowLong(hwnd, GWL_EXSTYLE, dwStyle);
			SetLayeredWindowAttributes(hwnd, RGB(0, 0, 0), 255, LWA_COLORKEY);

		}
		else {
			SetWindowLong(hwnd, GWL_EXSTYLE, GetWindowLong(hwnd, GWL_EXSTYLE) | WS_EX_LAYERED);
			// Make red pixels transparent:
			SetLayeredWindowAttributes(hwnd, RGB(255, 0, 0), 0, LWA_COLORKEY);
		}
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
		DialogBoxA(g_hInstance, MAKEINTRESOURCEA(IDD_ABOUTBOX), g_hWnd, About);
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
		if (GetWindowLong(g_hwndForeground, GWL_EXSTYLE) & WS_EX_TOPMOST)
			SetWindowPos(g_hwndForeground, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
		else
			SetWindowPos(g_hwndForeground, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
	}
}


// Message handler for About box
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);
	char tmp[MAX_PATH];
	char about[1024];
	DWORD dummy, dwSize;
	LPDRAWITEMSTRUCT lpdis;
	HWND hwnd = NULL;
	HCURSOR cursorHand = NULL;
	HINSTANCE hInstance = GetModuleHandle(NULL);

	switch (message) {

	case WM_INITDIALOG:

		sprintf_s(about, 256, "SpoutCapture - Version ");
		// Get product version number
		if (GetModuleFileNameA(hInstance, tmp, MAX_PATH)) {
			dwSize = GetFileVersionInfoSizeA(tmp, &dummy);
			if (dwSize > 0) {
				vector<BYTE> data(dwSize);
				if (GetFileVersionInfoA(tmp, NULL, dwSize, &data[0])) {
					LPVOID pvProductVersion = NULL;
					unsigned int iProductVersionLen = 0;
					if (VerQueryValueA(&data[0], ("\\StringFileInfo\\080904E4\\ProductVersion"), &pvProductVersion, &iProductVersionLen)) {
						sprintf_s(tmp, MAX_PATH, "%s\n", (char *)pvProductVersion);
						strcat_s(about, 1024, tmp);
					}
				}
			}
		}
		strcat_s(about, 1024, "\n\n");
		strcat_s(about, 1024, "Capture and send the desktop.\n");
		strcat_s(about, 1024, "Send the part under the app window.\n\n");
		strcat_s(about, 1024, "If you find SpoutCapture useful\n");
		strcat_s(about, 1024, "please donate to the Spout project.");

		SetDlgItemTextA(hDlg, IDC_ABOUT_TEXT, (LPCSTR)about);

		//
		// Hyperlink hand cursor
		//
		cursorHand = LoadCursor(NULL, IDC_HAND);
		hwnd = GetDlgItem(hDlg, IDC_SPOUT_URL);
		SetClassLongA(hwnd, GCLP_HCURSOR, (long)cursorHand);
		break;


	case WM_DRAWITEM:

		// The blue hyperlinks
		lpdis = (LPDRAWITEMSTRUCT)lParam;
		if (lpdis->itemID == -1) break;
		SetTextColor(lpdis->hDC, RGB(6, 69, 173));
		switch (lpdis->CtlID) {
			case IDC_SPOUT_URL:
				DrawTextA(lpdis->hDC, "http://spout.zeal.co", -1, &lpdis->rcItem, DT_LEFT);
				break;
			default:
				break;
		}
		break;

	case WM_COMMAND:

		if (LOWORD(wParam) == IDC_SPOUT_URL) {
			sprintf_s(tmp, MAX_PATH, "http://spout.zeal.co");
			ShellExecuteA(hDlg, "open", tmp, NULL, NULL, SW_SHOWNORMAL);
			EndDialog(hDlg, 0);
			return (INT_PTR)TRUE;
		}

		if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL) {
			EndDialog(hDlg, LOWORD(wParam));
			return (INT_PTR)TRUE;
		}
		break;
	}
	return (INT_PTR)FALSE;
}
