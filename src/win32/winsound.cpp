// ---------------------------------------------------------------------------
//	M88 - PC-88 Emulator
//	Copyright (C) cisc 1997, 1999.
// ---------------------------------------------------------------------------
//	$Id: winsound.cpp,v 1.21 1999/11/26 10:14:13 cisc Exp $

#include "headers.h"
#include "WinSound.h"
#include "misc.h"
#include "pc88/config.h"
#include "status.h"
#include "soundds.h"
#include "soundwo.h"

//#define LOGNAME "winsound"
#include "diag.h"

using namespace PC8801;
using namespace WinSoundDriver;

// ---------------------------------------------------------------------------
//	構築/消滅
//
WinSound::WinSound()
: driver(0), hmmio(0)
{
	dumping = 0;
}

WinSound::~WinSound()
{
	DumpEnd();
	Cleanup();
}

// ---------------------------------------------------------------------------
//	初期化
//
bool WinSound::Init
(PC88* pc, HWND hwindow, uint rate, uint32 clock, uint buflen)
{
	currentrate = 100;
	currentbuflen = 0;
	hwnd = hwindow;
	
	if (!Sound::Init(pc, 8000, clock, 0))
		return false;
	return true;
}

// ---------------------------------------------------------------------------
//	後処理
//
void WinSound::Cleanup()
{
	if (driver)
	{
		driver->Cleanup();
		delete driver;
		driver = 0;
	}
	Sound::Cleanup();
}

// ---------------------------------------------------------------------------
//	合成・再生レート変更
//
bool WinSound::ChangeRate(uint rate, uint32 clock, uint buflen, bool waveout)
{
	if (currentrate != rate || currentbuflen != buflen || wodrv != waveout)
	{
		if (dumping)
		{
			statusdisplay.Show(70, 3000, "音設定の変更は出力中にはできません");
			return false;
		}

		bool resetdriver = true;
		if (currentrate + rate == 44100 + SOUND_55K && wodrv == waveout)
			resetdriver = false;

		samprate = rate;
		currentrate = rate;
		currentbuflen = buflen;
		wodrv = waveout;
		
		if (rate == SOUND_55K)
		{
			samprate = 44100;
		}
		else if (rate < 8000)
		{
			rate = 100; samprate = 0;
		}
		
		// DirectSound: サンプリングレート * バッファ長 / 2
		// waveOut:     サンプリングレート * バッファ長 * 2
		int bufsize;
		if (wodrv)
			bufsize = (samprate * buflen / 1000 * 1) & ~15;
		else
			bufsize = (samprate * buflen / 1000 / 2) & ~15;
		
		if (resetdriver)
		{
			if (driver)
			{
				driver->Cleanup();
				delete driver;
				driver = 0;
			}

			if (rate < 1000)
				bufsize = 0;
		}
			
		if (!SetRate(rate, clock, bufsize))
			return false;
		
		if (resetdriver && bufsize > 0)
		{
			if (wodrv)
				driver = new DriverWO;
			else
				driver = new DriverDS;
			
			if (!driver || !driver->Init(this, hwnd, samprate, 2, buflen))
			{
				statusdisplay.Show(70, 3000, "オーディオデバイスを使用できません");
				delete driver; driver = 0;
				SetRate(rate, clock, 0);
			}
		}
	}
	return true;
}

// ---------------------------------------------------------------------------
//	設定更新
//
void WinSound::ApplyConfig(const Config* config)
{
	static const uint srate[] = { 0, 11025, 22050, 44100, SOUND_55K, 48000, 55467 };
	
	bool wo = (config->flag2 & Config::usewaveoutdrv) != 0;
	ChangeRate(srate[Limit(config->sound, 6, 0)], config->opnclock, config->soundbuffer, wo);
	
	if (driver)
		driver->MixAlways(0 != (config->flags & Config::mixsoundalways));
	
	Sound::ApplyConfig(config);
}

// ---------------------------------------------------------------------------
//	ダンプ開始
//
bool WinSound::DumpBegin(char* filename)
{
	if (hmmio)
		return false;
	if (!driver)
	{
		statusdisplay.Show(70, 3000, "無音モードでは PCM の書き出しは行えません");
		return false;
	}

	memset(&ckparent, 0, sizeof(MMCKINFO));
	memset(&ckdata, 0, sizeof(MMCKINFO));

	// mmioOpen
	hmmio = mmioOpen(filename, NULL, MMIO_CREATE | MMIO_WRITE | MMIO_ALLOCBUF); 
	if (!hmmio)
		return false;
	
	// WAVE chunk
	ckparent.fccType = mmioFOURCC('W', 'A', 'V', 'E');
	if (mmioCreateChunk(hmmio, &ckparent, MMIO_CREATERIFF))
	{
		mmioClose(hmmio, 0);
		hmmio = 0;
		return false;
	}

	// fmt chunk
	MMCKINFO cksub;
	memset(&cksub, 0, sizeof(MMCKINFO));
	cksub.ckid = mmioFOURCC('f', 'm', 't', ' ');
	mmioCreateChunk(hmmio, &cksub, 0);
	
	WAVEFORMATEX format;
	format.wFormatTag = WAVE_FORMAT_PCM;
	format.nChannels = 2;
	format.nSamplesPerSec = samprate;
	format.wBitsPerSample = 16;
	format.nAvgBytesPerSec = format.nChannels * format.nSamplesPerSec * 2;
	format.nBlockAlign = format.nChannels * 2;
	format.cbSize = 0;

	mmioWrite(hmmio, HPSTR(&format), sizeof(format));
	mmioAscend(hmmio, &cksub, 0);

	// data chunk
	ckdata.ckid = mmioFOURCC('d', 'a', 't', 'a');
	mmioCreateChunk(hmmio, &ckdata, 0);
	
	FillWhenEmpty(false);
	dumping = 1;
	dumpedsample = 0;
	statusdisplay.Show(100, 0, "録音待機中～");
	return true;
}

// ---------------------------------------------------------------------------
//	ダンプ終了
//
bool WinSound::DumpEnd()
{
	if (dumping)
	{
		dumping = 0;
		CriticalSection::Lock lock(cs);

		if (ckdata.dwFlags & MMIO_DIRTY) 
			mmioAscend(hmmio, &ckdata, 0);
		if (ckparent.dwFlags & MMIO_DIRTY)
			mmioAscend(hmmio, &ckparent, 0);
		if (hmmio)
			mmioClose(hmmio, 0), hmmio = 0;
		FillWhenEmpty(true);

		int curtime = dumpedsample / currentrate;
		statusdisplay.Show(100, 2500, "録音おわり～ [%.2d:%.2d]", curtime/60, curtime%60);
	}
	return true;
}

// ---------------------------------------------------------------------------
//	合成の場合
//
void WinSound::Mix(Sample* dest, int samples, Sample* dest2, int samples2)
{
	Sound::Mix(dest, samples, dest2, samples2);
	if (dumping)
	{
		CriticalSection::Lock lock(cs);
		if (dumping)
		{
			Dump(dest, samples);
			Dump(dest2, samples2);
		}
	}
}

void WinSound::Dump(Sample* dest, int samples)
{
	if (dumping == 1)
	{
		int i;
		uint32* s = (uint32*) dest;
		for (i=0; i<samples && !*s; i++, s++)
			;
		dest += i*2, samples -= i;
		if (samples > 0)
			dumping = 2;
	}

	if (samples)
	{
		mmioWrite(hmmio, (char*) dest, samples * sizeof(Sample) * 2);
		int prevtime = dumpedsample / currentrate;
		dumpedsample += samples;
		int curtime = dumpedsample / currentrate;
		if (prevtime != curtime)
		{
			statusdisplay.Show(101, 0, "録音中～ [%.2d:%.2d]", curtime/60, curtime%60);
		}
	}
}
