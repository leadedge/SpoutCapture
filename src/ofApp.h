#pragma once

#include "ofMain.h"
#include "resource.h"
#include "ofxWinMenu.h" // Addon for a windows menu
#include "..\apps\SpoutSDK\Spout.h" // Spout 2.007 beta (subject to change)
#include <dxgi1_2.h> // Desktop Duplication

class ofApp : public ofBaseApp {

public:

	void setup();
	void update();
	void exit();
	void draw();
	void windowResized(int w, int h);

	// Desktop sender
	SpoutSender desktopSender;

	// Desktop readback texture
	ofTexture desktopTexture;

	// Window sender
	SpoutSender windowSender;
	unsigned int windowWidth = 0;
	unsigned int windowHeight = 0;
	ofFbo windowFbo;

	// Menu
	HWND g_hWnd = NULL;
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

	// Flags
	bool bDesktop = true;
	bool bResized = false;

};
