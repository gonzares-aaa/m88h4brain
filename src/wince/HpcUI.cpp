/*===========================================================================
		M88 for HandheldPC
		Original Version (c)cisc
		Handheld PC Version Programmed by Y.Taniuchi ( TAN-Y )

		HpcUI クラス実装
===========================================================================*/

#include "m88h.h"
#include "romaji.h"
#include "quicksave.h"

static void GetFileNameTitle(char* title, const char* name);
static bool OpenDiskImage( HWND hWndParent, HINSTANCE hInstance,
									LPTSTR szRetBuf, DWORD dwBufSize );

using namespace PC8801;


/*---------------------------------------------------------------------------
		コンストラクタ
---------------------------------------------------------------------------*/

HpcUI::HpcUI( HINSTANCE hInst )
	: WCEUI( hInst )
{
	m_pDraw = &m_drawHVGA;
	m_pKeyIF = &m_keyif;

	m_hWndCB = NULL;
	m_iPrevBasicModeMenu = 0;

	waveOutGetVolume(NULL, &mute_before_vol);	// Mute前のボリューム値を退避
	waveOutSetVolume(NULL, 0);					// ボリュームを0にする
	isMute = true;

}


/*---------------------------------------------------------------------------
		デストラクタ
---------------------------------------------------------------------------*/

HpcUI::~HpcUI()
{
}


/*---------------------------------------------------------------------------
		M88 の初期化(サブクラス用)
---------------------------------------------------------------------------*/

bool HpcUI::InitM88Sub()
{

	m_hMenu = ::LoadMenu( GetInstance(), MAKEINTRESOURCE( IDR_MENU_M88P ) );
	if( m_hMenu == NULL )
	{
		return false;
	}

	// ローマ字テーブルの初期化
	romaji_init();

	m_pDiskImgMgr = new CDiskImageManager( GetDiskManager() );

	if( GetM88Config()->flags & Config::usearrowfor10 )
	{
		HpcKeyIF::SetUseArrowFor10Flag( true );
	} else {
		HpcKeyIF::SetUseArrowFor10Flag( false );
	}

	return true;
}



/*---------------------------------------------------------------------------
		M88 の後始末(サブクラス用)
---------------------------------------------------------------------------*/

void HpcUI::CleanupM88Sub()
{
	if( m_pDiskImgMgr != NULL )
	{
		delete m_pDiskImgMgr;
		m_pDiskImgMgr = NULL;
	}
}


/*---------------------------------------------------------------------------
		ウインドウクラスのパラメータ設定
		追加で必要であればオーバーライド（メニュー等）
		ここではDrawインスタンスの選択に使用
---------------------------------------------------------------------------*/

void HpcUI::InitWindowClass( WNDCLASS *pWcl )
{
int iType = 0;

	if (GetM88DefFile() != NULL)
	{
		iType = GetM88DefFile()->GetValue(CM88DefFile::DrawType);
	}

	switch(iType)
	{
	case 1:
		{
			m_pDraw = &m_drawVGA;
		}
		break;

	case 2:
		{
			m_pDraw = &m_drawFullVGA;
		}
		break;

	default:
		{
			m_pDraw = &m_drawHVGA;
		}
		break;
	}
}

/*---------------------------------------------------------------------------
		ウインドウ生成
---------------------------------------------------------------------------*/

HWND HpcUI::CreateMainWindow( void )
{

//	DWORD wstyle = WS_CAPTION | WS_OVERLAPPED | WS_SYSMENU
//					| WS_MINIMIZEBOX | WS_VISIBLE;
	DWORD wstyle = WS_VISIBLE;

	//hwnd = CreateWindowEx(
	HWND hWnd = CreateWindow(
								//WS_EX_ACCEPTFILES,
								GetWindowClassName(),
								GetWindowTitle(),
								wstyle,
								0,					// x
								0,					// y
								CW_USEDEFAULT,		// w
								CW_USEDEFAULT,		// h
								NULL,
								NULL,
								GetInstance(),
								NULL
								);


	return hWnd;
}

/*---------------------------------------------------------------------------
		ローマ字入力用タイマーの自動管理
---------------------------------------------------------------------------*/
static void UpdateRomajiTimer(HWND hWnd, PC8801::HpcKeyIF* pKeyIF)
{
	static bool s_bTimerRunning = false;
	
	// ローマ字入力ON かつ カナキーがロックされている時だけタイマーを回す
	bool bShouldRun = pKeyIF->IsRomajiMode() && pKeyIF->IsKanaLocked();
	
	if (bShouldRun && !s_bTimerRunning) {
		::SetTimer(hWnd, 1, 16, NULL);
		s_bTimerRunning = true;
	} else if (!bShouldRun && s_bTimerRunning) {
		::KillTimer(hWnd, 1);
		s_bTimerRunning = false;
	}
}

/*---------------------------------------------------------------------------
		ウインドウ関数
---------------------------------------------------------------------------*/

LRESULT CALLBACK HpcUI::WndProc( HWND hWnd, UINT uMsg,
										WPARAM wParam, LPARAM lParam )
{
static int iDownKeyCode = 0xff;


	switch( uMsg )
	{
		case WM_CREATE:
		{

			OnCreate( hWnd );
			return 0;
		}
		break;

		// ★追加: タイマーを受け取って HpcKeyIF に流す
		case WM_TIMER:
		{
			m_keyif.OnTimer();
			return 0;
		}
		break;

		case WM_COMMAND:
		{
			OnCommand( hWnd, wParam, lParam );
			return 0;
		}
		break;

		case WM_KEYUP:
		{
			m_keyif.KeyUp( (uint)wParam, (uint32)lParam );
			//  カナモード切替キー(0x7b)が離された時だけタイマー判定を行う！
			if (wParam == 0x7b) {
				UpdateRomajiTimer(hWnd, &m_keyif);
			}
			return 0;
		}
		break;

		case WM_KEYDOWN:
		{
			m_keyif.KeyDown( (uint)wParam, (uint32)lParam );
			return 0;
		}
		break;

		case WM_LBUTTONUP:
		{
			if( CommandBar_IsVisible( m_hWndCB ) )
			{
				Activate( true );
				CommandBar_Show( m_hWndCB, FALSE );
			} else {
				Activate( false );
				CheckBasicModeRadioItem();
				CommandBar_Show( m_hWndCB, TRUE );
			}
			return 0;
		}


		case WM_CLOSE:
		{
			Activate( false );
			::DestroyWindow( hWnd );
			return 0;
		}
		break;

		case WM_DESTROY:
		{
			// ウインドウが閉じられるときにタイマーを止める
			::KillTimer( hWnd, 1 );
			::PostQuitMessage( 0 );
			return 0;
		}
	}


	return DefWindowProc( hWnd, uMsg, wParam, lParam );
}


/*---------------------------------------------------------------------------
		WM_CREATE イベント
---------------------------------------------------------------------------*/

void HpcUI::OnCreate( HWND hWnd )
{
	//
	//	アイコン設定
	//
	HICON hIcon = (HICON)LoadImage( GetInstance(),
									MAKEINTRESOURCE(IDI_ICON_M88),
									IMAGE_ICON, 16, 16,
									LR_DEFAULTCOLOR );
	SendMessage( hWnd, WM_SETICON, FALSE, (WPARAM)hIcon );

	//
	//	コマンドバー生成
	//
	m_hWndCB = CommandBar_Create( GetInstance(), hWnd, 1 );
	CommandBar_InsertMenubar( m_hWndCB, GetInstance(), IDR_MENU_M88P, 0);
	CommandBar_AddAdornments( m_hWndCB, CMDBAR_HELP, 0 );

	CommandBar_Show( m_hWndCB, FALSE );

	//
	//	メニュー反映
	//
	CheckBasicModeRadioItem();
}


/*---------------------------------------------------------------------------
		WM_COMMAND イベント
---------------------------------------------------------------------------*/

void HpcUI::OnCommand( HWND hWnd, WPARAM wParam, LPARAM lParam )
{
HMENU hMenu = CommandBar_GetMenu( m_hWndCB, 0 );

	switch( LOWORD(wParam) )
	{
		case IDM_RESET:
		{
			if( ::MessageBox( hWnd, 
							_T("Reset?"), _T("M88/handheld"),
							MB_YESNO | MB_ICONQUESTION ) == IDYES )
			{
				ApplyConfig();
				GetCore()->Reset();
			}
		}
		break;

		case IDM_MUTE:	// Mute処理
		{
			if (!isMute) {	// MuteされていないのでMuteにする
				waveOutGetVolume(NULL, &mute_before_vol);	// Mute前のボリューム値を退避
				waveOutSetVolume(NULL, 0);					// ボリュームを0にする
				isMute = true;
				CheckBasicModeRadioItem();
			} else {		// Mute前に戻す
				waveOutSetVolume(NULL, mute_before_vol);	// 退避していたボリューム値に設定
				isMute = false;
				CheckBasicModeRadioItem();
			}
			CommandBar_DrawMenuBar( m_hWndCB, 0 );
		}
		break;

		// Romaji inputが選択された時の処理
		case IDM_ROMAJI:
		{
			// ローマ字モードのフラグを反転させる
			m_keyif.ToggleRomajiMode();
			// メニューのチェックマーク状態を更新
			CheckBasicModeRadioItem();
			// メニューバーを再描画
			CommandBar_DrawMenuBar( m_hWndCB, 0 );
		}
		break;
		
		
		case IDM_NMODE:
		{
			GetM88Config()->basicmode = Config::N80;
			CheckBasicModeRadioItem();
//			::CheckMenuRadioItem( hMenu, IDM_N88V1, IDM_N88V2,
//									LOWORD( wParam ), MF_BYCOMMAND );
		}
		break;
		case IDM_N88V1:
		{
			GetM88Config()->basicmode = Config::N88V1;
			CheckBasicModeRadioItem();
//			::CheckMenuRadioItem( hMenu, IDM_N88V1, IDM_N88V2,
//									LOWORD( wParam ), MF_BYCOMMAND );
		}
		break;
		case IDM_N88V1H:
		{
			GetM88Config()->basicmode = Config::N88V1H;
			CheckBasicModeRadioItem();
//			::CheckMenuRadioItem( hMenu, IDM_N88V1, IDM_N88V2,
//									LOWORD( wParam ), MF_BYCOMMAND );
		}
		break;
		case IDM_N88V2:
		{
			GetM88Config()->basicmode = Config::N88V2;
			CheckBasicModeRadioItem();
//			::CheckMenuRadioItem( hMenu, IDM_N88V1, IDM_N88V2,
//									LOWORD( wParam ), MF_BYCOMMAND );
			CommandBar_DrawMenuBar( m_hWndCB, 0 );
		}
		break;

		case IDM_CONFIG:
		{
		HpcConfigDialog config( GetM88Config(),
								GetDiskImageManager(),
								GetM88DefFile());

			Activate( false );
			m_pDraw->Activate( false );

			config.Show( GetHWnd() );

			if( GetM88Config()->flags & Config::usearrowfor10 )
			{
				HpcKeyIF::SetUseArrowFor10Flag( true );
			} else {
				HpcKeyIF::SetUseArrowFor10Flag( false );
			}

			m_pDraw->RequestPaint();
			m_pDraw->Activate( true );
			Activate( true );
		}
		break;

		case IDM_EXIT:
		{
			if( ::MessageBox( hWnd, 
							_T("Exit?"), _T("M88/handheld"),
							MB_YESNO | MB_ICONQUESTION ) == IDYES )
			{
				::SendMessage( hWnd, WM_CLOSE, 0, 0 );
			}
		}
		break;

		case IDM_DRIVE_1:
		{
		CDiskChangeDialog diskchange( GetInstance(), m_pDiskImgMgr, 0 );

			CDialog::Create( hWnd, &diskchange );
		}
		break;

		case IDM_DRIVE_1_IC:
		{
		WCHAR szBuf[MAX_PATH + 1];
			szBuf[0] = 0;
			if( OpenDiskImage( hWnd, GetInstance(), szBuf, MAX_PATH ) )
			{
			LPSTR szName = WideToMulti( szBuf );

				m_pDiskImgMgr->OpenDiskImage( 0, szName, 0, 0, false );
				delete[] szName;
			}
		}
		break;

		case IDM_DRIVE_2:
		{
		CDiskChangeDialog diskchange( GetInstance(), m_pDiskImgMgr, 1 );

			CDialog::Create( hWnd, &diskchange );
		}
		break;

		case IDM_DRIVE_2_IC:
		{
		WCHAR szBuf[MAX_PATH + 1];
			szBuf[0] = 0;
			if( OpenDiskImage( hWnd, GetInstance(), szBuf, MAX_PATH ) )
			{
			LPSTR szName = WideToMulti( szBuf );

				m_pDiskImgMgr->OpenDiskImage( 1, szName, 0, 0, false );
				delete[] szName;
			}
		}
		break;

		case IDM_BOTHDRIVE:
		{
		WCHAR szBuf[MAX_PATH + 1];

			szBuf[0] = 0;
			if( OpenDiskImage( hWnd, GetInstance(), szBuf, MAX_PATH ) )
			{
			LPSTR szName = WideToMulti( szBuf );

				m_pDiskImgMgr->OpenDiskImage( szName );
				delete[] szName;
//				CreateDiskMenu( 0 );
//				CreateDiskMenu( 1 );
			}
		}
		break;

		case IDM_ABOUTM88:
		{
		CAboutDialog about( GetInstance() );

			CDialog::Create( hWnd, &about );
		}
		break;


		// === どこでもセーブ (Quick Save) ===
        case IDM_QUICKSAVE_SLOT1: quicksave_save(0, GetCore(), GetCore()->GetDiskManager()); break;
        case IDM_QUICKSAVE_SLOT2: quicksave_save(1, GetCore(), GetCore()->GetDiskManager()); break;
        case IDM_QUICKSAVE_SLOT3: quicksave_save(2, GetCore(), GetCore()->GetDiskManager()); break;
        case IDM_QUICKSAVE_SLOT4: quicksave_save(3, GetCore(), GetCore()->GetDiskManager()); break;
        case IDM_QUICKSAVE_SLOT5: quicksave_save(4, GetCore(), GetCore()->GetDiskManager()); break;

        // === どこでもロード (Quick Load) ===
        case IDM_QUICKLOAD_SLOT1: 
        case IDM_QUICKLOAD_SLOT2: 
        case IDM_QUICKLOAD_SLOT3: 
        case IDM_QUICKLOAD_SLOT4: 
        case IDM_QUICKLOAD_SLOT5: 
        {
            // 1. 選択されたスロット番号を判定
            int slot = LOWORD(wParam) - IDM_QUICKLOAD_SLOT1;
            
            // 2. コアにステートデータをロードさせる
            quicksave_load(slot, GetCore(), GetCore()->GetDiskManager());
            
            // 3. ロードされたディスク状態を UI (m_pDiskImgMgr) に強制同期させる
            for(int i = 0; i < 2; i++) {
                const char* name = GetCore()->GetDiskManager()->GetDiskName(i);
                int idx = GetCore()->GetDiskManager()->GetCurrentDisk(i);
                
                if (name && name[0] != '\0' && idx >= 0) {
                    // UIクラスに正しいファイルパスとインデックスを認識させる
					bool ro = GetCore()->GetDiskManager()->IsReadOnly(i);
					
					// ポインタの参照切れによる消失バグを防ぐため、
                    // 文字列を安全なローカル変数にコピーしてから OpenDiskImage に渡す！
                    char nameCopy[MAX_PATH];
                    strncpy(nameCopy, name, MAX_PATH);
                    nameCopy[MAX_PATH - 1] = '\0';
					m_pDiskImgMgr->OpenDiskImage(i, nameCopy, ro, idx, false);
                }
                
                // メニューの「Drive 1 - [ファイル名]」の表示を直ちに更新する
                CreateDiskMenu(i);
            }
			// UI更新の巻き添えで壊れてしまったコアの状態を直すため、
            // もう一度だけステートを上書きロードして、ハードウェア状態を完璧に確定させる！
            quicksave_load(slot, GetCore(), GetCore()->GetDiskManager());
        }
        break;

	}

	m_pDraw->RequestPaint();
	Activate( true );
	CommandBar_Show( m_hWndCB, FALSE );
}


/*---------------------------------------------------------------------------
		BASIC MODE のラジオボタンをチェック
---------------------------------------------------------------------------*/

void HpcUI::CheckBasicModeRadioItem( void )
{
HMENU hMenu = CommandBar_GetMenu( m_hWndCB, 0 );
int iMenuItem = 0;

	switch( GetM88Config()->basicmode )
	{
		case Config::N80:
		{
			iMenuItem = IDM_NMODE;
		}
		break;
		case Config::N88V1:
		{
			iMenuItem = IDM_N88V1;
		}
		break;
		case Config::N88V1H:
		{
			iMenuItem = IDM_N88V1;
		}
		break;
		case Config::N88V2:
		{
			iMenuItem = IDM_N88V2;
		}
		break;
	}

	if( m_iPrevBasicModeMenu )
	{
		::CheckMenuItem( hMenu, m_iPrevBasicModeMenu, MF_UNCHECKED );
	}

	if( iMenuItem )
	{
		::CheckMenuItem( hMenu, iMenuItem, MF_CHECKED );
		//CommandBar_DrawMenuBar( m_hWndCB, 0 );
	}

	m_iPrevBasicModeMenu = iMenuItem;
	
	if (isMute) {
		::CheckMenuItem( hMenu, IDM_MUTE, MF_CHECKED );
	} else {
		::CheckMenuItem( hMenu, IDM_MUTE, MF_UNCHECKED );
	}

	// ローマ字モードのON/OFFに応じてチェックマークを更新
	if (m_keyif.IsRomajiMode()) {
		::CheckMenuItem( hMenu, IDM_ROMAJI, MF_CHECKED );
	} else {
		::CheckMenuItem( hMenu, IDM_ROMAJI, MF_UNCHECKED );
	}
}



/*---------------------------------------------------------------------------
		ディスクイメージ選択用メニュー作成
---------------------------------------------------------------------------*/

bool HpcUI::CreateDiskMenu( uint drive )
{
HMENU hMenu = CommandBar_GetMenu( m_hWndCB, 0 );

	TCHAR buf[MAX_PATH + 16];
	
	CDiskImageManager::DiskInfo& dinfo =
									*(m_pDiskImgMgr->GetDiskInfo( drive ));
	HMENU hmenuprev = dinfo.hmenu;
	dinfo.currentdisk = -1;

	int ndisks = Min(GetDiskManager()->GetNumDisks(drive), 30);
	if (ndisks)
	{
		// メニュー作成
		dinfo.hmenu = CreatePopupMenu();
		if (!dinfo.hmenu)
			return false;
		
		for (int i=0; i<ndisks; i++)
		{
			const char* title_tmp = GetDiskManager()->GetImageTitle(drive, i);
			LPWSTR title = MultiToWide( title_tmp );
			if (!title) break;
			if (!title[0]) title = _T("(untitled)");
			
			wsprintf(buf, i < 9 ? _T("&%d %.16s") : _T("%d %.16s"),
						i+1, title);
			AppendMenu(	dinfo.hmenu,
						MF_STRING | (i && !(i % 20) ? MF_MENUBARBREAK : 0),
						dinfo.idchgdisk + i,
						buf );
			delete[] title;
		}

		AppendMenu(dinfo.hmenu, MF_SEPARATOR, 0, 0);
		AppendMenu(dinfo.hmenu, MF_STRING, dinfo.idchgdisk + 63,
					_T("&N No disk"));
		AppendMenu(dinfo.hmenu, MF_STRING, dinfo.idchgdisk + 64,
					_T("&0 Change disk"));
//		SetMenuDefaultItem(dinfo.hmenu, dinfo.idchgdisk + 64, FALSE );
	}

	MENUITEMINFO mii;
	memset(&mii, 0, sizeof(mii));
	mii.cbSize = sizeof(mii);
	mii.fType = MFT_STRING;
	mii.fMask = MIIM_TYPE | MIIM_SUBMENU;
	mii.dwTypeData = buf;

	if (!ndisks)
	{
		wsprintf(buf, _T("Drive &%d..."), drive+1);
		mii.hSubMenu = 0;
	}
	else
	{
		char title_tmp[MAX_PATH] = "";
		GetFileNameTitle( title_tmp,
							m_pDiskImgMgr->GetDiskImageName( drive ) );
		LPWSTR title = MultiToWide( title_tmp );

		wsprintf(buf, _T("Drive &%d - %s"), drive+1, title);
		mii.hSubMenu = dinfo.hmenu;

		delete[] title;
	}
	SetMenuItemInfo(hMenu, drive ? IDM_DRIVE_2 : IDM_DRIVE_1, false, &mii);
	if (hmenuprev)
		DestroyMenu(hmenuprev);
	
//	if (ndisks)
//		SelectDisk(drive, dinfo.currentdisk, true);

	CommandBar_DrawMenuBar( m_hWndCB, 0 );
	return true;
}


// ---------------------------------------------------------------------------
//	ファイルネームの部分を取り出す
//
static void GetFileNameTitle(char* title, const char* name)
{
	if (name)
	{
		char* ptr = (char *)name;
		while( *ptr != NULL && *ptr != '\\' )
		{
			ptr++;
		}

		strcpy( title,  ( ptr ? ptr+1 : name ) );
		ptr = title;

		while( *ptr != NULL && *ptr != '.' )
		{
			ptr++;
		}

		if (ptr)
			*ptr = 0;
	}
}

/*---------------------------------------------------------------------------
		.D88選択
---------------------------------------------------------------------------*/

static bool OpenDiskImage( HWND hWndParent, HINSTANCE hInstance,
									LPTSTR szRetBuf, DWORD dwBufSize )
{
    OPENFILENAME ofn;

    // 実行ファイルのあるフォルダパスを取得する
    WCHAR szInitDir[MAX_PATH];
    GetModuleFileName(NULL, szInitDir, MAX_PATH);
    WCHAR* p = wcsrchr(szInitDir, L'\\');
    if (p) {
        *p = L'\0'; // ファイル名部分をカットしてディレクトリパスにする
    }

    ::ZeroMemory( &ofn, sizeof(OPENFILENAME) );
    ofn.lStructSize = sizeof(OPENFILENAME);
    ofn.hwndOwner = hWndParent;
    ofn.hInstance = hInstance;
    ofn.lpstrFilter =
            _T("D88イメージ{*.D88|*.88D|*.DAT}\0*.D88;*.88D;*.DAT\0\0");
    ofn.lpstrFile = szRetBuf;
    ofn.nMaxFile = dwBufSize;
    ofn.Flags = OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;
    ofn.lpstrInitialDir = szInitDir;
    ofn.lpstrDefExt = NULL;

    if( ::GetOpenFileName( &ofn ) )
    {
        return true;
    }

    return false;
}