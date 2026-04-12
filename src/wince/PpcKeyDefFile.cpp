/*===========================================================================
		M88 for PocketPC
		Original Version (c)cisc
		Pocket PC Version Programmed by Y.Taniuchi ( TAN-Y )

		キー定義ファイルクラス実装
===========================================================================*/

#include "m88p.h"

/*---------------------------------------------------------------------------
		コンストラクタ
---------------------------------------------------------------------------*/

PpcKeyDefFile::PpcKeyDefFile( const char *szFileName, GXKeyList *pGXkl )
{
	m_szFileName = NULL;
	SetFileName( szFileName );

	m_pGXkl = pGXkl;
	InitKey();
}


/*---------------------------------------------------------------------------
		デストラクタ
---------------------------------------------------------------------------*/

PpcKeyDefFile::~PpcKeyDefFile()
{
	if( m_szFileName != NULL )
	{
		free( m_szFileName );
		m_szFileName = NULL;
	}
}


/*---------------------------------------------------------------------------
		ファイル名設定
---------------------------------------------------------------------------*/

void PpcKeyDefFile::SetFileName( const char *szFileName )
{
	if( m_szFileName != NULL )
	{
		free( m_szFileName );
		m_szFileName = NULL;
	}

	m_szFileName = (char *)malloc( strlen( szFileName ) + 1 );
	if( m_szFileName != NULL )
	{
		strcpy( m_szFileName, szFileName );
	}
}


/*---------------------------------------------------------------------------
		キー定義の初期化
---------------------------------------------------------------------------*/

void PpcKeyDefFile::InitKey( void )
{
	for( int i = 0; i < 256; i++ )
	{
		m_aiKeyCode[i] = -1;
	}

	if( m_pGXkl == NULL )
	{
		return;
	}

	//
	//	デフォルトのキー配置をセット。
	//
	m_aiKeyCode[m_pGXkl->vkUp] = 0x57;			//	tenkey 6
	m_aiKeyCode[m_pGXkl->vkDown] = 0x55;		//	tenkey 4
	m_aiKeyCode[m_pGXkl->vkLeft] = 0x52;		//	tenkey 8
	m_aiKeyCode[m_pGXkl->vkRight] = 0x5a;		//	tenkey 2
	m_aiKeyCode[m_pGXkl->vkStart] = 0x42;		//	ret
	m_aiKeyCode[m_pGXkl->vkA] = 0x40;			//	space
	m_aiKeyCode[m_pGXkl->vkB] = 0x42;			//	ret
	m_aiKeyCode[m_pGXkl->vkC] = 0x40;			//	space

	m_aiKeyCode[VK_F23] = 0x40;					//	space
	m_aiKeyCode[0xc1] = 0x42;					//	ret
}


/*---------------------------------------------------------------------------
		ロード
---------------------------------------------------------------------------*/

bool PpcKeyDefFile::Load()
{
FileIO file;

	//
	//	ファイル読み込み
	//

	if( file.Open( m_szFileName, FileIO::readonly ) )
	{
	const char *szLine = NULL;

		while( ( szLine = ReadLine( &file ) ) != NULL )
		{
			char *szVal = strchr( szLine, '=' );
			if( szVal != NULL )
			{
				*szVal = 0;
				szVal++;
				int iVal = atoi( szVal );
				if( iVal < KEYCODE_MIN || iVal > KEYCODE_MAX )
				{
					iVal = -1;
				}
				m_aiKeyCode[atoi( szLine )] = iVal;
			}
		}

		return true;
	}

	return false;
}


/*---------------------------------------------------------------------------
		セーブ
---------------------------------------------------------------------------*/

bool PpcKeyDefFile::Save()
{
FileIO file;

	if( file.Open( m_szFileName, FileIO::create ) == false )
	{
		return false;
	}

	for( int i = 0; i < 256; i++ )
	{
	char szNumTemp[64];

		if( m_aiKeyCode[i] != -1 )
		{
			sprintf( szNumTemp, "%d=%d\r\n", i, m_aiKeyCode[i] );
			int iLen = strlen( szNumTemp );
			if( file.Write( szNumTemp, iLen ) < iLen )
			{
				return false;
			}
		}
	}


	file.Close();

	return true;
}

