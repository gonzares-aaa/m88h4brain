/*===========================================================================
		M88 for HandheldPC
		Original Version (c)cisc
		Handheld PC Version Programmed by Y.Taniuchi ( TAN-Y )

		HpcKeyIF クラス実装
===========================================================================*/

#include "m88h.h"
#include "romaji.h"

using namespace PC8801;

#define BIT0 0, 1
#define BIT1 1, 2
#define BIT2 2, 4
#define BIT3 3, 8
#define BIT4 4, 16
#define BIT5 5, 32
#define BIT6 6, 64
#define BIT7 7, 128
#define ENDP { 0x100, 0, 0 }


//
//	デフォルトはカーソルキーはそのまま
//
bool HpcKeyIF::m_blUseArrowFor10 = false;


/*---------------------------------------------------------------------------
		コンストラクタ
---------------------------------------------------------------------------*/

HpcKeyIF::HpcKeyIF()
	: WCEKeyIF()
{
	m_dwLockKeyFlag = 0;
	m_bUseRomaji = true;
	m_bModifierMode = false;

	m_mapA = new KEYPUSHINFO[256][4];
	m_mapB = new KEYPUSHINFO[256][4];
	m_mapC = new KEYPUSHINFO[256][4];

	// サイズ計算用
	int mapSize = 256 * 4 * sizeof(KEYPUSHINFO);

	
	// ==============================================================
	// マップキャッシュの初期化 (起動時に1回だけ生成)
	// ==============================================================
	
	// 1. マップA (通常マップの保存)
	// ※ sizeof(m_mapA) だとポインタのサイズ(4バイト)になってしまうため mapSize を使います
	memcpy(m_mapA, m_aaKeyPushInfo, mapSize);

	// 2. マップB (マップAをベースに、メインキーの数字・記号を作成)
	memcpy(m_mapB, m_mapA, mapSize);
	// E -> 3
	m_mapB[0x45][0].uiPort = 0x06; m_mapB[0x45][0].uiSwitchBit = 3; m_mapB[0x45][0].uiSwitchBitPattern = 8;
	// I -> 8
	m_mapB[0x49][0].uiPort = 0x07; m_mapB[0x49][0].uiSwitchBit = 0; m_mapB[0x49][0].uiSwitchBitPattern = 1;
	// O -> 9
	m_mapB[0x4f][0].uiPort = 0x07; m_mapB[0x4f][0].uiSwitchBit = 1; m_mapB[0x4f][0].uiSwitchBitPattern = 2;
	// P -> 0
	m_mapB[0x50][0].uiPort = 0x06; m_mapB[0x50][0].uiSwitchBit = 0; m_mapB[0x50][0].uiSwitchBitPattern = 1;
	// Q -> 1
	m_mapB[0x51][0].uiPort = 0x06; m_mapB[0x51][0].uiSwitchBit = 1; m_mapB[0x51][0].uiSwitchBitPattern = 2;
	// R -> 4
	m_mapB[0x52][0].uiPort = 0x06; m_mapB[0x52][0].uiSwitchBit = 4; m_mapB[0x52][0].uiSwitchBitPattern = 16;
	// T -> 5
	m_mapB[0x54][0].uiPort = 0x06; m_mapB[0x54][0].uiSwitchBit = 5; m_mapB[0x54][0].uiSwitchBitPattern = 32;
	// U -> 7
	m_mapB[0x55][0].uiPort = 0x06; m_mapB[0x55][0].uiSwitchBit = 7; m_mapB[0x55][0].uiSwitchBitPattern = 128;
	// W -> 2
	m_mapB[0x57][0].uiPort = 0x06; m_mapB[0x57][0].uiSwitchBit = 2; m_mapB[0x57][0].uiSwitchBitPattern = 4;
	// Y -> 6
	m_mapB[0x59][0].uiPort = 0x06; m_mapB[0x59][0].uiSwitchBit = 6; m_mapB[0x59][0].uiSwitchBitPattern = 64;

	// D -> ^
	m_mapB[0x44][0].uiPort = 0x05; m_mapB[0x44][0].uiSwitchBit = 6; m_mapB[0x44][0].uiSwitchBitPattern = 64;
	// F -> '\'
	m_mapB[0x46][0].uiPort = 0x05; m_mapB[0x46][0].uiSwitchBit = 4; m_mapB[0x46][0].uiSwitchBitPattern = 16;
	// G -> @
	m_mapB[0x47][0].uiPort = 0x02; m_mapB[0x47][0].uiSwitchBit = 0; m_mapB[0x47][0].uiSwitchBitPattern = 1;
	// H -> [
	m_mapB[0x48][0].uiPort = 0x05; m_mapB[0x48][0].uiSwitchBit = 3; m_mapB[0x48][0].uiSwitchBitPattern = 8;
	// J -> ;
	m_mapB[0x4a][0].uiPort = 0x07; m_mapB[0x4a][0].uiSwitchBit = 3; m_mapB[0x4a][0].uiSwitchBitPattern = 8;
	// K -> :
	m_mapB[0x4b][0].uiPort = 0x07; m_mapB[0x4b][0].uiSwitchBit = 2; m_mapB[0x4b][0].uiSwitchBitPattern = 4;
	// L -> ]
	m_mapB[0x4c][0].uiPort = 0x05; m_mapB[0x4c][0].uiSwitchBit = 5; m_mapB[0x4c][0].uiSwitchBitPattern = 32;

	// V -> ,
	m_mapB[0x56][0].uiPort = 0x07; m_mapB[0x56][0].uiSwitchBit = 4; m_mapB[0x56][0].uiSwitchBitPattern = 16;
	// B -> .
	m_mapB[0x42][0].uiPort = 0x07; m_mapB[0x42][0].uiSwitchBit = 5; m_mapB[0x42][0].uiSwitchBitPattern = 32;
	// N -> /
	m_mapB[0x4e][0].uiPort = 0x07; m_mapB[0x4e][0].uiSwitchBit = 6; m_mapB[0x4e][0].uiSwitchBitPattern = 64;
	// M -> _
	m_mapB[0x4d][0].uiPort = 0x07; m_mapB[0x4d][0].uiSwitchBit = 7; m_mapB[0x4d][0].uiSwitchBitPattern = 128;

	// 3. マップC (マップBをベースに、QWERTYUIOPの段だけをテンキー化)
	memcpy(m_mapC, m_mapB, mapSize);
	// Q -> テンキー 1 (Port0, Bit1)
	m_mapC[0x51][0].uiPort = 0x00; m_mapC[0x51][0].uiSwitchBit = 1; m_mapC[0x51][0].uiSwitchBitPattern = 2;
	// W -> テンキー 2 (Port0, Bit2)
	m_mapC[0x57][0].uiPort = 0x00; m_mapC[0x57][0].uiSwitchBit = 2; m_mapC[0x57][0].uiSwitchBitPattern = 4;
	// E -> テンキー 3 (Port0, Bit3)
	m_mapC[0x45][0].uiPort = 0x00; m_mapC[0x45][0].uiSwitchBit = 3; m_mapC[0x45][0].uiSwitchBitPattern = 8;
	// R -> テンキー 4 (Port0, Bit4)
	m_mapC[0x52][0].uiPort = 0x00; m_mapC[0x52][0].uiSwitchBit = 4; m_mapC[0x52][0].uiSwitchBitPattern = 16;
	// T -> テンキー 5 (Port0, Bit5)
	m_mapC[0x54][0].uiPort = 0x00; m_mapC[0x54][0].uiSwitchBit = 5; m_mapC[0x54][0].uiSwitchBitPattern = 32;
	// Y -> テンキー 6 (Port0, Bit6)
	m_mapC[0x59][0].uiPort = 0x00; m_mapC[0x59][0].uiSwitchBit = 6; m_mapC[0x59][0].uiSwitchBitPattern = 64;
	// U -> テンキー 7 (Port0, Bit7)
	m_mapC[0x55][0].uiPort = 0x00; m_mapC[0x55][0].uiSwitchBit = 7; m_mapC[0x55][0].uiSwitchBitPattern = 128;
	// I -> テンキー 8 (Port1, Bit0)
	m_mapC[0x49][0].uiPort = 0x01; m_mapC[0x49][0].uiSwitchBit = 0; m_mapC[0x49][0].uiSwitchBitPattern = 1;
	// O -> テンキー 9 (Port1, Bit1)
	m_mapC[0x4f][0].uiPort = 0x01; m_mapC[0x4f][0].uiSwitchBit = 1; m_mapC[0x4f][0].uiSwitchBitPattern = 2;
	// P -> テンキー 0 (Port0, Bit0)
	m_mapC[0x50][0].uiPort = 0x00; m_mapC[0x50][0].uiSwitchBit = 0; m_mapC[0x50][0].uiSwitchBitPattern = 1;
}


/*---------------------------------------------------------------------------
		デストラクタ
---------------------------------------------------------------------------*/

HpcKeyIF::~HpcKeyIF()
{
	delete[] m_mapA;
	delete[] m_mapB;
	delete[] m_mapC;
}

/*---------------------------------------------------------------------------
		マップの再適用処理 (カナキーや文字切替キーが押された時に呼ぶ)
---------------------------------------------------------------------------*/
void HpcKeyIF::UpdateKeyMap()
{
	int mapSize = 256 * 4 * sizeof(KEYPUSHINFO);

	if (!m_bModifierMode) {
		// 文字切替OFFなら常に「マップA」
		memcpy(m_aaKeyPushInfo, m_mapA, mapSize);
	} else {
		// 文字切替ONの場合、状態に応じてBかCを分ける
		if (m_bUseRomaji && (m_dwLockKeyFlag & HPCKEYIF_LOCK_KANA)) {
			// ローマ字ON かつ カナモード中 -> 「マップC」(テンキー)
			memcpy(m_aaKeyPushInfo, m_mapC, mapSize);
		} else {
			// それ以外 -> 「マップB」(メインキー)
			memcpy(m_aaKeyPushInfo, m_mapB, mapSize);
		}
	}
}
/*---------------------------------------------------------------------------
		キー上げ
---------------------------------------------------------------------------*/

void HpcKeyIF::KeyUp( uint uiVkCode, uint32 uiKeyData )
{
KEYPUSHINFO *pKeyPush = m_aaKeyPushInfo[uiVkCode & 0xff];

//	if (uiVkCode == 0xf0)
	if (uiVkCode == 0x79)	// 0x79=HOMEキー でCAPSLOCK
	{
		if ((m_dwLockKeyFlag & HPCKEYIF_LOCK_CAPS) == 0)
		{
			m_dwLockKeyFlag |= HPCKEYIF_LOCK_CAPS;
			return;
		}
		m_dwLockKeyFlag &= ~HPCKEYIF_LOCK_CAPS;
	}
//	if (uiVkCode == 0xf2)
	if (uiVkCode == 0x7b)	// 0x7b=記号キー でカナ
	{
		if ((m_dwLockKeyFlag & HPCKEYIF_LOCK_KANA) == 0)
		{
			m_dwLockKeyFlag |= HPCKEYIF_LOCK_KANA;
			// ★カナモードに入るとき、ローマ字バッファをクリアして状態をリセットする
			if (m_bUseRomaji) romaji_clear((void*)this);
			UpdateKeyMap(); // カナONでマップが変わる可能性があるので更新
			return;
		}
		m_dwLockKeyFlag &= ~HPCKEYIF_LOCK_KANA;
		// ★カナモードを抜けるときもクリア（出力中の仮想キーを離上させるため）
		if (m_bUseRomaji) romaji_clear((void*)this);
		UpdateKeyMap(); // カナOFFでも更新
	}

	if (uiVkCode == 0x7a)	// 0x7a=文字切替キー
	{
		m_bModifierMode = !m_bModifierMode;
		UpdateKeyMap(); // 文字切替のトグルに応じて更新
	}

	for( int i = 0; i < 4 && pKeyPush->uiPort < 0x100; i++ )
	{
		WCEKeyIF::KeyUp( pKeyPush->uiPort, pKeyPush->uiSwitchBit );
		pKeyPush++;
	}
}



/*---------------------------------------------------------------------------
		キー下げ
---------------------------------------------------------------------------*/

void HpcKeyIF::KeyDown( uint uiVkCode, uint32 uiKeyData )
{
KEYPUSHINFO *pKeyPush = m_aaKeyPushInfo[uiVkCode & 0xff];

	if( uiKeyData & ( 1 << 30 ) )
	{
		return;
	}

//	if (uiVkCode == 0xf0 && (m_dwLockKeyFlag & HPCKEYIF_LOCK_CAPS))
	if (uiVkCode == 0x79 && (m_dwLockKeyFlag & HPCKEYIF_LOCK_CAPS))	// 0x79=HOMEキー でCAPSLOCK
	{
		return;
	}
//	if (uiVkCode == 0xf2 && (m_dwLockKeyFlag & HPCKEYIF_LOCK_KANA))
	if (uiVkCode == 0x7b && (m_dwLockKeyFlag & HPCKEYIF_LOCK_KANA))	// 0x7b=記号キー でカナ
	{
		return;
	}

	// ローマ字入力モード時のインターセプト処理
	// 文字切替キーが押されていない(マップAの)時だけローマ字インターセプト
	if ((m_dwLockKeyFlag & HPCKEYIF_LOCK_KANA) && m_bUseRomaji && !m_bModifierMode) {
		int ascii = 0;
		// A-ZのVKコード(0x41-0x5A)はASCIIコードと一致する
		if (uiVkCode >= 0x41 && uiVkCode <= 0x5a) {
			ascii = uiVkCode;
		} else if (uiVkCode == 0xbd) {
			ascii = '-'; // ハイフン（長音など）
		} else if (uiVkCode == 0x20) {
			ascii = ' '; // スペース
		}
		// Brainのキーボード仕様に合わせて、カンマやピリオドが必要ならここにelse ifを追加

		if (ascii != 0) {
			// romaji_input が 0 を返せば、ローマ字バッファに吸収されたということ
			// 実機のエミュレータ側にはキーダウンイベントを流さずにリターンする
			if (romaji_input(ascii, (void*)this) == 0) {
				return; // QUASI88に吸収されたのでPC-88本体にはイベントを流さない
			}
		}
	}
	
	for( int i = 0; i < 4 && pKeyPush->uiPort < 0x100; i++ )
	{
		WCEKeyIF::KeyDown( pKeyPush->uiPort, pKeyPush->uiSwitchBit );
		pKeyPush++;
	}
}



/*---------------------------------------------------------------------------
		カーソルキーをテンキーモードにするかの切り替え。
		判定テーブルを書き換える。
		判定テーブルがstaticなので、これ関係は全部staticにした
		（どうせインスタンスは一つだし）
---------------------------------------------------------------------------*/

void HpcKeyIF::SetUseArrowFor10Flag( bool blUseArrowFor10 )
{
	m_blUseArrowFor10 = blUseArrowFor10;

	if( m_blUseArrowFor10 == true )
	{
//テンキー
///*	VK_NUMPAD4		0x64 */	{ { 0x00, BIT4 }, ENDP },
///*	VK_NUMPAD8		0x68 */	{ { 0x01, BIT0 }, ENDP },
///*	VK_NUMPAD6		0x66 */	{ { 0x00, BIT6 }, ENDP },
///*	VK_NUMPAD2		0x62 */	{ { 0x00, BIT2 }, ENDP },
		m_aaKeyPushInfo[0x25][0].uiPort = 0x00;
		m_aaKeyPushInfo[0x25][0].uiSwitchBit = 4;
		m_aaKeyPushInfo[0x25][0].uiSwitchBitPattern = 16;
		m_aaKeyPushInfo[0x26][0].uiPort = 0x01;
		m_aaKeyPushInfo[0x26][0].uiSwitchBit = 0;
		m_aaKeyPushInfo[0x26][0].uiSwitchBitPattern = 1;
		m_aaKeyPushInfo[0x27][0].uiPort = 0x00;
		m_aaKeyPushInfo[0x27][0].uiSwitchBit = 6;
		m_aaKeyPushInfo[0x27][0].uiSwitchBitPattern = 64;
		m_aaKeyPushInfo[0x28][0].uiPort = 0x00;
		m_aaKeyPushInfo[0x28][0].uiSwitchBit = 2;
		m_aaKeyPushInfo[0x28][0].uiSwitchBitPattern = 4;
	} else {
//カーソルキー
///*	VK_LEFT			0x25 */	{ { 0x0a, BIT2 }, ENDP },
///*	VK_UP			0x26 */	{ { 0x08, BIT1 }, ENDP },
///*	VK_RIGHT		0x27 */	{ { 0x08, BIT2 }, ENDP },
///*	VK_DOWN			0x28 */	{ { 0x0a, BIT1 }, ENDP },
		m_aaKeyPushInfo[0x25][0].uiPort = 0x0a;
		m_aaKeyPushInfo[0x25][0].uiSwitchBit = 2;
		m_aaKeyPushInfo[0x25][0].uiSwitchBitPattern = 4;
		m_aaKeyPushInfo[0x26][0].uiPort = 0x08;
		m_aaKeyPushInfo[0x26][0].uiSwitchBit = 1;
		m_aaKeyPushInfo[0x26][0].uiSwitchBitPattern = 2;
		m_aaKeyPushInfo[0x27][0].uiPort = 0x08;
		m_aaKeyPushInfo[0x27][0].uiSwitchBit = 2;
		m_aaKeyPushInfo[0x27][0].uiSwitchBitPattern = 4;
		m_aaKeyPushInfo[0x28][0].uiPort = 0x0a;
		m_aaKeyPushInfo[0x28][0].uiSwitchBit = 1;
		m_aaKeyPushInfo[0x28][0].uiSwitchBitPattern = 2;
	}


}

/*---------------------------------------------------------------------------
		キー入力判定用テーブル
---------------------------------------------------------------------------*/

HpcKeyIF::KEYPUSHINFO HpcKeyIF::m_aaKeyPushInfo[][4] = {

/*					0x00 */	{ ENDP },
/*	VK_LBUTTON		0x01 */	{ ENDP },
/*	VK_RBUTTON		0x02 */	{ ENDP },
/*	VK_CANCEL		0x03 */	{ ENDP },
/*	VK_MBUTTON		0x04 */	{ ENDP },
/*					0x05 */	{ ENDP },
/*					0x06 */	{ ENDP },
/*					0x07 */	{ ENDP },
/*	VK_BACK			0x08 */	{ { 0x08, BIT3 }, ENDP },
/*	VK_TAB			0x09 */	{ { 0x0a, BIT0 }, ENDP },
/*					0x0A */	{ ENDP },
/*					0x0B */	{ ENDP },
/*	VK_CLEAR		0x0C */	{ { 0x08, BIT0 }, ENDP },
/*	VK_RETURN		0x0D */	{ { 0x01, BIT7 },{ 0x0e, BIT0 }, ENDP },
/*					0x0E */	{ ENDP },
/*					0x0F */	{ ENDP },

/*	VK_SHIFT		0x10 */	{ { 0x08, BIT6 }, { 0x0e, BIT2 }, ENDP },
/*	VK_CONTROL		0x11 */	{ { 0x08, BIT7 }, ENDP },
/*	VK_MENU			0x12 */	{ { 0x08, BIT4 }, ENDP },
/*	VK_PAUSE		0x13 */	{ { 0x09, BIT0 }, ENDP },
///*	VK_CAPITAL		0x14 */	{ { 0x0a, BIT7 }, ENDP },
/*	VK_CAPITAL->SFT	0x14 */	{ { 0x08, BIT6 }, { 0x0e, BIT2 }, ENDP },
/*	VK_KANA			0x15 */	{ { 0x08, BIT5 }, ENDP },
/*					0x16 */	{ ENDP },
/*	VK_JUNJA		0x17 */	{ ENDP },
/*	VK_FINAL		0x18 */	{ ENDP },
/*	VK_KANJI		0x19 */	{ ENDP },
/*					0x1A */	{ ENDP },
/*	VK_ESCAPE		0x1B */	{ { 0x09, BIT7 }, ENDP },
/*	VK_CONVERT		0x1c */	{ ENDP },
/*	VK_NOCONVERT	0x1d */	{ ENDP },
/*					0x1e */	{ ENDP },
/*					0x1f */	{ ENDP },

/*	VK_SPACE		0x20 */	{ { 0x09, BIT6 }, ENDP },
/*	VK_PRIOR		0x21 */	{ { 0x0b, BIT0 }, ENDP },
/*	VK_NEXT			0x22 */	{ { 0x0b, BIT1 }, ENDP },
/*	VK_END			0x23 */	{ { 0x0a, BIT3 }, ENDP },
/*	VK_HOME			0x24 */	{ { 0x08, BIT0 }, ENDP },
/*	VK_LEFT			0x25 */	{ { 0x0a, BIT2 }, ENDP },
/*	VK_UP			0x26 */	{ { 0x08, BIT1 }, ENDP },
/*	VK_RIGHT		0x27 */	{ { 0x08, BIT2 }, ENDP },
/*	VK_DOWN			0x28 */	{ { 0x0a, BIT1 }, ENDP },
/*	VK_SELECT		0x29 */	{ ENDP },
/*	VK_PRINT		0x2A */	{ ENDP },
/*	VK_EXECUTE		0x2B */	{ ENDP },
/*	VK_SNAPSHOT		0x2C */	{ ENDP },
/*	VK_INSERT		0x2D */	{ { 0x0c, BIT6 }, ENDP },
/*	VK_DELETE		0x2E */	{ { 0x0c, BIT7 }, ENDP },
/*	VK_HELP			0x2F */	{ { 0x0a, BIT3 }, ENDP },

/*	VK_0			0x30 */	{ { 0x06, BIT0 }, ENDP },
/*	VK_1			0x31 */	{ { 0x06, BIT1 }, ENDP },
/*	VK_2			0x32 */	{ { 0x06, BIT2 }, ENDP },
/*	VK_3			0x33 */	{ { 0x06, BIT3 }, ENDP },
/*	VK_4			0x34 */	{ { 0x06, BIT4 }, ENDP },
/*	VK_5			0x35 */	{ { 0x06, BIT5 }, ENDP },
/*	VK_6			0x36 */	{ { 0x06, BIT6 }, ENDP },
/*	VK_7			0x37 */	{ { 0x06, BIT7 }, ENDP },
/*	VK_8			0x38 */	{ { 0x07, BIT0 }, ENDP },
/*	VK_9			0x39 */	{ { 0x07, BIT1 }, ENDP },
/*					0x3a */	{ ENDP },
/*					0x3b */	{ ENDP },
/*					0x3c */	{ ENDP },
/*					0x3d */	{ ENDP },
/*					0x3e */	{ ENDP },
/*					0x3f */	{ ENDP },

/*					0x40 */	{ ENDP },
/*	VK_A			0x41 */	{ { 0x02, BIT1 }, ENDP },
/*	VK_B			0x42 */	{ { 0x02, BIT2 }, ENDP },
/*	VK_C			0x43 */	{ { 0x02, BIT3 }, ENDP },
/*	VK_D			0x44 */	{ { 0x02, BIT4 }, ENDP },
/*	VK_E			0x45 */	{ { 0x02, BIT5 }, ENDP },
/*	VK_F			0x46 */	{ { 0x02, BIT6 }, ENDP },
/*	VK_G			0x47 */	{ { 0x02, BIT7 }, ENDP },
/*	VK_H			0x48 */	{ { 0x03, BIT0 }, ENDP },
/*	VK_I			0x49 */	{ { 0x03, BIT1 }, ENDP },
/*	VK_J			0x4a */	{ { 0x03, BIT2 }, ENDP },
/*	VK_K			0x4b */	{ { 0x03, BIT3 }, ENDP },
/*	VK_L			0x4c */	{ { 0x03, BIT4 }, ENDP },
/*	VK_M			0x4d */	{ { 0x03, BIT5 }, ENDP },
/*	VK_N			0x4e */	{ { 0x03, BIT6 }, ENDP },
/*	VK_O			0x4f */	{ { 0x03, BIT7 }, ENDP },

/*	VK_P			0x50 */	{ { 0x04, BIT0 }, ENDP },
/*	VK_Q			0x51 */	{ { 0x04, BIT1 }, ENDP },
/*	VK_R			0x52 */	{ { 0x04, BIT2 }, ENDP },
/*	VK_S			0x53 */	{ { 0x04, BIT3 }, ENDP },
/*	VK_T			0x54 */	{ { 0x04, BIT4 }, ENDP },
/*	VK_U			0x55 */	{ { 0x04, BIT5 }, ENDP },
/*	VK_V			0x56 */	{ { 0x04, BIT6 }, ENDP },
/*	VK_W			0x57 */	{ { 0x04, BIT7 }, ENDP },
/*	VK_X			0x58 */	{ { 0x05, BIT0 }, ENDP },
/*	VK_Y			0x59 */	{ { 0x05, BIT1 }, ENDP },
/*	VK_Z			0x5a */	{ { 0x05, BIT2 }, ENDP },
/*	VK_LWIN			0x5B */	{ ENDP },
/*	VK_RWIN			0x5C */	{ ENDP },
/*	VK_APPS			0x5D */	{ ENDP },
/*					0x5e */	{ ENDP },
/*					0x5f */	{ ENDP },

/*	VK_NUMPAD0		0x60 */	{ { 0x00, BIT0 }, ENDP },
/*	VK_NUMPAD1		0x61 */	{ { 0x00, BIT1 }, ENDP },
/*	VK_NUMPAD2		0x62 */	{ { 0x00, BIT2 }, ENDP },
/*	VK_NUMPAD3		0x63 */	{ { 0x00, BIT3 }, ENDP },
/*	VK_NUMPAD4		0x64 */	{ { 0x00, BIT4 }, ENDP },
/*	VK_NUMPAD5		0x65 */	{ { 0x00, BIT5 }, ENDP },
/*	VK_NUMPAD6		0x66 */	{ { 0x00, BIT6 }, ENDP },
/*	VK_NUMPAD7		0x67 */	{ { 0x00, BIT7 }, ENDP },
/*	VK_NUMPAD8		0x68 */	{ { 0x01, BIT0 }, ENDP },
/*	VK_NUMPAD9		0x69 */	{ { 0x01, BIT1 }, ENDP },
/*	VK_MULTIPLY		0x6A */	{ { 0x01, BIT2 }, ENDP },
/*	VK_ADD			0x6B */	{ { 0x01, BIT3 }, ENDP },
/*	VK_SEPARATOR	0x6C */	{ { 0x01, BIT5 }, ENDP },
/*	VK_SUBTRACT		0x6D */	{ { 0x0a, BIT5 }, ENDP },
/*	VK_DECIMAL		0x6E */	{ { 0x01, BIT6 }, ENDP },
/*	VK_DIVIDE		0x6F */	{ { 0x0a, BIT6 }, ENDP },

/*	VK_F1			0x70 */	{ { 0x09, BIT1 }, ENDP },
/*	VK_F2			0x71 */	{ { 0x09, BIT2 }, ENDP },
/*	VK_F3			0x72 */	{ { 0x09, BIT3 }, ENDP },
/*	VK_F4			0x73 */	{ { 0x09, BIT4 }, ENDP },
/*	VK_F5			0x74 */	{ { 0x09, BIT5 }, ENDP },
/*	VK_F6			0x75 */	{ { 0x09, BIT1 }, ENDP },
///*	VK_F7			0x76 */	{ { 0x09, BIT2 }, ENDP },
/*	VK_F7->F5		0x76 */	{ { 0x09, BIT5 }, ENDP },
///*	VK_F8			0x77 */	{ { 0x09, BIT3 }, ENDP },
/*	VK_F8->TAB		0x77 */	{ { 0x0a, BIT0 }, ENDP },
///*	VK_F9			0x78 */	{ { 0x09, BIT4 }, ENDP },
/*	VK_F9->CTRL		0x78 */	{ { 0x08, BIT7 }, ENDP },
///*	VK_F10			0x79 */	{ { 0x09, BIT5 }, ENDP },
/*	VK_F10->CAPS	0x79 */	{ { 0x0a, BIT7 }, ENDP },
/*	VK_F11			0x7A */	{ ENDP },
///*	VK_F12			0x7B */	{ ENDP },
/*	VK_F12->KANA	0x7B */	{ { 0x08, BIT5 }, ENDP },		//KANA
/*					0x7C */	{ ENDP },
/*					0x7D */	{ ENDP },
/*					0x7E */	{ ENDP },
/*					0x7F */	{ ENDP },

/*					0x80 */	{ ENDP },
/*					0x81 */	{ ENDP },
/*					0x82 */	{ ENDP },
/*					0x83 */	{ ENDP },
/*					0x84 */	{ ENDP },
/*					0x85 */	{ ENDP },
/*					0x86 */	{ ENDP },
/*					0x87 */	{ ENDP },
/*					0x88 */	{ ENDP },
/*					0x89 */	{ ENDP },
/*					0x8A */	{ ENDP },
/*					0x8B */	{ ENDP },
/*					0x8C */	{ ENDP },
/*					0x8D */	{ ENDP },
/*					0x8E */	{ ENDP },
/*					0x8F */	{ ENDP },

/*					0x90 */	{ ENDP },
/*					0x91 */	{ ENDP },
/*					0x92 */	{ ENDP },
/*					0x93 */	{ ENDP },
/*					0x94 */	{ ENDP },
/*					0x95 */	{ ENDP },
/*					0x96 */	{ ENDP },
/*					0x97 */	{ ENDP },
/*					0x98 */	{ ENDP },
/*					0x99 */	{ ENDP },
/*					0x9A */	{ ENDP },
/*					0x9B */	{ ENDP },
/*					0x9C */	{ ENDP },
/*					0x9D */	{ ENDP },
/*					0x9E */	{ ENDP },
/*					0x9F */	{ ENDP },

/*					0xA0 */	{ ENDP },
/*					0xA1 */	{ ENDP },
/*					0xA2 */	{ ENDP },
/*					0xA3 */	{ ENDP },
/*					0xA4 */	{ ENDP },
/*					0xA5 */	{ ENDP },
/*					0xA6 */	{ ENDP },
/*					0xA7 */	{ ENDP },
/*					0xA8 */	{ ENDP },
/*					0xA9 */	{ ENDP },
/*					0xAA */	{ ENDP },
/*					0xAB */	{ ENDP },
/*					0xAC */	{ ENDP },
/*					0xAD */	{ ENDP },
/*					0xAE */	{ ENDP },
/*					0xAF */	{ ENDP },

/*					0xB0 */	{ ENDP },
/*					0xB1 */	{ ENDP },
/*					0xB2 */	{ ENDP },
/*					0xB3 */	{ ENDP },
/*					0xB4 */	{ ENDP },
/*					0xB5 */	{ ENDP },
/*					0xB6 */	{ ENDP },
/*					0xB7 */	{ ENDP },
/*					0xB8 */	{ ENDP },
/*					0xB9 */	{ ENDP },
/*	:				0xBA */	{ { 0x07, BIT2 }, ENDP },
/*	;				0xBB */	{ { 0x07, BIT3 }, ENDP },
/*	,				0xBC */	{ { 0x07, BIT4 }, ENDP },
/*	-				0xBD */	{ { 0x05, BIT7 }, ENDP },
/*	.				0xBE */	{ { 0x07, BIT5 }, ENDP },
/*	/				0xBF */	{ { 0x07, BIT6 }, ENDP },

/*	@				0xC0 */	{ { 0x02, BIT0 }, ENDP },
/*					0xC1 */	{ ENDP },
/*					0xC2 */	{ ENDP },
/*					0xC3 */	{ ENDP },
/*					0xC4 */	{ ENDP },
/*					0xC5 */	{ ENDP },
/*					0xC6 */	{ ENDP },
/*					0xC7 */	{ ENDP },
/*					0xC8 */	{ ENDP },
/*					0xC9 */	{ ENDP },
/*					0xCA */	{ ENDP },
/*					0xCB */	{ ENDP },
/*					0xCC */	{ ENDP },
/*					0xCD */	{ ENDP },
/*					0xCE */	{ ENDP },
/*					0xCF */	{ ENDP },

/*					0xD0 */	{ ENDP },
/*					0xD1 */	{ ENDP },
/*					0xD2 */	{ ENDP },
/*					0xD3 */	{ ENDP },
/*					0xD4 */	{ ENDP },
/*					0xD5 */	{ ENDP },
/*					0xD6 */	{ ENDP },
/*					0xD7 */	{ ENDP },
/*					0xD8 */	{ ENDP },
/*					0xD9 */	{ ENDP },
/*					0xDA */	{ ENDP },
/*	[				0xDB */	{ { 0x05, BIT3 }, ENDP },
/*	\				0xDC */	{ { 0x05, BIT4 }, ENDP },
/*	]				0xDD */	{ { 0x05, BIT5 }, ENDP },
/*	^				0xDE */	{ { 0x05, BIT6 }, ENDP },
/*	_(NEC98?)		0xDF */	{ { 0x07, BIT7 }, ENDP },

/*					0xE0 */	{ ENDP },
/*					0xE1 */	{ ENDP },
/*	_				0xE2 */	{ { 0x07, BIT7 }, ENDP },
/*					0xE3 */	{ ENDP },
/*					0xE4 */	{ ENDP },
/*					0xE5 */	{ ENDP },
/*					0xE6 */	{ ENDP },
/*					0xE7 */	{ ENDP },
/*					0xE8 */	{ ENDP },
/*					0xE9 */	{ ENDP },
/*					0xEA */	{ ENDP },
/*					0xEB */	{ ENDP },
/*					0xEC */	{ ENDP },
/*					0xED */	{ ENDP },
/*					0xEE */	{ ENDP },
/*					0xEF */	{ ENDP },

/*					0xF0 */	{ { 0x0a, BIT7 }, ENDP },		//CAPS
/*					0xF1 */	{ ENDP },
/*					0xF2 */	{ { 0x08, BIT5 }, ENDP },		//KANA
/*					0xF3 */	{ { 0x09, BIT0 }, ENDP },		//STOP(半角/全角)
/*					0xF4 */	{ ENDP },
/*					0xF5 */	{ ENDP },
/*					0xF6 */	{ ENDP },
/*					0xF7 */	{ ENDP },
/*					0xF8 */	{ ENDP },
/*					0xF9 */	{ ENDP },
/*					0xFA */	{ ENDP },
/*					0xFB */	{ ENDP },
/*					0xFC */	{ ENDP },
/*					0xFD */	{ ENDP },
/*					0xFE */	{ ENDP },
/*					0xFF */	{ ENDP }


};

// --- ファイルの末尾に追加: タイマー処理 ---
void HpcKeyIF::OnTimer()
{
	// ★修正: PC-88が「カナモード」かつ「ローマ字入力ON」の時だけ出力駆動
	if ((m_dwLockKeyFlag & HPCKEYIF_LOCK_KANA) && m_bUseRomaji) {
		romaji_output((void*)this);
	}
}

/*---------------------------------------------------------------------------
		ローマ字入力モジュールからのコールバックブリッジ
---------------------------------------------------------------------------*/
extern "C" void RomajiHook_KeyDown(void* pKeyIF, int port, int bit)
{
	if (pKeyIF) {
		((PC8801::WCEKeyIF*)pKeyIF)->KeyDown(port, bit);
	}
}

extern "C" void RomajiHook_KeyUp(void* pKeyIF, int port, int bit)
{
	if (pKeyIF) {
		((PC8801::WCEKeyIF*)pKeyIF)->KeyUp(port, bit);
	}
}