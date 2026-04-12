/*===========================================================================
		M88 for HandheldPC
		Original Version (c)cisc
		Handheld PC Version Programmed by Y.Taniuchi ( TAN-Y )

		HpcVGADraw クラス実装
===========================================================================*/

#include "m88sig3.h"




/*--------------------------------------------------------------------------
		クラスとしての初期化（サブクラス用）
--------------------------------------------------------------------------*/

bool HpcVGADraw::Init00( void )
{
RECT rect;

	m_iDrawX = 64;
	memset( m_binfo.colors, 0, sizeof(RGBQUAD) * 256 );
	if (::GetWindowRect(GetHWnd(), &rect))
	{
		m_iDrawX = rect.right / 2 - 320;
		if (m_iDrawX < 0)
		{
			m_iDrawX = 0;
		}
	}

	//
	//	とりあえず 16bit カラー限定
	//
	m_binfo.header.biSize          = sizeof(BITMAPINFOHEADER);
	m_binfo.header.biWidth         = 640;
	m_binfo.header.biHeight        = -480;
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
		実際に描画する
--------------------------------------------------------------------------*/

void HpcVGADraw::DrawToScreen( uint8 *pImageSrc, RECT *pRect )
{
HDC hDC = ::GetDC( GetHWnd() );
HDC hMemDC = ::CreateCompatibleDC( hDC );
HBITMAP hOldBitmap = (HBITMAP)::SelectObject( hMemDC, m_hBitmap );

	uint8 *pDstTmpOrg = ((uint8 *)(m_pDst) + pRect->left);
	uint8 *pSrcTmpOrg = ((uint8 *)(pImageSrc) + pRect->left );

	//	暫定で1ラインスキップ
	for( LONG y = (pRect->top & ~1); y <= (pRect->bottom & ~1); y+=2 )
	{
		uint8 *pDstTmp = pDstTmpOrg + y * 640 * 2;
		uint8 *pSrcTmp = pSrcTmpOrg + GetWidth() * y;
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

			wPixel = ((WORD)(dwRed & 0xf8) << 8)
						| ((WORD)(dwGreen & 0xfc) << 3
						| (WORD)(dwBlue) >> 3);

			*(WORD *)pDstTmp = wPixel;
			pDstTmp += 2;
		}
	}

	::BitBlt( hDC, m_iDrawX, pRect->top,
				640, pRect->bottom - pRect->top + 1,
				hMemDC, 0, pRect->top, SRCCOPY );


	::DeleteDC( hMemDC );
	::ReleaseDC( GetHWnd(), hDC );

}


/*--------------------------------------------------------------------------
		実際に描画する(Full)
--------------------------------------------------------------------------*/

void HpcVGAFullDraw::DrawToScreen( uint8 *pImageSrc, RECT *pRect )
{
HDC hDC = ::GetDC( GetHWnd() );
HDC hMemDC = ::CreateCompatibleDC( hDC );
HBITMAP hOldBitmap = (HBITMAP)::SelectObject( hMemDC, m_hBitmap );

	uint8 *pDstTmpOrg = ((uint8 *)(m_pDst) + pRect->left);
	uint8 *pSrcTmpOrg = ((uint8 *)(pImageSrc) + pRect->left );

	for( LONG y = pRect->top; y <= pRect->bottom; y++ )
	{
		uint8 *pDstTmp = pDstTmpOrg + y * 640 * 2;
		uint8 *pSrcTmp = pSrcTmpOrg + GetWidth() * y;
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

			wPixel = ((WORD)(dwRed & 0xf8) << 8)
						| ((WORD)(dwGreen & 0xfc) << 3
						| (WORD)(dwBlue) >> 3);

			*(WORD *)pDstTmp = wPixel;
			pDstTmp += 2;
		}
	}

	::BitBlt( hDC, m_iDrawX, pRect->top,
				640, pRect->bottom - pRect->top + 1,
				hMemDC, 0, pRect->top, SRCCOPY );


	::DeleteDC( hMemDC );
	::ReleaseDC( GetHWnd(), hDC );

}

