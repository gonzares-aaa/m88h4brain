// ---------------------------------------------------------------------------
//  M88 - PC88 emulator
//  Copyright (C) cisc 1998, 1999.
// ---------------------------------------------------------------------------
//	GDI による画面描画 (HiColor 以上)
// ---------------------------------------------------------------------------
//	$Id: DrawGDI.cpp,v 1.7 1999/09/25 03:16:01 cisc Exp $

//	bug:パレット～(T-T

#include "headers.h"
#include "drawgdi.h"

//	画面合成に heap で割り当てたメモリを使うか？
static const bool imageinheap = false;

// ---------------------------------------------------------------------------
//	構築/消滅
//
WinDrawGDI::WinDrawGDI()
{
	hwnd = 0;
	hbitmap = 0;
	updatepal = false;
	bitmapimage = 0;
	bpl = 640;
}

WinDrawGDI::~WinDrawGDI()
{
	Cleanup();
}

// ---------------------------------------------------------------------------
//	初期化処理
//
bool WinDrawGDI::Init(HWND hwindow)
{
	hwnd = hwindow;
	height = 400;

	if (!MakeBitmap()) return false;

	if (imageinheap)
	{
		image = new uint8[640*400];
		if (!image) return false;
	}
	else
		image = bitmapimage;
	memset(image, 0x40, 640*400);
	status |= Draw::shouldrefresh;
	return true;
}

// ---------------------------------------------------------------------------
//	後片付け
//
bool WinDrawGDI::Cleanup()
{
	if (hbitmap)
	{
		DeleteObject(hbitmap); hbitmap = 0;
	}
	if (imageinheap)
	{
		delete[] image; image = 0;
	}
	return true;
}

// ---------------------------------------------------------------------------
//	BITMAP 作成
//
bool WinDrawGDI::MakeBitmap()
{
	binfo.header.biSize          = sizeof(BITMAPINFOHEADER);
	binfo.header.biWidth         = 640;
	binfo.header.biHeight        = -400;
	binfo.header.biPlanes        = 1;
	binfo.header.biBitCount      = 8;
	binfo.header.biCompression   = BI_RGB;
	binfo.header.biSizeImage     = 0;
	binfo.header.biXPelsPerMeter = 0;
	binfo.header.biYPelsPerMeter = 0;
	binfo.header.biClrUsed       = 256;
	binfo.header.biClrImportant  = 0;

	// パレットない場合
	
	HDC hdc = GetDC(hwnd);
	memset(binfo.colors, 0, sizeof(RGBQUAD) * 256);

	hbitmap = CreateDIBSection( hdc,
								(BITMAPINFO*) &binfo,
								DIB_RGB_COLORS,
								(void**)(&bitmapimage),
								NULL, 
								0 );

	ReleaseDC(hwnd, hdc);

	if (!hbitmap)
		return false;
	
	return true;
}

// ---------------------------------------------------------------------------
//	パレット設定
//	index 番目のパレットに pe をセット
//
void WinDrawGDI::SetPalette(PALETTEENTRY* pe)
{
	for (int i=0; i<0x90; i++)
	{
#if 1
		binfo.colors[i+0x40].rgbRed = pe[i].peRed;
		binfo.colors[i+0x40].rgbBlue = pe[i].peBlue;
		binfo.colors[i+0x40].rgbGreen = pe[i].peGreen;
#else
		int c = (pe[i].peRed * 76 + pe[i].peGreen * 150 + pe[i].peBlue * 29) >> 8;
		c = Min(((c + 84) / 85) * 85, 255);
		binfo.colors[i+0x40].rgbRed = c;
		binfo.colors[i+0x40].rgbBlue = c;
		binfo.colors[i+0x40].rgbGreen = c;
#endif
	}
	updatepal = true;
}

// ---------------------------------------------------------------------------
//	WM_QUERYNEWPALETTE
//
void WinDrawGDI::QueryNewPalette()
{
}

// ---------------------------------------------------------------------------
//	描画
//
void WinDrawGDI::DrawScreen(int top, int bottom, bool refresh)
{
	if (imageinheap && top <= bottom)
	{
		uint8* src = image + top * 640;
		uint8* dest = bitmapimage + top * 640;
		memcpy(dest, src, 640 * (bottom - top+ 1));
	}

	if (refresh)
	{
		top = 0;
		bottom = 399;
	}
	
	if (top <= bottom || updatepal)
	{
		HDC hdc = GetDC(hwnd);
		HDC hmemdc = CreateCompatibleDC(hdc);
		HBITMAP oldbitmap = (HBITMAP) SelectObject(hmemdc, hbitmap);
		if (updatepal)
		{
			updatepal = false;
			SetDIBColorTable(hmemdc, 64, 144, &binfo.colors[64]);
			top = 0; bottom = height - 1;
		}
		BitBlt(hdc, 0, top, 640, Min(height, bottom) - top + 1, hmemdc, 0, top, SRCCOPY);
//		StretchBlt(hdc, 0, top/2, 640, (Min(height, bottom) - top + 1)/2, hmemdc, 0, top, 640, Min(height, bottom) - top + 1, SRCCOPY);
		if (bottom >= int(height))
		{
			HBRUSH hbrush = CreateSolidBrush(RGB(0, 0, 0));
			hbrush = (HBRUSH) SelectObject(hdc, hbrush);
			PatBlt(hdc, 0, height, 640, bottom-height+1, PATCOPY);
			SelectObject(hdc, hbrush);
		}
	
		SelectObject(hmemdc, oldbitmap);
		DeleteDC(hmemdc);
		ReleaseDC(hwnd, hdc);
	}
}

// ---------------------------------------------------------------------------
//	画面イメージの使用要求
//
bool WinDrawGDI::Lock(uint8** pimage, int* pbpl)
{
	*pimage = image;
	*pbpl = bpl;
	return true;
}

// ---------------------------------------------------------------------------
//	画面イメージの使用終了
//
bool WinDrawGDI::Unlock()
{
	status &= ~Draw::shouldrefresh;
	return true;
}

// ---------------------------------------------------------------------------
//	画面有効範囲を変更
//
void WinDrawGDI::Resize(uint w, uint h)
{
	height = h;
}
