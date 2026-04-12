// ---------------------------------------------------------------------------
//  M88 - PC-8801 emulator
//	Copyright (C) cisc 1998, 1999.
// ---------------------------------------------------------------------------
//	$Id: soundmon.h,v 1.8 1999/12/28 10:34:53 cisc Exp $

#if !defined(win32_soundmon_h)
#define win32_soundmon_h

#include "device.h"
#include "instthnk.h"

// ---------------------------------------------------------------------------

namespace PC8801
{
	class OPNIF;
};


class OPNMonitor
{
public:
	OPNMonitor();
	~OPNMonitor();

	bool Init(PC8801::OPNIF* opn); 
	void Show(HINSTANCE, HWND, bool show);
	bool IsOpen() { return !!hwnd; }

private:
	enum
	{
		bkcol = 0x400000,
	};

	BOOL DlgProc(HWND, UINT, WPARAM, LPARAM);
	static BOOL CALLBACK DlgProcGate(OPNMonitor*, HWND, UINT, WPARAM, LPARAM);
	void Paint(HWND);
	void Draw(HWND, HDC);
	void DrawMain(HDC);
	void PutLine(const char*, ...);
	void Update();
	
	InstanceThunk dlgproc;
	HWND hwnd;
	PC8801::OPNIF* opn;
	const uint8* regs;
	
	HDC hdc;
	int y;
	int fontheight;
	int w, h;
	uint mask;
};

#endif // !defined(win32_soundmon_h)
