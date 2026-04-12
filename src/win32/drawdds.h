// ---------------------------------------------------------------------------
//  M88 - PC8801 emulator
//  Copyright (C) cisc 1998, 1999.
// ---------------------------------------------------------------------------
//	DirectDraw ‚É‚æ‚é‘S‰æ–Ê•`‰æ
// ---------------------------------------------------------------------------
//	$Id: DrawDDS.h,v 1.5 2000/01/10 08:25:33 cisc Exp $

#if !defined(win32_drawdds_h)
#define win32_drawdds_h

#include "windraw.h"

// ---------------------------------------------------------------------------

class WinDrawDDS : public WinDrawSub
{
public:
	WinDrawDDS(bool force480 = true);
	~WinDrawDDS();

	bool Init(HWND hwnd);
	void Resize(uint w, uint h);
	bool Cleanup();

	void SetPalette(PALETTEENTRY* pal);
	void QueryNewPalette();
	void DrawScreen(int top, int bottom, bool refresh);
	bool Lock(uint8** pimage, int* pbpl);
	bool Unlock();
	void SetGUIMode(bool guimode);
	void Flip();
	bool SetFlipMode(bool f);

private:
	void FillBlankArea();
	bool RestoreSurface();
	bool SetScreenMode();
	bool CreateDDPalette();
	bool CreateDD2();
	bool CreateDDS();

	static HRESULT WINAPI EDMCallBack(LPDDSURFACEDESC, LPVOID);

	HWND hwnd;

	LPDIRECTDRAW2 ddraw;
	LPDIRECTDRAWPALETTE ddpal;
	LPDIRECTDRAWSURFACE ddsscrn;
	LPDIRECTDRAWCLIPPER ddcscrn;
	LPDIRECTDRAWSURFACE ddswork;
	LPDIRECTDRAWSURFACE ddsback;
	
	int lines;		// 400 or 480
	int screenheight;
	
	bool palchanged;
	bool locked;
	bool guimode;
	bool shouldflip;
	bool bsvideo;
	bool doflip;
	HANDLE hevflip;
	RECT rcprev[2];
	
	PALETTEENTRY palentry[256];
};


#endif // !defined(win32_drawdds_h)
