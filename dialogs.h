#ifndef DIALOGS_H
#define DIALOGS_H

#include "animation.h"
#include "engine.h"

// There are eight predefined colors
static const int NUM_COLORS = 8;

static const COLORREF PredefinedColors[NUM_COLORS] = {
	RGB( 78,150,237), RGB(237, 78, 78), RGB(119,237, 78), RGB(237,149, 78),
	RGB(111,217,224), RGB(224,111,209), RGB(224,215,111), RGB(109, 75,  5)
};


// Global info about the applications
struct ApplicationInfo
{
	HINSTANCE hInstance;

	HWND hColorsLabel;
	HWND hColorBtn[NUM_COLORS + 1];
	HWND hMainWnd;
	HWND hBonesCheckbox;
	HWND hNamesCheckbox;
	HWND hListBox;
	HWND hRenderWnd;
	HWND hTimeSlider;
	HWND hPlayButton;

	int                   selectedColor;
	bool			      playing;
	unsigned long         playStartTime;
	unsigned int          playStartFrame;

	Animation*            anim;
	Engine*               engine;
	class FileManager*    fileManager;
	class EffectManager*  effectManager;
	class TextureManager* textureManager;

	// Dragging
	enum { NONE, ROTATE, MOVE, ZOOM } dragmode;
	long   xstart;
	long   ystart;
	Camera startCam;

	ApplicationInfo()
	{
		hMainWnd     = hListBox = hRenderWnd = NULL;
		dragmode     = NONE;
	}

	~ApplicationInfo()
	{
		DestroyWindow( hRenderWnd );
		DestroyWindow( hListBox );
		DestroyWindow( hMainWnd );
	}
};

Model*     DlgOpenModel( ApplicationInfo *info );
Animation* DlgOpenAnimation( ApplicationInfo *info, const Model* model );

INT_PTR CALLBACK SelectFileDlgProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK ModelDetailsDlgProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

#endif