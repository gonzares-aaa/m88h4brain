// ---------------------------------------------------------------------------
//  M88 - PC-8801 emulator
//	Copyright (C) cisc 1998, 1999.
// ---------------------------------------------------------------------------
//	$Id: wincfg.cpp,v 1.3 1999/12/28 10:34:55 cisc Exp $

#include "headers.h"
#include "resource.h"
#include "wincfg.h"
#include "misc.h"
#include "messages.h"

using namespace PC8801;

// ---------------------------------------------------------------------------
//	構築/消滅
//
WinConfig::WinConfig()
: pplist(0), npages(0),
  ccpu(config, orgconfig),
  cscrn(config, orgconfig),
  csound(config, orgconfig),
  cvol(config, orgconfig),
  cfunc(config, orgconfig),
  cswitch(config, orgconfig),
  cenv(config, orgconfig)
{
	propproc.SetDestination(PropProcGate, this);

	page = 0;
	hwndps = 0;

	Add(&ccpu);
	Add(&cscrn);
	Add(&csound);
	Add(&cvol);
	Add(&cswitch);
	Add(&cfunc);
	Add(&cenv);
}

WinConfig::~WinConfig()
{
	for (PPNode* n = pplist; n;)
	{
		PPNode* next = n->next;
		delete n;
		n = next;
	}
}

// ---------------------------------------------------------------------------
//	設定ダイアログの実行
//
bool WinConfig::Show(HINSTANCE hinstance, HWND hwnd, Config* conf)
{
	if (!hwndps)
	{
		orgconfig = config = *conf;
		hinst = hinstance;
		hwndparent = hwnd;

		PROPSHEETHEADER psh;
		PROPSHEETPAGE* psp = new PROPSHEETPAGE[npages];
		if (!psp)
			return false;

		ccpu.Init(hinst);
		cscrn.Init(hinst);
		csound.Init(hinst);
		cvol.Init(hinst);
		cfunc.Init(hinst);
		cswitch.Init(hinst);
		cenv.Init(hinst);

		int i=0;
		for (PPNode* node = pplist; node && i < MAXPROPPAGES; node = node->next)
		{
			memset(&psp[i], 0, sizeof(PROPSHEETPAGE));
			psp[i].dwSize = sizeof(PROPSHEETPAGE);
			i += node->sheet->Setup(this, &psp[i]) ? 1 : 0;
		}
		if (i)
		{
			memset(&psh, 0, sizeof(psh));
			psh.dwSize = PROPSHEETHEADER_V1_SIZE;
			psh.dwFlags = PSH_PROPSHEETPAGE | PSH_MODELESS; 
			psh.hwndParent = hwndparent;
			psh.hInstance = hinst;
			psh.pszCaption = "設定";
			psh.nPages = i;
			psh.nStartPage = Min(page, i-1);
			psh.ppsp = psp;
			psh.pfnCallback = (PFNPROPSHEETCALLBACK) (void*) propproc;

			hwndps = (HWND) PropertySheet(&psh);
		}
	}
	else
	{
		SetFocus(hwndps);
	}
	return false;
}

void WinConfig::Close()
{
	if (hwndps)
	{
		DestroyWindow(hwndps);
		hwndps = 0;
	}
}

// ---------------------------------------------------------------------------
//	PropSheetProc
//
bool WinConfig::ProcMsg(MSG& msg)
{
	if (hwndps)
	{
		if (PropSheet_IsDialogMessage(hwndps, &msg))
		{
			if (!PropSheet_GetCurrentPageHwnd(hwndps))
				Close();
			return true;
		}
	}
	return false;
}

int WinConfig::PropProc(HWND hwnd, UINT m, LPARAM)
{
	switch (m)
	{
	case PSCB_INITIALIZED:
		hwndps = hwnd;
		break;
	}
	return 0;
}

int CALLBACK WinConfig::PropProcGate
(WinConfig* config, HWND hwnd, UINT m, LPARAM l)
{
	return config->PropProc(hwnd, m, l);
}

// ---------------------------------------------------------------------------

bool IFCALL WinConfig::Add(IConfigPropSheet* sheet)
{
	PPNode** pnode;
	for (pnode = &pplist; *pnode; pnode = &((*pnode)->next))
		;

	PPNode* newnode = new PPNode;
	if (!newnode)
		return false;
	
	*pnode = newnode;
	newnode->sheet = sheet;
	newnode->next = 0;
	npages++;

	return true;
}

bool IFCALL WinConfig::Remove(IConfigPropSheet* sheet)
{
	for (PPNode** pnode = &pplist; *pnode; pnode = &((*pnode)->next))
	{
		if ((*pnode)->sheet == sheet)
		{
			PPNode* d = *pnode;
			*pnode = d->next;
			delete d;
			npages--;
			return true;
		}
	}
	return false;
}

bool IFCALL WinConfig::PageSelected(IConfigPropSheet* sheet)
{
	PPNode* node;
	for (page=0, node = pplist; node; node=node->next)
	{
		if (node->sheet == sheet)
			break;
		page++;
	}
	if (!node)
	{
		page = 0;
		return false;
	}
	return true;
}

bool IFCALL WinConfig::PageChanged(HWND hdlg)
{
	PropSheet_Changed( hwndps, hdlg );
	return true;
}

bool IFCALL WinConfig::Apply()
{
	orgconfig = config;
	PostMessage(hwndparent, WM_M88_APPLYCONFIG, (WPARAM) &config, 0);
	return true;
}

void IFCALL WinConfig::_ChangeVolume(bool current)
{
	PostMessage(hwndparent, 
				WM_M88_CHANGEVOLUME, 
				(WPARAM) (current ? &config : &orgconfig), 
				0);
}

