// ---------------------------------------------------------------------------
//  M88 - PC-8801 emulator
//	Copyright (C) cisc 1998, 1999.
// ---------------------------------------------------------------------------
//	$Id: memmon.h,v 1.5 1999/12/28 10:34:52 cisc Exp $

#if !defined(win32_memmon_h)
#define win32_memmon_h

#include "instthnk.h"
#include "device.h"
#include "pc88/memview.h"

// ---------------------------------------------------------------------------

class PC88;

namespace PC8801
{

class MemoryMonitor
{
public:
	MemoryMonitor();
	~MemoryMonitor();

	bool Init(PC88*); 
	void Show(HINSTANCE, HWND, bool show);
	void Update();
	bool IsOpen() { return !!hwnd; }

private:
	enum
	{
		bkcol = 0x400000,
	};

	BOOL DlgProc(HWND, UINT, WPARAM, LPARAM);
	static BOOL CALLBACK DlgProcGate(MemoryMonitor*, HWND, UINT, WPARAM, LPARAM);
	BOOL EDlgProc(HWND, UINT, WPARAM, LPARAM);
	static BOOL CALLBACK EDlgProcGate(MemoryMonitor*, HWND, UINT, WPARAM, LPARAM);
	void Paint(HWND);
	void Draw(HWND, HDC);
	void DrawMain(HDC);
	void PutLine(const char*, ...);
	void SetBank();
	
	InstanceThunk dlgproc;
	InstanceThunk edlgproc;
	HWND hwnd;
	HINSTANCE hinst;
	const uint8* regs;
	
	MemoryViewer mv;
	MemoryBus* bus;
	HDC hdc;
	int y;
	int fontheight;
	int fontwidth;
	int addr;
	int height;
	int editaddr;
	MemoryViewer::Type a0;
	MemoryViewer::Type a6;
	MemoryViewer::Type af;
};

}

#endif // !defined(win32_memmon_h)
