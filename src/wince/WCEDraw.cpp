/*===========================================================================
		M88 for PocketPC
		Original Version (c)cisc
		Pocket PC Version Programmed by Y.Taniuchi ( TAN-Y )

		CE 用 Draw クラス実装
===========================================================================*/

#include "m88ce.h"


/*--------------------------------------------------------------------------
		コンストラクタ
--------------------------------------------------------------------------*/

WCEDraw::WCEDraw()
{
	m_hWnd = NULL;
	m_hEventDraw = NULL;
	m_iBPL = 0;
	m_blLocked = false;
	m_uiStatus = 0;

	m_uiWidth = 0;
	m_uiHeight = 0;
	m_uiBPP = 0;

	m_pImageSrc = NULL;

	m_pThread = new tl4::CThread( this );
}


/*--------------------------------------------------------------------------
		デストラクタ
--------------------------------------------------------------------------*/

WCEDraw::~WCEDraw()
{
	Cleanup();

	delete m_pThread;
}


/*--------------------------------------------------------------------------
		クラスとしての初期化
--------------------------------------------------------------------------*/

bool WCEDraw::Init0( HWND hWnd )
{
HWND hWndBup = m_hWnd;

	m_hWnd = hWnd;
	if( Init00() == false )
	{
		m_hWnd = hWndBup;
		return false;
	}

	m_hEventDraw = ::CreateEvent( NULL, FALSE, FALSE, NULL );

	m_uiStatus |= Draw::shouldrefresh;

	m_blActive = false;
	m_blDrawing = false;
	m_blDrawAll = false;
	m_blThreadLoop = false;
	m_blPalChanged = false;

	if( m_pThread->Start() == FALSE )
	{
		return false;
	}

	return true;
}



/*--------------------------------------------------------------------------
		初期化
--------------------------------------------------------------------------*/

bool WCEDraw::Init(uint width, uint height, uint bpp)
{
//	Cleanup();

	m_pImageSrc = AllocVirtualDrawArea( width, height, bpp );

	if( m_pImageSrc == NULL )
	{
		return false;
	}

	m_uiWidth = width;
	m_uiHeight = height;
	m_uiBPP = bpp;


	return true;
}


/*--------------------------------------------------------------------------
		後始末
--------------------------------------------------------------------------*/

bool WCEDraw::Cleanup()
{
bool blRet = true;

	if( m_pThread->IsRunning() )
	{
		for( int i = 0; i < 30; i++ )
		{
			m_blThreadLoop = false;
			::SetEvent( m_hEventDraw );
			if( ::WaitForSingleObject( *m_pThread, 100 ) != WAIT_TIMEOUT )
			{
				break;
			}
		}
		if( m_pThread->IsRunning() )
		{
			m_pThread->Terminate( 0 );
		}

		::CloseHandle( m_hEventDraw );
		m_hEventDraw = NULL;
	}

	CleanupSub( m_pImageSrc );
	m_pImageSrc = NULL;

	return blRet;
}


/*--------------------------------------------------------------------------
		サーフェスのロック
--------------------------------------------------------------------------*/

bool WCEDraw::Lock(uint8** pimage, int* pbpl)
{
	if( m_pImageSrc == NULL )
	{
		return false;
	}

	if( m_blLocked )
	{
		return false;
	}

	m_csdraw.lock();
	m_blLocked = true;

	*pimage = m_pImageSrc;
	*pbpl = (int)m_uiWidth;


	return true;
}


/*--------------------------------------------------------------------------
		サーフェスのアンロック
--------------------------------------------------------------------------*/

bool WCEDraw::Unlock()
{
	if( m_blLocked == false )
	{
		return false;
	}

	m_csdraw.unlock();
	m_blLocked = false;

	m_uiStatus &= ~Draw::shouldrefresh;

	return true;
}


/*--------------------------------------------------------------------------
		ステータスの取得
--------------------------------------------------------------------------*/

uint WCEDraw::GetStatus()
{
	return m_uiStatus | (!m_blDrawing && m_blActive ? readytodraw : 0);
}


/*--------------------------------------------------------------------------
		リサイズ
--------------------------------------------------------------------------*/

void WCEDraw::Resize(uint width, uint height)
{
//	とりあえずなにもしないことにしておこう.....
//CriticalSection::Lock lock(m_csdraw);
//
//	Init( width, height, m_uiBPP );
}


/*--------------------------------------------------------------------------
		パレット設定
--------------------------------------------------------------------------*/

void WCEDraw::SetPalette(uint index, uint nents, const Palette* pal)
{
	memcpy( m_aPal + index, pal, nents * sizeof(Palette) );
	m_blPalChanged = true;
}


/*--------------------------------------------------------------------------
		フリップモード設定
--------------------------------------------------------------------------*/

bool WCEDraw::SetFlipMode(bool blFlipMode)
{
	return false;		//	Flip 不可
}


/*--------------------------------------------------------------------------
		全体再描画指示
--------------------------------------------------------------------------*/

void WCEDraw::RequestPaint()
{
	m_blDrawAll = true;
	if( m_blDrawing == false )
	{
		SetEvent(m_hEventDraw);
	}
}


/*--------------------------------------------------------------------------
		描画指示
--------------------------------------------------------------------------*/

void WCEDraw::DrawScreen(const Region& region)
{
	if( m_blDrawing == false )
	{
		m_blDrawing = true;
		m_rectDrawArea.left = 0;
		m_rectDrawArea.top = Max(0, region.top);
		m_rectDrawArea.right = m_uiWidth - 1;
		m_rectDrawArea.bottom = Min(m_uiHeight - 1, region.bottom);
		SetEvent(m_hEventDraw);
	}
}



/*--------------------------------------------------------------------------
		描画スレッドの開始／停止
--------------------------------------------------------------------------*/

void WCEDraw::Activate( bool blActive )
{
CriticalSection::Lock lock(m_csdraw);

	m_blActive = blActive;
}


/*--------------------------------------------------------------------------
		描画スレッド
--------------------------------------------------------------------------*/

DWORD WCEDraw::Run()
{
	m_blThreadLoop = true;

	while( m_blThreadLoop )
	{
		PaintWindow();
		::WaitForSingleObject( m_hEventDraw, 1000 );
	}


	return 0;
}

void WCEDraw::PaintWindow()
{
CriticalSection::Lock lock(m_csdraw);

	if( m_blDrawing && m_blActive )
	{
	RECT rect;

		if( m_blDrawAll )
		{
			m_blDrawAll = false;
			rect.left = 0;
			rect.top = 0;
			rect.right = m_uiWidth - 1;
			rect.bottom = m_uiHeight - 1;
		} else {
			rect = m_rectDrawArea;
			if( rect.top > rect.bottom )
			{
				//m_blDrawing = false;
				//return;
				rect.left = 0;
				rect.top = 0;
				rect.right = m_uiWidth - 1;
				rect.bottom = m_uiHeight - 1;
			}
		}


		//
		//	実際に描画する。
		//
		DrawToScreen( m_pImageSrc, &rect );

		m_blDrawing = false;
	}

}


