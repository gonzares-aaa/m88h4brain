/*===========================================================================
		M88 for PocketPC / HandheldPC
		Original Version (c)cisc
		WindowsCE Version Programmed by Y.Taniuchi ( TAN-Y )

		共通メイン
===========================================================================*/

#include "m88ce.h"
#include "strcv.h"
#include <winnls.h>

CComModule _Module;

int M88ceMain( WCEUI *pUI, int nwinmode )
{
WCHAR wszModuleName[MAX_PATH];
LPWSTR wszFileName = wszModuleName;
LPWSTR wszTmp = wszModuleName;
HANDLE hMutexMultiBoot = NULL;
int iRetCode = 0;
//INITCOMMONCONTROLSEX cc;

	//
	//	多重起動防止
	//
	hMutexMultiBoot = ::CreateMutex( NULL, FALSE,
										WCEUI::GetWindowClassName() );

	if( ::GetLastError() == ERROR_ALREADY_EXISTS )
	{
		//
		//	起動済みでウインドウがあったらフォーカス移動して終了
		//
		HWND hWnd = ::FindWindow( WCEUI::GetWindowClassName(),
									WCEUI::GetWindowTitle());
		if( hWnd )
		{
			::SetForegroundWindow( (HWND)(((DWORD)hWnd) | 0x01) );
		}
		return 0;
	}


	// jp localization
	SetSystemDefaultLCID(1041);
	PostMessage(HWND_BROADCAST, WM_WININICHANGE, 0, INI_INTL);

	_Module.Init( NULL, pUI->GetInstance() );

	//cc.dwSize = sizeof(INITCOMMONCONTROLSEX);
	//cc.dwICC = ICC_TAB_CLASSES;
	//::InitCommonControlsEx( &cc );
	::InitCommonControls();

	//
	//	カレントディレクトリ（＝.exe の存在位置）を見つける
	//
	GetModuleFileName( NULL, wszModuleName, MAX_PATH );
	while( *wszTmp != 0 )
	{
		if( *wszTmp == '\\' )
		{
			wszFileName = wszTmp;
		}
		wszTmp++;
	}
	*wszFileName = 0;

	LPSTR szDir = WideToMulti( wszModuleName );
	FileIO::SetCurrentDirectory( szDir );

	// Add Font
//	::MessageBox( NULL, _T(szDir), _T("M88"), MB_OK | MB_TOPMOST );
//	AddFontResource(_T("jptahoma.ttc"));

	delete[] szDir;


	if ( pUI->InitWindow( nwinmode ) )
	{
		iRetCode = pUI->Main();
	} else {
		::MessageBox( NULL, _T("ウインドウ初期化に失敗しました。"), _T("M88"),
						MB_OK | MB_TOPMOST );
	}

	::CloseHandle( hMutexMultiBoot );

	_Module.Term();

	return iRetCode;
}


