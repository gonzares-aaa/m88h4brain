// ----------------------------------------------------------------------------
//	M88 - PC-8801 series emulator
//	Copyright (C) cisc 1999.
// ----------------------------------------------------------------------------
//	$Id: error.cpp,v 1.5 1999/12/12 12:40:38 cisc Exp $

#include "headers.h"
#include "error.h"

Error::Errno Error::err = Error::unknown;

const char* Error::ErrorText[Error::nerrors] =
{
	"原因不明のエラーが発生しました.",
	"PC88 の ROM ファイルが見つかりません.\nファイル名を確認してください.",
	"メモリの割り当てに失敗しました.",
	"画面の初期化に失敗しました.",
	"スレッドを作成できません.",
	"テキストフォントが見つかりません.",
	"実行ファイルに異常があります.\nウィルス感染が疑われます.",
};

const char* Error::GetErrorText()
{
	return ErrorText[err];
}

void Error::SetError(Errno e)
{
	err = e;
}
