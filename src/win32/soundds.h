// ---------------------------------------------------------------------------
//	M88 - PC-88 Emulator
//	Copyright (C) cisc 1999.
// ---------------------------------------------------------------------------
//	DirectSound based driver
// ---------------------------------------------------------------------------
//	$Id: soundds.h,v 1.1 1999/06/30 14:07:03 cisc Exp $

#if !defined(win32_soundds_h)
#define win32_soundds_h

#include "sounddrv.h"

// ---------------------------------------------------------------------------

namespace WinSoundDriver
{

class DriverDS : public Driver
{
	static const uint num_blocks;
	static const uint timer_resolution;

public:
	DriverDS();
	~DriverDS();

	bool Init(SoundBuffer* sb, HWND hwnd, uint rate, uint ch, uint buflen);
	bool Cleanup();

private:
	static void CALLBACK TimeProc(UINT, UINT, DWORD, DWORD, DWORD);
	void Send();

	LPDIRECTSOUND lpds;
	LPDIRECTSOUNDBUFFER lpdsb_primary;
	LPDIRECTSOUNDBUFFER lpdsb;
	UINT timerid;
	LONG sending;
	
	uint nextwrite;
	uint buffer_length;
};

}

#endif // !defined(win32_soundds_h)
