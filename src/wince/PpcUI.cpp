/*===========================================================================
		M88 for PocketPC
		Original Version (c)cisc
		Pocket PC Version Programmed by Y.Taniuchi ( TAN-Y )

		PpcUI クラス実装
===========================================================================*/

#include "m88p.h"

/*---------------------------------------------------------------------------
		コンストラクタ
---------------------------------------------------------------------------*/

PpcUI::PpcUI( HINSTANCE hInst )
	: WCEUI( hInst )
{
	m_pDraw = &m_draw;
	m_pKeyIF = &m_keyif;
	m_pKeyDef = NULL;
}


/*---------------------------------------------------------------------------
		デストラクタ
---------------------------------------------------------------------------*/

PpcUI::~PpcUI()
{
}


/*---------------------------------------------------------------------------
		M88 の初期化(サブクラス用)
---------------------------------------------------------------------------*/

bool PpcUI::InitM88Sub()
{

//	m_hMenu = ::LoadMenu( GetInstance(), MAKEINTRESOURCE( IDR_MENU_M88P ) );
//	if( m_hMenu == NULL )
//	{
//		return false;
//	}

	::GXOpenInput();
	m_gxkl = ::GXGetDefaultKeys( GX_NORMALKEYS );

	m_pKeyDef = new PpcKeyDefFile( M88P_KEYDEFFILENAME, &m_gxkl );
	m_pKeyDef->InitKey();
	m_pKeyDef->Load();

	return true;
}


/*---------------------------------------------------------------------------
		M88 の後始末(サブクラス用)
---------------------------------------------------------------------------*/

void PpcUI::CleanupM88Sub()
{
	if( m_pKeyDef != NULL )
	{
		m_pKeyDef->Save();
		delete m_pKeyDef;
		m_pKeyDef = NULL;
	}
	::GXCloseInput();
}


/*---------------------------------------------------------------------------
		ウインドウ生成
---------------------------------------------------------------------------*/

HWND PpcUI::CreateMainWindow( void )
{

//	DWORD wstyle = WS_CAPTION | WS_OVERLAPPED | WS_SYSMENU | WS_MINIMIZEBOX;
	DWORD wstyle = WS_VISIBLE | WS_POPUP;

	//hwnd = CreateWindowEx(
	HWND hWnd = CreateWindow(
								//WS_EX_ACCEPTFILES,
								GetWindowClassName(),
								GetWindowTitle(),
								wstyle,
								0,					// x
								0,					// y
								240,				// w
								320,				// h
								NULL,
								NULL,
								GetInstance(),
								NULL
								);

	return hWnd;
}


/*---------------------------------------------------------------------------
		ウインドウ関数
---------------------------------------------------------------------------*/

LRESULT CALLBACK PpcUI::WndProc( HWND hWnd, UINT uMsg,
										WPARAM wParam, LPARAM lParam )
{
static int iDownKeyCode = 0xff;


	switch( uMsg )
	{
		case WM_KILLFOCUS:
		{
			//	フォーカスを失ったら停止
//			Activate( false );
		}
		break;

		case WM_SETFOCUS:
		{
			//	フォーカスが戻ったら開始
//			Activate( true );
		}
		break;

//		case WM_COMMAND:
//		{
//			::SendMessage( hWnd, WM_CLOSE, 0, 0 );
//		}
//		break;

		case WM_KEYDOWN:
		{
			short vkKey = (short)wParam;

			if( lParam & ( 1 << 30 ) )
			{
				//	前から押されていたら処理しない
				return 0;
			}

			if( m_pKeyDef->GetKeyCode( vkKey ) >= 0 )
			{
				m_keyif.KeyDown( m_pKeyDef->GetKeyCode( vkKey ) );
				return 0;
			}
/*
			if( vkKey == m_gxkl.vkUp )
			{
				m_keyif.KeyDown( 0x57 );	//	tenkey 6
				return 0;
			}
			if( vkKey == m_gxkl.vkDown )
			{
				m_keyif.KeyDown( 0x55 );	//	tenkey 4
				return 0;
			}
			if( vkKey == m_gxkl.vkLeft )
			{
				m_keyif.KeyDown( 0x52 );	//	tenkey 8
				return 0;
			}
			if( vkKey == m_gxkl.vkRight )
			{
				m_keyif.KeyDown( 0x5a );	//	tenkey 2
				return 0;
			}
			if( vkKey == m_gxkl.vkStart )
			{
				m_keyif.KeyDown( 0x42 );	//	ret
				return 0;
			}
			if( vkKey == 0xc1 )
			{
				m_keyif.KeyDown( 0x42 );	//	ret
				return 0;
			}
			if( vkKey == VK_F23 )
			{
				m_keyif.KeyDown( 0x40 );	//	space
				return 0;
			}
			if( vkKey == m_gxkl.vkA )
			{
				m_keyif.KeyDown( 0x40 );	//	space
				return 0;
			}
			if( vkKey == m_gxkl.vkB )
			{
				m_keyif.KeyDown( 0x42 );	//	ret
				return 0;
			}
			if( vkKey == m_gxkl.vkC )
			{
				m_keyif.KeyDown( 0x40 );	//	space
				return 0;
			}
*/
		}
		break;

		case WM_KEYUP:
		{
			short vkKey = (short)wParam;

			if( m_pKeyDef->GetKeyCode( vkKey ) >= 0 )
			{
				m_keyif.KeyUp( m_pKeyDef->GetKeyCode( vkKey ) );
				return 0;
			}
/*
			if( vkKey == m_gxkl.vkUp )
			{
				m_keyif.KeyUp( 0x57 );	//	tenkey 6
				return 0;
			}
			if( vkKey == m_gxkl.vkDown )
			{
				m_keyif.KeyUp( 0x55 );	//	tenkey 4
				return 0;
			}
			if( vkKey == m_gxkl.vkLeft )
			{
				m_keyif.KeyUp( 0x52 );	//	tenkey 8
				return 0;
			}
			if( vkKey == m_gxkl.vkRight )
			{
				m_keyif.KeyUp( 0x5a );	//	tenkey 2
				return 0;
			}
			if( vkKey == m_gxkl.vkStart )
			{
				m_keyif.KeyUp( 0x42 );	//	ret
				return 0;
			}
			if( vkKey == 0xc1 )
			{
				m_keyif.KeyUp( 0x42 );	//	ret
				return 0;
			}
			if( vkKey == VK_F23 )
			{
				m_keyif.KeyUp( 0x40 );	//	space
				return 0;
			}
			if( vkKey == m_gxkl.vkA )
			{
				m_keyif.KeyUp( 0x40 );	//	space
				return 0;
			}
			if( vkKey == m_gxkl.vkB )
			{
				m_keyif.KeyUp( 0x42 );	//	ret
				return 0;
			}
			if( vkKey == m_gxkl.vkC )
			{
				m_keyif.KeyUp( 0x40 );	//	space
				return 0;
			}
*/
		}
		break;

		case WM_LBUTTONDOWN:
		{
		int iKeyCode = m_draw.GetKeyb()
							->GetKeytopCode( LOWORD(lParam), HIWORD(lParam) );

			iDownKeyCode = iKeyCode;
			if( iKeyCode < 0xff )
			{
				m_draw.GetKeyb()->KeyDown( iKeyCode );
				if( m_draw.GetKeyb()->GetPressedStatus( iKeyCode ) )
				{
					m_keyif.KeyDown( iKeyCode );
				} else {
					if( iKeyCode == 0x09 )
					{
						//
						//	ShiftLockが解除の場合はSHIFT/CTRL/GRPHは解除
						//
						m_keyif.KeyUp( 0x0d );
						m_keyif.KeyUp( 0x44 );
						m_keyif.KeyUp( 0x08 );
						m_keyif.KeyUp( 0x3f );
					}
				}
				m_draw.SetDrawKeybStatus( GAPIDraw::kbdraw );
			}
		}
		break;

		case WM_LBUTTONUP:
		{
		int iKeyCode = m_draw.GetKeyb()
							->GetKeytopCode( LOWORD(lParam), HIWORD(lParam) );

			if( iDownKeyCode == iKeyCode  )
			{
				//
				//	タッチしたときの同じキーの場合のみ有効
				//
				m_draw.GetKeyb()->KeyUp( iKeyCode );
				if( m_draw.GetKeyb()->GetPressedStatus( iKeyCode )
					== false )
				{
					m_keyif.KeyUp( iKeyCode );
				}
				m_draw.SetDrawKeybStatus( GAPIDraw::kbdraw );

				switch( iKeyCode )
				{
					case 0:
					{
					PpcConfigDialog config( GetM88Config(),
											GetDiskImageManager(),
											m_pKeyDef );

						Activate( false );
						m_draw.ActivateGAPI( false );

						::SHFullScreen( GetHWnd(),
									SHFS_SHOWTASKBAR | SHFS_SHOWSIPBUTTON );
						config.Show( GetHWnd() );

						m_draw.RequestPaint();
						m_draw.ActivateGAPI( true );
						Activate( true );
					}
					break;

					case 0x63:
					{
						Activate( false );
						if( ::MessageBox( hWnd, 
										_T("Reset?"), _T("M88/pocket"),
										MB_YESNO | MB_ICONQUESTION ) == IDYES )
						{
							ApplyConfig();
							GetCore()->Reset();
						}
						m_draw.RequestPaint();
						Activate( true );
					}
					break;

					case 0x64:
					{
						Activate( false );
						if( ::MessageBox( hWnd, 
										_T("Exit?"), _T("M88/pocket"),
										MB_YESNO | MB_ICONQUESTION ) == IDYES )
						{
							::SendMessage( hWnd, WM_CLOSE, 0, 0 );
						}
						m_draw.RequestPaint();
						Activate( true );
					}
					break;

				}

			} else {
				m_draw.GetKeyb()->KeyUp( iDownKeyCode );
				if( m_draw.GetKeyb()->GetPressedStatus( iDownKeyCode )
					== false )
				{
					m_keyif.KeyUp( iDownKeyCode );
				}
				m_draw.SetDrawKeybStatus( GAPIDraw::kbdraw );
			}
			iDownKeyCode = 0xff;

		}
		break;

		case WM_CLOSE:
		{
			Activate( false );
			m_draw.ActivateGAPI( false );
			::DestroyWindow( hWnd );
		}
		break;

		case WM_DESTROY:
		{
			::PostQuitMessage( 0 );
		}

		default:
		{
			return DefWindowProc( hWnd, uMsg, wParam, lParam );
		}
	}


	return 0;
}



