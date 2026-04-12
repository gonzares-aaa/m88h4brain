/*===========================================================================
		M88 for PocketPC
		Original Version (c)cisc
		Pocket PC Version Programmed by Y.Taniuchi ( TAN-Y )

		PpcKeyIF クラス実装
===========================================================================*/

#include "m88p.h"

//extern int m_ppiKeyInputTable[16 * 8][4];

struct KEYPUSHINFO
{
	uint uiPort;
	uint uiSwitchBit;
	uint uiSwitchBitPattern;
};

#define BIT0 0, 1
#define BIT1 1, 2
#define BIT2 2, 4
#define BIT3 3, 8
#define BIT4 4, 16
#define BIT5 5, 32
#define BIT6 6, 64
#define BIT7 7, 128
#define ENDP { 0x100, 0, 0 }

extern KEYPUSHINFO g_aaKeyPushInfo[][4];

using namespace PC8801;


/*---------------------------------------------------------------------------
		コンストラクタ
---------------------------------------------------------------------------*/

PpcKeyIF::PpcKeyIF()
	: WCEKeyIF()
{
}


/*---------------------------------------------------------------------------
		デストラクタ
---------------------------------------------------------------------------*/

PpcKeyIF::~PpcKeyIF()
{
}


/*---------------------------------------------------------------------------
		キー上げ
---------------------------------------------------------------------------*/

void PpcKeyIF::KeyUp( int iKeyCode )
{
KEYPUSHINFO *pKeyPush = g_aaKeyPushInfo[iKeyCode];

	for( int i = 0; i < 4 && pKeyPush->uiPort < 0x100; i++ )
	{
		WCEKeyIF::KeyUp( pKeyPush->uiPort, pKeyPush->uiSwitchBit );
		pKeyPush++;
	}
}


/*
void PpcKeyIF::KeyUp( int iKeyCode )
{
KEYPUSHINFO *pKeyPush = g_aaKeyPushInfo[iKeyCode];

	for( int i = 0; i < 4 && pKeyPush->uiPort < 0x100; i++ )
	{
		if( m_aauiKeyPushedCntTbl[pKeyPush->uiPort][pKeyPush->uiSwitchBit]
				> 0 )
		{
			m_aauiKeyPushedCntTbl[pKeyPush->uiPort][pKeyPush->uiSwitchBit]--;

			if( m_aauiKeyPushedCntTbl[pKeyPush->uiPort][pKeyPush->uiSwitchBit]
					== 0 )
			{
				m_auiKeyTbl[pKeyPush->uiPort] |= pKeyPush->uiSwitchBitPattern;
				m_auiKeyTbl[pKeyPush->uiPort] |= 0xff;
			}
		}
		pKeyPush++;
	}
}
*/

/*---------------------------------------------------------------------------
		キー下げ
---------------------------------------------------------------------------*/

void PpcKeyIF::KeyDown( int iKeyCode )
{
KEYPUSHINFO *pKeyPush = g_aaKeyPushInfo[iKeyCode];

	for( int i = 0; i < 4 && pKeyPush->uiPort < 0x100; i++ )
	{
		WCEKeyIF::KeyDown( pKeyPush->uiPort, pKeyPush->uiSwitchBit );
		pKeyPush++;
	}
}


/*
void PpcKeyIF::KeyDown( int iKeyCode )
{
KEYPUSHINFO *pKeyPush = g_aaKeyPushInfo[iKeyCode];

	for( int i = 0; i < 4 && pKeyPush->uiPort < 0x100; i++ )
	{
		m_aauiKeyPushedCntTbl[pKeyPush->uiPort][pKeyPush->uiSwitchBit]++;

		uint uiPat = ~pKeyPush->uiSwitchBitPattern;
		m_auiKeyTbl[pKeyPush->uiPort] &= uiPat;

		pKeyPush++;
	}
}
*/



/*---------------------------------------------------------------------------
		キー入力判定用テーブル
---------------------------------------------------------------------------*/

static KEYPUSHINFO g_aaKeyPushInfo[][4] = {
/*	0x00	"MENU"	*/	{ ENDP },
/*	0x01	"F1"	*/	{ { 0x09, BIT1 }, ENDP },
/*	0x02	"F2"	*/	{ { 0x09, BIT2 }, ENDP },
/*	0x03	"F3"	*/	{ { 0x09, BIT3 }, ENDP },
/*	0x04	"F4"	*/	{ { 0x09, BIT4 }, ENDP },
/*	0x05	"F5"	*/	{ { 0x09, BIT5 }, ENDP },
/*	0x06	"STP"	*/	{ { 0x09, BIT0 }, ENDP },
/*	0x07	"CPY"	*/	{ { 0x0a, BIT4 }, ENDP },
/*	0x08	"CTRL"	*/	{ { 0x08, BIT7 }, ENDP },
/*	0x09	"SLK"	*/	{ ENDP },
/*	0x0a	"ESC"	*/	{ { 0x09, BIT7 }, ENDP },
/*	0x0b	"TAB"	*/	{ { 0x0a, BIT0 }, ENDP },
/*	0x0c	"CAP"	*/	{ { 0x0a, BIT7 }, ENDP },
/*	0x0d	"SHFT"	*/	{ { 0x08, BIT6 }, { 0x0e, BIT2 }, ENDP },
/*	0x0e	"ｶﾅ"	*/	{ { 0x08, BIT5 }, ENDP },
/*	0x0f	"1"		*/	{ { 0x06, BIT1 }, ENDP },
/*	0x10	"2"		*/	{ { 0x06, BIT2 }, ENDP },
/*	0x11	"3"		*/	{ { 0x06, BIT3 }, ENDP },
/*	0x12	"4"		*/	{ { 0x06, BIT4 }, ENDP },
/*	0x13	"5"		*/	{ { 0x06, BIT5 }, ENDP },
/*	0x14	"6"		*/	{ { 0x06, BIT6 }, ENDP },
/*	0x15	"7"		*/	{ { 0x06, BIT7 }, ENDP },
/*	0x16	"8"		*/	{ { 0x07, BIT0 }, ENDP },
/*	0x17	"9"		*/	{ { 0x07, BIT1 }, ENDP },
/*	0x18	"0"		*/	{ { 0x06, BIT0 }, ENDP },
/*	0x19	"-"		*/	{ { 0x05, BIT7 }, ENDP },
/*	0x1a	"^"		*/	{ { 0x05, BIT6 }, ENDP },
/*	0x1b	"\\"	*/	{ { 0x05, BIT4 }, ENDP },
/*	0x1c	"Q"		*/	{ { 0x04, BIT1 }, ENDP },
/*	0x1d	"W"		*/	{ { 0x04, BIT7 }, ENDP },
/*	0x1e	"E"		*/	{ { 0x02, BIT5 }, ENDP },
/*	0x1f	"R"		*/	{ { 0x04, BIT2 }, ENDP },
/*	0x20	"T"		*/	{ { 0x04, BIT4 }, ENDP },
/*	0x21	"Y"		*/	{ { 0x05, BIT1 }, ENDP },
/*	0x22	"U"		*/	{ { 0x04, BIT5 }, ENDP },
/*	0x23	"I"		*/	{ { 0x03, BIT1 }, ENDP },
/*	0x24	"O"		*/	{ { 0x03, BIT7 }, ENDP },
/*	0x25	"P"		*/	{ { 0x04, BIT0 }, ENDP },
/*	0x26	"@"		*/	{ { 0x02, BIT0 }, ENDP },
/*	0x27	"["		*/	{ { 0x05, BIT3 }, ENDP },
/*	0x28	"A"		*/	{ { 0x02, BIT1 }, ENDP },
/*	0x29	"S"		*/	{ { 0x04, BIT3 }, ENDP },
/*	0x2a	"D"		*/	{ { 0x02, BIT4 }, ENDP },
/*	0x2b	"F"		*/	{ { 0x02, BIT6 }, ENDP },
/*	0x2c	"G"		*/	{ { 0x02, BIT7 }, ENDP },
/*	0x2d	"H"		*/	{ { 0x03, BIT0 }, ENDP },
/*	0x2e	"J"		*/	{ { 0x03, BIT2 }, ENDP },
/*	0x2f	"K"		*/	{ { 0x03, BIT3 }, ENDP },
/*	0x30	"L"		*/	{ { 0x03, BIT4 }, ENDP },
/*	0x31	";"		*/	{ { 0x07, BIT3 }, ENDP },
/*	0x32	":"		*/	{ { 0x07, BIT2 }, ENDP },
/*	0x33	"]"		*/	{ { 0x05, BIT5 }, ENDP },
/*	0x34	"Z"		*/	{ { 0x05, BIT2 }, ENDP },
/*	0x35	"X"		*/	{ { 0x05, BIT0 }, ENDP },
/*	0x36	"C"		*/	{ { 0x02, BIT3 }, ENDP },
/*	0x37	"V"		*/	{ { 0x04, BIT6 }, ENDP },
/*	0x38	"B"		*/	{ { 0x02, BIT2 }, ENDP },
/*	0x39	"N"		*/	{ { 0x03, BIT6 }, ENDP },
/*	0x3a	"M"		*/	{ { 0x03, BIT5 }, ENDP },
/*	0x3b	","		*/	{ { 0x07, BIT4 }, ENDP },
/*	0x3c	"."		*/	{ { 0x07, BIT5 }, ENDP },
/*	0x3d	"/"		*/	{ { 0x07, BIT6 }, ENDP },
/*	0x3e	"_"		*/	{ { 0x07, BIT7 }, ENDP },
/*	0x3f	"GRPH"	*/	{ { 0x08, BIT4 }, ENDP },
/*	0x40" SPACE "	*/	{ { 0x09, BIT6 }, ENDP },
/*	0x41	"BS"	*/	{ { 0x08, BIT3 }, ENDP },
/*	0x42	"RET"	*/	{ { 0x01, BIT7 },{ 0x0e, BIT0 }, ENDP },
/*	0x43	"<-"	*/	{ { 0x01, BIT7 },{ 0x0e, BIT0 }, ENDP },
/*	0x44	"SHFT"	*/	{ { 0x08, BIT6 }, { 0x0e, BIT3 }, ENDP },
/*	0x45	"RU"	*/	{ { 0x0b, BIT1 }, ENDP },
/*	0x46	"RD"	*/	{ { 0x0b, BIT0 }, ENDP },
/*	0x47	"IS"	*/	{ { 0x0c, BIT6 }, ENDP },
/*	0x48	"DL"	*/	{ { 0x0c, BIT7 }, ENDP },
/*	0x49	"^"		*/	{ { 0x08, BIT1 }, ENDP },
/*	0x4a	"v"		*/	{ { 0x0a, BIT1 }, ENDP },
/*	0x4b	"<"		*/	{ { 0x0a, BIT2 }, ENDP },
/*	0x4c	">"		*/	{ { 0x08, BIT2 }, ENDP },
/*	0x4d	"HC"	*/	{ { 0x08, BIT0 }, ENDP },
/*	0x4e	"HP"	*/	{ { 0x0a, BIT3 }, ENDP },
/*	0x4f	"-"		*/	{ { 0x0a, BIT5 }, ENDP },
/*	0x50	"/"		*/	{ { 0x0a, BIT6 }, ENDP },
/*	0x51	"7"		*/	{ { 0x00, BIT7 }, ENDP },
/*	0x52	"8"		*/	{ { 0x01, BIT0 }, ENDP },
/*	0x53	"9"		*/	{ { 0x01, BIT1 }, ENDP },
/*	0x54	"*"		*/	{ { 0x01, BIT2 }, ENDP },
/*	0x55	"4"		*/	{ { 0x00, BIT4 }, ENDP },
/*	0x56	"5"		*/	{ { 0x00, BIT5 }, ENDP },
/*	0x57	"6"		*/	{ { 0x00, BIT6 }, ENDP },
/*	0x58	"+"		*/	{ { 0x01, BIT3 }, ENDP },
/*	0x59	"1"		*/	{ { 0x00, BIT1 }, ENDP },
/*	0x5a	"2"		*/	{ { 0x00, BIT2 }, ENDP },
/*	0x5b	"3"		*/	{ { 0x00, BIT3 }, ENDP },
/*	0x5c	"="		*/	{ { 0x01, BIT4 }, ENDP },
/*	0x5d	"0"		*/	{ { 0x00, BIT0 }, ENDP },
/*	0x5e	","		*/	{ { 0x01, BIT5 }, ENDP },
/*	0x5f	"."		*/	{ { 0x01, BIT6 }, ENDP },
/*	0x60	"RT"	*/	{ { 0x01, BIT7 },{ 0x0e, BIT1 }, ENDP },
/*	0x61	" Z "	*/	{ { 0x05, BIT2 }, ENDP },
/*	0x62	" X "	*/	{ { 0x05, BIT0 }, ENDP },
/*	0x63	"REST"	*/	{ ENDP },
/*	0x64	"EXIT"	*/	{ ENDP }
	};


/*
static int m_ppiKeyInputTable[16 * 8][4] = {
			//	Ten key 0 1 2 3 4 5 6 7
			{ 0x5d, -1 },
			{ 0x59, -1 },
			{ 0x5a, -1 },
			{ 0x5b, -1 },
			{ 0x55, -1 },
			{ 0x56, -1 },
			{ 0x57, -1 },
			{ 0x51, -1 },

			//	Ten key 8 9 * + = , . RET
			{ 0x52, -1 },
			{ 0x53, -1 },
			{ 0x54, -1 },
			{ 0x58, -1 },
			{ 0x5c, -1 },
			{ 0x5e, -1 },
			{ 0x5f, -1 },
			{ 0x42, 0x43, 0x60, -1 },

			//	@ A B C D E F G
			{ 0x26, -1 },
			{ 0x28, -1 },
			{ 0x38, -1 },
			{ 0x36, -1 },
			{ 0x2a, -1 },
			{ 0x1e, -1 },
			{ 0x2b, -1 },
			{ 0x2c, -1 },

			//	H I J K L M N O
			{ 0x2d, -1 },
			{ 0x23, -1 },
			{ 0x2e, -1 },
			{ 0x2f, -1 },
			{ 0x30, -1 },
			{ 0x3a, -1 },
			{ 0x39, -1 },
			{ 0x24, -1 },

			//	P Q R S T U V W
			{ 0x25, -1 },
			{ 0x1c, -1 },
			{ 0x1f, -1 },
			{ 0x29, -1 },
			{ 0x20, -1 },
			{ 0x22, -1 },
			{ 0x37, -1 },
			{ 0x1d, -1 },

			//	X Y Z [ \ ] ^ -
			{ 0x35, 0x62, -1 },
			{ 0x21, -1 },
			{ 0x34, 0x61, -1 },
			{ 0x27, -1 },
			{ 0x1b, -1 },
			{ 0x33, -1 },
			{ 0x1a, -1 },
			{ 0x19, -1 },

			//	0 1 2 3 4 5 6 7
			{ 0x18, -1 },
			{ 0x0f, -1 },
			{ 0x10, -1 },
			{ 0x11, -1 },
			{ 0x12, -1 },
			{ 0x13, -1 },
			{ 0x14, -1 },
			{ 0x15, -1 },

			//	8 9 : ; , . / _
			{ 0x16, -1 },
			{ 0x17, -1 },
			{ 0x32, -1 },
			{ 0x31, -1 },
			{ 0x3b, -1 },
			{ 0x3c, -1 },
			{ 0x3d, -1 },
			{ 0x3e, -1 },

			//	CLR ↑ → BS GRPH カナ SHIFT CTRL
			{ 0x4d, -1 },
			{ 0x49, -1 },
			{ 0x4c, -1 },
			{ 0x41, -1 },
			{ 0x3f, -1 },
			{ 0x0e, -1 },
			{ 0x0d, 0x44, -1 },
			{ 0x08, -1 },

			// STOP F1 F2 F3 F4 F5 SPACE ESC
			{ 0x06, -1 },
			{ 0x01, -1 },
			{ 0x02, -1 },
			{ 0x03, -1 },
			{ 0x04, -1 },
			{ 0x05, -1 },
			{ 0x40, -1 },
			{ 0x0a, -1 },

			//	TAB ↓ ← HELP COPY - / CAPSLOCK
			{ 0x0b, -1 },
			{ 0x4a, -1 },
			{ 0x4b, -1 },
			{ 0x4e, -1 },
			{ 0x07, -1 },
			{ -1 },
			{ -1 },
			{ 0x0c, -1 },

			//	ROLLDOWN ROLLUP ??? ??? ??? ??? ??? ???
			{ 0x45, -1 },
			{ 0x46, -1 },
			{ -1 },
			{ -1 },
			{ -1 },
			{ -1 },
			{ -1 },
			{ -1 },

			//	F1 F2 F3 F4 F5 BS INS DEL
			{ 0x01, -1 },
			{ 0x02, -1 },
			{ 0x03, -1 },
			{ 0x04, -1 },
			{ 0x05, -1 },
			{ 0x41, -1 },
			{ 0x47, -1 },
			{ 0x48, -1 },

			//	F6 F7 F8 F9 F10 変換 決定 SPACE
			{ -1 },
			{ -1 },
			{ -1 },
			{ -1 },
			{ -1 },
			{ -1 },
			{ -1 },
			{ -1 },

			//	RET(FK) RET(tenkey) SHIFT-L SHIFT-R PC 全角 ??? ???
			{ 0x42, 0x43, -1 },
			{ 0x60, -1 },
			{ 0x0d, -1 },
			{ 0x44, -1 },
			{ -1 },
			{ -1 },
			{ -1 },
			{ -1 },

			//	??? ??? ??? ??? ??? ??? ??? ???
			{ -1 },
			{ -1 },
			{ -1 },
			{ -1 },
			{ -1 },
			{ -1 },
			{ -1 },
			{ -1 }
		};
*/

