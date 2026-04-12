//
//	Windows 系 includes
//	すべての標準ヘッダーを含む
//
//	$Id: headers.h,v 1.8 1999/10/10 01:47:21 cisc Exp $
//

#ifndef WIN_HEADERS_H
#define WIN_HEADERS_H

#define STRICT
#define WIN32_LEAN_AND_MEAN
//#define _WIN32_IE		0x200
#define _WIN32_IE		0x300		//	ListViewの都合で
// #define VC_EXTRALEAN			// これって，MFC 専用 ?

#include <windows.h>
#include <commctrl.h>
#include <commdlg.h>
#include <mmsystem.h>
//#include <ddraw.h>			//	CE で DirectX は非サポート(by TAN-Y)
//#include <dsound.h>			//	同上
#include <stdio.h>
#include <stdarg.h>
#include <math.h>
#include <string.h>
//#include <process.h>			//	保留
#include <assert.h>
//#include <time.h>				//	保留

#ifdef _MSC_VER
	#undef max
	#define max _MAX
	#undef min
	#define min _MIN
#endif

#endif	// WIN_HEADERS_H
