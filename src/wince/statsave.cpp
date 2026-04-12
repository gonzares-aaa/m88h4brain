// ---------------------------------------------------------------------------
// statsave.cpp (M88h4brain どこでもセーブ完全版)
// ---------------------------------------------------------------------------
#include "headers.h"
#include "compiler.h"
#include "dosio.h"
#include "pc88.h"
#include "pc88/memory.h"
#include "pc88/crtc.h"
#include "pc88/screen.h"
#include "pc88/pd8257.h"
#include "pc88/intc.h"
#include "pc88/fdc.h"
#include "pc88/subsys.h"
#include "pc88/opnif.h"
#include "pc88/calender.h"
#include "pc88/beep.h"
#include "pc88/kanjirom.h"
#include "statsave.h"
#include "diskmgr.h"

// ディスク状態保存用の構造体定義
struct FDDFileState {
    char filename[2][MAX_PATH];
    int index[2];
    int readonly[2];
};

// --- xmil4brain 由来の構造体・定数定義 ---

typedef struct {
    char    name[16];
    char    vername[28];
    UINT32  ver;
} NP2FHDR;

static const NP2FHDR m88flagdef = {
    "M88h4brain",
    "create by M88h",
    100
};

enum {
    STATFLAG_BIN    = 0,
    STATFLAG_TERM,
    STATFLAG_DISK,
    STATFLAG_EVT
};

enum {
    SFFILEH_WRITE   = 0x0001,
    SFFILEH_BLOCK   = 0x0002,
    SFFILEH_ERROR   = 0x0004
};

typedef struct {
    _STFLAGH    sfh;
    UINT        stat;
    FILEH       fh;
    UINT        secpos;
    NP2FHDR     f;
} _SFFILEH, *SFFILEH;


// --- xmil4brain 由来のファイルI/O機構 ---

static SFFILEH statflag_open(const OEMCHAR *filename, OEMCHAR *err, UINT errlen) {
    FILEH   fh;
    SFFILEH ret;

    fh = file_open_rb(filename);
    if (fh == FILEH_INVALID) goto sfo_err1;

    ret = (SFFILEH)_MALLOC(sizeof(_SFFILEH), filename);
    if (ret == NULL) goto sfo_err2;

    if ((file_read(fh, &ret->f, sizeof(NP2FHDR)) == sizeof(NP2FHDR)) &&
        (!memcmp(&ret->f, &m88flagdef, sizeof(m88flagdef)))) {
        ZeroMemory(ret, sizeof(_SFFILEH));
        ret->fh = fh;
        ret->secpos = sizeof(NP2FHDR);
        if ((err) && (errlen > 0)) {
            err[0] = '\0';
            ret->sfh.err = err;
            ret->sfh.errlen = errlen;
        }
        return(ret);
    }
    _MFREE(ret);

sfo_err2:
    file_close(fh);
sfo_err1:
    return(NULL);
}

static int statflag_closesection(SFFILEH sffh) {
    UINT    leng;
    UINT8   zero[16];

    if (sffh == NULL) goto sfcs_err1;

    if (sffh->stat == (SFFILEH_BLOCK | SFFILEH_WRITE)) {
        leng = (0 - sffh->sfh.hdr.size) & 15;
        if (leng) {
            ZeroMemory(zero, sizeof(zero));
            if (file_write(sffh->fh, zero, leng) != leng) goto sfcs_err2;
        }
        if ((file_seek(sffh->fh, (long)sffh->secpos, FSEEK_SET) != (long)sffh->secpos) ||
            (file_write(sffh->fh, &sffh->sfh.hdr, sizeof(sffh->sfh.hdr)) != sizeof(sffh->sfh.hdr))) {
            goto sfcs_err2;
        }
    }
    if (sffh->stat & SFFILEH_BLOCK) {
        sffh->stat &= ~SFFILEH_BLOCK;
        sffh->secpos += sizeof(sffh->sfh.hdr) + ((sffh->sfh.hdr.size + 15) & (~15));
        if (file_seek(sffh->fh, (long)sffh->secpos, FSEEK_SET) != (long)sffh->secpos) {
            goto sfcs_err2;
        }
    }
    return(STATFLAG_SUCCESS);

sfcs_err2:
    sffh->stat = SFFILEH_ERROR;
sfcs_err1:
    return(STATFLAG_FAILURE);
}

static int statflag_readsection(SFFILEH sffh) {
    int     ret;

    ret = statflag_closesection(sffh);
    if (ret != STATFLAG_SUCCESS) return(ret);

    if ((sffh->stat == 0) &&
        (file_read(sffh->fh, &sffh->sfh.hdr, sizeof(sffh->sfh.hdr)) == sizeof(sffh->sfh.hdr))) {
        sffh->stat = SFFILEH_BLOCK;
        sffh->sfh.pos = 0;
        return(STATFLAG_SUCCESS);
    }
    sffh->stat = SFFILEH_ERROR;
    return(STATFLAG_FAILURE);
}

int statflag_read(STFLAGH sfh, void *buf, UINT size) {
    if ((sfh == NULL) || (buf == NULL) || ((sfh->pos + size) > sfh->hdr.size)) {
        goto sfr_err;
    }
    if (size) {
        if (file_read(((SFFILEH)sfh)->fh, buf, size) != size) {
            goto sfr_err;
        }
        sfh->pos += size;
    }
    return(STATFLAG_SUCCESS);
sfr_err:
    return(STATFLAG_FAILURE);
}

static SFFILEH statflag_create(const OEMCHAR *filename) {
    SFFILEH ret;
    FILEH   fh;

    ret = (SFFILEH)_MALLOC(sizeof(_SFFILEH), filename);
    if (ret == NULL) goto sfc_err1;

    fh = file_create(filename);
    if (fh == FILEH_INVALID) goto sfc_err2;

    if (file_write(fh, &m88flagdef, sizeof(NP2FHDR)) == sizeof(NP2FHDR)) {
        ZeroMemory(ret, sizeof(_SFFILEH));
        ret->stat = SFFILEH_WRITE;
        ret->fh = fh;
        ret->secpos = sizeof(NP2FHDR);
        return(ret);
    }
    file_close(fh);
    file_delete(filename);

sfc_err2:
    _MFREE(ret);
sfc_err1:
    return(NULL);
}

static int statflag_createsection(SFFILEH sffh, const SFENTRY *tbl) {
    int     ret;

    ret = statflag_closesection(sffh);
    if (ret != STATFLAG_SUCCESS) return(ret);

    if (sffh->stat != SFFILEH_WRITE) {
        sffh->stat = SFFILEH_ERROR;
        return(STATFLAG_FAILURE);
    }
    CopyMemory(sffh->sfh.hdr.index, tbl->index, sizeof(sffh->sfh.hdr.index));
    sffh->sfh.hdr.ver = tbl->ver;
    sffh->sfh.hdr.size = 0;
    return(STATFLAG_SUCCESS);
}

int statflag_write(STFLAGH sfh, const void *buf, UINT size) {
    SFFILEH sffh;

    if (sfh == NULL) goto sfw_err1;

    sffh = (SFFILEH)sfh;
    if (!(sffh->stat & SFFILEH_WRITE)) goto sfw_err2;

    if (!(sffh->stat & SFFILEH_BLOCK)) {
        sffh->stat |= SFFILEH_BLOCK;
        sfh->pos = 0;
        if (file_write(sffh->fh, &sfh->hdr, sizeof(sfh->hdr)) != sizeof(sfh->hdr)) {
            goto sfw_err2;
        }
    }
    if (size) {
        if ((buf == NULL) || (file_write(sffh->fh, buf, size) != size)) goto sfw_err2;
        sfh->pos += size;
        if (sfh->hdr.size < sfh->pos) {
            sfh->hdr.size = sfh->pos;
        }
    }
    return(STATFLAG_SUCCESS);

sfw_err2:
    sffh->stat = SFFILEH_ERROR;
sfw_err1:
    return(STATFLAG_FAILURE);
}

static void statflag_close(SFFILEH sffh) {
    if (sffh) {
        statflag_closesection(sffh);
        file_close(sffh->fh);
        _MFREE(sffh);
    }
}

void statflag_seterr(STFLAGH sfh, const OEMCHAR *str) {
    if ((sfh) && (sfh->errlen)) {
        // 必要に応じて文字結合などを行う
    }
}

// --- デバイス連携用 テンプレート関数 ---
// 仮想関数の解決を避けるため、テンプレートで各クラスのメソッドに直接アクセスします

template <class T>
static bool SaveDeviceState(SFFILEH sffh, const char* name, T* dev) {
    if (!dev) return false;

    SFENTRY tbl;
    ZeroMemory(&tbl, sizeof(SFENTRY));
    strncpy(tbl.index, name, 9);
    tbl.type = STATFLAG_BIN;

    statflag_createsection(sffh, &tbl);
    
    uint size = dev->GetStatusSize();
    if (size > 0) {
        uint8* buf = new uint8[size];
        dev->SaveStatus(buf);
        statflag_write(&sffh->sfh, buf, size);
        delete[] buf;
    } else {
        statflag_write(&sffh->sfh, NULL, 0);
    }
    return true;
}

template <class T>
static void LoadDeviceState(STFLAGH sfh, T* dev) {
    if (!dev) return;
    
    uint size = dev->GetStatusSize();
    if (size > 0) {
        uint8* buf = new uint8[size];
        statflag_read(sfh, buf, size);
        dev->LoadStatus(buf);
        delete[] buf;
    }
}

// --- どこでもセーブ・ロード 本体 ---

int statsave_save(const OEMCHAR *filename, PC88* pc88, DiskManager* diskmgr) {
    SFFILEH sffh = statflag_create(filename);
    if (sffh == NULL) return STATFLAG_FAILURE;

	// FDDのファイルパス情報を保存
    if (diskmgr) {
        FDDFileState ffs;
        ZeroMemory(&ffs, sizeof(ffs));
        for (int i = 0; i < 2; i++) {
            const char* name = diskmgr->GetDiskName(i);
            if (name) {
                strncpy(ffs.filename[i], name, MAX_PATH);
                ffs.index[i] = diskmgr->GetCurrentDisk(i);
                ffs.readonly[i] = diskmgr->IsReadOnly(i) ? 1 : 0;
            } else {
                ffs.index[i] = -1; // -1 はディスク未挿入
            }
        }
        SFENTRY tbl_fdd;
        ZeroMemory(&tbl_fdd, sizeof(SFENTRY));
        strncpy(tbl_fdd.index, "FDDFILE", 9);
        tbl_fdd.type = STATFLAG_BIN;
        statflag_createsection(sffh, &tbl_fdd);
        statflag_write(&sffh->sfh, &ffs, sizeof(ffs));
    }

    // 各デバイスのセーブ
    SaveDeviceState(sffh, "CPU1",   pc88->GetCPU1());
    SaveDeviceState(sffh, "CPU2",   pc88->GetCPU2());
    SaveDeviceState(sffh, "MEM1",   pc88->GetMem1());
    SaveDeviceState(sffh, "CRTC",   pc88->GetCRTC());
	SaveDeviceState(sffh, "SCRN",   pc88->GetScreen());
    SaveDeviceState(sffh, "DMAC",   pc88->GetDMAC());
    SaveDeviceState(sffh, "INTC",   pc88->GetINTC());
    SaveDeviceState(sffh, "FDC",    pc88->GetFDC());
    SaveDeviceState(sffh, "OPN1",   pc88->GetOPN1());
    SaveDeviceState(sffh, "OPN2",   pc88->GetOPN2());
    SaveDeviceState(sffh, "SUBSYS", pc88->GetSubSys());
	SaveDeviceState(sffh, "CALN",   pc88->GetCalender());
    SaveDeviceState(sffh, "BEEP",   pc88->GetBEEP());
    SaveDeviceState(sffh, "KNJ1",   pc88->GetKanji1());
    SaveDeviceState(sffh, "KNJ2",   pc88->GetKanji2());
	
    // 終端マーク
    SFENTRY tbl_term;
    ZeroMemory(&tbl_term, sizeof(SFENTRY));
    strncpy(tbl_term.index, "TERMINATE", 9);
    tbl_term.type = STATFLAG_TERM;
    statflag_createsection(sffh, &tbl_term);
    
    statflag_close(sffh);
    return STATFLAG_SUCCESS;
}

int statsave_load(const OEMCHAR *filename, PC88* pc88, DiskManager* diskmgr) {
    SFFILEH sffh = statflag_open(filename, NULL, 0);
    if (sffh == NULL) return STATFLAG_FAILURE;

    int ret = STATFLAG_SUCCESS;
    bool done = false;

    // ※サウンドの一時停止が必要な場合はここで実行

    while (!done && ret != STATFLAG_FAILURE) {
        ret |= statflag_readsection(sffh);
        if (ret != STATFLAG_SUCCESS) break;

        const char* index = sffh->sfh.hdr.index;

        if (strncmp(index, "TERMINATE", 9) == 0) {
            done = true;
            break;
        }

        // ヘッダ名から対象デバイスを判別してロード
        if      (strncmp(index, "CPU1", 4) == 0)   { LoadDeviceState(&sffh->sfh, pc88->GetCPU1()); }
        else if (strncmp(index, "CPU2", 4) == 0)   { LoadDeviceState(&sffh->sfh, pc88->GetCPU2()); }
        else if (strncmp(index, "MEM1", 4) == 0)   { LoadDeviceState(&sffh->sfh, pc88->GetMem1()); }
        else if (strncmp(index, "CRTC", 4) == 0)   { LoadDeviceState(&sffh->sfh, pc88->GetCRTC()); }
		else if (strncmp(index, "SCRN", 4) == 0)   { LoadDeviceState(&sffh->sfh, pc88->GetScreen()); }
        else if (strncmp(index, "DMAC", 4) == 0)   { LoadDeviceState(&sffh->sfh, pc88->GetDMAC()); }
        else if (strncmp(index, "INTC", 4) == 0)   { LoadDeviceState(&sffh->sfh, pc88->GetINTC()); }
        else if (strncmp(index, "FDC", 3) == 0)    { LoadDeviceState(&sffh->sfh, pc88->GetFDC()); }
        else if (strncmp(index, "OPN1", 4) == 0)   { LoadDeviceState(&sffh->sfh, pc88->GetOPN1()); }
        else if (strncmp(index, "OPN2", 4) == 0)   { LoadDeviceState(&sffh->sfh, pc88->GetOPN2()); }
        else if (strncmp(index, "SUBSYS", 6) == 0) { LoadDeviceState(&sffh->sfh, pc88->GetSubSys()); }
		else if (strncmp(index, "CALN", 4) == 0)   { LoadDeviceState(&sffh->sfh, pc88->GetCalender()); }
        else if (strncmp(index, "BEEP", 4) == 0)   { LoadDeviceState(&sffh->sfh, pc88->GetBEEP()); }
        else if (strncmp(index, "KNJ1", 4) == 0)   { LoadDeviceState(&sffh->sfh, pc88->GetKanji1()); }
        else if (strncmp(index, "KNJ2", 4) == 0)   { LoadDeviceState(&sffh->sfh, pc88->GetKanji2()); }
        else if (strncmp(index, "FDDFILE", 7) == 0) {
            if (diskmgr) {	// ディスクの再マウント処理
                FDDFileState ffs;
                statflag_read(&sffh->sfh, &ffs, sizeof(ffs));
                for (int i = 0; i < 2; i++) {
                    if (ffs.index[i] >= 0 && ffs.filename[i][0] != '\0') {
                        diskmgr->Mount(i, ffs.filename[i], ffs.readonly[i] != 0, ffs.index[i], false);
                    } else {
                        diskmgr->Unmount(i);
                    }
                }
            }
        }
    }
    statflag_close(sffh);

    return ret;
}