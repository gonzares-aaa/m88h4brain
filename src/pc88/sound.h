// ---------------------------------------------------------------------------
//	M88 - PC-8801 Emulator
//	Copyright (C) cisc 1997, 1999.
// ---------------------------------------------------------------------------
//	$Id: sound.h,v 1.24 1999/11/26 10:13:51 cisc Exp $

#ifndef PC88_SOUND_H
#define PC88_SOUND_H

#include "device.h"
#include "opna.h"
#include "psg.h"
#include "soundbuf.h"

// ---------------------------------------------------------------------------

#define SOUND_55K	55555	// Special

class PC88;
class Scheduler;

namespace PC8801
{
class Sound;
class Config;
class OPNIF;

class Sound 
: public Device, public SoundBuffer, public ISoundControl
{
public:
	enum IDFunc
	{
		updatecounter = 0,
	};

public:
	Sound();
	~Sound();
	
	bool Init(PC88* pc, uint rate, uint32 clock, int bufsize);
	void Cleanup();	
	
	void ApplyConfig(const Config* config);
	bool SetRate(uint rate, uint32 clock, int bufsize);

	void IOCALL UpdateCounter(uint);
	
	bool IFCALL Connect(ISoundSource* src);
	bool IFCALL Disconnect(ISoundSource* src);
	bool IFCALL Update(ISoundSource* src=0);
	int  IFCALL GetSubsampleTime(ISoundSource* src);
	
protected:
	void Mix(Sample* d1, int s1, Sample* d2, int s2);
	
	uint samplingrate;		// サンプリングレート
	uint rate50;			// samplingrate / 50
	long refcount;

private:
	struct SSNode
	{
		ISoundSource* ss;
		SSNode* next;
	};

	enum { ssrev = 1, };
	struct Status
	{
		uint8 rev;
		uint8 port40;
		uint32 prevtime;
	};

	PC88* pc;
	int32* mixingbuf;
	
	uint32 prevtime;
	uint32 cfgflg;
	int tdiff;
	uint mixthreshold;
	int buffersize;
	
	bool enabled;
	
	SSNode* sslist;
	CriticalSection cs_ss;
};

}


#endif // PC88_SOUND_H
