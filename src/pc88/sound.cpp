// ---------------------------------------------------------------------------
//	M88 - PC-8801 Emulator
//	Copyright (C) cisc 1997, 1999.
// ---------------------------------------------------------------------------
//	$Id: sound.cpp,v 1.25 1999/11/26 10:13:51 cisc Exp $

#include "headers.h"
#include "types.h"
#include "misc.h"
#include "schedule.h"
#include "pc88/sound.h"
#include "pc88/pc88.h"
#include "pc88/config.h"
#include "opnif.h"

//#define LOGNAME "sound"
#include "diag.h"

using namespace PC8801;

// ---------------------------------------------------------------------------
//	生成・破棄
//
Sound::Sound()
: Device(0), sslist(0), mixingbuf(0), enabled(false), cfgflg(0), refcount(0)
{
}

Sound::~Sound()
{
	Cleanup();
}

// ---------------------------------------------------------------------------
//	初期化とか
//	rate = SOUND_55K, 44100, 22050, 11025, 0 のどれか
//
bool Sound::Init(PC88* pc88, uint rate, uint32 opnclock, int bufsize)
{
	pc = pc88;
	prevtime = pc->GetCPUTick();
	enabled = false;
	mixthreshold = 16;
	
	if (!SetRate(rate, opnclock, bufsize))
		return false;
	
	pc88->AddEvent(5000, this, STATIC_CAST(TimeFunc, &Sound::UpdateCounter), 0, true);
	return true;
}

// ---------------------------------------------------------------------------
//	レート設定
//	rate:		SOUND_55K, 44100, 22050, 11025, 0 のどれか
//	clock:		OPN に与えるクロック
//	bufsize:	バッファ長 (サンプル単位?)
//
bool Sound::SetRate(uint rate, uint32 clock, int bufsize)
{
	if (rate == SOUND_55K)
		rate = 44100;

	for (SSNode* n = sslist; n; n = n->next)
		n->ss->SetRate(rate);
	
	enabled = false;
	samplingrate = rate;
	
	SoundBuffer::Cleanup();
	delete[] mixingbuf;	mixingbuf = 0;

	buffersize = bufsize;
	if (bufsize > 0)
	{
		if (!SoundBuffer::Init(2, bufsize))
			return false;

		mixingbuf = new int32[2 * bufsize];
		if (!mixingbuf)
			return false;

		rate50 = rate / 50;
		tdiff = 0;
		enabled = true;
	}
	return true;
}

// ---------------------------------------------------------------------------
//	後片付け
//
void Sound::Cleanup()
{
	for (SSNode* n = sslist; n; )
	{
		SSNode* next = n->next;
		delete[] n;
		n = next;
	}
	sslist = 0;

	SoundBuffer::Cleanup();
	delete[] mixingbuf; mixingbuf = 0;
}

// ---------------------------------------------------------------------------
//	音合成
//
void Sound::Mix(Sample* dest, int nsamples, Sample* dest2, int nsamples2)
{
	int mixsamples = Min(nsamples + nsamples2, buffersize);
	if (mixsamples)
	{
		LOG1("\t\t\t\tMix %d samples\n", mixsamples);
		memset(mixingbuf, 0, mixsamples * 2 * sizeof(int32));
		{
			CriticalSection::Lock lock(cs_ss);
			for (SSNode* s = sslist; s; s = s->next)
				s->ss->Mix(mixingbuf, mixsamples);
		}
		int32* src = mixingbuf;
		for (; nsamples>0; nsamples--)
		{
			*dest++ = Limit(*src++, 32767, -32768);
			*dest++ = Limit(*src++, 32767, -32768);
		}
		for (; nsamples2>0; nsamples2--)
		{
			*dest2++ = Limit(*src++, 32767, -32768);
			*dest2++ = Limit(*src++, 32767, -32768);
		}
	}
}

// ---------------------------------------------------------------------------
//	設定更新
//
void Sound::ApplyConfig(const Config* config)
{
//	SetVolume(config);
	mixthreshold = (config->flags & Config::precisemixing) ? 100 : 2000;
}

// ---------------------------------------------------------------------------
//	音源を追加する
//	Sound が持つ音源リストに，ss で指定された音源を追加，
//	ss の SetRate を呼び出す．
//
//	arg:	ss		追加する音源 (ISoundSource)
//	ret:	S_OK, E_FAIL, E_OUTOFMEMORY
//
bool Sound::Connect(ISoundSource* ss)
{
	CriticalSection::Lock lock(cs_ss);

	// 音源は既に登録済みか？
	for (SSNode* n = sslist; n; n=n->next)
	{
		if (n->ss == ss)
			return false;
	}
	
	SSNode* nn = new SSNode;
	if (nn)
	{
		nn->ss = ss;
		nn->next = sslist;
		sslist = nn;
		ss->SetRate(samplingrate);
		return true;
	}
	return false;
}

// ---------------------------------------------------------------------------
//	音源リストから指定された音源を削除する
//
//	arg:	ss		削除する音源
//	ret:	S_OK, E_HANDLE
//
bool Sound::Disconnect(ISoundSource* ss)
{
	CriticalSection::Lock lock(cs_ss);
	
	for (SSNode** r = &sslist; *r; r=&((*r)->next))
	{
		if ((*r)->ss == ss)
		{
			SSNode* d = *r;
			*r = d->next;
			delete d;
			return true;
		}
	}
	return false;
}

// ---------------------------------------------------------------------------
//	更新処理
//	(指定された)音源の Mix を呼び出し，現在の時間まで更新する	
//	音源の内部状態が変わり，音が変化する直前の段階で呼び出すと
//	精度の高い音再現が可能になる(かも)．
//
//	arg:	src		更新する音源を指定(今の実装では無視されます)
//
bool Sound::Update(ISoundSource* /*src*/)
{
	uint32 currenttime = pc->GetCPUTick();
	
	uint32 time = currenttime - prevtime;
	if (enabled && time > mixthreshold)
	{
		prevtime = currenttime;
		// nsamples = 経過時間(s) * サンプリングレート
		// sample = ticks * rate / clock / 100000
		// sample = ticks * (rate/50) / clock / 2000

		// MulDiv(a, b, c) = (int64) a * b / c 
		//int a = MulDiv(time, rate50, pc->GetEffectiveSpeed()) + tdiff;
		int a = (int)((__int64)time * rate50 / pc->GetEffectiveSpeed())
					+ tdiff;
		int samples = a / 2000;
		tdiff = a % 2000;
		
		LOG1("Store = %5d samples\n", samples);
		Put(samples);
	}
	return true;
}

// ---------------------------------------------------------------------------
//	今まで合成された時間の，1サンプル未満の端数(0-1999)を求める
//
int IFCALL Sound::GetSubsampleTime(ISoundSource* /*src*/)
{
	return tdiff;
}

// ---------------------------------------------------------------------------
//	定期的に内部カウンタを更新
//
void IOCALL Sound::UpdateCounter(uint)
{
	if ((pc->GetCPUTick() - prevtime) > 40000)
	{
		LOG0("Update Counter\n");
		Update(0);
	}
}
