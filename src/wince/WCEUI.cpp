/*===========================================================================
		M88 for PocketPC / HandheldPC
		Original Version (c)cisc
		WindowsCE Version Programmed by Y.Taniuchi ( TAN-Y )

		WCEUI クラス実装
===========================================================================*/

#include "m88ce.h"

WCEUI *WCEUI::m_pThis = NULL;

LPCTSTR WCEUI::m_szClassName = _T("M88p2 WinUI");
LPCTSTR WCEUI::m_szTitle = _T("M88/ce");


/*---------------------------------------------------------------------------
		コンストラクタ
---------------------------------------------------------------------------*/

WCEUI::WCEUI( HINSTANCE hInst )
{
	m_pThis = NULL;
	m_hWnd = NULL;
	m_hInstance = hInst;
	m_pDiskImageManager = NULL;
	m_pDefFile = NULL;
}


/*---------------------------------------------------------------------------
		デストラクタ
---------------------------------------------------------------------------*/

WCEUI::~WCEUI()
{
}


/*---------------------------------------------------------------------------
		M88 の初期化
---------------------------------------------------------------------------*/

bool WCEUI::InitM88()
{
	if (m_pDefFile == NULL)
	{
		m_pDefFile = new CM88DefFile( M88CE_DEFFILENAME );
		if( m_pDefFile == NULL )
		{
			return false;
		}
		m_pDefFile->Load();
	}

	//	てすと用逃げ(Soundを実装したらそっちで設定するように....)
//	m_core.GetOPN1()->SetRate( 100 );	//	SamplingRate
//	m_core.GetOPN2()->SetRate( 100 );

	//	てすと
//	m_config.basicmode = PC8801::Config::N88V2;
//	m_config.dipsw = 1829;
//	m_config.clock = 80;

	//	正規
	m_config.basicmode = (PC8801::Config::BASICMode)(
								m_pDefFile->GetValue( CM88DefFile::BASICMode )
							);
	m_config.dipsw = m_pDefFile->GetValue( CM88DefFile::Switches );
	m_config.clock = m_pDefFile->GetValue( CM88DefFile::CPUClock );
	m_config.sound = m_pDefFile->GetValue( CM88DefFile::Sound );
	m_config.soundbuffer = m_pDefFile->GetValue( CM88DefFile::SoundBuffer );
	m_config.flags = m_pDefFile->GetValue( CM88DefFile::Flags );
	m_config.flag2 = m_pDefFile->GetValue( CM88DefFile::Flag2 );

	DiskManager *pDiskMgr = new DiskManager();
	if( pDiskMgr == NULL || pDiskMgr->Init() == false )
	{
		RELEASE_OBJ( pDiskMgr );
		return false;
	}
	m_pDiskImageManager = new CDiskImageManager( pDiskMgr );

	if( m_core.Init( m_hWnd, m_pDraw, pDiskMgr, m_pKeyIF ) == false )
	{
		return false;
	}

	if( InitM88Sub() == false )
	{
		return false;
	}


	ApplyConfig();
//	m_pPpcConfig->OpenDiskImage( "SOR.D88" );

	//
	//	開始
	//
//	m_core.Activate( true );
//	m_pKeyIF->Activate( true );
//	m_core.Reset();
//	m_pDraw->Activate( true );

	m_core.Reset();
	Activate( true );
	m_core.Reset();

	return true;
}


/*---------------------------------------------------------------------------
		M88 の後始末
---------------------------------------------------------------------------*/

void WCEUI::CleanupM88()
{
	m_pKeyIF->Activate( false );
	m_core.Cleanup();
	m_pDraw->Cleanup();

	CleanupM88Sub();

	if( m_pDiskImageManager != NULL )
	{
		delete m_pDiskImageManager->GetDiskManager();
		delete m_pDiskImageManager;
		m_pDiskImageManager = NULL;
	}

	if( m_pDefFile != NULL )
	{
		m_pDefFile->SetValue( CM88DefFile::BASICMode, m_config.basicmode );
		m_pDefFile->SetValue( CM88DefFile::Switches, m_config.dipsw );
		m_pDefFile->SetValue( CM88DefFile::CPUClock, m_config.clock );
		m_pDefFile->SetValue( CM88DefFile::Sound, m_config.sound );
		m_pDefFile->SetValue( CM88DefFile::SoundBuffer, m_config.soundbuffer );
		m_pDefFile->SetValue( CM88DefFile::Flags, m_config.flags );
		m_pDefFile->SetValue( CM88DefFile::Flag2, m_config.flag2 );
		

		m_pDefFile->Save();
		delete m_pDefFile;
		m_pDefFile = NULL;
	}
}


/*---------------------------------------------------------------------------
		設定の反映
---------------------------------------------------------------------------*/

void WCEUI::ApplyConfig()
{
	m_core.ApplyConfig( &m_config );
}


/*---------------------------------------------------------------------------
		アクティブ設定
---------------------------------------------------------------------------*/

void WCEUI::Activate( bool blActive )
{
	if( blActive )
	{
		m_core.Activate( blActive );
		m_pDraw->Activate( blActive );
		m_pKeyIF->Activate( blActive );
	} else {
		m_pKeyIF->Activate( blActive );
		m_pDraw->Activate( blActive );
		m_core.Activate( blActive );
	}
}


/*---------------------------------------------------------------------------
		ウインドウの初期化
---------------------------------------------------------------------------*/

bool WCEUI::InitWindow( int nWinMode )
{
	if (m_pDefFile == NULL)
	{
		m_pDefFile = new CM88DefFile( M88CE_DEFFILENAME );
		if( m_pDefFile == NULL )
		{
			return false;
		}
		m_pDefFile->Load();
	}

	WNDCLASS wcl;

	wcl.hInstance = m_hInstance;
	wcl.lpszClassName = m_szClassName;
	wcl.lpfnWndProc = WndProcStatic;
	wcl.style = CS_HREDRAW | CS_VREDRAW;
	wcl.hIcon = LoadIcon(m_hInstance, MAKEINTRESOURCE(IDI_ICON_M88));
	wcl.hCursor = LoadCursor(NULL, IDC_ARROW);
	wcl.cbClsExtra = 0;
	wcl.cbWndExtra = 0;
	wcl.hbrBackground = (HBRUSH) GetStockObject(BLACK_BRUSH);
	wcl.lpszMenuName = NULL;

	InitWindowClass( &wcl );

	if( !::RegisterClass( &wcl ) )
	{
	TCHAR szTmp[16];

		::wsprintf( szTmp, _T("%08X"), ::GetLastError() );
		OutputDebugString( szTmp );
		OutputDebugString( _T(" - RegisterClass\n") );
		return false;
	}

	m_pThis = this;

	m_hWnd = CreateMainWindow();
	if( m_hWnd == NULL )
	{
		OutputDebugString( _T("CreateWindow\n") );
		return false;
	}

	::ShowWindow( m_hWnd, SW_SHOW );
	::UpdateWindow( m_hWnd );

	if ( m_pDraw->Init0( m_hWnd ) == NULL )
	{
		OutputDebugString( _T("Draw.Init0\n") );
		return false;
	}

	return true;
}


/*---------------------------------------------------------------------------
		ウインドウクラスのパラメータ設定
		追加で必要であればオーバーライド（メニュー等）
---------------------------------------------------------------------------*/

void WCEUI::InitWindowClass( WNDCLASS *pWcl )
{
}


/*---------------------------------------------------------------------------
		メイン
---------------------------------------------------------------------------*/

int WCEUI::Main()
{
MSG msg;

	if( InitM88() == false )
	{
		::MessageBox( NULL, _T("初期化に失敗しました。"), _T("M88"),
						MB_OK | MB_TOPMOST );
		return 1;
	}

	while( ::GetMessage( &msg, NULL, 0, 0 ) )
	{
		TranslateMessage( &msg );
		DispatchMessage( &msg );
	}

	CleanupM88();

	return msg.wParam;
}


/*---------------------------------------------------------------------------
		ウインドウ関数
---------------------------------------------------------------------------*/

LRESULT CALLBACK WCEUI::WndProcStatic( HWND hWnd, UINT uMsg,
										WPARAM wParam, LPARAM lParam )
{
	return m_pThis->WndProc( hWnd, uMsg, wParam, lParam );
}



