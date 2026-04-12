/*===========================================================================
		M88 for PocketPC
		Original Version (c)cisc
		Pocket PC Version Programmed by Y.Taniuchi ( TAN-Y )

		プロパティページ関係クラス実装
===========================================================================*/

#include "m88p.h"

#define BSTATE(b) (b ? BST_CHECKED : BST_UNCHECKED)

LPCWSTR aszKeyNames[];

using namespace PC8801;



static bool OpenDiskImage( HWND hWndParent, HINSTANCE hInstance,
									LPTSTR szRetBuf, DWORD dwBufSize )
{
OPENFILENAME ofn;

	::ZeroMemory( &ofn, sizeof(OPENFILENAME) );
	ofn.lStructSize = sizeof(OPENFILENAME);
	ofn.hwndOwner = hWndParent;
	ofn.hInstance = hInstance;
	ofn.lpstrFilter =
			_T("D88イメージ{*.D88|*.88D|*.DAT}\0*.D88;*.88D;*.DAT\0\0");
	ofn.lpstrFile = szRetBuf;
	ofn.nMaxFile = dwBufSize;
	ofn.Flags = OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;
	ofn.lpstrInitialDir = NULL;
	ofn.lpstrDefExt = NULL;

	if( ::GetOpenFileName( &ofn ) )
	{
		return true;
	}

	return false;
}


/*---------------------------------------------------------------------------
		コンストラクタ
---------------------------------------------------------------------------*/

PpcConfigDialog::PpcConfigDialog( PC8801::Config *pConfig,
									CDiskImageManager *pDiskImageManager,
									PpcKeyDefFile *pKeyDef )
	: CConfigDialog( CONFIGDIALOG_PAGEMAX, pConfig, pDiskImageManager )
{
	m_pKeyDef = pKeyDef;
}


/*---------------------------------------------------------------------------
		デストラクタ
---------------------------------------------------------------------------*/

PpcConfigDialog::~PpcConfigDialog()
{
	RELEASE_OBJ( m_pPropAbout );
	RELEASE_OBJ( m_pPropKeyMap );
	RELEASE_OBJ( m_pPropDipSW2 );
	RELEASE_OBJ( m_pPropDipSW1 );
	RELEASE_OBJ( m_pPropSound2 );
	RELEASE_OBJ( m_pPropSound1 );
	RELEASE_OBJ( m_pPropCPU );
	RELEASE_OBJ( m_pPropDisk );
	RELEASE_OBJ( m_pPropControl );
}


/*---------------------------------------------------------------------------
		ダイアログフラグ取得
---------------------------------------------------------------------------*/

DWORD PpcConfigDialog::GetDialogFlag()
{
	return PSH_NOAPPLYNOW | PSH_MAXIMIZE;
}


/*---------------------------------------------------------------------------
		PSP生成
---------------------------------------------------------------------------*/

bool PpcConfigDialog::InitPSP( HPROPSHEETPAGE *phPSP )
{
	m_pPropControl = new CPropControl( GetConfig() );
	phPSP[0] = m_pPropControl->Create();

	m_pPropDisk = new CPropDisk( GetConfig(), GetDiskImageManager() );
	phPSP[1] = m_pPropDisk->Create();

	m_pPropCPU = new CPropCPU( GetConfig() );
	phPSP[2] = m_pPropCPU->Create();

	m_pPropSound1 = new CPropSound1( GetConfig() );
	phPSP[3] = m_pPropSound1->Create();

	m_pPropSound2 = new CPropSound2( GetConfig() );
	phPSP[4] = m_pPropSound2->Create();

	m_pPropDipSW1 = new CPropDipSW1( GetConfig() );
	phPSP[5] = m_pPropDipSW1->Create();

	m_pPropDipSW2 = new CPropDipSW2( GetConfig() );
	phPSP[6] = m_pPropDipSW2->Create();

	m_pPropKeyMap = new CPropKeyMap( GetConfig(), m_pKeyDef );
	phPSP[7] = m_pPropKeyMap->Create();

	m_pPropAbout = new CPropAbout( GetConfig() );
	phPSP[8] = m_pPropAbout->Create();

	return true;
}


/*---------------------------------------------------------------------------
		PROPSHEETHEADER の初期化
---------------------------------------------------------------------------*/

void PpcConfigDialog::InitPSPHeader( PROPSHEETHEADER *pPSPHeader )
{
}




/*===========================================================================
		CPropControl
===========================================================================*/

void CPropControl::SetActive(HWND hdlg)
{
	int iDlgItem = 0;
	switch( GetConfig()->basicmode )
	{
		case PC8801::Config::N80:
		{
			iDlgItem = IDC_CONTROL_N;
		}
		break;
		case PC8801::Config::N88V1:
		{
			iDlgItem = IDC_CONTROL_N88V1S;
		}
		break;
		case PC8801::Config::N88V1H:
		{
			iDlgItem = IDC_CONTROL_N88V1H;
		}
		break;
		case PC8801::Config::N88V2:
		{
			iDlgItem = IDC_CONTROL_N88V2;
		}
		break;
	}
	if( iDlgItem )
	{
		::SendDlgItemMessage( hdlg, iDlgItem,
								BM_SETCHECK, BST_CHECKED, 0 );
	}

	::SetWindowLong( hdlg, DWL_MSGRESULT, 0 );
}


bool CPropControl::Command(HWND hdlg, HWND hwctl, UINT nc, UINT id)
{
	switch( id )
	{
		case IDC_CONTROL_N:
		{
			GetConfig()->basicmode = PC8801::Config::N80;
			return true;
		}
		break;
		case IDC_CONTROL_N88V1S:
		{
			GetConfig()->basicmode = PC8801::Config::N88V1;
			return true;
		}
		break;
		case IDC_CONTROL_N88V1H:
		{
			GetConfig()->basicmode = PC8801::Config::N88V1H;
			return true;
		}
		break;
		case IDC_CONTROL_N88V2:
		{
			GetConfig()->basicmode = PC8801::Config::N88V2;
			return true;
		}
		break;
	}

	return false;
}


/*===========================================================================
		CPropDisk
===========================================================================*/

void CPropDisk::SetActive(HWND hdlg)
{
	SetDiskImageList( hdlg, 0 );
	SetDiskImageList( hdlg, 1 );

	::SetWindowLong( hdlg, DWL_MSGRESULT, 0 );
}


bool CPropDisk::Command(HWND hdlg, HWND hwctl, UINT nc, UINT id)
{

	switch( id )
	{
		case IDC_DISK_DRV1IMGSEL:
		{
			if( nc == CBN_SELCHANGE )
			{
				int iSel = ::SendDlgItemMessage( hdlg,
												IDC_DISK_DRV1IMGSEL,
												CB_GETCURSEL, 0, 0 );

				m_pDiskImageManager->
					OpenDiskImage( 0, "", 0, iSel - 1, false );
			}
		}
		break;
		case IDC_DISK_DRV2IMGSEL:
		{
			if( nc == CBN_SELCHANGE )
			{
				int iSel = ::SendDlgItemMessage( hdlg,
												IDC_DISK_DRV2IMGSEL,
												CB_GETCURSEL, 0, 0 );

				m_pDiskImageManager->
					OpenDiskImage( 1, "", 0, iSel - 1, false );
			}
		}
		break;

		case IDC_DISK_DRV1CHG:
		{
		WCHAR szBuf[MAX_PATH + 1];

			if( OpenDiskImage( hdlg, _Module.GetResourceInstance(),
								szBuf, MAX_PATH ) )
			{
			LPSTR szName = WideToMulti( szBuf );

				m_pDiskImageManager->
					OpenDiskImage( 0, szName, 0, 0, false );
				delete[] szName;
				SetDiskImageList( hdlg, 0 );
			}
		}
		break;
		case IDC_DISK_DRV2CHG:
		{
		WCHAR szBuf[MAX_PATH + 1];

			if( OpenDiskImage( hdlg, _Module.GetResourceInstance(),
								szBuf, MAX_PATH ) )
			{
			LPSTR szName = WideToMulti( szBuf );

				m_pDiskImageManager->
					OpenDiskImage( 1, szName, 0, 0, false );
				delete[] szName;
				SetDiskImageList( hdlg, 1 );
			}
		}
		break;

		case IDC_DISK_CHGBOTH:
		{
		WCHAR szBuf[MAX_PATH + 1];

			if( OpenDiskImage( hdlg, _Module.GetResourceInstance(),
								szBuf, MAX_PATH ) )
			{
			LPSTR szName = WideToMulti( szBuf );

				m_pDiskImageManager->OpenDiskImage( szName );
				delete[] szName;
				SetDiskImageList( hdlg, 0 );
				SetDiskImageList( hdlg, 1 );
			}
		}
		break;
	}

	return false;
}


void CPropDisk::SetDiskImageList( HWND hWnd, uint uiDrive )
{
DiskManager *pDiskMgr = m_pDiskImageManager->GetDiskManager();
int iDlgItem;
int iDisks = Min(pDiskMgr->GetNumDisks(uiDrive), 60);

	if( uiDrive == 0 )
	{
		iDlgItem = IDC_DISK_DRV1IMGSEL;
	} else {
		iDlgItem = IDC_DISK_DRV2IMGSEL;
	}

	::SendDlgItemMessage( hWnd, iDlgItem, CB_RESETCONTENT, 0, 0 );


	::SendDlgItemMessage( hWnd, iDlgItem, CB_ADDSTRING, 0,
							(LPARAM)_T("No disk"));


	for( int i = 0; i < iDisks; i++ )
	{
		const char *szTitle = pDiskMgr->GetImageTitle( uiDrive, i );

		if( szTitle == NULL )
		{
			break;
		}

		if( szTitle[0] == 0 )
		{
			::SendDlgItemMessage( hWnd, iDlgItem, CB_ADDSTRING, 0,
									(LPARAM)_T("(untitled)"));
			continue;
		}

		LPWSTR wszTitle = MultiToWide( szTitle );
		::SendDlgItemMessage( hWnd, iDlgItem, CB_ADDSTRING, 0,
								(LPARAM)wszTitle );
		delete[] wszTitle;
	}

	::SendDlgItemMessage( hWnd, iDlgItem, CB_SETCURSEL,
							pDiskMgr->GetCurrentDisk( uiDrive ) + 1, 0 );


	if( uiDrive == 0 )
	{
		iDlgItem = IDC_DISK_DRV1IMGFILE;
	} else {
		iDlgItem = IDC_DISK_DRV2IMGFILE;
	}

	LPWSTR wszFileName =
		MultiToWide( m_pDiskImageManager->GetDiskImageName( uiDrive ) );

	::SendDlgItemMessage( hWnd, iDlgItem, WM_SETTEXT, 0, (LPARAM)wszFileName );
	delete[] wszFileName;

}

/*===========================================================================
		CPropKeyMap / CPropKeyMapKeySel
===========================================================================*/

void CPropKeyMap::InitDialog(HWND hdlg)
{
HWND hWndList = GetDlgItem( IDC_LIST1 );
LV_COLUMN col;

	ListView_SetExtendedListViewStyle( hWndList, LVS_EX_FULLROWSELECT );

	col.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
	col.fmt = LVCFMT_LEFT;

	col.cx = 32;
	col.pszText = _T("");
	col.iSubItem = 0;
	ListView_InsertColumn( hWndList, 0, &col );

	col.cx = 64;
	col.pszText = _T("実キー");
	col.iSubItem = 1;
	ListView_InsertColumn( hWndList, 1, &col );

	col.cx = 108;
	col.pszText = _T("PC88上でのキー");
	col.iSubItem = 2;
	ListView_InsertColumn( hWndList, 2, &col );

	LPCWSTR *pszKeyName = aszKeyNames;
	hWndList = GetDlgItem( IDC_LIST2 );

	while( *pszKeyName != 0 )
	{
		::SendMessage( hWndList, LB_ADDSTRING, 0, (LPARAM)*pszKeyName );
		pszKeyName++;
	}

	m_pKeySelSubClass->SubclassWindow( GetDlgItem( IDC_LIST2 ) );
}


void CPropKeyMap::SetActive(HWND hdlg)
{
	ListView_DeleteAllItems( GetDlgItem( IDC_LIST1 ) );
	UpdateConfigKeyList();
}


bool CPropKeyMap::Command(HWND hdlg, HWND hwctl, UINT nc, UINT id)
{

	if( id == IDC_DELETE )
	{
	HWND hWndList = GetDlgItem( IDC_LIST1 );
	int iSelected = ListView_GetNextItem( hWndList, -1,
											LVNI_ALL | LVNI_SELECTED );
		if( iSelected >= 0 )
		{
		TCHAR szTmp[16];
		int iVKey;

			ListView_GetItemText( hWndList, iSelected, 0, szTmp, 15 );
			iVKey = _ttoi( szTmp );

			ListView_DeleteItem( hWndList, iSelected );
			m_pKeyDef->SetKeyCode( iVKey, -1 );
		}

	}

	return false;
}


void CPropKeyMap::UpdateConfigKeyList( void )
{
HWND hWndList = GetDlgItem( IDC_LIST1 );
int iLastLine = ListView_GetItemCount( hWndList );
int aiVKList[256];
int i, n;

	for( i = 0; i < iLastLine; i++ )
	{
	TCHAR szTmp[16];
	int iVKey;

		ListView_GetItemText( hWndList, i, 0, szTmp, 15 );
		iVKey = _ttoi( szTmp );

		aiVKList[i] = iVKey;
	}

	for( i = 0; i < 256; i++ )
	{
		if( m_pKeyDef->GetKeyCode( i ) > -1 )
		{
		WCHAR szTmp[24];
		LV_ITEM item;

			item.mask = LVIF_TEXT;

			for( n = 0; n < iLastLine; n++ )
			{
				if( i == aiVKList[n] )
				{
					//
					//	すでにあったら抜け
					//
					item.iItem = n;
					break;
				}
			}

			if( n == iLastLine )
			{
				//
				//	なかったら新しい行追加。
				//
				aiVKList[iLastLine] = i;
				::wsprintf( szTmp, _T("%d"), i );
				item.iItem = iLastLine;
				item.iSubItem = 0;
				item.pszText = (LPTSTR)szTmp;
				ListView_InsertItem( hWndList, &item );

				iLastLine++;
			}

			item.iSubItem = 1;
			if( i == m_pKeyDef->GetGXKeyList()->vkUp )
			{
				item.pszText = _T("↑");
			} else if( i == m_pKeyDef->GetGXKeyList()->vkDown )
			{
				item.pszText = _T("↓");
			} else if( i == m_pKeyDef->GetGXKeyList()->vkLeft )
			{
				item.pszText = _T("←");
			} else if( i == m_pKeyDef->GetGXKeyList()->vkRight )
			{
				item.pszText = _T("→");
			} else if( i == m_pKeyDef->GetGXKeyList()->vkStart )
			{
				item.pszText = _T("Start");
			} else if( i == m_pKeyDef->GetGXKeyList()->vkA )
			{
				item.pszText = _T("A");
			} else if( i == m_pKeyDef->GetGXKeyList()->vkB )
			{
				item.pszText = _T("B");
			} else if( i == m_pKeyDef->GetGXKeyList()->vkC )
			{
				item.pszText = _T("C");
			} else
			{
				item.pszText = _T("-");
			}
			ListView_SetItem( hWndList, &item );


			item.iSubItem = 2;
			item.pszText = (LPTSTR)aszKeyNames[m_pKeyDef->GetKeyCode( i )];
			ListView_SetItem( hWndList, &item );
		}
	}
}


LRESULT CPropKeyMapKeySel::OnKeyUp( UINT uMsg, WPARAM wParam, 
							LPARAM lParam, BOOL &blHandled )
{
	//::MessageBox( m_hWnd, L"aaa", L"bbb", MB_OK );

	//
	//	ここでキー設定を更新する
	//
	int iPC88KeyCode = SendMessage( LB_GETCURSEL, 0, 0 );
	if( iPC88KeyCode == 0 || iPC88KeyCode == 9 || iPC88KeyCode == LB_ERR )
	{
		//
		//	選択不能の項目を選んだ
		//
		return 0;
	}
	m_pPropKeyMap->m_pKeyDef->SetKeyCode( (int)wParam, iPC88KeyCode );
	m_pPropKeyMap->UpdateConfigKeyList();

	return 0;
}


LRESULT CPropKeyMapKeySel::OnKeyDown( UINT uMsg, WPARAM wParam, 
							LPARAM lParam, BOOL &blHandled )
{

	return 0;
}


static LPCWSTR aszKeyNames[] = {
	L"----",
	L"F1", 	L"F2", L"F3", L"F4", L"F5",
	L"STOP", L"COPY", L"CTRL", L"----",
	L"ESC", L"TAB", L"CAPS", L"左SHIFT", L"カナ",
	L"1", L"2", L"3", L"4", L"5", L"6", L"7", L"8", L"9", L"0",
	L"-", L"^", L"\\",
	L"Q", L"W", L"E", L"R", L"T", L"Y", L"U", L"I", L"O", L"P",
	L"@", L"[",
	L"A", L"S", L"D", L"F", L"G", L"H", L"J", L"K", L"L",
	L";", L":", L"]",
	L"Z", L"X", L"C", L"V", L"B", L"N", L"M",
	L",", L".", L"/", L"_",
	L"GRAPH", L"SPACE",
	L"BS", L"RETURN", L"RETURN", L"右SHIFT",
	L"ROLL UP", L"ROLL DOWN", L"INS", L"DEL",
	L"↑", L"↓", L"←", L"→",
	L"HOME/CLR", L"HELP", L"テンキー -", L"テンキー /",
	L"テンキー 7", L"テンキー 8", L"テンキー 9", L"テンキー *",
	L"テンキー 4", L"テンキー 5", L"テンキー 6", L"テンキー +",
	L"テンキー 1", L"テンキー 2", L"テンキー 3", L"テンキー =",
	L"テンキー 0", L"テンキー ,", L"テンキー .", L"テンキー RETURN",
	L"\0"
};


/*===========================================================================
		CPropAbout
===========================================================================*/

void CPropAbout::InitDialog(HWND hdlg)
{
	::SendDlgItemMessage( hdlg, IDC_ABOUT_TEXT, WM_SETTEXT, 0,
					(LPARAM)
						_T("M88/pocket  rel ")_T(M88P_REL)_T("\n")
						_T("based on M88 for Win32\nrel 2.15\n\n")
						_T("Copyright (C) 1998, 1999 cisc\n")
						_T("Copyright (C) 2001, 2002 TAN-Y")
					);
	::SendDlgItemMessage( hdlg, IDC_ABOUT_BOX, WM_SETTEXT, 0,
					(LPARAM)
						_T("オリジナルの作者であるciscさんへ")
						_T("大感謝。\r\n")
						_T("感想・要望・バグ報告などは\r\n")
						_T("http://www.aosoft.jp/\r\n")
						_T("へどうぞ。\r\n")
					);
}


