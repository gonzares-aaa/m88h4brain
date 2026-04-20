// ---------------------------------------------------------------------------
// quicksave.h (M88h4brain どこでもセーブ定義)
// ---------------------------------------------------------------------------
#ifndef QUICKSAVE_H
#define QUICKSAVE_H

#include <windows.h>
#include <tchar.h>
#include "pc88.h"
#include "diskmgr.h"

#ifdef __cplusplus
extern "C" {
#endif

#define QUICKSAVE_SLOTS 10

#pragma pack(push, 1)
typedef struct {
	char	fdd0[MAX_PATH];
	char	fdd1[MAX_PATH];
	unsigned short year;
	unsigned short month;
	unsigned short day;
	unsigned short hour;
	unsigned short minute;
} QS_METAINFO;
#pragma pack(pop)

extern QS_METAINFO qs_meta;

int quicksave_save(int slot, PC88* pc88, DiskManager* diskmgr);
int quicksave_load(int slot, PC88* pc88, DiskManager* diskmgr);
int quicksave_is_available(int slot);

void quicksave_getpath(char *path, int slot);
int statsave_read_meta(const char *filename, QS_METAINFO *meta);

bool quicksave_do_save_with_dialog(int slot, HWND hWnd, PC88* pc88, DiskManager* diskmgr);
bool quicksave_do_load_with_dialog(int slot, HWND hWnd, PC88* pc88, DiskManager* diskmgr);

#ifdef __cplusplus
}
#endif

#endif