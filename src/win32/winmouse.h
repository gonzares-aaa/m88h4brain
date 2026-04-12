// ---------------------------------------------------------------------------
//	M88 - PC-8801 Emulator
//	Copyright (C) cisc 1998, 1999.
// ---------------------------------------------------------------------------
//	$Id: winmouse.h,v 1.4 1999/10/10 01:47:22 cisc Exp $

#if !defined(win32_winmouse_h)
#define win32_winmouse_h

#include "Device.h"

class WinUI;
class PC88;

namespace PC8801
{

class Config;

class WinMouse : public Device  
{
public:
	enum
	{
		strobe=0, vsync,
		getmove=0, getbutton,
	};
public:
	WinMouse(const ID& id);
	~WinMouse();

	bool Init(WinUI* ui, PC88* pc);
	const Descriptor* IFCALL GetDesc() const { return &descriptor; } 
	
	bool WinMouse::Enable(bool en);

	uint IOCALL GetMove(uint);
	uint IOCALL GetButton(uint);
	void IOCALL Strobe(uint, uint data);
	void IOCALL VSync(uint, uint);
	
	void ApplyConfig(const Config* config);

private:
	void CaptureMovement();
	POINT GetWindowCenter();
	
	WinUI* ui;
	PC88* pc;
	POINT move;
	bool enable;
	uint8 port40;
	bool joymode;
	int phase;
	int32 triggertime;
	int32 activetime;
	int sensibility;
	
	int data;
	int orgmouseparams[3];

private:
	static const Descriptor descriptor;
	static const InFuncPtr indef[];
	static const OutFuncPtr outdef[];
};

}

#endif // !defined(win32_winmouse_h)
