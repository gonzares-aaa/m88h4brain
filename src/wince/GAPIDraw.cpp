/*===========================================================================
		M88 for PocketPC
		Original Version (c)cisc
		Pocket PC Version Programmed by Y.Taniuchi ( TAN-Y )

		GAPIDraw クラス実装
===========================================================================*/

#include "m88p.h"


/*--------------------------------------------------------------------------
		コンストラクタ
--------------------------------------------------------------------------*/

GAPIDraw::GAPIDraw()
	: WCEDraw()
{
}


/*--------------------------------------------------------------------------
		デストラクタ
--------------------------------------------------------------------------*/

GAPIDraw::~GAPIDraw()
{
	if( GetHWnd() != NULL )
	{
		ActivateGAPI( false );
	}

	if( m_pKeyb != NULL )
	{
		delete m_pKeyb;
	}
}


/*--------------------------------------------------------------------------
		クラスとしての初期化（サブクラス用）
--------------------------------------------------------------------------*/

bool GAPIDraw::Init00( void )
{
	if( ActivateGAPI( true ) == false )
	{
		return false;
	}

	m_blActiveGAPI = false;
	m_lDstXXH = ( m_gxdp.cyHeight - 1 ) * m_gxdp.cbyPitch;

	if( m_gxdp.ffFormat | kfDirect565 ) {
		m_pKeyb = new CGAPISoftKeyb( 200, m_gxdp.cyHeight - 1,
										&m_gxdp, 0xffff, 0x8410 );
	} else {
		m_pKeyb = new CGAPISoftKeyb( 200, m_gxdp.cyHeight - 1,
										&m_gxdp, 0xffff, 0x4210 );
	}

	m_pKeyb->InitKeyTbl();
	SetDrawKeybStatus( kbredraw );

	return true;
}


/*--------------------------------------------------------------------------
		全体再描画指示
--------------------------------------------------------------------------*/

void GAPIDraw::RequestPaint()
{
	SetDrawKeybStatus( kbredraw );
	WCEDraw::RequestPaint();
}


/*--------------------------------------------------------------------------
		仮想VRAMバッファの確保（M88コアに渡される）
--------------------------------------------------------------------------*/

uint8 *GAPIDraw::AllocVirtualDrawArea( uint width, uint height, uint bpp )
{
	return (uint8 *)malloc( width * height * bpp / 8 );
}


/*--------------------------------------------------------------------------
		後始末
--------------------------------------------------------------------------*/

void GAPIDraw::CleanupSub( uint8 *pImageSrc )
{

	if( pImageSrc != NULL )
	{
		free( pImageSrc );
	}
}


/*--------------------------------------------------------------------------
		GAPI の有効／無効化
--------------------------------------------------------------------------*/

bool GAPIDraw::ActivateGAPI( bool blActive )
{
CriticalSection::Lock lock(m_csdraw);

	if( blActive )
	{
		if( ::GXOpenDisplay( GetHWnd(), GX_FULLSCREEN ) == 0 )
		{
			return false;
		}

		m_gxdp = ::GXGetDisplayProperties();

	} else {
		::GXCloseDisplay();
	}

	m_blActiveGAPI = blActive;

	return true;
}


/*--------------------------------------------------------------------------
		実際に描画する
--------------------------------------------------------------------------*/

void GAPIDraw::DrawToScreen( uint8 *pImageSrc, RECT *pRect )
{
void *pDst = ::GXBeginDraw();

	if( pDst != NULL )
	{
		if( m_gxdp.ffFormat | kfDirect565 )
		{
			DrawWindowH565( pDst, pImageSrc, pRect );	//16bit(555)横win
		} else
		if( m_gxdp.ffFormat | kfDirect555 )
		{
			DrawWindowH555( pDst, pImageSrc, pRect );	//16bit(565)横win
		}

		//
		//	キーボード全体再描画
		//
		switch( m_dks )
		{
			case kbdraw:
				m_pKeyb->Draw( (LPBYTE)pDst );
			break;
			case kbredraw:
				m_pKeyb->Redraw( (LPBYTE)pDst );
			break;
		}
		SetDrawKeybStatus( kbnormal );
		

		::GXEndDraw();
	}
}


/*--------------------------------------------------------------------------
		実描画ルーチン
--------------------------------------------------------------------------*/

//
//	16bpp(555) / 横画面
//
void GAPIDraw::DrawWindowH555( void *pDst, const uint8 *pSrc, RECT *pRect )
{
uint8 *pDstTmp, *pDstTmpOrg;
const uint8 *pSrcTmp;


	pSrcTmp = pSrc + pRect->left + pRect->top * GetWidth() * 2;
	pDstTmpOrg = ((uint8 *)(pDst) + m_lDstXXH );

	pRect->top >>= 1;
	pRect->bottom >>= 1;
	pRect->left >>= 1;
	pRect->right >>= 1;

	for( LONG y = pRect->top; y <= pRect->bottom; y++ )
	{
		pDstTmp = pDstTmpOrg + y * m_gxdp.cbxPitch;
		pSrcTmp = pSrc + GetWidth() * y * 2;
		for( LONG x = pRect->left; x <= pRect->right; x++ )
		{
		DWORD dwRed;
		DWORD dwGreen;
		DWORD dwBlue;
		WORD wPixel;

			dwRed = GetPalette()[*pSrcTmp].red;
			dwGreen = GetPalette()[*pSrcTmp].green;
			dwBlue = GetPalette()[*pSrcTmp].blue;
			pSrcTmp++;
			dwRed += GetPalette()[*pSrcTmp].red;
			dwGreen += GetPalette()[*pSrcTmp].green;
			dwBlue += GetPalette()[*pSrcTmp].blue;
			pSrcTmp++;
			dwRed >>= 1;
			dwGreen >>= 1;
			dwBlue >>= 1;

//			wPixel = ((WORD)(dwRed & 0x1f) << 10)
//						| ((WORD)(dwGreen & 0x1f) << 5
//						| (WORD)(dwBlue & 0x1f));
			wPixel = ((WORD)(dwRed & 0xf8) << 7)
						| ((WORD)(dwGreen & 0xf8) << 2
						| (WORD)(dwBlue) >> 3);

			*(WORD *)pDstTmp = wPixel;
			pDstTmp -= m_gxdp.cbyPitch;
		}
		//pSrcTmp += GetWidth();
		//pSrcTmp += 2;
	}
}


//
//	16bpp(565) / 横画面
//
void GAPIDraw::DrawWindowH565( void *pDst, const uint8 *pSrc, RECT *pRect )
{
uint8 *pDstTmp, *pDstTmpOrg;
const uint8 *pSrcTmp;


	pSrcTmp = pSrc + pRect->left + pRect->top * GetWidth() * 2;
	pDstTmpOrg = ((uint8 *)(pDst) + m_lDstXXH );

	pRect->top >>= 1;
	pRect->bottom >>= 1;
	pRect->left >>= 1;
	pRect->right >>= 1;

	for( LONG y = pRect->top; y <= pRect->bottom; y++ )
	{
		pDstTmp = pDstTmpOrg + y * m_gxdp.cbxPitch;
		pSrcTmp = pSrc + GetWidth() * y * 2;
		for( LONG x = pRect->left; x <= pRect->right; x++ )
		{
		DWORD dwRed;
		DWORD dwGreen;
		DWORD dwBlue;
		WORD wPixel;

			dwRed = GetPalette()[*pSrcTmp].red;
			dwGreen = GetPalette()[*pSrcTmp].green;
			dwBlue = GetPalette()[*pSrcTmp].blue;
			pSrcTmp++;
			dwRed += GetPalette()[*pSrcTmp].red;
			dwGreen += GetPalette()[*pSrcTmp].green;
			dwBlue += GetPalette()[*pSrcTmp].blue;
			pSrcTmp++;
			dwRed >>= 1;
			dwGreen >>= 1;
			dwBlue >>= 1;

//			wPixel = ((WORD)(dwRed & 0x1f) << 11)
//						| ((WORD)(dwGreen & 0x3f) << 5
//						| (WORD)(dwBlue & 0x1f));

			wPixel = ((WORD)(dwRed & 0xf8) << 8)
						| ((WORD)(dwGreen & 0xfc) << 3
						| (WORD)(dwBlue) >> 3);

			*(WORD *)pDstTmp = wPixel;
			pDstTmp -= m_gxdp.cbyPitch;
		}
		//pSrcTmp += GetWidth();
		//pSrcTmp += 2;
	}


/*	//	テスト用に書いた暫定縦表示ルーチン
	for( LONG y = pRect->top; y < pRect->bottom; y += 2 )
	{
		pDstTmp = ((uint8 *)(pDst) + ( y >> 1 ) * m_gxdp.cbyPitch);
		pSrcTmp = pSrc + 640 * y;
		for( LONG x = pRect->left; x < pRect->right; x += 2 )
		{
		DWORD dwRed;
		DWORD dwGreen;
		DWORD dwBlue;
		WORD wPixel;

			dwRed = GetPalette()[*pSrcTmp].red;
			dwGreen = GetPalette()[*pSrcTmp].green;
			dwBlue = GetPalette()[*pSrcTmp].blue;
			pSrcTmp++;
			dwRed += GetPalette()[*pSrcTmp].red;
			dwGreen += GetPalette()[*pSrcTmp].green;
			dwBlue += GetPalette()[*pSrcTmp].blue;
			pSrcTmp++;
			dwRed >>= 1;
			dwGreen >>= 1;
			dwBlue >>= 1;

			wPixel = ((WORD)(dwRed & 0x1f) << 11)
						| ((WORD)(dwGreen & 0x3f) << 5
						| (WORD)(dwBlue & 0x1f));
			*(WORD *)pDstTmp = wPixel;
			pDstTmp += 2;
		}
	}
*/
}


