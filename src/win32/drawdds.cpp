// ---------------------------------------------------------------------------
//  M88 - PC8801 emulator
//  Copyright (C) cisc 1998, 2000.
// ---------------------------------------------------------------------------
//	DirectDraw による全画面描画
// ---------------------------------------------------------------------------
//	$Id: DrawDDS.cpp,v 1.9 2000/01/10 08:25:32 cisc Exp $

#include "headers.h"
#include "drawdds.h"
#include "misc.h"

#define RELCOM(x)  if (x) x->Release(), x=0; else 0

// ---------------------------------------------------------------------------
//	構築
//
WinDrawDDS::WinDrawDDS(bool force480)
: hevflip(0), doflip(0)
{
	ddraw = 0;
	ddsscrn = 0;
	ddcscrn = 0;
	ddpal = 0;
	ddswork = 0;
	palchanged = false;
	guimode = true;
	lines = force480 ? 480 : 0;
}

// ---------------------------------------------------------------------------
//	破棄
//
WinDrawDDS::~WinDrawDDS()
{
	Cleanup();
}

// ---------------------------------------------------------------------------
//	初期化
//
bool WinDrawDDS::Init(HWND hwindow)
{
	hwnd = hwindow;

	screenheight = 400;

	if (!CreateDD2()) return false;

	if (DD_OK != ddraw->SetCooperativeLevel
						(hwnd, DDSCL_EXCLUSIVE | DDSCL_FULLSCREEN | DDSCL_ALLOWREBOOT))
		return false;

	if (!SetScreenMode()) return false;

	if (!CreateDDS()) return false;

	CreateDDPalette();

	guimode = true;
	locked = false;
	SetGUIMode(false);
	status |= Draw::shouldrefresh | Draw::flippable;
	hevflip = CreateEvent(0, 0, 0, 0);
	
	return true;
}

// ---------------------------------------------------------------------------
//	Cleanup
//
bool WinDrawDDS::Cleanup()
{
	if (locked)
	{
		Unlock();
	}
	if (ddraw)
	{
		ddraw->SetCooperativeLevel(hwnd, DDSCL_NORMAL);
	}
	CloseHandle(hevflip);
	
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
bool WinDrawDDS::CreateDD2()
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
bool WinDrawDDS::CreateDDS()
{
	// 表示サーフェスを作成
	DDSURFACEDESC ddsd;
	memset(&ddsd, 0, sizeof(DDSURFACEDESC));
	ddsd.dwSize = sizeof(ddsd);
	ddsd.dwFlags = DDSD_CAPS | DDSD_BACKBUFFERCOUNT;
	ddsd.dwBackBufferCount = 1;
	ddsd.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE | DDSCAPS_FLIP | DDSCAPS_COMPLEX;

	if (DD_OK != ddraw->CreateSurface(&ddsd, &ddsscrn, 0))
		return false;
	DDSCAPS ddsc;
	ddsc.dwCaps = DDSCAPS_BACKBUFFER;
	ddsscrn->GetAttachedSurface(&ddsc, &ddsback);
	ddsback->GetCaps(&ddsc);
	bsvideo = (ddsc.dwCaps & DDSCAPS_VIDEOMEMORY) != 0;

	// クリッパーを作成
	if (DD_OK != ddraw->CreateClipper(0, &ddcscrn, 0))
		return false;

	ddcscrn->SetHWnd(0, hwnd);

	// 作業用サーフェスを作成
	memset(&ddsd, 0, sizeof(DDSURFACEDESC));
	ddsd.dwSize = sizeof(ddsd);
	ddsd.dwFlags = DDSD_WIDTH | DDSD_HEIGHT | DDSD_CAPS;
	ddsd.dwWidth = 640;
	ddsd.dwHeight = 400;
	ddsd.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN | DDSCAPS_SYSTEMMEMORY;
	
	if (DD_OK != ddraw->CreateSurface(&ddsd, &ddswork, 0))
		return false;

	return true;
}

// ---------------------------------------------------------------------------
//	パレット準備
//
bool WinDrawDDS::CreateDDPalette()
{
	int i;

	HDC hdc = GetDC(hwnd);
	GetSystemPaletteEntries(hdc, 0, 256, palentry);
	ReleaseDC(hwnd, hdc);

	for (i=0x40; i<0xd0; i++)
	{
		palentry[i].peFlags = PC_RESERVED | PC_NOCOLLAPSE;
	}
	if (DD_OK == ddraw->CreatePalette(DDPCAPS_8BIT, palentry, &ddpal, 0))
	{
		ddsscrn->SetPalette(ddpal);
	}
	return true;
}

// ---------------------------------------------------------------------------
//	描画
//	flip は Flip() で行うのが筋だとおもうけど，DrawScreen とメイン処理は
//	別スレッドなのでこっちで flip する
//
void WinDrawDDS::DrawScreen(int top, int bottom, bool refresh)
{
	RECT rect;
	SetRect(&rect, 0, top, 640, Min(400, bottom+1));

	if (refresh)
	{
		rect.top = 0, rect.bottom = 400;
		FillBlankArea();
	}

	// 作業領域を更新
	HRESULT r;
	if (guimode || !doflip)
	{
		if (rect.top < rect.bottom)
		{
			RECT rectdest;
			rectdest.left = 0 + rect.left;
			rectdest.right = 0 + rect.right;
			rectdest.top = (lines - 400) / 2 + rect.top;
			rectdest.bottom = (lines - 400) / 2 + rect.bottom;
			
			r = ddsscrn->Blt(&rectdest, ddswork, &rect, 0, 0);
		}
		if (palchanged)
		{
			palchanged = false;
			ddpal->SetEntries(0, 0x40, 0x90, &palentry[64]);
		}
	}
	else
	{
		bool updated = false;
		
		RECT rcn;
		if (bsvideo)
		{
			rcn.left = 0;
//			rcn.top = Min(rect.top, Min(rcprev[0].top, rcprev[1].top));
			rcn.top = Min(rect.top, rcprev[0].top);
			rcn.right = 640;
//			rcn.bottom = Max(rect.bottom, Max(rcprev[0].bottom, rcprev[1].bottom));
			rcn.bottom = Max(rect.bottom, rcprev[0].bottom);
			rcprev[1] = rcprev[0];
			rcprev[0] = rect;
		}
		else
		{
			rcn = rect;
		}
		updated = true;

		if (rcn.top < rcn.bottom)
			r = ddsback->BltFast(0, (lines-400)/2+rcn.top, 
								 ddswork, &rcn, DDBLTFAST_NOCOLORKEY);
		ddsscrn->Flip(0, DDFLIP_WAIT);

		if (palchanged || !updated)
		{
			ddraw->WaitForVerticalBlank(DDWAITVB_BLOCKBEGIN, 0);
			if (palchanged)
			{
				palchanged = false;
				ddpal->SetEntries(0, 0x40, 0x90, &palentry[64]);
			}
		}
	}
	if (r == DDERR_SURFACELOST)
	{
		RestoreSurface();
	}
	SetEvent(hevflip);
}

void WinDrawDDS::Flip()
{
	if (!guimode)
	{
		WaitForSingleObject(hevflip, 20);
//		if (shouldflip)
//			ddsscrn->Flip(0, DDFLIP_WAIT), shouldflip = false;
//		else
//			ddraw->WaitForVerticalBlank(DDWAITVB_BLOCKBEGIN, 0);
	}
}

// ---------------------------------------------------------------------------
//	WM_QUERYNEWPALETTE
//
void WinDrawDDS::QueryNewPalette()
{
}

// ---------------------------------------------------------------------------
//	パレットを設定
//
void WinDrawDDS::SetPalette(PALETTEENTRY* pe)
{
	for (int i=0; i<0x90; i++)
	{
#if 1
		palentry[i+0x40].peRed = pe[i].peRed;
		palentry[i+0x40].peBlue = pe[i].peBlue;
		palentry[i+0x40].peGreen = pe[i].peGreen;
#else
		int c = (pe[i].peRed * 76 + pe[i].peGreen * 150 + pe[i].peBlue * 29) >> 8;
		c = Min(((c + 84) / 85) * 85, 255);
		palentry[i+0x40].peRed = c;
		palentry[i+0x40].peBlue = c;
		palentry[i+0x40].peGreen = c;
#endif
	}
	palchanged = true;
}

// ---------------------------------------------------------------------------
//	画面イメージの使用要求
//
bool WinDrawDDS::Lock(uint8** pimage, int* pbpl)
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
bool WinDrawDDS::Unlock()
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
//	画面モードを切り替える
//
bool WinDrawDDS::SetScreenMode()
{
	DDSURFACEDESC ddsd;
	memset(&ddsd, 0, sizeof(ddsd));
	ddsd.dwSize = sizeof(ddsd);
	ddsd.dwFlags = DDSD_WIDTH | DDSD_REFRESHRATE;
	ddsd.dwWidth = 640;
	ddsd.dwRefreshRate = doflip ? 60 : 0;
	
	if (!lines)
	{
		if (DD_OK != ddraw->EnumDisplayModes(0, &ddsd, reinterpret_cast<void*>(this), EDMCallBack))
			return false;
		if (!lines)
			lines = 480;
	}

	if (DD_OK != ddraw->SetDisplayMode(640, lines, 8, doflip ? 60 : 0, 0))
		return false;

	return true;
}

HRESULT WINAPI WinDrawDDS::EDMCallBack(LPDDSURFACEDESC pddsd, LPVOID context)
{
	WinDrawDDS* wd = reinterpret_cast<WinDrawDDS*> (context);

	if (pddsd->ddpfPixelFormat.dwRGBBitCount == 8)
	{
		if (pddsd->dwHeight == 400)
		{
			wd->lines = 400;
			return DDENUMRET_CANCEL;
		}
		if (pddsd->dwHeight == 480 && !wd->lines)
		{
			wd->lines = 480;
		}
	}
	return DDENUMRET_OK;
}

// ---------------------------------------------------------------------------
//	ロストしたサーフェスを戻す
//
bool WinDrawDDS::RestoreSurface()
{
	if (DD_OK != ddsscrn->Restore() || DD_OK != ddswork->Restore())
	{
		return false;
	}

	status |= Draw::shouldrefresh;
	FillBlankArea();
	return true;
}

// ---------------------------------------------------------------------------
//	非表示領域を消す
//
void WinDrawDDS::FillBlankArea()
{
	if (lines > 400)
	{
		DDBLTFX ddbltfx;
		ddbltfx.dwSize = sizeof(ddbltfx);
		ddbltfx.dwFillColor = 0;
		
		RECT rect;
		rect.left = 0;		rect.top = 0;
		rect.right = 640;	rect.bottom = (lines-400)/2;
		ddsscrn->Blt(&rect, NULL, NULL, DDBLT_COLORFILL, &ddbltfx);
		ddsback->Blt(&rect, NULL, NULL, DDBLT_COLORFILL, &ddbltfx);
		
		rect.top = (lines+400) / 2 + (400 - screenheight);
		rect.bottom = lines;
		ddsscrn->Blt(&rect, NULL, NULL, DDBLT_COLORFILL, &ddbltfx);
		ddsback->Blt(&rect, NULL, NULL, DDBLT_COLORFILL, &ddbltfx);
	}
}

// ---------------------------------------------------------------------------
//	GUI モード切り替え
//
void WinDrawDDS::SetGUIMode(bool newguimode)
{
	if (newguimode != guimode)
	{
		guimode = newguimode;
		if (guimode)
		{
			ddraw->FlipToGDISurface();
			ddsscrn->SetClipper(ddcscrn);
			status &= ~Draw::flippable;
		}
		else
		{
			ddsscrn->SetClipper(0);

			FillBlankArea();

			RECT rectsrc;
			rectsrc.left = 0;		rectsrc.top = 0;
			rectsrc.right = 640;	rectsrc.bottom = screenheight;

			RECT rectdest;
			rectdest.left = 0 + rectsrc.left;
			rectdest.right = 0 + rectsrc.right;
			rectdest.top = (lines - 400) / 2 + rectsrc.top;
			rectdest.bottom = (lines - 400) / 2 + rectsrc.bottom;
			
			ddsscrn->Blt(&rectdest, ddswork, &rectsrc, 0, 0);
			ddsback->Blt(&rectdest, ddswork, &rectsrc, 0, 0);
			if (doflip)
				status |= Draw::flippable;
		}
	}
}

// ---------------------------------------------------------------------------
//	表示領域を設定する
//
void WinDrawDDS::Resize(uint w, uint h)
{
	screenheight = h;
}

// ---------------------------------------------------------------------------
//	フリップを行うかどうか設定
//
bool WinDrawDDS::SetFlipMode(bool f)
{
	doflip = f;
	status &= ~Draw::flippable;
	if (doflip && !guimode)
	{
		status |= Draw::flippable;
		SetRect(&rcprev[0], 0, 0, 640, 400); 
	}
	else
	{
		ddraw->FlipToGDISurface();
	}
	return true;
}
