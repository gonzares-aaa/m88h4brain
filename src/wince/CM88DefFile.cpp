/*===========================================================================
		M88 for CE
		Original Version (c)cisc
		Pocket PC Version Programmed by Y.Taniuchi ( TAN-Y )

		CE 用コアクラス実装
===========================================================================*/

#include "m88ce.h"
#include "misc.h"
#include "file.h"

extern char *g_szConfKeyNames[];
extern const int g_aiDefValues[];

using namespace PC8801;

/*---------------------------------------------------------------------------
		コンストラクタ
---------------------------------------------------------------------------*/

CM88DefFile::CM88DefFile( const char *szFileName )
{
	m_szFileName = (char *)malloc( strlen( szFileName ) + 1 );
	if( m_szFileName != NULL )
	{
		strcpy( m_szFileName, szFileName );
	}
}


/*---------------------------------------------------------------------------
		デストラクタ
---------------------------------------------------------------------------*/

CM88DefFile::~CM88DefFile()
{
	if( m_szFileName != NULL )
	{
		free( m_szFileName );
		m_szFileName = NULL;
	}
}


/*---------------------------------------------------------------------------
		定義読み込み
		読めない時はデフォルト値設定。
---------------------------------------------------------------------------*/

bool CM88DefFile::Load()
{
FileIO file;

	//
	//	メンバ初期化
	//
	for( int i = 0; i < END_KEY; i++ )
	{
		m_aiValues[i] = g_aiDefValues[i];
	}

	//
	//	ファイル読み込み
	//

	if( file.Open( m_szFileName, FileIO::readonly ) )
	{
	const char *szLine = NULL;

		while( ( szLine = ReadLine( &file ) ) != NULL )
		{
			for( int i = 0; i < END_KEY; i++ )
			{
			int iLen = strlen( g_szConfKeyNames[i] );
				if( !strncmp( szLine, g_szConfKeyNames[i], iLen ) )
				{
//					iLen++;
					m_aiValues[i] = atoi( szLine + iLen );
					break;
				}
			}
		}
		file.Close();
	}


	//
	//	パラメータ補正
	//

	m_aiValues[Flag2] &= ~(Config::mask0 | Config::mask1 | Config::mask2);
	m_aiValues[CPUClock] = Limit( m_aiValues[CPUClock], 1000, 1 );
	m_aiValues[RefreshTiming] = Limit( m_aiValues[RefreshTiming], 4, 1 );
	m_aiValues[Sound] = Limit( m_aiValues[Sound], 6, 0 );
	m_aiValues[OPNClock] = Limit( m_aiValues[OPNClock], 10000000, 1000000 );
	m_aiValues[ERAMBank] = Limit( m_aiValues[ERAMBank], 256, 0 );
	m_aiValues[SoundBuffer] = Limit( m_aiValues[SoundBuffer], 1000, 100 );
	m_aiValues[MouseSensibility] =
		Limit( m_aiValues[MouseSensibility], 10, 1 );
	m_aiValues[CPUMode] = Limit( m_aiValues[CPUMode], 2, 0 );

	return true;
}


/*---------------------------------------------------------------------------
		定義保存
---------------------------------------------------------------------------*/

bool CM88DefFile::Save()
{
FileIO file;

	if( file.Open( m_szFileName, FileIO::create ) == false )
	{
		return false;
	}

	for( int i = 0; i < END_KEY; i++ )
	{
	int iLen;
	char szNumTemp[32];

		iLen = strlen(g_szConfKeyNames[i]);
		if( file.Write( g_szConfKeyNames[i], iLen ) < iLen )
		{
			return false;
		}
//		if( file.Write( "=", 1 ) < 1 )
//		{
//			return false;
//		}
		sprintf( szNumTemp, "%d\r\n", m_aiValues[i] );
		iLen = strlen(szNumTemp);
		if( file.Write( szNumTemp, iLen ) < iLen )
		{
			return false;
		}
	}

	file.Close();

	return true;
}




/*---------------------------------------------------------------------------
		定義キー名
---------------------------------------------------------------------------*/

#define VOLUME_BIAS 100

static char *g_szConfKeyNames[] = {
			"Flags=",
			"CPUClock=",
			"Speed=",
			"RefreshTiming=",
			"BASICMode=",
			"Sound=",
			"OPNClock=",
			"Switches=",
			"SoundBuffer=",
			"MouseSensibility=",
			"CPUMode=",
			"KeyboardType=",
			"VolumeFM=",
			"VolumeSSG=",
			"VolumeADPCM=",
			"VolumeRhythm=",
			"VolumeBD=",
			"VolumeSD=",
			"VolumeTOP=",
			"VolumeHH=",
			"VolumeTOM=",
			"VolumeRIM=",
			"Flag2=",
			"ERAMBank=",
			"DrawType="
		};

static const int g_aiDefValues[] = {
			Config::subcpucontrol | Config::savedirectory,
			40,
			1000,
			3,
			Config::N88V2,
			0,
			3993600,
			1829,
			200,
			4,
			Config::msauto,
			0,
			VOLUME_BIAS,
			VOLUME_BIAS,
			VOLUME_BIAS,
			VOLUME_BIAS,
			VOLUME_BIAS,
			VOLUME_BIAS,
			VOLUME_BIAS,
			VOLUME_BIAS,
			VOLUME_BIAS,
			VOLUME_BIAS,
			0,
			4,
			0
		};


/*
[M88p2 for Windows]
Directory=D:\mydata\EMU
Flags=13242395
CPUClock=80
Speed=1000
RefreshTiming=2
BASICMode=49
Sound=3
Switches=1829
SoundBuffer=200
MouseSensibility=4
CPUMode=2
KeyboardType=0
VolumeFM=100
VolumeSSG=100
VolumeADPCM=100
VolumeRhythm=100
VolumeBD=100
VolumeSD=100
VolumeTOP=100
VolumeHH=100
VolumeTOM=100
VolumeRIM=100
Flag2=0
ERAMBank=4
*/
