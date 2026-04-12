//
//	Unicode <-> MultiByte 文字列相互コンバータ
//	出力結果の文字列は結果受け取り先で free すること。
//
//	by TAN-Y
//

//

#include "headers.h"
#include "strcv.h"

LPSTR WideToMulti (LPCWSTR szSrc)
{
int iLen = ::WideCharToMultiByte( CP_ACP, 0,szSrc, -1, NULL, NULL,
									NULL, NULL );
LPSTR szResult = new CHAR[iLen];

	::WideCharToMultiByte( CP_ACP, 0, szSrc, -1, szResult, iLen, NULL, NULL );
	return szResult;
}


LPWSTR MultiToWide (LPCSTR szSrc)
{
int iLen = ::MultiByteToWideChar( CP_ACP, 0,szSrc, -1, NULL, NULL );
LPWSTR szResult = new WCHAR[iLen];

	::MultiByteToWideChar( CP_ACP, 0, szSrc, -1, szResult, iLen );
	return szResult;
}


