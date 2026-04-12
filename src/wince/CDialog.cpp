/*===========================================================================
		M88 for HandheldPC
		Original Version (c)cisc
		Handheld PC Version Programmed by Y.Taniuchi ( TAN-Y )

		ダイアログクラス実装
===========================================================================*/

#include "m88h.h"

static CDialog *g_pActiveDialog = NULL;


/*---------------------------------------------------------------------------
		コンストラクタ
---------------------------------------------------------------------------*/

CDialog::CDialog( HINSTANCE hInstSrc, LPCTSTR szTemplateSrc )
{
	m_hDlg = NULL;
	m_hInst = hInstSrc;
	m_szTemplate = szTemplateSrc;
}


/*---------------------------------------------------------------------------
		デストラクタ
---------------------------------------------------------------------------*/

CDialog::~CDialog()
{
}


/*---------------------------------------------------------------------------
		ダイアログの作成
---------------------------------------------------------------------------*/

int CDialog::Create( HWND hWndOwner, CDialog *pDlg )
{
int iRet = 0;

	if( g_pActiveDialog != NULL )
	{
		//
		//	g_pActiveDialog が NULL でなければ、他スレッドでダイアログ使用中
		//
		return -1;
	}

	g_pActiveDialog = pDlg;
	iRet = ::DialogBox( pDlg->m_hInst, pDlg->m_szTemplate,
						hWndOwner, MainDlgProc );
	g_pActiveDialog = NULL;

	return iRet;
}


/*---------------------------------------------------------------------------
		ウインドウプロシージャ処理
---------------------------------------------------------------------------*/

BOOL CALLBACK CDialog::MainDlgProc( HWND hWnd, UINT uMsg,
										WPARAM wParam, LPARAM lParam )
{
	if( g_pActiveDialog == NULL )
	{
		return FALSE;
	}

	g_pActiveDialog->m_hDlg = hWnd;
	return g_pActiveDialog->DlgProc( uMsg, wParam, lParam );
}


BOOL CDialog::DlgProc( UINT uMsg, WPARAM wParam, LPARAM lParam )
{
	if( uMsg == WM_COMMAND )
	{
		if( LOWORD(wParam) == IDCANCEL )
		{
			End( 1 );
		}
	}

	return FALSE;
}



/*===========================================================================
		CAboutDialog
===========================================================================*/
/*---------------------------------------------------------------------------
		コンストラクタ
---------------------------------------------------------------------------*/

CAboutDialog::CAboutDialog( HINSTANCE hInst )
	: CDialog( hInst, MAKEINTRESOURCE(IDD_ABOUT_H) )
{
}


/*---------------------------------------------------------------------------
		ダイアログ関数
---------------------------------------------------------------------------*/
BOOL CAboutDialog::DlgProc( UINT uMsg, WPARAM wParam, LPARAM lParam )
{
	switch( uMsg )
	{
		case WM_INITDIALOG:
		{
			SetDlgItemText( IDC_ABOUT_TEXT, 
								_T("M88/handheld  rel ")_T(M88P_REL)
								_T(" based on M88 for Win32 rel 2.15\n")
								_T("Copyright (C) 1998, 1999 cisc\n")
								_T("Copyright (C) 2001 TAN-Y")
							);
			SetDlgItemText( IDC_ABOUT_BOX,
								_T("オリジナルの作者であるciscさんへ")
								_T("大感謝。\r\n")
								_T("感想・要望・バグ報告などは\r\n")
								_T("http://www.aosoft.jp/\r\n")
								_T("へどうぞ。\r\n")
							);
		}
	}

	return CDialog::DlgProc( uMsg, wParam, lParam );
}
