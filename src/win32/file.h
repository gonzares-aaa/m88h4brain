// ---------------------------------------------------------------------------
//	M88 - PC-8801 Emulator
//	Copyright (C) cisc 1998, 1999.
// ---------------------------------------------------------------------------
//	generic file io class
// ---------------------------------------------------------------------------
//	$Id: file.h,v 1.6 1999/11/26 10:14:09 cisc Exp $

#if !defined(win32_file_h)
#define win32_file_h

#include "types.h"
#include "strcv.h"		//	add by TAN-Y

// ---------------------------------------------------------------------------

//	擬似カレントディレクトリ対応
class FileIO
{
public:
	enum Flags
	{
		open		= 0x000001,
		readonly	= 0x000002,
		create		= 0x000004,
	};

	enum SeekMethod
	{
		begin = 0, current = 1, end = 2,
	};

	enum Error
	{
		success = 0,
		file_not_found,
		sharing_violation,
		unknown = -1
	};

public:
	FileIO();
	FileIO(const char* filename, uint flg = 0);
	virtual ~FileIO();

	bool Open(const char* filename, uint flg = 0);
	bool CreateNew(const char* filename);
	bool Reopen(uint flg = 0);
	void Close();
	Error GetError() { return error; }

	int32 Read(void* dest, int32 len);
	int32 Write(const void* src, int32 len);
	bool Seek(int32 fpos, SeekMethod method);
	int32 Tellp();
	bool SetEndOfFile();

	uint GetFlags() { return flags; }
	void SetLogicalOrigin(int32 origin) { lorigin = origin; }

	static void SetCurrentDirectory( const char *szDirectory );

private:
	HANDLE hfile;
	uint flags;
	uint32 lorigin;
	Error error;
	char path[MAX_PATH];

	static char m_szCurrentDirectory[MAX_PATH];

	FileIO(const FileIO&);
	const FileIO& operator=(const FileIO&);

	void GetFullPath( char *szDst, const char *szFileName );
};

// ---------------------------------------------------------------------------

//	内部処理を Unicode 化
class FileFinder
{
public:
	FileFinder() : hff(INVALID_HANDLE_VALUE), searcher(0)
	{
		szFileName = NULL;
		szAlternateFileName = NULL;
	}
	
	~FileFinder()
	{
		free(searcher);
		free(szFileName);
		free(szAlternateFileName);
		if (hff != INVALID_HANDLE_VALUE)
			FindClose(hff);
	}
	
	bool FindFile(char* szSearch)
	{
		hff = INVALID_HANDLE_VALUE;
		free(searcher);
		free(szFileName);
		free(szAlternateFileName);
//		searcher = _strdup(szSearch);
		searcher = MultiToWide(szSearch);
		return searcher != 0;
	}

	bool FindNext()
	{
		if (!searcher)
			return false;
		if (hff == INVALID_HANDLE_VALUE)
		{
			hff = FindFirstFile(searcher, &wfd);
			
			if( hff != INVALID_HANDLE_VALUE )
			{
				szFileName = WideToMulti( wfd.cFileName );
				szAlternateFileName = WideToMulti( wfd.cFileName );
				return true;
			} else {
				return false;
			}
		}
		else
			if( FindNextFile(hff, &wfd) != 0 )
			{
				szFileName = WideToMulti( wfd.cFileName );
				szAlternateFileName = WideToMulti( wfd.cFileName );
				return true;
			} else {
				return false;
			}
	}

	const char* GetFileName()	{ return szFileName; }
	DWORD GetFileAttr()			{ return wfd.dwFileAttributes; }
	const char* GetAltName()	{ return szAlternateFileName; }

private:
	wchar_t* searcher;
	HANDLE hff;
	WIN32_FIND_DATA wfd;

	char *szFileName;
	char *szAlternateFileName;
};

#endif // 
