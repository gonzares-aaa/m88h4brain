// ---------------------------------------------------------------------------
//  M88 - PC-8801 emulator
//	Copyright (C) cisc 1998, 1999.
// ---------------------------------------------------------------------------
//	Mouse Device
//	based on mouse code written by norimy.
// ---------------------------------------------------------------------------
//	$Id: winmouse.cpp,v 1.7 1999/11/26 10:14:13 cisc Exp $

#include "headers.h"
#include "winmouse.h"
#include "pc88/config.h"
#include "pc88/pc88.h"
#include "ui.h"
#include "status.h"

//#define LOGNAME "mouse"
#include "diag.h"

using namespace PC8801;

// ---------------------------------------------------------------------------
//	構築
//
WinMouse::WinMouse(const ID& id)
: Device(id)
{
	enable = false;
}

WinMouse::~WinMouse()
{
	Enable(false);
}

// ---------------------------------------------------------------------------
//	初期化
//
bool WinMouse::Init(WinUI* _ui, PC88* pc88)
{
	ui = _ui;
	pc = pc88;
	activetime = 0;
	return true;
}

// ---------------------------------------------------------------------------
//	マウスキャプチャリング開始・停止
//
bool WinMouse::Enable(bool en)
{
	if (enable != en)
	{
		enable = en;
		if (enable)
		{
			int mouseparams[3] = { 999, 999, 1 };
			
			if (!SystemParametersInfo(SPI_GETMOUSE, 0, orgmouseparams, 0))
				return false;
			if (!SystemParametersInfo(SPI_SETMOUSE, 0, mouseparams, 0))
				return false;

			ShowCursor(false);
			activetime = GetTickCount() + 500;
			
			RECT rect;
			POINT center;
			GetWindowRect(ui->GetHWnd(), &rect);
			center.x = (rect.left + rect.right) / 2;
			center.y = (rect.top + rect.bottom) / 2;
			rect.left   = center.x - 180;
			rect.right  = center.x + 180;
			rect.top    = center.y - 180;
			rect.bottom = center.y + 180;
					
			LOG4("rect: %d %d %d %d\n", rect.left, rect.top, rect.right, rect.bottom);
			ClipCursor(&rect);
			return true;
		}
		else
		{
			SystemParametersInfo(SPI_SETMOUSE, 0, orgmouseparams, 0);
			ShowCursor(true);
			ClipCursor(0);
			return true;
		}
	}
	return true;
}

// ---------------------------------------------------------------------------
//	入力
//
uint IOCALL WinMouse::GetMove(uint)
{
	LOG1("%c", 'w' + phase);
	if (joymode)
	{
		if (data == -1)
		{
			CaptureMovement();
			data = 0xff;
			if (move.x >=  sensibility) data &= ~4;
			if (move.x <= -sensibility) data &= ~8;
			if (move.y >=  sensibility) data &= ~1;
			if (move.y <= -sensibility) data &= ~2;
		}
		return data;
	}
	else
	{
		switch (phase & 3)
		{
		case 1:	// x
			return (move.x >> 4) | 0xf0;

		case 2:	// y
			return (move.x     ) | 0xf0;

		case 3:	// z
			return (move.y >> 4) | 0xf0;

		case 0:	// u
			return (move.y     ) | 0xf0;
		}
		return 0xff;
	}
}

uint IOCALL WinMouse::GetButton(uint)
{
	return (~ui->GetMouseButton()) | 0xfc;
}

// ---------------------------------------------------------------------------
//	ストローブ信号
//
void IOCALL WinMouse::Strobe(uint, uint data)
{
	data &= 0x40;
	if (port40 ^ data)
	{
		port40 = data;
		
		if (phase <= 0 || int(pc->GetTime() - triggertime) > 18*4)
		{
			if (data)
			{
				triggertime = pc->GetTime();
				CaptureMovement();
				phase = 1;
				LOG3("\nStrobe (%4d, %4d, %d): ", move.x, move.y, triggertime);
			}
			return;
		}
		LOG1("[%d]", pc->GetTime() - triggertime);

		phase = (phase + 1) & 3;
	}
}

// ---------------------------------------------------------------------------
//	マウスの移動状況を取得
//
void WinMouse::CaptureMovement()
{
	move.x = move.y = 0;
	
	if (enable && (!activetime || (int32(GetTickCount()) - activetime > 0)))
	{
		activetime = 0;
		POINT point;
		if (GetCursorPos(&point))
		{
			POINT c = GetWindowCenter();
			move.x = (c.x - point.x) / 2;
			move.y = (c.y - point.y) / 2;
//			statusdisplay.Show(10, 0, "CaptureMove %d, %d", move.x, move.y);
			
			SetCursorPos(c.x, c.y);
			if (Abs(move.x) < 320 && Abs(move.y) < 200) 
			{
				move.x = Limit(move.x, 127, -127);
				move.y = Limit(move.y, 127, -127);
			}
			else 
				move.x = move.y = 0;
		}
	}
//	else
//		statusdisplay.Show(10, 0, "Capture disabled");
}

// ---------------------------------------------------------------------------
//	ウィンドウ位置の取得
//
POINT WinMouse::GetWindowCenter()
{
	RECT rect;
	GetWindowRect(ui->GetHWnd(), &rect);
	
	POINT p;
	p.x = (rect.right + rect.left ) / 2;
	p.y = (rect.bottom + rect.top ) / 2;
	return p;
}

// ---------------------------------------------------------------------------
//
//
void IOCALL WinMouse::VSync(uint, uint)
{
	data = -1;
}

// ---------------------------------------------------------------------------
//	コンフィギュレーション更新
//
void WinMouse::ApplyConfig(const Config* config)
{
	joymode = (config->flags & Config::mousejoymode) != 0;
	sensibility = config->mousesensibility;
}


// ---------------------------------------------------------------------------
//	device description
//
const Device::Descriptor WinMouse::descriptor = { indef, outdef };

const Device::OutFuncPtr WinMouse::outdef[] = 
{
	STATIC_CAST(Device::OutFuncPtr, Strobe),
	STATIC_CAST(Device::OutFuncPtr, VSync),
};

const Device::InFuncPtr WinMouse::indef[] = 
{
	STATIC_CAST(Device::InFuncPtr, GetMove),
	STATIC_CAST(Device::InFuncPtr, GetButton),
};
