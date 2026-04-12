// ---------------------------------------------------------------------------
//  M88 - PC-8801 emulator
//	Copyright (C) cisc 1999.
// ---------------------------------------------------------------------------
//	$Id: memmon.cpp,v 1.8 1999/12/28 10:34:51 cisc Exp $

#include "headers.h"
#include "resource.h"
#include "pc88/pc88.h"
#include "memmon.h"
#include "misc.h"
#include "device_i.h"

using namespace PC8801;

// ---------------------------------------------------------------------------
//	構築/消滅
//
MemoryMonitor::MemoryMonitor()
{
	hwnd = 0;
	addr = 0;
	fontheight = 16;
	height = 16;
	dlgproc.SetDestination(DlgProcGate, this);
	edlgproc.SetDestination(EDlgProcGate, this);
	a0 = MemoryViewer::mainram;
	a6 = MemoryViewer::n88rom;
	af = MemoryViewer::tvram;
}

MemoryMonitor::~MemoryMonitor()
{
}

bool MemoryMonitor::Init(PC88* pc88)
{
	mv.Init(pc88);
	bus = mv.GetBus();
	return true;
}

// ---------------------------------------------------------------------------
//	ダイアログ表示
//
void MemoryMonitor::Show(HINSTANCE hinstance, HWND hwndparent, bool show)
{
	if (show)
	{
		if (!hwnd)
		{
			hinst = hinstance;
			hwnd = CreateDialog(hinst, MAKEINTRESOURCE(IDD_MEMORY), 
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
BOOL MemoryMonitor::DlgProc(HWND hdlg, UINT msg, WPARAM wp, LPARAM lp)
{
	switch (msg)
	{
	case WM_INITDIALOG:
		SetTimer(hdlg, 10, 200, 0);
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
			si.nMax = Max(1, 0x1010-height);
			si.nPage = height;
			SetScrollInfo(hwnd, SB_VERT, &si, TRUE);
		}
		return true;
	
	case WM_LBUTTONDBLCLK:
		if (fontwidth > 0)
		{
			int x = LOWORD(lp) / fontwidth;
			int y = HIWORD(lp) / fontheight;
			if (6 <= x && x <= 5+48)
			{
				editaddr = addr + y * 16 + (x - 6) / 3;
				if (editaddr < 0x10000)
				{
					DialogBox(hinst, MAKEINTRESOURCE(IDD_MEMORY_EDIT), 
								hdlg, DLGPROC((void*) edlgproc));
				}
			}
		}
		return 0;

	case WM_VSCROLL:
		switch (LOWORD(wp))
		{
		case SB_LINEUP:
			addr = Max(addr - 0x10, 0);
			break;
		case SB_LINEDOWN:
			addr = Min(addr + 0x10, 0xfff0);
			break;
		case SB_PAGEUP:
			addr = Max(addr - height * 0x10, 0);
			break;
		case SB_PAGEDOWN:
			addr = Min(addr + height * 0x10, 0xfff0);
			break;
		case SB_TOP:
			addr = 0;
			break;
		case SB_BOTTOM:
			addr = 0x10000;
			break;
		case SB_THUMBTRACK:
		case SB_THUMBPOSITION:
			addr = Limit(int16(HIWORD(wp)), 0xfff, 0) * 16;
			break;
		}
		SetScrollPos(hdlg, SB_VERT, (addr / 0x10), TRUE);
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

BOOL CALLBACK MemoryMonitor::DlgProcGate
(MemoryMonitor* about, HWND hwnd, UINT m, WPARAM w, LPARAM l)
{
	return about->DlgProc(hwnd, m, w, l);
}

void MemoryMonitor::SetBank()
{
	MemoryViewer::Type t6;
	if (a0 == MemoryViewer::n88rom)
		t6 = a6;
	else
		t6 = a0;
	mv.SelectBank(a0, t6, MemoryViewer::mainram, MemoryViewer::mainram, af);
	Update();
}


void MemoryMonitor::PutLine(const char* msg, ...)
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

void MemoryMonitor::DrawMain(HDC _hdc)
{
	hdc = _hdc;
	y = 0;
	char buf[128];
	addr = Limit(addr, 0x10010 - height * 0x10, 0);
	int a = addr;
	GetCharWidth32(hdc, '0', '0', &fontwidth); 

	for (int y=0; y<height && a < 0x10000; y++)
	{
		char* ptrt = buf;
		char* ptrc = buf+48;
		for (int x=0; x<16; x++)
		{
			int d = bus->Read8(a++ & 0xffff);
			ToHex(&ptrt, d & 255);
			*ptrt++ = ' ';
			*ptrc++ = d >= 0x20 && d < 0xfd && d != 0x7f ? d : '.'; 
		}
		PutLine("%.4x: %.48s %.16s ", (a - 0x10) & 0xffff, buf, buf+48);
	}
}

void MemoryMonitor::Draw(HWND hwnd, HDC hdc)
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
					SHIFTJIS_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
					DEFAULT_QUALITY, FIXED_PITCH, "ＭＳゴシック");
	
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

void MemoryMonitor::Paint(HWND hwnd)
{
	PAINTSTRUCT ps;
	HDC hdc = BeginPaint(hwnd, &ps);
	Draw(hwnd, hdc);
	EndPaint(hwnd, &ps);
}

void MemoryMonitor::Update()
{
	if (hwnd)
	{
		InvalidateRect(hwnd, 0, false);
		RedrawWindow(hwnd, 0, 0, RDW_UPDATENOW);
	}
}

BOOL MemoryMonitor::EDlgProc(HWND hdlg, UINT msg, WPARAM wp, LPARAM lp)
{
	char buf[16];
	switch (msg)
	{
	case WM_INITDIALOG:
		wsprintf(buf, "%.4x:", editaddr);
		SetDlgItemText(hdlg, IDC_MEMORY_EDIT_TEXT, buf);
		wsprintf(buf, "%.2x", bus->Read8(editaddr));
		SetDlgItemText(hdlg, IDC_MEMORY_EDITBOX, buf);
		SetFocus(GetDlgItem(hdlg, IDC_MEMORY_EDITBOX));
		SendDlgItemMessage(hdlg, IDC_MEMORY_EDITBOX, EM_SETSEL, 0, -1);
		return 0;

	case WM_COMMAND:
		if (HIWORD(wp) == BN_CLICKED)
		{
			switch (LOWORD(wp))
			{
				char* dum;
			case IDOK:
				GetDlgItemText(hdlg, IDC_MEMORY_EDITBOX, buf, 5);
				if (isxdigit(buf[0]))
					bus->Write8(editaddr, strtoul(buf, &dum, 16));
			case IDCANCEL:
				EndDialog(hdlg, false);
				break;
			}
		}
		return 0;		

	case WM_CLOSE:
		EndDialog(hdlg, false);
		return true;

	default:
		return false;
	}
}

BOOL CALLBACK MemoryMonitor::EDlgProcGate
(MemoryMonitor* about, HWND hwnd, UINT m, WPARAM w, LPARAM l)
{
	return about->EDlgProc(hwnd, m, w, l);
}
