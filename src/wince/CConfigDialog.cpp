/*===========================================================================
		M88 for PocketPC
		Original Version (c)cisc
		Pocket PC Version Programmed by Y.Taniuchi ( TAN-Y )

		設定ダイアログクラス実装
===========================================================================*/

#include "m88ce.h"



/*---------------------------------------------------------------------------
		コンストラクタ
---------------------------------------------------------------------------*/

CConfigDialog::CConfigDialog( int iPageCnt, PC8801::Config *pConfig,
								CDiskImageManager *pDiskImageManager )
{
	m_pConfig = pConfig;
	m_iPageCnt = iPageCnt;
	m_pDiskImageManager = pDiskImageManager;

	m_phPSPs = new HPROPSHEETPAGE[iPageCnt];
}


/*---------------------------------------------------------------------------
		デストラクタ
---------------------------------------------------------------------------*/

CConfigDialog::~CConfigDialog()
{
	if( m_phPSPs != NULL )
	{
		delete[] m_phPSPs;
	}

	m_phPSPs = NULL;
}


/*---------------------------------------------------------------------------
		ダイアログフラグ取得
---------------------------------------------------------------------------*/

DWORD CConfigDialog::GetDialogFlag()
{
	return PSH_NOAPPLYNOW;
}


/*---------------------------------------------------------------------------
		表示
---------------------------------------------------------------------------*/

int CConfigDialog::Show( HWND hWndParent )
{
PROPSHEETHEADER pspheader;


	if( InitPSP( m_phPSPs ) == false )
	{
		return -1;
	}

	pspheader.dwSize = sizeof( PROPSHEETHEADER );
	pspheader.dwFlags = GetDialogFlag() | PSH_USECALLBACK;
	pspheader.hwndParent = hWndParent;
	pspheader.hInstance = _Module.GetResourceInstance();
	pspheader.pszIcon = NULL;
	pspheader.pszCaption = _T("M88/ce Configuration");
	pspheader.nPages = m_iPageCnt;
	pspheader.nStartPage = 0;
	pspheader.phpage = m_phPSPs;
	pspheader.pfnCallback = PropSheetCallback;

	InitPSPHeader( &pspheader );

//	::SetWindowLong( hWndParent, GWL_STYLE, WS_VISIBLE | WS_OVERLAPPED );
	int iRetCode = ::PropertySheet( &pspheader );
//	::SetWindowLong( hWndParent, GWL_STYLE, WS_VISIBLE | WS_POPUP );

	return iRetCode;
}


/*---------------------------------------------------------------------------
		コールバック
---------------------------------------------------------------------------*/

int CALLBACK CConfigDialog::PropSheetCallback(HWND hWnd,
											UINT uMsg, LPARAM lParam)
{

#ifdef PSCB_GETVERSION
	switch( uMsg )
	{
		//
		//	H/PCはダメ?
		//
		case PSCB_GETVERSION:
		{
			return COMCTL32_VERSION;
		}
	}
#endif

	return 0;
}



