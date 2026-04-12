/*===========================================================================
		M88 for HandheldPC
		Original Version (c)cisc
		Handheld PC Version Programmed by Y.Taniuchi ( TAN-Y )

		HpcDraw クラス実装
===========================================================================*/

#include "m88h.h"


/*--------------------------------------------------------------------------
		コンストラクタ
--------------------------------------------------------------------------*/

HpcDraw::HpcDraw()
	: WCEDraw()
{
	m_pDst = NULL;
}


/*--------------------------------------------------------------------------
		デストラクタ
--------------------------------------------------------------------------*/

HpcDraw::~HpcDraw()
{
}



/*--------------------------------------------------------------------------
		クラスとしての初期化（サブクラス用）
--------------------------------------------------------------------------*/

bool HpcDraw::Init00( void )
{
RECT rect;

	m_iDrawX = 64;
	memset( m_binfo.colors, 0, sizeof(RGBQUAD) * 256 );
	if (::GetWindowRect(GetHWnd(), &rect))
	{
		m_iDrawX = rect.right / 2 - 160;
		if (m_iDrawX < 0)
		{
			m_iDrawX = 0;
		}
	}

	//
	//	とりあえず 16bit カラー限定
	//
	m_binfo.header.biSize          = sizeof(BITMAPINFOHEADER);
	m_binfo.header.biWidth         = 320;
	m_binfo.header.biHeight        = -240;
	m_binfo.header.biPlanes        = 1;
	m_binfo.header.biBitCount      = 16;
	m_binfo.header.biCompression   = BI_BITFIELDS;
	m_binfo.header.biSizeImage     = 0;
	m_binfo.header.biXPelsPerMeter = 0;
	m_binfo.header.biYPelsPerMeter = 0;
	m_binfo.header.biClrUsed       = 0;
	m_binfo.header.biClrImportant  = 0;

	*(DWORD *)(&m_binfo.colors[0]) = 0xf800;
	*(DWORD *)(&m_binfo.colors[1]) = 0x07e0;
	*(DWORD *)(&m_binfo.colors[2]) = 0x001f;

	HDC hDC = ::GetDC( GetHWnd() );

	m_hBitmap = CreateDIBSection( hDC,
									(BITMAPINFO*) &m_binfo,
									DIB_RGB_COLORS,
									(void**)(&m_pDst),
									NULL, 
									0 );

	::ReleaseDC( GetHWnd(), hDC );
	if( m_hBitmap == NULL )
	{
		return false;
	}

	return true;
}


/*--------------------------------------------------------------------------
		仮想VRAMバッファの確保（M88コアに渡される）
--------------------------------------------------------------------------*/

uint8 *HpcDraw::AllocVirtualDrawArea( uint width, uint height, uint bpp )
{
	return (uint8 *)malloc( width * height * bpp / 8 );
}


/*--------------------------------------------------------------------------
		後始末
--------------------------------------------------------------------------*/

void HpcDraw::CleanupSub( uint8 *pImageSrc )
{
	if( m_hBitmap != NULL )
	{
		::DeleteObject( m_hBitmap );
		m_hBitmap = NULL;
	}

	if( pImageSrc != NULL )
	{
		free( pImageSrc );
	}
}


/*--------------------------------------------------------------------------
		実際に描画する
--------------------------------------------------------------------------*/

void HpcDraw::DrawToScreen( uint8 *pImageSrc, RECT *pRect )
{
HDC hDC = ::GetDC( GetHWnd() );
HDC hMemDC = ::CreateCompatibleDC( hDC );
HBITMAP hOldBitmap = (HBITMAP)::SelectObject( hMemDC, m_hBitmap );
RECT realRect;

	realRect.top = pRect->top >> 1;
	realRect.bottom = pRect->bottom >> 1;
	realRect.left = pRect->left >> 1;
	realRect.right = pRect->right >> 1;


	uint8 *pDstTmpOrg = ((uint8 *)(m_pDst) + ( realRect.left >> 1 ));
	uint8 *pSrcTmpOrg = ((uint8 *)(pImageSrc) + realRect.left );

	for( LONG y = pRect->top; y <= pRect->bottom; y += 2 )
	{
		uint8 *pDstTmp = pDstTmpOrg + ( y >> 1 ) * 320 * 2;
		uint8 *pSrcTmp = pSrcTmpOrg + GetWidth() * y;
		for( LONG x = pRect->left; x <= pRect->right; x += 2 )
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

			wPixel = ((WORD)(dwRed & 0xf8) << 8)
						| ((WORD)(dwGreen & 0xfc) << 3
						| (WORD)(dwBlue) >> 3);

			*(WORD *)pDstTmp = wPixel;
			pDstTmp += 2;
		}
	}

	::BitBlt( hDC, m_iDrawX, realRect.top,
				320, realRect.bottom - realRect.top + 1,
				hMemDC, 0, realRect.top, SRCCOPY );


	::DeleteDC( hMemDC );
	::ReleaseDC( GetHWnd(), hDC );

}

