#include "headers.h"
#include "compiler.h"
#include <windows.h>
#include "dosio.h"
#include "pc88.h"
#include "statsave.h"
#include "quicksave.h"

// 内部データ用 (char型)
static const char* get_filename_only(const char* path) {
    if (!path || !path[0]) return "(None)";
    const char* p = strrchr(path, '\\');
    if (!p) p = strrchr(path, '/');
    return p ? p + 1 : path;
}

void quicksave_getpath(char *path, int slot) {
    TCHAR* wpath = new TCHAR[MAX_PATH];
    TCHAR* filename = new TCHAR[MAX_PATH];

    GetModuleFileName(NULL, wpath, MAX_PATH);
    TCHAR *p = wcsrchr(wpath, L'\\');
    if (p) *(p + 1) = L'\0';
    
    wsprintf(filename, L"m88hstat.%03d", slot + 1);
    wcscat(wpath, filename);

    // コアで使うためにANSI(char)へ変換
    WideCharToMultiByte(CP_ACP, 0, wpath, -1, path, MAX_PATH, NULL, NULL);

    delete[] filename;
    delete[] wpath;
}

int quicksave_save(int slot, PC88* pc88, DiskManager* diskmgr) {
    if (slot < 0 || slot >= QUICKSAVE_SLOTS) return STATFLAG_FAILURE;
    
    char* path = new char[MAX_PATH];
    quicksave_getpath(path, slot);
    
    int ret = statsave_save(path, pc88, diskmgr);
    
    delete[] path;
    return ret;
}

int quicksave_load(int slot, PC88* pc88, DiskManager* diskmgr) {
    if (slot < 0 || slot >= QUICKSAVE_SLOTS) return STATFLAG_FAILURE;
    if (!quicksave_is_available(slot)) return STATFLAG_FAILURE;
    
    char* path = new char[MAX_PATH];
    quicksave_getpath(path, slot);
    
    int ret = statsave_load(path, pc88, diskmgr);
    
    delete[] path;
    return ret;
}

int quicksave_is_available(int slot) {
    if (slot < 0 || slot >= QUICKSAVE_SLOTS) return 0;
    
    char* path = new char[MAX_PATH];
    TCHAR* wpath = new TCHAR[MAX_PATH];
    
    quicksave_getpath(path, slot);
    MultiByteToWideChar(CP_ACP, 0, path, -1, wpath, MAX_PATH);
    
    HANDLE hFile = CreateFile(wpath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    
    // ハンドル判定前にヒープメモリを解放
    delete[] wpath;
    delete[] path;
    
    if (hFile != INVALID_HANDLE_VALUE) {
        CloseHandle(hFile);
        return 1;
    }
    return 0;
}

// --- ダイアログ付き実行ラッパー ---

bool quicksave_do_save_with_dialog(int slot, HWND hWnd, PC88* pc88, DiskManager* diskmgr) {
    if (quicksave_is_available(slot)) {
        char* filename = new char[MAX_PATH];
        quicksave_getpath(filename, slot); 
        
        QS_METAINFO meta;
        TCHAR* msg = new TCHAR[1024];
        
        if (statsave_read_meta(filename, &meta) == STATFLAG_SUCCESS) {
            WCHAR* fdd0_w = new WCHAR[MAX_PATH];
            WCHAR* fdd1_w = new WCHAR[MAX_PATH];
            wcscpy(fdd0_w, L"(None)");
            wcscpy(fdd1_w, L"(None)");
            
            const char* d0_str = get_filename_only(meta.fdd0);
            const char* d1_str = get_filename_only(meta.fdd1);

            if (d0_str && d0_str[0]) MultiByteToWideChar(CP_ACP, 0, d0_str, -1, fdd0_w, MAX_PATH);
            if (d1_str && d1_str[0]) MultiByteToWideChar(CP_ACP, 0, d1_str, -1, fdd1_w, MAX_PATH);

            wsprintf(msg, L"Overwrite Slot %d?\n[Existing Data]\n%04d/%02d/%02d %02d:%02d\nFDD0: %s\nFDD1: %s", 
                slot + 1, meta.year, meta.month, meta.day, meta.hour, meta.minute, fdd0_w, fdd1_w);
            
            delete[] fdd1_w;
            delete[] fdd0_w;
        } else {
            wsprintf(msg, L"Overwrite Slot %d?\n(Old format data exists)", slot + 1);
        }
        
        int msgResult = MessageBox(hWnd, msg, L"Confirm Overwrite", MB_YESNO | MB_ICONQUESTION);
        
        // MessageBox後にヒープメモリを解放
        delete[] msg;
        delete[] filename;
        
        if (msgResult != IDYES) return false;
    }

    if (quicksave_save(slot, pc88, diskmgr) != STATFLAG_SUCCESS) {
        MessageBox(hWnd, L"Save failed.", L"Error", MB_OK | MB_ICONSTOP);
        return false;
    }
    
    return true;
}

bool quicksave_do_load_with_dialog(int slot, HWND hWnd, PC88* pc88, DiskManager* diskmgr) {
    if (!quicksave_is_available(slot)) {
        MessageBox(hWnd, L"No save data in this slot.", L"Information", MB_OK | MB_ICONEXCLAMATION);
        return false;
    }

    char* filename = new char[MAX_PATH];
    quicksave_getpath(filename, slot); 

    QS_METAINFO meta;
    TCHAR* msg = new TCHAR[1024];
    
    if (statsave_read_meta(filename, &meta) == STATFLAG_SUCCESS) {
        WCHAR* fdd0_w = new WCHAR[MAX_PATH];
        WCHAR* fdd1_w = new WCHAR[MAX_PATH];
        wcscpy(fdd0_w, L"(None)");
        wcscpy(fdd1_w, L"(None)");
        
        const char* d0_str = get_filename_only(meta.fdd0);
        const char* d1_str = get_filename_only(meta.fdd1);

        if (d0_str && d0_str[0]) MultiByteToWideChar(CP_ACP, 0, d0_str, -1, fdd0_w, MAX_PATH);
        if (d1_str && d1_str[0]) MultiByteToWideChar(CP_ACP, 0, d1_str, -1, fdd1_w, MAX_PATH);

        wsprintf(msg, L"Load Slot %d?\n\n%04d/%02d/%02d %02d:%02d\nFDD0: %s\nFDD1: %s", 
            slot + 1, meta.year, meta.month, meta.day, meta.hour, meta.minute, fdd0_w, fdd1_w);
        
        delete[] fdd1_w;
        delete[] fdd0_w;
    } else {
        wsprintf(msg, L"Load Slot %d?\n(Old format data)", slot + 1);
    }

    int msgResult = MessageBox(hWnd, msg, L"Confirm Load", MB_YESNO | MB_ICONQUESTION);
    
    // MessageBox後にヒープメモリを解放
    delete[] msg;
    delete[] filename;
    
    if (msgResult != IDYES) return false;

    if (quicksave_load(slot, pc88, diskmgr) != STATFLAG_SUCCESS) {
        MessageBox(hWnd, L"Load failed.", L"Error", MB_OK | MB_ICONSTOP);
        return false;
    }
    
    return true;
}