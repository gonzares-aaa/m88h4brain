#include "headers.h"
#include "compiler.h"
#include <windows.h>
#include "dosio.h"
#include "pc88.h"
#include "statsave.h"
#include "quicksave.h"

static void quicksave_getpath(OEMCHAR *path, int slot) {
    OEMCHAR filename[MAX_PATH];
    TCHAR wpath[MAX_PATH]; // Windows CE は標準で Unicode (WCHAR)

    // OSのAPIを使って、実行ファイルのフルパスを取得
    GetModuleFileName(NULL, wpath, MAX_PATH);

    // Unicode から char (OEMCHAR / SJIS) に変換して path に格納
    WideCharToMultiByte(CP_ACP, 0, wpath, -1, path, MAX_PATH, NULL, NULL);

    // ファイル名部分をカットしてディレクトリパスだけにする
    file_cutname(path);
    
    // セーブファイル名を作成: m88hstat.001, m88hstat.002, ... など
    OEMSPRINTF(filename, OEMTEXT("m88hstat.%03d"), slot + 1);
    
    // パスとファイル名を結合
    file_catname(path, filename, MAX_PATH);
}

int quicksave_save(int slot, PC88* pc88, DiskManager* diskmgr) {
    OEMCHAR path[MAX_PATH];
    
    if (slot < 0 || slot >= QUICKSAVE_SLOTS) {
        return STATFLAG_FAILURE;
    }
    quicksave_getpath(path, slot);
	return statsave_save(path, pc88, diskmgr); // 引数を追加
}

int quicksave_load(int slot, PC88* pc88, DiskManager* diskmgr) {
    OEMCHAR path[MAX_PATH];
    
    if (slot < 0 || slot >= QUICKSAVE_SLOTS) {
        return STATFLAG_FAILURE;
    }
    quicksave_getpath(path, slot);
    if (!quicksave_is_available(slot)) {
        return STATFLAG_FAILURE;
    }
	return statsave_load(path, pc88, diskmgr); // 引数を追加
}

int quicksave_is_available(int slot) {
    OEMCHAR path[MAX_PATH];
    
    if (slot < 0 || slot >= QUICKSAVE_SLOTS) {
        return 0;
    }
    quicksave_getpath(path, slot);
    
    // xmil4brain の dosio の仕組みを使う (ファイルが存在すれば -1 以外が返る)
    if (file_attr(path) != -1) {
        return 1;
    }
    return 0;
}