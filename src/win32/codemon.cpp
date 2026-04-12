// ---------------------------------------------------------------------------
//  M88 - PC-8801 emulator
//	Copyright (C) cisc 1999.
// ---------------------------------------------------------------------------
//	$Id: codemon.cpp,v 1.5 1999/12/28 10:34:51 cisc Exp $

#include "headers.h"
#include "resource.h"
#include "pc88/pc88.h"
#include "codemon.h"
#include "misc.h"
#include "device_i.h"

using namespace PC8801;

// ---------------------------------------------------------------------------
//	構築/消滅
//
CodeMonitor::CodeMonitor()
{
	hwnd = 0;
	addr = 0;
	fontheight = 16;
	height = 16;
	dlgproc.SetDestination(DlgProcGate, this);
	af = MemoryViewer::tvram;
	a0 = a6 = MemoryViewer::n88rom;
}

CodeMonitor::~CodeMonitor()
{
}

bool CodeMonitor::Init(PC88* pc88)
{
	if (!mv.Init(pc88))
		return false;
	bus = mv.GetBus();
	diag.Init(bus);
	return true;
}

// ---------------------------------------------------------------------------
//	ダイアログ表示
//
void CodeMonitor::Show(HINSTANCE hinst, HWND hwndparent, bool show)
{
	if (show)
	{
		if (!hwnd)
		{
			hwnd = CreateDialog(hinst, MAKEINTRESOURCE(IDD_CODEMON), 
								hwndparent, DLGPROC((void*)dlgproc));
			SCROLLINFO si;
			si.cbSize = sizeof(SCROLLINFO);
			si.fMask = SIF_ALL;
			si.nMin = 0;
			si.nMax = 0xfff;
			si.nPage = 16;
			si.nPos = 0;
			SetScrollInfo(hwnd, SB_VERT, &si, TRUE);
		}
	}
	else
	{
		if (hwnd)
			SendMessage(hwnd, WM_CLOSE, 0, 0);
	}
}

// ---------------------------------------------------------------------------
//	ダイアログ処理
//
BOOL CodeMonitor::DlgProc(HWND hdlg, UINT msg, WPARAM wp, LPARAM lp)
{
	switch (msg)
	{
	case WM_INITDIALOG:
		SetTimer(hdlg, 10, 1000, 0);
		SetBank();
		return 0;

	case WM_CLOSE:
		KillTimer(hdlg, 1);
		DestroyWindow(hdlg);
		hwnd = 0;
		return true;

	case WM_PAINT:
		Paint(hdlg);
		return true;

	case WM_TIMER:
		Update();
		return true;
	
	case WM_SIZE:
		{
			height = (HIWORD(lp)+fontheight-1) / fontheight;
			SCROLLINFO si;
			si.cbSize = sizeof(SCROLLINFO);
			si.fMask = SIF_PAGE | SIF_RANGE;
			si.nMin = 0;
			si.nMax = 0xffff;
			si.nPage = height;
			SetScrollInfo(hwnd, SB_VERT, &si, TRUE);
		}
		return true;
	
	case WM_VSCROLL:
		switch (LOWORD(wp))
		{
			int i;
		case SB_LINEUP:
			addr = diag.InstDec(addr);
			break;
		case SB_LINEDOWN:
			addr = diag.InstInc(addr);
			break;
		case SB_PAGEUP:
			for (i=0; i<height && addr > 0; i++)
				addr = diag.InstDec(addr);
			addr = Max(0, addr);
			break;
		case SB_PAGEDOWN:
			for (i=0; i<height && addr < 0x10000; i++)
				addr = diag.InstInc(addr);
			addr = Min(0xffff, addr);
			break;
		case SB_TOP:
			addr = 0;
			break;
		case SB_BOTTOM:
			addr = 0xffff;
			break;
		case SB_THUMBTRACK:
		case SB_THUMBPOSITION:
			addr = Limit(HIWORD(wp), 0xffff, 0);
			break;
		}
		SetScrollPos(hdlg, SB_VERT, addr, TRUE);
		Update();
		return true;

	case WM_KEYDOWN:
		{
			int sn = -1;
			switch (wp)
			{
			case VK_UP:
				sn = SB_LINEUP;
				break;
			case VK_PRIOR:
				sn = SB_PAGEUP;
				break;
			case VK_NEXT:
				sn = SB_PAGEDOWN;
				break;
			case VK_DOWN:
				sn = SB_LINEDOWN;
				break;
			case VK_HOME:
				sn = SB_TOP;
				break;
			case VK_END:
				sn = SB_BOTTOM;
				break;
			}
			if (sn != -1)
				SendMessage(hwnd, WM_VSCROLL, MAKELONG(sn, 0), 0L);  
			break;
		}
	case WM_COMMAND:
		switch (LOWORD(wp))
		{
		case IDM_MEM_0_RAM:	a0 = MemoryViewer::mainram;	SetBank(); break;
		case IDM_MEM_0_N88:	a0 = MemoryViewer::n88rom;	SetBank(); break;
		case IDM_MEM_0_N:	a0 = MemoryViewer::nrom;	SetBank(); break;
		case IDM_MEM_6_N88:	a6 = MemoryViewer::n88rom;	SetBank(); break;
		case IDM_MEM_6_E0:	a6 = MemoryViewer::n88e0;	SetBank(); break;
		case IDM_MEM_6_E1:	a6 = MemoryViewer::n88e1;	SetBank(); break;
		case IDM_MEM_6_E2:	a6 = MemoryViewer::n88e2;	SetBank(); break;
		case IDM_MEM_6_E3:	a6 = MemoryViewer::n88e3;	SetBank(); break;
		case IDM_MEM_F_RAM:	af = MemoryViewer::mainram;	SetBank(); break;
		case IDM_MEM_F_TVRAM:af= MemoryViewer::tvram;	SetBank(); break;
		}
		return false;
			
	case WM_INITMENU:
		{
			HMENU hmenu = (HMENU) wp;
			CheckMenuItem(hmenu, IDM_MEM_0_RAM, (a0 == MemoryViewer::mainram) ? MF_CHECKED : MF_UNCHECKED);
			CheckMenuItem(hmenu, IDM_MEM_0_N88, (a0 == MemoryViewer::n88rom ) ? MF_CHECKED : MF_UNCHECKED);
			CheckMenuItem(hmenu, IDM_MEM_0_N,   (a0 == MemoryViewer::nrom   ) ? MF_CHECKED : MF_UNCHECKED);
			CheckMenuItem(hmenu, IDM_MEM_6_N88, (a6 == MemoryViewer::n88rom ) ? MF_CHECKED : MF_UNCHECKED);
			CheckMenuItem(hmenu, IDM_MEM_6_E0,  (a6 == MemoryViewer::n88e0  ) ? MF_CHECKED : MF_UNCHECKED);
			CheckMenuItem(hmenu, IDM_MEM_6_E1,  (a6 == MemoryViewer::n88e1  ) ? MF_CHECKED : MF_UNCHECKED);
			CheckMenuItem(hmenu, IDM_MEM_6_E2,  (a6 == MemoryViewer::n88e2  ) ? MF_CHECKED : MF_UNCHECKED);
			CheckMenuItem(hmenu, IDM_MEM_6_E3,  (a6 == MemoryViewer::n88e3  ) ? MF_CHECKED : MF_UNCHECKED);
			CheckMenuItem(hmenu, IDM_MEM_F_RAM, (af == MemoryViewer::mainram) ? MF_CHECKED : MF_UNCHECKED);
			CheckMenuItem(hmenu, IDM_MEM_F_TVRAM,(af== MemoryViewer::tvram  ) ? MF_CHECKED : MF_UNCHECKED);
			int x = a0 != MemoryViewer::n88rom ? MF_GRAYED : MF_ENABLED;
			EnableMenuItem(hmenu, IDM_MEM_6_N88, x);
			EnableMenuItem(hmenu, IDM_MEM_6_E0,  x);
			EnableMenuItem(hmenu, IDM_MEM_6_E1,  x);
			EnableMenuItem(hmenu, IDM_MEM_6_E2,  x);
			EnableMenuItem(hmenu, IDM_MEM_6_E3,  x);
			return false;
		}
	default:
		return false;
	}
	return false;
}

BOOL CALLBACK CodeMonitor::DlgProcGate
(CodeMonitor* about, HWND hwnd, UINT m, WPARAM w, LPARAM l)
{
	return about->DlgProc(hwnd, m, w, l);
}

void CodeMonitor::SetBank()
{
	MemoryViewer::Type t6;
	if (a0 == MemoryViewer::n88rom)
		t6 = a6;
	else
		t6 = a0;
	mv.SelectBank(a0, t6, MemoryViewer::mainram, MemoryViewer::mainram, af);
	Update();
}

void CodeMonitor::PutLine(const char* msg, ...)
{
	char buf[256];
	va_list marker;
	va_start(marker, msg);
	int len = wvsprintf(buf, msg, marker);
	va_end(marker);
	TextOut(hdc, 0, y, buf, len);
	y += fontheight;
}

static void ToHex(char** p, uint d)
{
	static const char hex[] = "0123456789abcdef";

	*(*p)++ = hex[d >> 4];
	*(*p)++ = hex[d & 15];
}

void CodeMonitor::DrawMain(HDC _hdc)
{
	hdc = _hdc;
	y = 0;
	char buf[128];
	addr = Limit(addr, 0xffff, 0);
	int a = addr;

	for (int y=0; y<height && a < 0x10000; y++)
	{
		int next = diag.Disassemble(a, buf+8);
		char* ptr = buf;
		int c, d = next-a;
		for (c=0; c<d; c++)
			ToHex(&ptr, bus->Read8((a+c) & 0xffff));
		for (; c<4; c++)
			*ptr++ = ' ', *ptr++ = ' ';
		PutLine("%.4x: %.8s   %s", a, buf, buf+8);
		a = next;
	}
}

void CodeMonitor::Draw(HWND hwnd, HDC hdc)
{
	RECT rect;
	GetClientRect(hwnd, &rect);

	HDC hmemdc = CreateCompatibleDC(hdc);
	HBITMAP hbitmap = CreateCompatibleBitmap(hdc, rect.right, rect.bottom);
	
	hbitmap = (HBITMAP) SelectObject(hmemdc, hbitmap);
	HBRUSH hbrush = (HBRUSH) SelectObject(hmemdc, CreateSolidBrush(bkcol));
	PatBlt(hmemdc, 0, 0, rect.right, rect.bottom, PATCOPY);
	DeleteObject(SelectObject(hmemdc, hbrush));
	
	HFONT hfont = CreateFont(fontheight, 0, 0, 0, 0, 0, 0, 0, 
					ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
					DEFAULT_QUALITY, FIXED_PITCH, "Terminal");
	
	SetTextColor(hmemdc, 0xffffff);
	SetBkColor(hmemdc, bkcol);
//	SetBkMode(hmemdc, TRANSPARENT);
	hfont = (HFONT) SelectObject(hmemdc, hfont);
	DrawMain(hmemdc);
	DeleteObject(SelectObject(hmemdc, hfont));
	
	BitBlt(hdc, 0, 0, rect.right, rect.bottom, hmemdc, 0, 0, SRCCOPY);

	DeleteObject(SelectObject(hmemdc, hbitmap));
	DeleteDC(hmemdc);
}

void CodeMonitor::Paint(HWND hwnd)
{
	PAINTSTRUCT ps;
	HDC hdc = BeginPaint(hwnd, &ps);
	Draw(hwnd, hdc);
	EndPaint(hwnd, &ps);
}

void CodeMonitor::Update()
{
	if (hwnd)
	{
		HDC hdc = GetDC(hwnd);
		Draw(hwnd, hdc);
		ReleaseDC(hwnd, hdc);
	}
}
