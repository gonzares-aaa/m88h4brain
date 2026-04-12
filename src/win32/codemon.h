// ---------------------------------------------------------------------------
//  M88 - PC-8801 emulator
//	Copyright (C) cisc 1998, 1999.
// ---------------------------------------------------------------------------
//	$Id: codemon.h,v 1.4 1999/12/28 10:34:51 cisc Exp $

#if !defined(win32_codemon_h)
#define win32_codemon_h

#include "instthnk.h"
#include "device.h"
#include "pc88/memview.h"
#include "Z80diag.h"

// ---------------------------------------------------------------------------

class PC88;

namespace PC8801
{

class CodeMonitor
{
public:
	CodeMonitor();
	~CodeMonitor();

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
	static BOOL CALLBACK DlgProcGate(CodeMonitor*, HWND, UINT, WPARAM, LPARAM);
	void Paint(HWND);
	void Draw(HWND, HDC);
	void DrawMain(HDC);
	void PutLine(const char*, ...);
	void SetBank();
	
	InstanceThunk dlgproc;
	HWND hwnd;
	const uint8* regs;
	
	MemoryViewer mv;
	MemoryBus* bus;
	HDC hdc;
	int y;
	int fontheight;
	int addr;
	int height;
	Z80Diag diag;
	
	MemoryViewer::Type a0;
	MemoryViewer::Type a6;
	MemoryViewer::Type af;
};

}

#endif // !defined(win32_codemon_h)
