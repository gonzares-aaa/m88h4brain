// ---------------------------------------------------------------------------
// quicksave.h (M88h4brain どこでもセーブ定義)
// ---------------------------------------------------------------------------
#ifndef QUICKSAVE_H
#define QUICKSAVE_H

#include "pc88.h"
#include "diskmgr.h"

#ifdef __cplusplus
extern "C" {
#endif

#define QUICKSAVE_SLOTS 5

// クイックセーブ/ロード関数 (PC88コアへのポインタを引数に追加)
int quicksave_save(int slot, PC88* pc88, DiskManager* diskmgr);
int quicksave_load(int slot, PC88* pc88, DiskManager* diskmgr);
int quicksave_is_available(int slot);

#ifdef __cplusplus
}
#endif

#endif