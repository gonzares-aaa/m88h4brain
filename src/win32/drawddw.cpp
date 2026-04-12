// ---------------------------------------------------------------------------
//  M88 - PC88 emulator
//  Copyright (C) cisc 1998.
// ---------------------------------------------------------------------------
//	DirectDraw によるウインドウ画面描画
//	8bpp 専用(T-T
// ---------------------------------------------------------------------------
//	$Id: drawddw.cpp,v 1.5 1999/09/25 03:16:02 cisc Exp $

#include "headers.h"
#include "drawddw.h"
#include "misc.h"

#define RELCOM(x)  if (x) x->Release(), x=0; else 0

// ---------------------------------------------------------------------------
//	構築/消滅
//
WinDrawDDW::WinDrawDDW()
{
	ddraw = 0;
	ddsscrn = 0;
	ddcscrn = 0;
	ddpal = 0;
	ddswork = 0;
	scrnhaspal = false;
	palchanged = false;
}

WinDrawDDW::~WinDrawDDW()
{
	Cleanup();
}

// ---------------------------------------------------------------------------
//	初期化
//
bool WinDrawDDW::Init(HWND hwindow)
{
	hwnd = hwindow;

	if (!CreateDD2())
		return false;

	if (DD_OK != ddraw->SetCooperativeLevel(hwnd, DDSCL_NORMAL))
		return false;

	if (!CreateDDS())
		return false;

	CreateDDPalette();
	status |= Draw::shouldrefresh;
	return true;
}

// ---------------------------------------------------------------------------
//	Cleanup
//
bool WinDrawDDW::Cleanup()
{
	RELCOM(ddpal);
	RELCOM(ddcscrn);
	RELCOM(ddswork);
	RELCOM(ddsscrn);
	RELCOM(ddraw);
	return true;
}

// ---------------------------------------------------------------------------
//	DirectDraw2 準備
//
bool WinDrawDDW::CreateDD2()
{
	LPDIRECTDRAW ddraw1;
	if (DD_OK != DirectDrawCreate(0, &ddraw1, 0))
		return false;
	bool r = (S_OK == ddraw1->QueryInterface(IID_IDirectDraw2, (void**)&ddraw));
	ddraw1->Release();
	return r;
}

// ---------------------------------------------------------------------------
//	Surface 準備
//
bool WinDrawDDW::CreateDDS()
{
	// 表示サーフェスを作成
	DDSURFACEDESC ddsd;
	memset(&ddsd, 0, sizeof(DDSURFACEDESC));
	ddsd.dwSize = sizeof(ddsd);
	ddsd.dwFlags = DDSD_CAPS;
	ddsd.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE;

	if (DD_OK != ddraw->CreateSurface(&ddsd, &ddsscrn, 0))
		return false;

	// クリッパーをつける
	if (DD_OK != ddraw->CreateClipper(0, &ddcscrn, 0))
		return false;

	ddcscrn->SetHWnd(0, hwnd);
	ddsscrn->SetClipper(ddcscrn);

	// 表示サーフェスの色フォーマットを得る
	DDPIXELFORMAT ddpf;
	memset(&ddpf, 0, sizeof(ddpf));
	ddpf.dwSize = sizeof(DDPIXELFORMAT);
	if (DD_OK != ddsscrn->GetPixelFormat(&ddpf))
		return false;
	if (!(ddpf.dwFlags & DDPF_RGB))
		return false;
	scrnhaspal = !!(ddpf.dwFlags & DDPF_PALETTEINDEXED8);

	// 作業用サーフェスを作成
	memset(&ddsd, 0, sizeof(DDSURFACEDESC));
	ddsd.dwSize = sizeof(ddsd);
	ddsd.dwFlags = DDSD_WIDTH | DDSD_HEIGHT | DDSD_CAPS;
	ddsd.dwWidth = 640;
	ddsd.dwHeight = 400;
	ddsd.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN | DDSCAPS_SYSTEMMEMORY;
	
	if (DD_OK != ddraw->CreateSurface(&ddsd, &ddswork, 0))
		return false;

	if (ddpf.dwRGBBitCount != 8)
		return false;
	
	return true;
}

// ---------------------------------------------------------------------------
//	パレット準備
//
bool WinDrawDDW::CreateDDPalette()
{
	int i;
	const int nsyscol = 10;
	for (i=0; i<nsyscol; i++)
	{
		palentry[i].peRed = i;
		palentry[i].peGreen = 0;
		palentry[i].peBlue = 0;
		palentry[i].peFlags = PC_EXPLICIT;
		palentry[255-i].peRed = 255-i;
		palentry[255-i].peGreen = 0;
		palentry[255-i].peBlue = 0;
		palentry[255-i].peFlags = PC_EXPLICIT;
	}
	for (i=nsyscol; i<0x40; i++)
	{
		palentry[i].peRed = 0;
		palentry[i].peGreen = 0;
		palentry[i].peBlue = 0;
		palentry[i].peFlags = PC_NOCOLLAPSE;
//		palentry[255-i].peRed = 0;
//		palentry[255-i].peGreen = 0;
//		palentry[255-i].peBlue = 0;
//		palentry[255-i].peFlags = PC_NOCOLLAPSE;
	}
	for (i=0x40; i<0xd0; i++)
	{
		palentry[i].peFlags = PC_RESERVED | PC_NOCOLLAPSE;
	}
	if (scrnhaspal)
	{
		if (DD_OK == ddraw->CreatePalette(DDPCAPS_8BIT, palentry, &ddpal, 0))
		{
			ddsscrn->SetPalette(ddpal);
		}
	}
	return true;
}

// ---------------------------------------------------------------------------
//	描画
//
void WinDrawDDW::DrawScreen(int top, int bottom, bool refresh)
{
	RECT rect;
	rect.left = 0;
	rect.top = top;
	rect.right = 640;
	rect.bottom = Min(400, bottom+1);

	if (palchanged)
	{
		palchanged = false;
		ddpal->SetEntries(0, 64, 144, &palentry[64]);
	}

	if (refresh)
		rect.top = 0, rect.bottom = 400;

	// 作業領域を更新
	if (rect.top < rect.bottom)
	{
		RECT rectdest;
		POINT srcpoint;
		srcpoint.x = 0, srcpoint.y = 0;
		ClientToScreen(hwnd, &srcpoint);
		rectdest.left = srcpoint.x + rect.left;
		rectdest.right = srcpoint.x + rect.right;
		rectdest.top = srcpoint.y + rect.top;
		rectdest.bottom = srcpoint.y + rect.bottom;
		
		if (DDERR_SURFACELOST == ddsscrn->Blt(&rectdest, ddswork, &rect, 0, 0))
		{
			RestoreSurface();
		}
	}
}

// ---------------------------------------------------------------------------
//	WM_QUERYNEWPALETTE
//
void WinDrawDDW::QueryNewPalette()
{
	if (scrnhaspal)
	{
		ddsscrn->SetPalette(ddpal);
	}
}

// ---------------------------------------------------------------------------
//	パレットを設定
//
void WinDrawDDW::SetPalette(PALETTEENTRY* pe)
{
	for (int i=0; i<0x90; i++)
	{
		palentry[i+0x40].peRed = pe[i].peRed;
		palentry[i+0x40].peBlue = pe[i].peBlue;
		palentry[i+0x40].peGreen = pe[i].peGreen;
	}
	palchanged = true;
}

// ---------------------------------------------------------------------------
//	画面イメージの使用要求
//
bool WinDrawDDW::Lock(uint8** pimage, int* pbpl)
{
	if (!locked)
	{
		locked = true;

		RECT rect;
		rect.left = 0, rect.top = 0;
		rect.right = 640, rect.bottom = 400;

		DDSURFACEDESC ddsd;
		memset(&ddsd, 0, sizeof(ddsd));
		ddsd.dwSize = sizeof(ddsd);
		if (DD_OK != ddswork->Lock(&rect, &ddsd, 0, 0))
			return false;

		*pimage = (uint8*) ddsd.lpSurface;
		*pbpl = ddsd.lPitch;

		return true;
	}
	return false;
}

// ---------------------------------------------------------------------------
//	画面イメージの使用終了
//
bool WinDrawDDW::Unlock()
{
	if (locked)
	{
		ddswork->Unlock(0);
		status &= ~Draw::shouldrefresh;
		locked = false;
	}
	return true;
}

// ---------------------------------------------------------------------------
//	ロストしたサーフェスを戻す
//
bool WinDrawDDW::RestoreSurface()
{
	if (DD_OK != ddsscrn->Restore() || DD_OK != ddswork->Restore())
		return false;

	status |= Draw::shouldrefresh;
	return true;
}


void WinDrawDDW::Resize(uint, uint)
{
}