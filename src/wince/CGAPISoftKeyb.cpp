/*===========================================================================
		ソフトウエアキーボードクラス

		Copyright & Programmed by Y.Taniuchi ( TAN-Y )
===========================================================================*/

#include <windows.h>
#include <stdio.h>
#include <gx.h>
#include "CGAPISoftKeyb.h"

extern unsigned char g_pbFont8x8[];
extern KeyInfo g_pKeyInfo[];

static unsigned char g_pKeyMap[80 * 5];

/*---------------------------------------------------------------------------
		コンストラクタ
---------------------------------------------------------------------------*/

CGAPISoftKeyb::CGAPISoftKeyb( long lXPos, long lYPos, 
								const GXDisplayProperties *pGxdp,
								WORD wPixel, WORD wPixelRightBar )
{
	memcpy( &m_gxdp, pGxdp, sizeof(GXDisplayProperties) );


	m_lKeybXPos = lXPos;
	m_lKeybYPos = lYPos;
	m_lViewIndex = lXPos * m_gxdp.cbxPitch + lYPos * m_gxdp.cbyPitch;
	m_lXIndex = m_gxdp.cbyPitch * 8;
	m_wPixel = wPixel;
	m_wPixelRightBar = wPixelRightBar;

	m_blLockedCaps = false;
	m_blLockedKana = false;
	m_blLockedShiftLock = false;
	m_blPressedShift = false;

	memset( m_pblPressedKeys, false, sizeof(bool) * KEYSTATUS_BUFSIZE );
	memset( m_pblDrawKeys, false, sizeof(bool) * KEYSTATUS_BUFSIZE );
}


/*---------------------------------------------------------------------------
		テーブルの初期化
		テーブル自体はグローバルに置いているけど、このメソッドを実行した
		インスタンスの設定に基づいて初期化するってことで....
---------------------------------------------------------------------------*/

void CGAPISoftKeyb::InitKeyTbl( void )
{
KeyInfo *pKeyInfoTmp = g_pKeyInfo;
int i;
unsigned char ucKeyIndex;

	for( i = 0; i < 80 * 5; i++ )
	{
		g_pKeyMap[i] = 0xff;
	}

	ucKeyIndex = 0;
	while( pKeyInfoTmp->m_lPosX >= 0 )
	{
	int iLen = strlen( pKeyInfoTmp->szKeyTopNorm );
	int iMapIndex = pKeyInfoTmp->m_lPosX + pKeyInfoTmp->m_lPosY * 80;

		for( i = 0; i < (iLen * 2); i++ )
		{
			g_pKeyMap[i + iMapIndex] = ucKeyIndex;
		}

		pKeyInfoTmp->m_lViewIndex = m_lViewIndex +
								pKeyInfoTmp->m_lPosY * m_gxdp.cbxPitch * 8 -
								pKeyInfoTmp->m_lPosX * m_gxdp.cbyPitch * 4;

		pKeyInfoTmp++;
		ucKeyIndex++;
	}

}


/*---------------------------------------------------------------------------
		キー上げ
---------------------------------------------------------------------------*/

void CGAPISoftKeyb::KeyUp( int iKeyCode )
{
	switch( iKeyCode )
	{
		case 0x08:
		case 0x3f:
		{
			//	CTRL,GRPH は Shift Lock 無効時のみ
			if( m_blLockedShiftLock == false )
			{
				m_pblPressedKeys[iKeyCode] = false;
				m_pblDrawKeys[iKeyCode] = true;
			}
		}
		break;
		case 0x09:
		case 0x0c:
		case 0x0e:
		{
			//	Shift Lock,CAPS,カナはキー上げ時なにもしない
		}
		break;
		case 0x0d:
		case 0x44:
		{
			if( m_blLockedShiftLock )
			{
				//	シフトロック有効の場合はCAPSみたいな挙動をする
				m_blPressedShift = ( m_pblPressedKeys[0x0d] |
										m_pblPressedKeys[0x44] );
				for( int i = 0x0f; i < 0x3f; i++ )
				{
					m_pblDrawKeys[i] = true;
				}
			} else {
				m_pblPressedKeys[iKeyCode] = false;
				m_blPressedShift = false;
				m_pblDrawKeys[iKeyCode] = true;
			}
		}
		break;
		default:
		{
			m_pblPressedKeys[iKeyCode] = false;
			m_pblDrawKeys[iKeyCode] = true;
		}
		break;
	}

}


/*---------------------------------------------------------------------------
		キー下げ
---------------------------------------------------------------------------*/

void CGAPISoftKeyb::KeyDown( int iKeyCode )
{
	switch( iKeyCode )
	{
		case 0x08:
		case 0x3f:
		{
			//	CTRL,GRPH は Shift Lock 有効時に CAPS と同じような挙動
			if( m_blLockedShiftLock )
			{
				m_pblPressedKeys[iKeyCode] =
					(m_pblPressedKeys[iKeyCode] ? false : true);
			} else {
				m_pblPressedKeys[iKeyCode] = true;
			}
			m_pblDrawKeys[iKeyCode] = true;

		}
		break;
		case 0x09:
		case 0x0c:
		case 0x0e:
		{
			//	Shift Lock,CAPS,カナは反転表示
			m_pblPressedKeys[iKeyCode] =
					(m_pblPressedKeys[iKeyCode] ? false : true);
			m_pblDrawKeys[iKeyCode] = true;

			//	描画判定用ステータス変更
			switch( iKeyCode )
			{
				case 0x09:
				{
					m_blLockedShiftLock =
						( m_blLockedShiftLock ? false : true);
					if( m_blLockedShiftLock == false )
					{
						m_blPressedShift = false;
						m_pblPressedKeys[0x0d] = false;
						m_pblDrawKeys[0x0d] = true;
						m_pblPressedKeys[0x44] = false;
						m_pblDrawKeys[0x44] = true;
						m_pblPressedKeys[0x08] = false;
						m_pblDrawKeys[0x08] = true;
						m_pblPressedKeys[0x3f] = false;
						m_pblDrawKeys[0x3f] = true;
					}
				}
				break;
				case 0x0c:
				{
					m_blLockedCaps = ( m_blLockedCaps ? false : true);
				}
				break;
				case 0x0e:
				{
					m_blLockedKana = ( m_blLockedKana ? false : true);
				}
				break;
			}

			//	一般キー系は再描画対象
			for( int i = 0x0f; i < 0x3f; i++ )
			{
				m_pblDrawKeys[i] = true;
			}
		}
		break;
		case 0x0d:
		case 0x44:
		{
			if( m_blLockedShiftLock )
			{
				//	シフトロック有効の場合はCAPSみたいな挙動をする
				m_pblPressedKeys[iKeyCode] =
						(m_pblPressedKeys[iKeyCode] ? false : true);
				m_pblDrawKeys[iKeyCode] = true;
			} else {
				m_pblPressedKeys[iKeyCode] = true;
				m_blPressedShift = true;
			}
			m_pblDrawKeys[iKeyCode] = true;
		}
		break;
		default:
		{
			m_pblPressedKeys[iKeyCode] = true;
			m_pblDrawKeys[iKeyCode] = true;
		}
		break;
	}
}


/*---------------------------------------------------------------------------
		キーボード全体再描画
---------------------------------------------------------------------------*/

void CGAPISoftKeyb::Redraw( LPBYTE pDst )
{
	memset( m_pblDrawKeys, true, sizeof(bool) * KEYSTATUS_BUFSIZE );
	Draw( pDst );
}


/*---------------------------------------------------------------------------
		キーボード描画
---------------------------------------------------------------------------*/

void CGAPISoftKeyb::Draw( LPBYTE pDst )
{
KeyInfo *pKeyInfoTmp = g_pKeyInfo;
bool *pblDrawKey = m_pblDrawKeys;
int i = 0;
LPCSTR *pszKeyTopNorms = &(g_pKeyInfo->szKeyTopNorm);
LPCSTR *pszKeyTopExts = pszKeyTopNorms;

	//
	//	カナ／シフトロック状態を確認
	//
	if( m_blLockedKana )
	{
		if( m_blLockedShiftLock && m_blPressedShift )
		{
			pszKeyTopExts = &g_pKeyInfo->szKeyTopKanaShift;
		} else {
			pszKeyTopExts = &g_pKeyInfo->szKeyTopKana;
		}
	} else {
		if( m_blLockedShiftLock && m_blPressedShift )
		{
			pszKeyTopExts = &g_pKeyInfo->szKeyTopNormShift;
		}
	}

	//
	//	描画
	//
	while( pKeyInfoTmp->m_lPosX >= 0 )
	{
		if( *pblDrawKey )
		{
			//
			//	描画フラグが立ってたら
			//

			//PutKeytop( pDst, i );
		LPCSTR szStr = *pszKeyTopExts;

			if( szStr == NULL )
			{
				szStr = *pszKeyTopNorms;
			}

			LPBYTE pDstTmp = pDst + pKeyInfoTmp->m_lViewIndex;
			while( *(szStr + 1) != NULL )
			{
				PutFont( pDstTmp, *szStr, m_pblPressedKeys[i], false );
				pDstTmp -= m_lXIndex;
				szStr++;
			}
			PutFont( pDstTmp, *szStr, m_pblPressedKeys[i], true );

		}
		*pblDrawKey = false;

		i++;
		pblDrawKey++;
		pKeyInfoTmp++;

		pszKeyTopNorms += sizeof(KeyInfo) / sizeof(LPCSTR *);
		pszKeyTopExts += sizeof(KeyInfo) / sizeof(LPCSTR *);
	}

}


/*---------------------------------------------------------------------------
		座標からキーコードを取得
---------------------------------------------------------------------------*/

int CGAPISoftKeyb::GetKeytopCode( WORD wXPos, WORD wYPos )
{
long lVXPos, lVYPos;		//	V は Virtual の V ....

	lVXPos = (m_lKeybYPos - wYPos) / 4;	//	/ 4
	lVYPos = (wXPos - m_lKeybXPos) / 8;	//	/ 8

	int iIndex = lVXPos + lVYPos * 80;
	if( iIndex < 0 || iIndex > ( 80 * 5 ) - 1 )
	{
		return 0xff;
	}

	return g_pKeyMap[iIndex];
}


/*---------------------------------------------------------------------------
		キーを表示する（左向き回転が前提）
---------------------------------------------------------------------------*/

void CGAPISoftKeyb::PutKeytop( LPBYTE pDst, int iKeytopCode )
{
KeyInfo *pKeyInfoTmp = g_pKeyInfo + iKeytopCode;
LPCSTR szStr = pKeyInfoTmp->szKeyTopNorm;

	if( *szStr == NULL )
	{
		return;
	}
	pDst += pKeyInfoTmp->m_lViewIndex;
	while( *(szStr + 1) != NULL )
	{
		PutFont( pDst, *szStr, m_pblPressedKeys[iKeytopCode], false );
		pDst -= m_lXIndex;
		szStr++;
	}
	PutFont( pDst, *szStr, m_pblPressedKeys[iKeytopCode], true );

}


/*---------------------------------------------------------------------------
		フォントを表示する（左向き回転が前提）
---------------------------------------------------------------------------*/

void CGAPISoftKeyb::PutFont( LPBYTE pDst, unsigned char ucCode,
								bool blPressed, bool blDrawRightBar )
{
const unsigned char *pbUseFont;
LPBYTE pDstTmp;

	if( ucCode >= 0x80 )
	{
		ucCode -= 0x40;
	} else {
		ucCode -= 0x20;
	}

	pbUseFont = g_pbFont8x8 + (DWORD)(ucCode) * 8;

	if( blDrawRightBar )
	{
	WORD wRightBar = m_wPixelRightBar;

		if( blPressed == false )
		{
			wRightBar = ~wRightBar;
		}

		for( long y = 0; y < 8; y++ )
		{
			pDstTmp = pDst + y * m_gxdp.cbxPitch;
			BYTE bData = *pbUseFont;
			if( blPressed == false )
			{
				bData = ~bData;
			}
			for( long x = 0; x < 7; x++ )
			{
				if( bData & 0x80 )
				{
					*(WORD *)pDstTmp = m_wPixel;
				} else {
					*(WORD *)pDstTmp = 0;
				}
				bData <<= 1;
				pDstTmp -= m_gxdp.cbyPitch;
			}
			*(WORD *)pDstTmp = wRightBar;
			pbUseFont++;
		}
	} else {
		for( long y = 0; y < 8; y++ )
		{
			pDstTmp = pDst + y * m_gxdp.cbxPitch;
			BYTE bData = *pbUseFont;
			if( blPressed == false )
			{
				bData = ~bData;
			}
			for( long x = 0; x < 8; x++ )
			{
				if( bData & 0x80 )
				{
					*(WORD *)pDstTmp = m_wPixel;
				} else {
					*(WORD *)pDstTmp = 0;
				}
				bData <<= 1;
				pDstTmp -= m_gxdp.cbyPitch;
			}
			pbUseFont++;
		}
	}

}


