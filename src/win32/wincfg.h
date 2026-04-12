// ---------------------------------------------------------------------------
//  M88 - PC-8801 emulator
//	Copyright (C) cisc 1998, 1999.
// ---------------------------------------------------------------------------
//	$Id: wincfg.h,v 1.2 1999/12/28 10:34:55 cisc Exp $

#if !defined(win32_wincfg_h)
#define win32_wincfg_h

#include "if/ifcommon.h"
#include "pc88/config.h"
#include "instthnk.h"
#include "cfgpage.h"

namespace PC8801
{

class WinConfig : public IConfigPropBase
{
public:
	WinConfig();
	~WinConfig();
	
	bool Show(HINSTANCE, HWND, Config* config);
	bool ProcMsg(MSG& msg);
	bool IsOpen() { return !!hwndps; }
	void Close();

private:
	struct PPNode
	{
		PPNode* next;
		IConfigPropSheet* sheet;
	};

	bool IFCALL Add(IConfigPropSheet* sheet);
	bool IFCALL Remove(IConfigPropSheet* sheet);

	bool IFCALL Apply();
	bool IFCALL PageSelected(IConfigPropSheet*);
	bool IFCALL PageChanged(HWND);
	void IFCALL _ChangeVolume(bool);

	int PropProc(HWND, UINT, LPARAM);
	static int CALLBACK PropProcGate(WinConfig*, HWND, UINT, LPARAM);

	// thunks
	InstanceThunk propproc;
	PPNode* pplist;
	
	HWND hwndparent;
	HWND hwndps;
	HINSTANCE hinst;
	Config config;
	Config orgconfig;
	int page;
	int npages;

	ConfigCPU ccpu;
	ConfigScreen cscrn;
	ConfigSound csound;
	ConfigVolume cvol;
	ConfigFunction cfunc;
	ConfigSwitch cswitch;
	ConfigEnv cenv;
};

}

#endif // !defined(win32_wincfg_h)
