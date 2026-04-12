/*===========================================================================
		GAPI 用ソフトウエアキーボードクラス

		Copyright & Programmed by Y.Taniuchi ( TAN-Y )
===========================================================================*/

#ifndef _INC_TANY_SOFTKEYBOARD
#define _INC_TANY_SOFTKEYBOARD

struct KeyInfo
{
	long m_lPosX;		//	キー表示位置 X
	long m_lPosY;		//	キー表示位置 Y
	long m_lViewIndex;	//	上二つのパラメータを元に事前にアドレス計算して格納

	LPCSTR szKeyTopNorm;		//	キートップ（通常）
	LPCSTR szKeyTopNormShift;	//	キートップ（通常シフト）
	LPCSTR szKeyTopKana;		//	キートップ（カナ）
	LPCSTR szKeyTopKanaShift;	//	キートップ（カナシフト）
};


#define KEYSTATUS_BUFSIZE 128

class CGAPISoftKeyb
{
private:

	GXDisplayProperties m_gxdp;
	long m_lKeybXPos;		//	キーボード表示位置X座標（実際のポケピ上の座標）
	long m_lKeybYPos;		//	キーボード表示位置Y座標（実際のポケピ上の座標）
	long m_lViewIndex;		//	キーボード表示位置へのインデックス
	long m_lXIndex;			//	一文字分の幅のインデックス値
	WORD m_wPixel;			//	使用するピクセルデータ
	WORD m_wPixelRightBar;	//	キー右端境界色

	bool m_blLockedCaps;
	bool m_blLockedKana;
	bool m_blLockedShiftLock;
	bool m_blPressedShift;

	void PutFont( LPBYTE pDst, unsigned char ucCode, bool blPressed,
					bool blDrawRightBar );
	void PutKeytop( LPBYTE pDst, int iKeytopCode );

	bool m_pblPressedKeys[KEYSTATUS_BUFSIZE];
	bool m_pblDrawKeys[KEYSTATUS_BUFSIZE];

public:
	CGAPISoftKeyb( long lXPos, long lYPos, const GXDisplayProperties *pGxdp,
					WORD wPixel, WORD wPixelRightBar );

	int GetKeytopCode( WORD wXPos, WORD wYPos );

	void Redraw( LPBYTE pDst );
	void Draw( LPBYTE pDst );

	void InitKeyTbl( void );

	void KeyUp( int iKeyCode );
	void KeyDown( int iKeyCode );

	bool GetPressedStatus( int iIndex )
	{
		return m_pblPressedKeys[iIndex];
	}
};



#endif
