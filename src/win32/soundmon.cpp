// ---------------------------------------------------------------------------
//  M88 - PC-8801 emulator
//	Copyright (C) cisc 1999.
// ---------------------------------------------------------------------------
//	$Id: soundmon.cpp,v 1.7 1999/12/28 10:34:52 cisc Exp $

#include "headers.h"
#include "resource.h"
#include "soundmon.h"
#include "misc.h"
#include "pc88/opnif.h"

using namespace PC8801;

// ---------------------------------------------------------------------------
//	構築/消滅
//
OPNMonitor::OPNMonitor()
{
	hwnd = 0;
	fontheight = 12;
	dlgproc.SetDestination(DlgProcGate, this);
	w = 600;
	h = 200;
}

OPNMonitor::~OPNMonitor()
{
}

bool OPNMonitor::Init(OPNIF* o)
{
	opn = o;
	regs = o->GetRegs();
	mask = 0;
	return true;
}

// ---------------------------------------------------------------------------
//	ダイアログ表示
//
void OPNMonitor::Show(HINSTANCE hinst, HWND hwndparent, bool show)
{
	if (show)
	{
		if (!hwnd)
		{
			hwnd = CreateDialog(hinst, MAKEINTRESOURCE(IDD_SOUNDMON), 
								hwndparent, DLGPROC((void*)dlgproc));
			RECT rect;
			rect.left = 0;	rect.right =  w;
			rect.top  = 0;  rect.bottom = h;
			
			AdjustWindowRectEx(&rect, WS_POPUP | WS_CAPTION | WS_SYSMENU | WS_THICKFRAME, false, 0);
			
			SetWindowPos(hwnd, 0, 0, 0, 
					rect.right-rect.left, rect.bottom - rect.top, 
					SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
			SetFocus(hwndparent);
		}
		else
		{
			SetFocus(hwnd);
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
BOOL OPNMonitor::DlgProc(HWND hdlg, UINT msg, WPARAM wp, LPARAM lp)
{
	switch (msg)
	{
	case WM_INITDIALOG:
		SetTimer(hdlg, 1, 50, 0);
		return 0;

	case WM_CLOSE:
		KillTimer(hdlg, 1);
		opn->SetChannelMask(0);
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
		w = LOWORD(lp); h = HIWORD(lp);
		fontheight = Limit(h / 11, 24, 8);
		InvalidateRect(hwnd, 0, true);
		return true;

	case WM_KEYDOWN:
		{
			if (wp == VK_RETURN)
				mask = 0;
			else
			{
				if (VK_NUMPAD0 <= wp && wp <= VK_NUMPAD9)
					wp -= VK_NUMPAD0;
				else if ('0' <= wp && wp <= '9')
					wp -= '0';
				else
					return true;

				if (wp)
					mask ^= 1 << (wp - 1);
				else
					mask = ~mask;
			}
			char text[] = "YM2608 [123456789]";
			for (int i=0; i<9; i++)
				if (mask & (1 << i))
					text[8+i] = '-';
			SetWindowText(hwnd, text);
			opn->SetChannelMask(mask);
		}
		return true;

	default:
		return false;
	}
}

BOOL CALLBACK OPNMonitor::DlgProcGate
(OPNMonitor* about, HWND hwnd, UINT m, WPARAM w, LPARAM l)
{
	return about->DlgProc(hwnd, m, w, l);
}

// ---------------------------------------------------------------------------
//	１行表示（書式付）
//
void OPNMonitor::PutLine(const char* msg, ...)
{
	char buf[256];
	va_list marker;
	va_start(marker, msg);
	int len = wvsprintf(buf, msg, marker);
	va_end(marker);
	TextOut(hdc, 0, y, buf, len);
	y += fontheight;
}

// ---------------------------------------------------------------------------
//	１バイトの数値を16進記述に変換
//
static void ToHex(char** p, uint d)
{
	static const char hex[] = "0123456789abcdef";

	*(*p)++ = hex[d >> 4];
	*(*p)++ = hex[d & 15];
}

// ---------------------------------------------------------------------------
//	FN/BLK を F-number に変換
//
static inline uint ToFnum(uint f)
{
	return (f & 2047) << ((f >> 11) & 7);
}

// ---------------------------------------------------------------------------
//	$bx 系の値を変換
//
static inline uint ToFB(uint f)
{
	return (f & 0xff00) | ((f & 0x38) << 1) | (f & 7);
}

// ---------------------------------------------------------------------------
//	状態を表示
//
void OPNMonitor::DrawMain(HDC _hdc)
{
	hdc = _hdc;
	y = 0;
	char buf[128];
	int y;

	//	0123456789abcdef0123456789abcdef
	//	AAA -- BBB -- CCC -- NN 4444 1
	//	AAA BBB CCC NN ------ 333 4444 1
	wsprintf(buf, "%.3x --/%.3x --/%.3x --/%.2x %.4x %x  ", 
			 (regs[0] + regs[1] * 256) & 0xfff, 
			 (regs[2] + regs[3] * 256) & 0xfff, 
			 (regs[4] + regs[5] * 256) & 0xfff,
			 regs[6], regs[11] + regs[12] * 256, regs[13] & 0x0f);
	for (y=0; y<3; y++)
	{
		const char* hex = "0123456789abcdef";
		const char* flg = " |-+";
		buf[7*y+4] = flg[((regs[ 7] >> y) &   1) + ((regs[ 7] >> (y+2)) & 2)];
		buf[7*y+5] = regs[8+y] & 16 ? '*' : hex[regs[8+y] & 0xf];
	}
/*
	wsprintf(buf, "%.3x %.3x %.3x %.2x ------ --- %.4x %x", 
			 (regs[0] + regs[1] * 256) & 0xfff, 
			 (regs[2] + regs[3] * 256) & 0xfff, 
			 (regs[4] + regs[5] * 256) & 0xfff,
			 regs[6], regs[11] + regs[12] * 256, regs[13]);
	buf[15] = regs[7] &   1 ? '-' : '1';
	buf[16] = regs[7] &   8 ? '-' : 'n';
	buf[17] = regs[7] &   2 ? '-' : '2';
	buf[18] = regs[7] &  16 ? '-' : 'n';
	buf[19] = regs[7] &   4 ? '-' : '3';
	buf[20] = regs[7] &  32 ? '-' : 'n';
	buf[22] = regs[ 8] & 16 ? '*' : (regs[ 8] > 9 ? regs[ 8] - 10 + 'a' : regs[ 8] + '0');
	buf[23] = regs[ 9] & 16 ? '*' : (regs[ 9] > 9 ? regs[ 9] - 10 + 'a' : regs[ 9] + '0');
	buf[24] = regs[10] & 16 ? '*' : (regs[10] > 9 ? regs[10] - 10 + 'a' : regs[10] + '0');
*/
	//	C1 C2 SSSS EEEE DDDD EE LLLL cc
	wsprintf(buf+32, "%.2x %.2x %.4x %.4x %.4x %.2x %.4x %.2x",
			 regs[0x100], regs[0x101],
			 (regs[0x102] + regs[0x103] * 256) & 0xffff, 
			 (regs[0x104] + regs[0x105] * 256) & 0xffff,
			 (regs[0x109] + regs[0x10a] * 256) & 0xffff,
			 regs[0x10b],
			 (regs[0x10c] + regs[0x10d] * 256) & 0xffff,
			 regs[0x110]);
	PutLine("%.32s %.32s", buf, buf+32);

	//	00001111222233334444555566667777
	//  -- - --- -- -- -- --            
	{
		char* ptr = buf;
		int i;
		for (i=0; i<3; i++) ToHex(&ptr, regs[0x10 + i]), *ptr++ = ' ';
		ptr[-1] = '/';
		for (i=8; i<14; i++) ToHex(&ptr, regs[0x10 + i]), *ptr++ = ' ';
		PutLine("%.2x %x %.3x %.2x %.2x %.2x %.2x             %.26s",
				regs[0x21], regs[0x22] & 0x0f, (regs[0x24] + regs[0x25] * 256) & 0x3ff,
				regs[0x26], regs[0x27], regs[0x28], regs[0x29],
				buf);
	}
	for (y=3; y<10; y++)
	{
		char* ptr = buf;
		for (int z=0; z<2; z++)
		{
			for (int c=0; c<3; c++)
			{
				for (int s=0; s<4; s++)
				{
					const static sct[4] = { 0*4, 2*4, 1*4, 3*4 };
					ToHex(&ptr, regs[z*0x100 + y*0x10 + sct[s] + c]);
				}
				*ptr++ = ' ';
			}
		}
		*ptr=0;
		PutLine("%x: %.26s    %x: %.26s", y, buf, y, buf+27);
	}
	if (regs[0x27] & 0xc0)
	{
		PutLine("a: %.5x    %.5x    %.5x %.5x a: %.5x    %.5x    %.5x",
				ToFnum(regs[0x0a4]*0x100+regs[0x0a0]), ToFnum(regs[0x0a5]*0x100+regs[0x0a1]), ToFnum(regs[0x0ad]*0x100+regs[0x0a9]), ToFnum(regs[0x0ae]*0x100+regs[0x0aa]),
				ToFnum(regs[0x1a4]*0x100+regs[0x1a0]), ToFnum(regs[0x1a5]*0x100+regs[0x1a1]), ToFnum(regs[0x1a6]*0x100+regs[0x1a2]));
		PutLine("b: %.4x  %.4x  %.4x  %.5x %.5x b: %.4x     %.4x     %.4x",
				ToFB  (regs[0x0b4]*0x100+regs[0x0b0]), ToFB  (regs[0x0b5]*0x100+regs[0x0b1]), ToFB(regs[0x0b6]*0x100+regs[0x0b2]),
				ToFnum(regs[0x0ac]*0x100+regs[0x0a8]), ToFnum(regs[0x0a6]*0x100+regs[0x0a2]),
				ToFB  (regs[0x1b4]*0x100+regs[0x1b0]), ToFB  (regs[0x1b5]*0x100+regs[0x1b1]), ToFB(regs[0x1b6]*0x100+regs[0x1b2]));
	}
	else
	{
		PutLine("a: %.5x    %.5x    %.5x       a: %.5x    %.5x    %.5x",
				ToFnum(regs[0x0a4]*0x100+regs[0x0a0]), ToFnum(regs[0x0a5]*0x100+regs[0x0a1]), ToFnum(regs[0x0a6]*0x100+regs[0x0a2]), 
				ToFnum(regs[0x1a4]*0x100+regs[0x1a0]), ToFnum(regs[0x1a5]*0x100+regs[0x1a1]), ToFnum(regs[0x1a6]*0x100+regs[0x1a2]));
		PutLine("b: %.4x     %.4x     %.4x        b: %.4x     %.4x     %.4x",
				ToFB(regs[0x0b4]*0x100+regs[0x0b0]), ToFB(regs[0x0b5]*0x100+regs[0x0b1]), ToFB(regs[0x0b6]*0x100+regs[0x0b2]),
				ToFB(regs[0x1b4]*0x100+regs[0x1b0]), ToFB(regs[0x1b5]*0x100+regs[0x1b1]), ToFB(regs[0x1b6]*0x100+regs[0x1b2]));
	}
}


void OPNMonitor::Draw(HWND hwnd, HDC hdc)
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

void OPNMonitor::Paint(HWND hwnd)
{
	PAINTSTRUCT ps;
	HDC hdc = BeginPaint(hwnd, &ps);
	Draw(hwnd, hdc);
	EndPaint(hwnd, &ps);
}

void OPNMonitor::Update()
{
	if (hwnd)
	{
		HDC hdc = GetDC(hwnd);
		Draw(hwnd, hdc);
		ReleaseDC(hwnd, hdc);
	}
}
