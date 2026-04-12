// ---------------------------------------------------------------------------
// statsave.h (M88h4brain どこでもセーブ定義)
// ---------------------------------------------------------------------------
#ifndef STATSAVE_H
#define STATSAVE_H

#include "compiler.h"
#include "pc88.h"
#include "diskmgr.h"

enum {
	STATFLAG_SUCCESS	= 0,
	STATFLAG_DISKCHG	= 0x0001,
	STATFLAG_VERCHG		= 0x0002,
	STATFLAG_WARNING	= 0x0080,
	STATFLAG_VERSION	= 0x0100,
	STATFLAG_FAILURE	= -1
};

typedef struct {
	char		index[10];
	UINT16		ver;
	UINT32		size;
} STFLAGHDR;

typedef struct {
	STFLAGHDR	hdr;
	UINT		pos;
	OEMCHAR		*err;
	int			errlen;
} _STFLAGH, *STFLAGH;

typedef struct {
	char	index[10];
	UINT16	ver;
	UINT16	type;
	void	*arg1;
	UINT	arg2;
} SFENTRY;

#ifdef __cplusplus
extern "C" {
#endif

// ファイルI/O系のガワはC言語互換で宣言
int statflag_read(STFLAGH sfh, void *ptr, UINT size);
int statflag_write(STFLAGH sfh, const void *ptr, UINT size);
void statflag_seterr(STFLAGH sfh, const OEMCHAR *str);

#ifdef __cplusplus
}
#endif

// PC88クラス(C++)のポインタを渡すため、extern "C" の外に配置します
int statsave_save(const OEMCHAR *filename, PC88* pc88, DiskManager* diskmgr);
int statsave_load(const OEMCHAR *filename, PC88* pc88, DiskManager* diskmgr);

#endif // STATSAVE_H