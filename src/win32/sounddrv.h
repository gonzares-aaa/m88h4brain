// ---------------------------------------------------------------------------
//	M88 - PC-88 Emulator
//	Copyright (C) cisc 1999.
// ---------------------------------------------------------------------------
//	Sound Implemention for Win32
// ---------------------------------------------------------------------------
//	$Id: sounddrv.h,v 1.1 1999/06/30 14:07:03 cisc Exp $

#if !defined(win32_sounddrv_h)
#define win32_sounddrv_h

#include "types.h"
#include "common/soundbuf.h"

// ---------------------------------------------------------------------------

namespace WinSoundDriver
{

class Driver
{
public:
	typedef SoundBuffer::Sample Sample;
	
	Driver() {}
	virtual ~Driver() {}

	virtual bool Init(SoundBuffer* sb, HWND hwnd, uint rate, uint ch, uint buflen) = 0;
	virtual bool Cleanup() = 0;
	void MixAlways(bool yes) { mixalways = yes; }

protected:
	SoundBuffer* src;
	uint buffersize;
	uint sampleshift;
	bool playing;
	bool mixalways;
};

}

#endif // !defined(win32_sounddrv_h)
