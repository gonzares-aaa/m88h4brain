// ---------------------------------------------------------------------------
//	M88 - PC-8801 Emulator
//	Copyright (C) cisc 1998, 1999.
// ---------------------------------------------------------------------------
//	$Id: WinJoy.h,v 1.3 1999/10/10 01:47:20 cisc Exp $

#if !defined(win32_winjoy_h)
#define win32_winjoy_h

#include "Device.h"

namespace PC8801
{
class Config;

class WinJoyPad : public Device  
{
public:
	enum
	{
		vsync = 0,
		getdir = 0, getbutton = 1,
	};
public:
	WinJoyPad();
	~WinJoyPad();

	bool Init();
	const Descriptor* IFCALL GetDesc() const { return &descriptor; } 
	
	void IOCALL Reset() {}
	uint IOCALL GetDirection(uint port);
	uint IOCALL GetButton(uint port);
	void IOCALL VSync(uint=0, uint=0);
	
	void ApplyConfig(const Config* config);

private:
	void Update();

	bool enabled;
	bool paravalid;
	uint button1, button2;
	uint data[2];

private:
	static const Descriptor descriptor;
	static const InFuncPtr indef[];
	static const OutFuncPtr outdef[];
};

}

#endif // !defined(win32_winjoy_h)
