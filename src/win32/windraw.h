// ---------------------------------------------------------------------------
//  M88 - PC88 emulator
//  Copyright (C) cisc 1998, 1999.
// ---------------------------------------------------------------------------
//	画面描画関係
// ---------------------------------------------------------------------------
//	$Id: windraw.h,v 1.13 2000/01/10 08:25:36 cisc Exp $

#if !defined(win32_windraw_h)
#define win32_windraw_h

#include "types.h"
#include "draw.h"
#include "critsect.h"

// ---------------------------------------------------------------------------

class WinDrawSub  
{
public:
	WinDrawSub() : status(0) {}
	virtual ~WinDrawSub() {}

	virtual bool Init(HWND hwnd) = 0;
	virtual void Resize(uint width, uint height) {}
	virtual bool Cleanup() = 0;
	virtual void SetPalette(PALETTEENTRY* pal) {}
	virtual void QueryNewPalette() {}
	virtual void DrawScreen(int top, int bottom, bool refresh) = 0;

	virtual bool Lock(uint8** pimage, int* pbpl) { return false; }
	virtual bool Unlock() { return true; }

	virtual void SetGUIMode(bool gui) { }
	virtual uint GetStatus() { return status; }
	virtual void Flip() {}
	virtual bool SetFlipMode(bool flip) { return false; }

protected:
	uint status;
};

// ---------------------------------------------------------------------------

class WinDraw : public Draw
{
public:
	enum
	{
		width = 640, height = 400,
	};

public:
	WinDraw();
	~WinDraw();
	bool Init0(HWND hwindow);
	
// - Draw Common Interface
	bool Init(uint w, uint h, uint bpp);
	bool Cleanup();

	bool Lock(uint8** pimage, int* pbpl);
	bool Unlock();
	
	void Resize(uint width, uint height);
	void SetPalette(uint index, uint nents, const Palette* pal);
	void DrawScreen(const Region& region);

	uint GetStatus();
	void Flip();
	bool SetFlipMode(bool f);
	
// - Unique Interface
	int GetDrawCount() { int ret = drawcount; drawcount = 0; return ret; }
	void RequestPaint();
	void QueryNewPalette(bool background);
	void Activate(bool f) { active = f; }
	
	void SetPriorityLow(bool low);
	void SetGUIFlag(bool flag);
	bool ChangeDisplayMode(bool fullscreen, bool force480 = true);
	void Refresh() { refresh = true; }

	int CaptureScreen(uint8* dest);

private:
	enum DisplayType { None, GDI, DDWin, DDFull };
	
	void PaintWindow();
	
	uint ThreadMain();
	static uint __stdcall ThreadEntry(LPVOID arg);
	
	uint idthread;
	HANDLE hthread;
	volatile bool shouldterminate;	// スレッド終了要求

	DisplayType drawtype;

	volatile bool drawing;			// 画面を書き換え中
	bool palchanged;				// パレット変更フラグ
	bool drawall;					// 画面全体を書き換える
	bool refresh;
	bool active;
	bool haspalette;				// パレットを持っている

	RECT drawarea;					// 書き換える領域
	int drawcount;
	int guicount;

	int screenheight;

	HWND hwnd;
	HANDLE hevredraw;
	WinDrawSub* draw;
		
	CriticalSection csdraw;
	bool locked;
	bool flipmode;

	PALETTEENTRY palette[0x100];
};

#endif // !defined(win32_windraw_h)
