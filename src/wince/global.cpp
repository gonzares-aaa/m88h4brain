/*===========================================================================
		M88 for PocketPC / Handheld PC
		Original Version (c)cisc
		WindowsCE Version Programmed by Y.Taniuchi ( TAN-Y )

		グローバル系
===========================================================================*/

#include "m88ce.h"
#include "strcv.h"

#define LINEBUF_MAX 255
static char g_szReadLineBuf[LINEBUF_MAX + 1];

/*---------------------------------------------------------------------------
		ファイルから一行読み込むサブルーチン
---------------------------------------------------------------------------*/

const char *ReadLine(FileIO *pFile)
{
char *szTemp = g_szReadLineBuf;
const char *szRet = g_szReadLineBuf;

	for( int i = 0; i <= LINEBUF_MAX; i++ )
	{
		if( pFile->Read( szTemp, 1 ) < 1 )
		{
			szRet = NULL;
			break;
		}
		if( *szTemp < 0x20 )
		{
			if( *szTemp == '\n' )
			{
				break;
			}
		} else {
			szTemp++;
		}
	}
	*szTemp = 0;

	return szRet;
}


/*---------------------------------------------------------------------------
		assert 実装
---------------------------------------------------------------------------*/

#ifndef  NDEBUG
#ifdef  __cplusplus
extern "C" {
#endif

void __cdecl _assert(void *expr, void *file, unsigned line)
{
TCHAR szNum[16];
LPWSTR wszMsg;

	::OutputDebugString( _T("assert : ") );

	wszMsg = MultiToWide( (char *)expr );
	::OutputDebugString( wszMsg );
	delete[] wszMsg;
	::OutputDebugString( _T(" : file = ") );

	wszMsg = MultiToWide( (char *)file );
	::OutputDebugString( wszMsg );
	delete[] wszMsg;
	::OutputDebugString( _T(" : line = ") );

	::wsprintf( szNum, _T("%08d"), line );
	::OutputDebugString( szNum );
	::OutputDebugString( _T("\n") );
}

#ifdef  __cplusplus
}
#endif
#endif


