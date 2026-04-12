/*===========================================================================
		M88 for PocketPC
		Original Version (c)cisc
		Pocket PC Version Programmed by Y.Taniuchi ( TAN-Y )

		PpcCore クラス実装
===========================================================================*/

#include "m88p.h"
#include "misc.h"

using namespace PC8801;


/*---------------------------------------------------------------------------
		コンストラクタ
---------------------------------------------------------------------------*/

PpcCore::PpcCore()
{
	m_pUI = NULL;
	m_hWnd = NULL;
	m_blThreadLoop = false;
	m_blActive = false;

	m_iClock = 40;
	m_iExeccount = 0;
	m_iSpeed = 100;
	m_iEffclock = 0;
	m_iTime = 0;

	m_pThread = new tl4::CThread( this );

}


/*---------------------------------------------------------------------------
		デストラクタ
---------------------------------------------------------------------------*/

PpcCore::~PpcCore()
{
	Cleanup();
	delete m_pThread;
}


/*---------------------------------------------------------------------------
		初期化
---------------------------------------------------------------------------*/

bool PpcCore::Init( PpcUI *pUI, HWND hWnd, Draw *pDraw, DiskManager *pDiskmgr,
						PC8801::PpcKeyIF *pKeyb )
{
	m_pUI = pUI;
	m_hWnd = hWnd;

	if( !PC88::Init( pDraw, pDiskmgr ) )
	{
		return false;
	}

	if( !ConnectDevices( pKeyb ) )
	{
		return false;
	}

	if( m_pThread->Start() == FALSE )
	{
		return false;
	}

	return true;
}


/*--------------------------------------------------------------------------
		デバイス接続
--------------------------------------------------------------------------*/

bool PpcCore::ConnectDevices(PC8801::PpcKeyIF* keyb)
{
	const static IOBus::Connector c_keyb[] = 
	{
//		{ PC88::pres, IOBus::portout, PpcKeyIF::reset },
//		{ PC88::vrtc, IOBus::portout, PpcKeyIF::vsync },
		{ 0x00, IOBus::portin, PpcKeyIF::in },
		{ 0x01, IOBus::portin, PpcKeyIF::in },
		{ 0x02, IOBus::portin, PpcKeyIF::in },
		{ 0x03, IOBus::portin, PpcKeyIF::in },
		{ 0x04, IOBus::portin, PpcKeyIF::in },
		{ 0x05, IOBus::portin, PpcKeyIF::in },
		{ 0x06, IOBus::portin, PpcKeyIF::in },
		{ 0x07, IOBus::portin, PpcKeyIF::in },
		{ 0x08, IOBus::portin, PpcKeyIF::in },
		{ 0x09, IOBus::portin, PpcKeyIF::in },
		{ 0x0a, IOBus::portin, PpcKeyIF::in },
		{ 0x0b, IOBus::portin, PpcKeyIF::in },
		{ 0x0c, IOBus::portin, PpcKeyIF::in },
		{ 0x0d, IOBus::portin, PpcKeyIF::in },
		{ 0x0e, IOBus::portin, PpcKeyIF::in },
		{ 0x0f, IOBus::portin, PpcKeyIF::in },
		{ 0, 0, 0 }
	};
	if (!bus1.Connect(keyb, c_keyb)) return false;

	return true;
}


/*--------------------------------------------------------------------------
		後始末
--------------------------------------------------------------------------*/

bool PpcCore::Cleanup()
{
bool blRet = true;

	if( m_pThread->IsRunning() )
	{
		m_blThreadLoop = false;
		if( ::WaitForSingleObject( *m_pThread, 3000 ) == WAIT_TIMEOUT )
		{
			m_pThread->Terminate( 0 );
		}
	}


	return blRet;
}


/*--------------------------------------------------------------------------
		リセット
--------------------------------------------------------------------------*/

void PpcCore::Reset()
{
	CriticalSection::Lock lock( m_cscpu );
	PC88::Reset();
}


/*--------------------------------------------------------------------------
		設定を反映させる
--------------------------------------------------------------------------*/

void PpcCore::ApplyConfig(PC8801::Config *pConfig)
{
	PC88::ApplyConfig( pConfig );
}


/*--------------------------------------------------------------------------
		処理スレッドの開始／停止
--------------------------------------------------------------------------*/

void PpcCore::Activate( bool blActive )
{
CriticalSection::Lock lock( m_cscpu );

	m_blActive = blActive;
}

/*--------------------------------------------------------------------------
		処理スレッド
--------------------------------------------------------------------------*/

DWORD PpcCore::Run()
{
	m_iTime = GetMachineTime();
	m_iEffclock = 0;

	m_blThreadLoop = true;
	while( m_blThreadLoop )
	{
		if( m_blActive )
		{
			ExecuteAsynchronus();		//	VSYNC 非同期モードのみ
		} else {
			::Sleep( 5 );
			m_iTime = GetMachineTime();
		}
	}


	return 0;
}



void PpcCore::ExecuteAsynchronus()
{
	if( m_iClock <= 0 )
	{
	DWORD dwMS;
	int iEclk = 0;

		m_iTime = GetMachineTime();

		do
		{
			if( m_iClock < 0 )
			{
				Execute( -1 * m_iClock, 500, m_iEffclock );
			} else {
				Execute( m_iEffclock, 500, m_iEffclock );
			}
			iEclk += 5;
			dwMS = GetMachineTime() - m_iTime;
		} while( dwMS < 10 );

		//	実効クロック計算
		m_iEffclock = Min( ( Min( 1000, m_iEffclock ) * iEclk / dwMS ) + 1,
							10000 );
	} else {
		int iDt = GetMachineTime() - m_iTime;
		int iMS = Limit( iDt, 50, 4 );
		m_iTime += iDt;
		Execute( m_iClock, iMS * m_iSpeed, m_iClock * m_iSpeed / 100 );

		int iIdleTime = m_iTime - GetMachineTime() + 10;
		if( iIdleTime > 0 )
		{
			::Sleep( iIdleTime );
		}
	}
}


void PpcCore::Execute( long lClock, long lLength, long lEffclock )
{
CriticalSection::Lock lock( m_cscpu );

	m_iExeccount += lClock * Proceed( lLength, lClock, lEffclock );
}

