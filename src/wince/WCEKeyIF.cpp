/*===========================================================================
		M88 for PocketPC
		Original Version (c)cisc
		Pocket PC Version Programmed by Y.Taniuchi ( TAN-Y )

		WCEKeyIF クラス実装
===========================================================================*/

#include "m88ce.h"


using namespace PC8801;

static uint auiKeyUpMask[] =
		{ 0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80 };
static uint auiKeyDownMask[] =
		{ 0xfe, 0xfd, 0xfb, 0xf7, 0xef, 0xdf, 0xbf, 0x7f };

/*---------------------------------------------------------------------------
		コンストラクタ
---------------------------------------------------------------------------*/

WCEKeyIF::WCEKeyIF()
	: Device( 0 )
{
	m_blActive = false;

	Reset();
}


/*---------------------------------------------------------------------------
		デストラクタ
---------------------------------------------------------------------------*/

WCEKeyIF::~WCEKeyIF()
{
}


/*---------------------------------------------------------------------------
		キー上げ
---------------------------------------------------------------------------*/

void WCEKeyIF::KeyUp( int iPort, uint uiBit )
{
	if( m_aauiKeyPushedCntTbl[iPort][uiBit] > 0 )
	{
		m_aauiKeyPushedCntTbl[iPort][uiBit]--;
		if( m_aauiKeyPushedCntTbl[iPort][uiBit] == 0 )
		{
			//m_auiKeyTbl[iPort] |= auiKeyUpMask[uiBit];
			m_auiKeyTblBackup[iPort] |= auiKeyUpMask[uiBit];
		}
	}
}


/*---------------------------------------------------------------------------
		キー下げ
---------------------------------------------------------------------------*/

void WCEKeyIF::KeyDown( int iPort, uint uiBit )
{
	m_aauiKeyPushedCntTbl[iPort][uiBit]++;
	m_auiKeyTbl[iPort] &= auiKeyDownMask[uiBit];
//	m_auiKeyTblBackup[iPort] = m_auiKeyTbl[iPort];
	m_auiKeyTblBackup[iPort] &= auiKeyDownMask[uiBit];
}



/*---------------------------------------------------------------------------
		PC88 コアからのキー入力
---------------------------------------------------------------------------*/

uint IOCALL WCEKeyIF::In(uint port)
{
	return m_auiKeyTbl[port];
}


/*---------------------------------------------------------------------------
		VSYNC
---------------------------------------------------------------------------*/

void IOCALL WCEKeyIF::VSync(uint, uint data)
{
	if( data && m_blActive )
	{
		//
		//	VSYNC 同期に合わせてバックアップのテーブルを本体テーブル
		//	へコピー
		//	→離しているキーをこの時点で反映
		//
		memcpy( m_auiKeyTbl, m_auiKeyTblBackup, sizeof(uint) * 16 );
	}
}




/*---------------------------------------------------------------------------
		Reset
---------------------------------------------------------------------------*/

void IOCALL WCEKeyIF::Reset(uint , uint)
{
	memset( m_auiKeyTbl, 0xff, sizeof(uint) * 16 );
	memset( m_auiKeyTblBackup, 0xff, sizeof(uint) * 16 );
	memset( m_aauiKeyPushedCntTbl, 0, sizeof(uint[16][8]) );
}



// ---------------------------------------------------------------------------
//	device description
//
const Device::Descriptor WCEKeyIF::descriptor = { indef, outdef };

const Device::OutFuncPtr WCEKeyIF::outdef[] = 
{
	STATIC_CAST(Device::OutFuncPtr, Reset),
	STATIC_CAST(Device::OutFuncPtr, VSync),
};

const Device::InFuncPtr WCEKeyIF::indef[] = 
{
	STATIC_CAST(Device::InFuncPtr, In),
};
