#pragma once

#include "ofMain.h"
#include "resource.h"
#include "ofxWinMenu.h" // Addon for a windows menu
#include "..\apps\SpoutGL\SpoutSender.h" // Spout 2.007 beta (subject to change)
#include <dxgi1_2.h> // Desktop Duplication

#include <shlwapi.h> // For PathRemoveFileSpecA
#pragma comment (lib, "shlwapi.lib")

class ofApp : public ofBaseApp {

public:

	void setup();
	void update();
	void exit();
	void draw();
	void windowResized(int w, int h);

	// Desktop sender
	SpoutSender desktopSender;

	// Window sender
	SpoutSender windowSender;
	unsigned int windowWidth = 0;
	unsigned int windowHeight = 0;
	unsigned char * windowBuffer = nullptr;

	// Application window position
	int positionTop = 0;
	int positionLeft = 0;

	// Window texture and fbo
	ofTexture windowTexture;
	ofFbo windowFbo;

	// Desktop readback texture
	ofTexture desktopTexture;

	// Screen draw font
	ofTrueTypeFont myFont;

	// Menu
	HWND g_hwndForeground = NULL;
	HINSTANCE g_hInstance = NULL;
	ofxWinMenu * menu;
	void appMenuFunction(string title, bool bChecked);
	bool bTopmost = false;
	void doTopmost(bool bTop);

	// Desktop duplication
	ID3D11Device* g_d3dDevice = NULL;
	ID3D11DeviceContext* g_d3dDeviceContext = NULL;
	IDXGIOutputDuplication* g_deskDupl = NULL;
	ID3D11Texture2D* g_pDeskTexture = NULL; // DX11 texture to shadow the screen
	HANDLE g_hSharehandle = NULL; // to create a texture (share handle is unused)
	unsigned int monitorWidth = 0;
	unsigned int monitorHeight = 0;
	bool setupDesktopDuplication();
	bool capture_desktop();
	
	// GDI capture
	HDC m_hWindowDC = NULL;
	HDC m_hWindowMemDC = NULL;
	HBITMAP m_hWindowBitmap = NULL;
	HBITMAP m_hWindowOld = NULL;
	bool capture_window(HWND hwnd);

	// Flags
	bool bInitialized = false;
	bool bDesktop = true;
	bool bRegion = false;
	bool bWindow = false;
	bool bResized = false;
	bool bShowfps = false;

	// For command line args to set the receiving sender name
	// e.g. "SpoutCapture" "Window Title"
	LPSTR lpCmdLine = nullptr;
};
