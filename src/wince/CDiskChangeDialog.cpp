/*===========================================================================
		M88 for HandheldPC
		Original Version (c)cisc
		Handheld PC Version Programmed by Y.Taniuchi ( TAN-Y )

		ディスク入れ替えダイアログクラス実装
===========================================================================*/

#include "m88h.h"

static CDialog *g_pActiveDialog = NULL;


/*---------------------------------------------------------------------------
		コンストラクタ
---------------------------------------------------------------------------*/

CDiskChangeDialog::CDiskChangeDialog( HINSTANCE hInst,
										CDiskImageManager *pDiskImgMgr,
										int iDrv )
	: CDialog( hInst, MAKEINTRESOURCE(IDD_DISKSEL) )
{
	m_pDiskImgMgr = pDiskImgMgr;
	m_iDrv = iDrv;
}


/*---------------------------------------------------------------------------
		ダイアログ関数
---------------------------------------------------------------------------*/
BOOL CDiskChangeDialog::DlgProc( UINT uMsg, WPARAM wParam, LPARAM lParam )
{
	switch( uMsg )
	{
		case WM_INITDIALOG:
		{
		DiskManager *pDiskMgr = m_pDiskImgMgr->GetDiskManager();
		int iDisks = Min(pDiskMgr->GetNumDisks( m_iDrv ), 60);

			SendDlgItemMessage( IDC_DISKLIST, LB_ADDSTRING, 0,
								(LPARAM)_T("No disk") );

			for( int i = 0; i < iDisks; i++ )
			{
				const char *szTitle = pDiskMgr->GetImageTitle( m_iDrv, i );

				if( szTitle == NULL )
				{
					break;
				}

				if( szTitle[0] == 0 )
				{
					SendDlgItemMessage( IDC_DISKLIST, LB_ADDSTRING, 0,
											(LPARAM)_T("(untitled)") );
					continue;
				}

				LPWSTR wszTitle = MultiToWide( szTitle );
				SendDlgItemMessage( IDC_DISKLIST, LB_ADDSTRING, 0,
										(LPARAM)wszTitle );
				delete[] wszTitle;
			}

			SendDlgItemMessage( IDC_DISKLIST, LB_SETCURSEL,
							pDiskMgr->GetCurrentDisk( m_iDrv ) + 1, 0 );

		}
		break;

		case WM_COMMAND:
		{
			switch( LOWORD(wParam) )
			{
				case IDOK:
				{
				int iSel = SendDlgItemMessage( IDC_DISKLIST,
												LB_GETCURSEL, 0, 0 );

					m_pDiskImgMgr->OpenDiskImage( m_iDrv, "",
													0, iSel - 1, false );
					End( 1 );
				}
				break;

				case IDCANCEL:
				{
					End( 1 );
				}
			}
		}
		break;


	}

	return CDialog::DlgProc( uMsg, wParam, lParam );
}
