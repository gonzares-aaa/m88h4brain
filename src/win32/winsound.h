// ---------------------------------------------------------------------------
//	M88 - PC-88 Emulator
//	Copyright (C) cisc 1997, 1999.
// ---------------------------------------------------------------------------
//	Sound Implemention for Win32
// ---------------------------------------------------------------------------
//	$Id: winsound.h,v 1.12 1999/11/26 10:14:13 cisc Exp $

#if !defined(win32_winsound_h)
#define win32_winsound_h

#include "types.h"
#include "pc88/sound.h"
#include "sounddrv.h"
#include "critsect.h"

class PC88;

namespace PC8801
{

class Config;

class WinSound : public Sound
{
private:
public:
	WinSound();
	~WinSound();

	bool Init(PC88* pc, HWND hwnd, uint rate, uint32 clock, uint buflen);
	bool ChangeRate(uint rate, uint32 clock, uint buflen, bool wo);
	
	void ApplyConfig(const Config* config);

	bool DumpBegin(char* filename);
	bool DumpEnd();
	bool IsDumping() { return dumping != 0; }
	
private:
	bool InitSoundBuffer(LPDIRECTSOUND lpds, uint rate);
	void Cleanup();
	void Mix(Sample* dest, int samples, Sample* dest2, int samples2);
	void Dump(Sample* dest, int samples);

	WinSoundDriver::Driver* driver;
	
	HWND hwnd;
	uint currentrate;
	uint currentbuflen;
	uint samprate;
	
	HMMIO hmmio;					// mmio handle
	MMCKINFO ckparent;				// RIFF チャンク
	MMCKINFO ckdata;				// data チャンク

	CriticalSection cs;

	int dumping;
	int dumpedsample;
	bool wodrv;
};

}

#endif // !defined(win32_winsound_h)
