// ---------------------------------------------------------------------------
//	M88 - PC-8801 Emulator
//	Copyright (C) cisc 1998, 1999.
// ---------------------------------------------------------------------------
//	$Id: WinJoy.cpp,v 1.4 1999/10/10 01:47:19 cisc Exp $

// #define	DIRECTINPUT_VERSION		0x0300

#include "headers.h"
#include "WinJoy.h"
#include "pc88/config.h"
#include "status.h"

using namespace PC8801;

// ---------------------------------------------------------------------------
//	構築/消滅
//
WinJoyPad::WinJoyPad()
: Device(0)
{
	enabled = false;
	button1 = JOY_BUTTON1 | JOY_BUTTON3;
	button2 = JOY_BUTTON2 | JOY_BUTTON4;
}

WinJoyPad::~WinJoyPad()
{

}

// ---------------------------------------------------------------------------
//	初期化
//
bool WinJoyPad::Init()
{
	paravalid = false;
	enabled = false;
	if (!joyGetNumDevs())
	{
		statusdisplay.Show(70, 3000, "ジョイスティック API を使用できません");
		return false;
	}

	JOYINFO joyinfo;
	if (joyGetPos(JOYSTICKID1, &joyinfo) == JOYERR_UNPLUGGED)
	{
		statusdisplay.Show(70, 3000, "ジョイスティックが接続されていません");
		return false;
	}
	enabled = true;
	return true;
}

// ---------------------------------------------------------------------------
//	入力
//
uint IOCALL WinJoyPad::GetDirection(uint)
{
	if (!paravalid)
		Update();
	return data[0];
}

uint IOCALL WinJoyPad::GetButton(uint)
{
	if (!paravalid)
		Update();
	return data[1];
}

// ---------------------------------------------------------------------------
//	ジョイスティックの状態を更新
//
void WinJoyPad::Update()
{
	const int threshold = 16384;
	JOYINFO joyinfo;
	if (enabled && joyGetPos(JOYSTICKID1, &joyinfo) == JOYERR_NOERROR)
	{
		data[0] =  (joyinfo.wYpos < (32768-threshold) ? 0 : 1)	// U
				 | (joyinfo.wYpos > (32768+threshold) ? 0 : 2)	// D
				 | (joyinfo.wXpos < (32768-threshold) ? 0 : 4)	// L
				 | (joyinfo.wXpos > (32768+threshold) ? 0 : 8)	// R
				 | 0xf0;
		data[1] =  (joyinfo.wButtons & button1 ? 0 : 1)
				 | (joyinfo.wButtons & button2 ? 0 : 2)
				 | 0xfc;
	}
	else
		data[0] = data[1] = 0xff;
	paravalid = true;
}

// ---------------------------------------------------------------------------
//	VSync たいみんぐ
//
void IOCALL WinJoyPad::VSync(uint, uint d)
{
	if (d)
		paravalid = false;
}
	
// ---------------------------------------------------------------------------
//	コンフィギュレーション更新
//
void WinJoyPad::ApplyConfig(const Config* config)
{
	if (config->flags & Config::swappadbuttons)
	{
		button1 = JOY_BUTTON2 | JOY_BUTTON4;
		button2 = JOY_BUTTON1 | JOY_BUTTON3;
	}
	else
	{
		button1 = JOY_BUTTON1 | JOY_BUTTON3;
		button2 = JOY_BUTTON2 | JOY_BUTTON4;
	}
}

// ---------------------------------------------------------------------------
//	device description
//
const Device::Descriptor WinJoyPad::descriptor = { indef, outdef };

const Device::OutFuncPtr WinJoyPad::outdef[] = 
{
	STATIC_CAST(Device::OutFuncPtr, VSync),
};

const Device::InFuncPtr WinJoyPad::indef[] = 
{
	STATIC_CAST(Device::InFuncPtr, GetDirection),
	STATIC_CAST(Device::InFuncPtr, GetButton),
};

