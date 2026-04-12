//
//	Unicode <-> MultiByte 文字列相互コンバータ
//	出力結果の文字列は結果受け取り先で free すること。
//


#ifndef TANY_STRCV_H
#define TANY_STRCV_H

extern LPSTR WideToMulti (LPCWSTR szSrc);
extern LPWSTR MultiToWide (LPCSTR szSrc);


#endif
