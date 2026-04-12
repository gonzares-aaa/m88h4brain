// ---------------------------------------------------------------------------
//	M88 - PC-8801 Emulator.
//	Copyright (C) cisc 1997, 1999.
// ---------------------------------------------------------------------------
//	$Id: wincore.h,v 1.28 2000/01/10 08:25:35 cisc Exp $

#if !defined(win32_wincore_h)
#define win32_wincore_h

// ---------------------------------------------------------------------------

#include "types.h"
#include "CritSect.h"
#include "pc88/pc88.h"
#include "pc88/config.h"
#include "winsound.h"
#include "winjoy.h"
#include "winmouse.h"

namespace PC8801
{
	class WinKeyIF;
	class ExternalDevice;
	class ExtendModule;
}

// ---------------------------------------------------------------------------

class WinCore : public PC88, public ISystem, public ILockCore
{
private:
	enum
	{
		num_externals = 4,
	};

public:
	WinCore();
	~WinCore();
	bool Init(WinUI* ui, HWND hwnd, Draw* draw, DiskManager* diskmgr, 
			  PC8801::WinKeyIF* keyb, IConfigPropBase* cpb); 
	bool Cleanup();

	void Reset();
	void ApplyConfig(PC8801::Config* config);
	
	long GetExecCount();
	void Wait(bool dowait);
	void SetGUIFlag(bool flag);
	void ActivateMouse(bool flag);
	bool EnableCPUDump(uint cpu, bool flag);
	
	bool SaveShapshot(const char* filename);
	bool LoadShapshot(const char* filename, const char* diskname = 0);

	PC8801::WinSound* GetSound() { return &sound; }

	void* IFCALL QueryIF(REFIID iid);
	void IFCALL Lock() { cs_cpu.lock(); }
	void IFCALL Unlock() { cs_cpu.unlock(); }

private:
//	Snapshot ヘッダー
	enum
	{
		ssmajor = 1, ssminor = 1,
	};

	struct SnapshotHeader
	{
		char id[16];
		uint8 major, minor;

		int8 disk[2];
		int datasize;
		PC8801::Config::BASICMode basicmode;
		int16 clock;
		uint16 erambanks;
		uint16 cpumode;
		uint16 mainsubratio;
		uint flags;
		uint flag2;
	};

	struct MLNode
	{
		PC8801::ExtendModule* mod;
		MLNode* next;
	};

private:
	bool ConnectDevices(PC8801::WinKeyIF* keyb);
	bool ConnectExternalDevices();

	void ExecuteAsynchronus();
	void ExecuteSynchronus();
	void Execute(long clock, long length, long ec);
	
	bool EnablePad(bool enable);
	bool EnableMouse(bool enable);

	uint32 GetMachineTime();
	
	uint ThreadMain();
	static uint __stdcall ThreadEntry(LPVOID arg);
	
	CriticalSection cs_cpu;
	WinUI* ui;

	HANDLE hthread;
	uint idthread;

	int clock;
	int execcount;
	int speed;
	int effclock;
	int time;
	
	volatile bool shouldterminate;
	volatile bool execute;
	bool padenable;
	bool mouseenable;
	int guicount;

	IConfigPropBase* cfgprop;

	MLNode* modlist;
	PC8801::ExternalDevice* extdev;
	PC8801::WinSound sound;
	PC8801::WinJoyPad pad;
	PC8801::WinMouse mouse;
	PC8801::Config config;
};

// ---------------------------------------------------------------------------
//	マウス使用許可
//
inline void WinCore::ActivateMouse(bool flag)
{
	mouse.Enable(flag && mouseenable && !guicount);
}

#endif // !defined(win32_wincore_h)
