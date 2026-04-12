// ---------------------------------------------------------------------------
//  M88 - PC88 emulator
//  Copyright (C) cisc 1998, 1999.
// ---------------------------------------------------------------------------
//	GDI ‚É‚æ‚é‰æ–Ê•`‰æ
// ---------------------------------------------------------------------------
//	$Id: DrawGDI.h,v 1.3 1999/08/01 14:19:14 cisc Exp $

#if !defined(win32_drawgdi_h)
#define win32_drawgdi_h

// ---------------------------------------------------------------------------

#include "windraw.h"

class WinDrawGDI : public WinDrawSub
{
public:
	WinDrawGDI();
	~WinDrawGDI();

	bool Init(HWND hwnd);
	void Resize(uint width, uint height);
	bool Cleanup();
	void SetPalette(PALETTEENTRY* pal);
	void QueryNewPalette();
	void DrawScreen(int top, int bottom, bool refresh);
	bool Lock(uint8** pimage, int* pbpl);
	bool Unlock();

private:
	struct BI256		// BITMAPINFO
	{
		BITMAPINFOHEADER header;
		RGBQUAD colors[256];
	};

private:
	bool	MakeBitmap();
	
	HBITMAP	hbitmap;
	uint8*	bitmapimage;
	HWND	hwnd;
	uint8*	image; 
	int		bpl;
	uint	height;
	bool	updatepal;
	BI256	binfo;
};

#endif // !defined(win32_drawgdi_h)
