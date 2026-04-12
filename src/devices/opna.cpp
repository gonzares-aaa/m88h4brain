// ---------------------------------------------------------------------------
//	OPN/OPNA Interface including ADPCM/Rhythm support
//	Copyright (C) cisc 1998, 1999.
// ---------------------------------------------------------------------------
//	$Id: opna.cpp,v 1.29 1999/12/28 11:13:34 cisc Exp $

#include "headers.h"
#include "misc.h"
#include "file.h"
#include "opna.h"
#include "fmgeninl.h"

#define LOGNAME "opna"
#include "diag.h"

// ---------------------------------------------------------------------------
//	ADPCM データの格納方式の違い (8bit/1bit) をエミュレートしない
//	このオプションを有効にすると ADPCM メモリへのアクセス(特に 8bit モード)が
//	多少軽くなるかも
//
//#define NO_BITTYPE_EMULATION

//#define BUILD_OPN
#define BUILD_OPNA

namespace FM
{

// ---------------------------------------------------------------------------
//	OPNBase

#if defined(BUILD_OPN) || defined(BUILD_OPNA)

OPNBase::OPNBase()
{
	prescale = -1;
}

void OPNBase::SetTimerA(uint addr, uint data)
{
	uint tmp;
	reg24[addr & 1] = uint8(data);
	tmp = (reg24[0] << 2) + (reg24[1] & 3);
	timera = (1024-tmp) * timer_step;
	LOG2("Timer A = %d   %d us\n", tmp, timera >> 16);
}

void OPNBase::SetTimerB(uint data)
{
	timerb = (256-data) * timer_step;
	LOG2("Timer B = %d   %d us\n", data, timerb >> 12);
}

void OPNBase::SetTimerControl(uint data)
{
	uint tmp = reg27 ^ data;
	reg27 = uint8(data);
	
	if (data & 0x10) ResetStatus(1);
	if (data & 0x20) ResetStatus(2);
	
	if (tmp & 0x01)
		timera_count = (data & 1) ? timera : 0;
	if (tmp & 0x02)
		timerb_count = (data & 2) ? timerb : 0;
}

//	パラメータセット
void OPNBase::SetParameter(Channel4* ch, uint addr, uint data)
{
	const static uint slottable[4] = { 0, 2, 1, 3 };
	const static uint8 sltable[16] = 
	{
		  0,   4,   8,  12,  16,  20,  24,  28,
		 32,  36,  40,  44,  48,  52,  56, 124,
	};
	
	if ((addr & 3) < 3)
	{
		uint slot = slottable[(addr >> 2) & 3];
		Operator* op = &ch->op[slot];

		switch ((addr >> 4) & 15)
		{
		case 3:	// 30-3E DT/MULTI
			op->SetDT((data >> 4) & 0x07);
			op->SetMULTI(data & 0x0f);
			break;
			
		case 4: // 40-4E TL
			op->SetTL(data & 0x7f, (reg27 & 0x80) && csmch == ch);
			break;
			
		case 5: // 50-5E KS/AR
			op->SetKS((data >> 6) & 3);
			op->SetAR((data & 0x1f) * 2);
			break;
			
		case 6: // 60-6E DR/AMON
			op->SetDR((data & 0x1f) * 2);
			op->SetAMON((data & 0x80) != 0);
			break;
			
		case 7: // 70-7E SR
			op->SetSR((data & 0x1f) * 2);
			break;
			
		case 8:	// 80-8E SL/RR
			op->SetSL(sltable[(data >> 4) & 15]);
			op->SetRR((data & 0x0f) * 4 + 2);
			break;
			
		case 9: // 90-9E SSG-EC
			op->SetSSGEC(data & 0x0f);
			break;
		}
	}
}

//	リセット
void OPNBase::Reset()
{
	status = 0;
	timera_count = 0;
	timerb_count = 0;
	SetPrescaler(0);
	psg.Reset();
}

//	プリスケーラ設定
void OPNBase::SetPrescaler(uint p)
{
	static const char table[3][2] = { { 6, 4 }, { 3, 2 }, { 2, 1 } };
	
	if (prescale != p)
	{
		prescale = p;
		
		uint fmclock = clock / table[p][0];
		uint ratio = (((fmclock / 12) << FM_RATIOBITS) + rate/2) / rate;
//		LOG1("ratio = %d\n", ratio);
		
		timer_step = int32(1.00 * 12*65536000000. / fmclock);
		FM::MakeTimeTable(ratio);
		psg.SetClock(clock / table[p][1], psgrate);
	}
}

//	初期化
bool OPNBase::Init(uint c, uint rf, uint rp)
{
	clock = c;
	rate = rf;
	psgrate = rp;
	return true;
}

//	タイマー時間処理
bool OPNBase::Count(int32 us)
{
	bool event = false;

	if (timera_count)
	{
		timera_count -= us << 16;
		if (timera_count <= 0)
		{
			event = true;
//			LOG1("Timer A (%d)\n", !!(reg27 & 4));
			if (reg27 & 0x80)
			{
				csmch->KeyControl(0);
				csmch->KeyControl(0xf0);
			}
			while (timera_count <= 0)
				timera_count += timera;
			if (reg27 & 4)
			{
				SetStatus(1);
			}
		}
	}
	if (timerb_count)
	{
		timerb_count -= us << 12;
		if (timerb_count <= 0)
		{
			event = true;
//			LOG1("Timer B (%d)\n", !!(reg27 & 8));
			while (timerb_count <= 0)
				timerb_count += timerb;
			if (reg27 & 8)
			{
				SetStatus(2);
			}
		}
	}
	return event;
}

int32 OPNBase::GetNextEvent()
{
	uint32 ta = ((timera_count + 0xffff) >> 16) - 1;
	uint32 tb = ((timerb_count + 0xfff) >> 12) - 1;
	uint32 t = ta < tb ? ta : tb;
	return t + 1;
}

//	音量設定
void FM::OPNBase::SetVolumeFM(int db)
{
	db = Min(db, 20);
	if (db > -192)
		fmvolume = int(16384.0 * pow(10, db / 40.0));
	else
		fmvolume = 0;
}

#endif // defined(BUILD_OPN) || defined(BUILD_OPNA)

// ---------------------------------------------------------------------------
//	YM2203
//
#ifdef BUILD_OPN

OPN::OPN()
{
	MakeTable();
	SetVolumeFM(0);
	SetVolumePSG(0);
	csmch = &ch[2];
}

//	リセット
void OPN::Reset()
{
	int i;
	for (i=0x20; i<0x28; i++) SetReg(i, 0);
	for (i=0x30; i<0xc0; i++) SetReg(i, 0);
	OPNBase::Reset();
	ch[0].Reset();
	ch[1].Reset();
	ch[2].Reset();
}

#define IStoSample(s)	((((s) >> (FM_ISHIFT+3)) * fmvolume) >> 14)

//	合成(2ch)
void OPN::Mix(Sample* buffer, int nsamples)
{
	psg.Mix(buffer, nsamples);
	
	// Set F-Number
	ch[0].SetFNum(fnum[0]);
	ch[1].SetFNum(fnum[1]);
	if (!(reg27 & 0xc0))
		ch[2].SetFNum(fnum[2]);
	else
	{	// 効果音
		ch[2].op[0].SetFNum(fnum3[1]);
		ch[2].op[1].SetFNum(fnum3[2]);
		ch[2].op[2].SetFNum(fnum3[0]);
		ch[2].op[3].SetFNum(fnum[2]);
	}
	
	if (ch[0].Prepare() | ch[1].Prepare() | ch[2].Prepare())
	{
		for (; nsamples > 0; nsamples--)
		{
			Sample sample = IStoSample(ch[0].Calc() + ch[1].Calc() + ch[2].Calc());
			*buffer++ += sample;
			*buffer++ += sample;
		}
	}
}
#undef IStoSample

uint OPN::GetReg(uint addr)
{
	if (addr < 0x10)
		return psg.GetReg(addr);
	else
		return 0;
}

//	レジスタアレイにデータを設定
void OPN::SetReg(uint addr, uint data)
{
//	LOG2("reg[%.2x] <- %.2x\n", addr, data);
	if (addr >= 0x100)
		return;
	
	switch (addr)
	{
	case  0: case  1: case  2: case  3: case  4: case  5: case  6: case  7:
	case  8: case  9: case 10: case 11: case 12: case 13: case 14: case 15:
		psg.SetReg(addr, data);
		break;

	case 0x24: case 0x25:
		SetTimerA(addr, data);
		break;

	case 0x26:
		SetTimerB(data);
		break;

	case 0x27:
		SetTimerControl(data);
		break;
	
	case 0x28:		// Key On/Off
		if ((data & 3) < 3)
			ch[data & 3].KeyControl(data);
		break;

	case 0x2d: case 0x2e: case 0x2f:
		SetPrescaler(addr-0x2d);
		break;

	// F-Number
	case 0xa0: case 0xa1: case 0xa2:
		fnum[addr & 3] = data + fnum2[addr & 3] * 0x100; 
		break;
	
	case 0xa4: case 0xa5: case 0xa6:
		fnum2[addr & 3] = uint8(data);
		break;

	case 0xa8: case 0xa9: case 0xaa:
		fnum3[addr & 3] = data + fnum2[addr - 0xa8 + 3] * 0x100; 
		break;
	
	case 0xac: case 0xad: case 0xae:
		fnum2[addr - 0xa4 + 3] = uint8(data);
		break;
	
	case 0xb0:	case 0xb1:  case 0xb2:
		ch[addr & 3].SetFB((data >> 3) & 7);
		ch[addr & 3].SetAlgorithm(data & 7);
		break;
	
	default:
		// LFO 無し
		if ((addr & 0xf0) == 0x60)
			data &= 0x1f;
		OPNBase::SetParameter(&ch[addr & 3], addr, data);
		break;
	}
}

//	ステータスフラグ設定
void OPN::SetStatus(uint8 bits)
{
	if (!(status & bits))
	{
		status |= bits;
		Intr(true);
	}
}

void OPN::ResetStatus(uint8 bit)
{
	status &= ~bit;
	if (!status)
		Intr(false);
}

//	サンプリングレート変更

bool OPN::SetRate(uint c, uint r, bool)
{
	OPNBase::Init(c, r, r);
	RebuildTimeTable();
	return true;
}

bool OPN::Init(uint c, uint r, bool, const char*)
{
	return OPNBase::Init(c, r, r);
}


#endif // BUILD_OPN

// ---------------------------------------------------------------------------
//	YM2608
//
#ifdef BUILD_OPNA

OPNA::OPNA()
{
	adpcmbuf = 0;
	memaddr = 0;
	startaddr = 0;
	mixl = mixr = 0;
	mixdelta = 16383;
	deltan = 256;

	MakeTable();
	csmch = &ch[2];

	for (int i=0; i<6; i++)
	{
		rhythm[i].sample = 0;
		rhythm[i].pos = 0;
		rhythm[i].size = 0;
		rhythm[i].volume = 0;
	}
	rhythmtvol = 0;
	adpcmvol = 0;
	control2 = 0;

	interpolation = false;
}

#if defined(LOGNAME) && defined(_DEBUG)
static uint8 regfile[0x200];
#endif

OPNA::~OPNA()
{
	delete[] adpcmbuf;
	for (int i=0; i<6; i++)
		delete[] rhythm[i].sample;

#if defined(LOGNAME) && defined(_DEBUG)
	for (int ch=0; ch<6; ch++)
	{
		uint8* ref = &regfile[ch > 2 ? 0x100+ch-3 : ch]; 

#define BIT(a, b, c) ((ref[a] >> b) & ((1 << c) - 1))
		LOG1("\n\nChannel %d:\n", ch);
		LOG4("DT      %4d  %4d  %4d  %4d\n", BIT(0x30, 4,3), BIT(0x38, 4,3), BIT(0x34, 4,3), BIT(0x3c, 4,3));
		LOG4("MULTI   %4d  %4d  %4d  %4d\n", BIT(0x30, 0,4), BIT(0x38, 0,4), BIT(0x34, 0,4), BIT(0x3c, 0,4));
		LOG4("TL      %4d  %4d  %4d  %4d\n", -BIT(0x40, 0,7), -BIT(0x48, 0,7), -BIT(0x44, 0,7), -BIT(0x4c, 0,7));
		LOG4("KS      %4d  %4d  %4d  %4d\n", BIT(0x50, 6,2), BIT(0x58, 6,2), BIT(0x54, 6,2), BIT(0x5c, 6,2));
		LOG4("AR      %4d  %4d  %4d  %4d\n", BIT(0x50, 0,5), BIT(0x58, 0,5), BIT(0x54, 0,5), BIT(0x5c, 0,5));
		LOG4("DR      %4d  %4d  %4d  %4d\n", BIT(0x60, 0,5), BIT(0x68, 0,5), BIT(0x64, 0,5), BIT(0x6c, 0,5));
		LOG4("SR      %4d  %4d  %4d  %4d\n", BIT(0x70, 0,5), BIT(0x78, 0,5), BIT(0x74, 0,5), BIT(0x7c, 0,5));
		LOG4("RR      %4d  %4d  %4d  %4d\n", BIT(0x80, 0,4), BIT(0x88, 0,4), BIT(0x84, 0,4), BIT(0x8c, 0,4));
		LOG4("SL      %4d  %4d  %4d  %4d\n", BIT(0x80, 4,4), BIT(0x88, 4,4), BIT(0x84, 4,4), BIT(0x8c, 4,4));
		LOG4("AMON    %4d  %4d  %4d  %4d\n", BIT(0x60, 7,1), BIT(0x68, 7,1), BIT(0x64, 7,1), BIT(0x6c, 7,1));
		LOG4("SSG-EG  %4d  %4d  %4d  %4d\n", BIT(0x90, 0,4), BIT(0x98, 0,4), BIT(0x94, 0,4), BIT(0x9c, 0,4));
		LOG1("FB      %4d\n", BIT(0xb0, 3, 3));
		LOG1("Connect %4d\n", BIT(0xb0, 0, 3));
		LOG2("AMS     %4d    PMS     %4d\n", BIT(0xb4, 4, 2), BIT(0xb4, 0, 3));
#undef BIT
	}
#endif
}

// ---------------------------------------------------------------------------
//	初期化
//
bool OPNA::Init(uint c, uint r, bool ipflag, const char* path)
{
	SetRate(c, r, ipflag);
	RebuildTimeTable();
	
	adpcmbuf = new uint8[0x40000];
	if (!adpcmbuf)
		return false;
	
	LoadRhythmSample(path);
	Reset();

	SetVolumeFM(0);
	SetVolumePSG(0);
	SetVolumeADPCM(0);
	SetVolumeRhythmTotal(0);
	for (int i=0; i<6; i++)
		SetVolumeRhythm(0, 0);
	SetChannelMask(0);
	return true;
}

// ---------------------------------------------------------------------------
//	リセット
//
void OPNA::Reset()
{
	int i;
	for (i=0x20; i<0x28; i++) SetReg(i, 0);
	for (i=0x30; i<0xc0; i++) SetReg(i, 0);
	for (i=0x130; i<0x1c0; i++) SetReg(i, 0);
	for (i=0x100; i<0x110; i++) SetReg(i, 0);
	for (i=0x10; i<0x20; i++) SetReg(i, 0);
	OPNBase::Reset();
	for (i=0; i<6; i++) pan[i] = 3;
	ch[0].Reset();
	ch[1].Reset();
	ch[2].Reset();
	ch[3].Reset();
	ch[4].Reset();
	ch[5].Reset();
	reg29 = 0x1f;
	stmask = ~0x1c;
	statusnext = 0;
	memaddr = 0;
	adpcmd = 127;
	adpcmx = 0;
	rhythmkey = 0;
	limitaddr = 0x3ffff;
	lfocount = 0;
	lfodcount = 0;
	adpcmplay = false;
	adplc = 0;
	adpld = 0x100;
	status = 0;
	UpdateStatus();
}

// ---------------------------------------------------------------------------
//	サンプリングレート変更
//
bool OPNA::SetRate(uint c, uint r, bool ipflag)
{
	interpolation = ipflag;
	
	int ra = r;
	if (interpolation)
	{
		ra = c / 36;
		do
		{
			ra >>= 1;
			mpratio = r * 16384 / ra;
			if (mpratio > 16383)
				interpolation = false;
		} while (mpratio <= 8192);
	}
	
	OPNBase::Init(c, ra, r);

	for (int i=0; i<6; i++)
	{
		rhythm[i].step = rhythm[i].rate * 1024 / r;
	}
	adplbase = int(8192. * (clock/72.) / r);
	adpld = deltan * adplbase >> 16;      
		
	RebuildTimeTable();

	lfodcount = reg22 & 0x08 ? lfotable[reg22 & 7] : 0;
	return true;
}

// ---------------------------------------------------------------------------
//	リズム音を読みこむ
//
bool OPNA::LoadRhythmSample(const char* path)
{
	static const char* rhythmname[6] =
	{
		"BD", "SD", "TOP", "HH", "TOM", "RIM",
	};

	int i;
	for (i=0; i<6; i++)
		rhythm[i].pos = ~0;

	for (i=0; i<6; i++)
	{
		FileIO file;
		uint32 fsize;
		char buf[MAX_PATH] = "";
		if (path)
			strncpy(buf, path, MAX_PATH);
		strncat(buf, "2608_", MAX_PATH);
		strncat(buf, rhythmname[i], MAX_PATH);
		strncat(buf, ".WAV", MAX_PATH);

		if (!file.Open(buf, FileIO::readonly))
		{
			if (i != 5)
				break;
			if (path)
				strncpy(buf, path, MAX_PATH);
			strncpy(buf, "2608_RYM.WAV", MAX_PATH);
			if (!file.Open(buf, FileIO::readonly))
				break;
		}
		
		struct
		{
			uint32 chunksize;
			uint16 tag;
			uint16 nch;
			uint32 rate;
			uint32 avgbytes;
			uint16 align;
			uint16 bps;
			uint16 size;
		} whdr;

		file.Seek(0x10, FileIO::begin);
		file.Read(&whdr, sizeof(whdr));
		
		uint8 subchunkname[4];
		fsize = 4 + whdr.chunksize - sizeof(whdr);
		do 
		{
			file.Seek(fsize, FileIO::current);
			file.Read(&subchunkname, 4);
			file.Read(&fsize, 4);
		} while (memcmp("data", subchunkname, 4));

		fsize /= 2;
		if (fsize >= 0x100000 || whdr.tag != 1 || whdr.nch != 1)
			break;
		fsize = Max(fsize, (1<<31)/1024);
		
		delete rhythm[i].sample;
		rhythm[i].sample = new int16[fsize];
		if (!rhythm[i].sample)
			break;
		
		file.Read(rhythm[i].sample, fsize * 2);
		
		rhythm[i].rate = whdr.rate;
		rhythm[i].step = rhythm[i].rate * 1024 / rate;
		rhythm[i].pos = rhythm[i].size = fsize * 1024;
	}
	if (i != 6)
	{
		for (i=0; i<6; i++)
		{
			delete[] rhythm[i].sample;
			rhythm[i].sample = 0;
		}
		return false;
	}
	return true;
}

// ---------------------------------------------------------------------------
//	チャンネルマスクの設定
//
void OPNA::SetChannelMask(uint mask)
{
	for (int i=0; i<6; i++)
		ch[i].Mute(!!(mask & (1 << i)));
	psg.SetChannelMask(mask >> 6);
}

// ---------------------------------------------------------------------------
//	レジスタアレイにデータを設定
//
void OPNA::SetReg(uint addr, uint data)
{
	addr &= 0x1ff;
#if defined(LOGNAME) && defined(_DEBUG)
	if (addr != 0x108 && addr != 0x110)
		LOG3("%d:reg[%.3x] <- %.2x", Diag::GetCPUTick(), addr, data);
	regfile[addr] = data;
#endif

	int	c = addr & 3;
	switch (addr)
	{
		uint modified;

	// Timer -----------------------------------------------------------------
		case 0x24: case 0x25:
			SetTimerA(addr, data);
			break;

		case 0x26:
			SetTimerB(data);
			break;

		case 0x27:
			SetTimerControl(data);
			break;

	// Misc ------------------------------------------------------------------
	case 0x28:		// Key On/Off
		if ((data & 3) < 3)
		{
			c = (data & 3) + (data & 4 ? 3 : 0);
			if (!((reg27 & 0x80) && (c == 2)))
				ch[c].KeyControl(data);
			else
			{
				if (!(data & 0x10)) ch[2].op[0].KeyOff();
				if (!(data & 0x20)) ch[2].op[1].KeyOff();
				if (!(data & 0x40)) ch[2].op[2].KeyOff();
				if (!(data & 0x80)) ch[2].op[3].KeyOff();
			}
		}
		break;

	// Status Mask -----------------------------------------------------------
	case 0x29:
		reg29 = data;
//		UpdateStatus(); //?
		break;
	
	// Prescaler -------------------------------------------------------------
	case 0x2d: case 0x2e: case 0x2f:
		SetPrescaler(addr-0x2d);
		break;
	
	// F-Number --------------------------------------------------------------
	case 0x1a0:	case 0x1a1: case 0x1a2:
		c += 3;
	case 0xa0:	case 0xa1: case 0xa2:
		fnum[c] = data + fnum2[c] * 0x100;
		break;

	case 0x1a4:	case 0x1a5: case 0x1a6:
		c += 3;
	case 0xa4 : case 0xa5: case 0xa6:
		fnum2[c] = uint8(data);
		break;

	case 0xa8:	case 0xa9: case 0xaa:
		fnum3[c] = data + fnum2[c+6] * 0x100;
		break;

	case 0xac : case 0xad: case 0xae:
		fnum2[c+6] = uint8(data);
		break;
		
	// Algorithm -------------------------------------------------------------
	
	case 0x1b0:	case 0x1b1:  case 0x1b2:
		c += 3;
	case 0xb0:	case 0xb1:  case 0xb2:
		ch[c].SetFB((data >> 3) & 7);
		ch[c].SetAlgorithm(data & 7);
		break;
	
	case 0x1b4: case 0x1b5: case 0x1b6:
		c += 3;
	case 0xb4: case 0xb5: case 0xb6:
		pan[c] = (data >> 6) & 3;
		ch[c].SetMS(data);
		break;

	// LFO -------------------------------------------------------------------
	
	case 0x22:
		modified = reg22 ^ data;
		reg22 = data;
		if (modified & 0x8)
		{
			lfocount = 0;
		}
		lfodcount = reg22 & 8 ? lfotable[reg22 & 7] : 0;
		break;
	
	// Rhythm ----------------------------------------------------------------
	case 0x10:			// DM/KEYON
		if (!(data & 0x80))  // KEY ON
		{
			rhythmkey |= data & 0x3f;
			if (data & 0x01) rhythm[0].pos = 0;
			if (data & 0x02) rhythm[1].pos = 0;
			if (data & 0x04) rhythm[2].pos = 0;
			if (data & 0x08) rhythm[3].pos = 0;
			if (data & 0x10) rhythm[4].pos = 0;
			if (data & 0x20) rhythm[5].pos = 0;
		}
		else
		{					// DUMP
			rhythmkey &= ~data;
		}
		break;

	case 0x11:
		rhythmtl = ~data & 63;
		break;

	case 0x18:		// Bass Drum
	case 0x19:		// Snare Drum
	case 0x1a:		// Top Cymbal
	case 0x1b:		// Hihat
	case 0x1c:		// Tom-tom
	case 0x1d:		// Rim shot
		rhythm[addr & 7].pan   = (data >> 6) & 3;
		rhythm[addr & 7].level = ~data & 31;
		break;

	// ADPCM -----------------------------------------------------------------
	case 0x100:		// Control Register 1
		if ((data & 0x80) && !adpcmplay)
		{
			adpcmplay = true;
			memaddr = startaddr;
			adpcmx = 0, adpcmd = 127;
			adplc = 0;
		}
		if (data & 1)
		{
			adpcmplay = false;
		}
		control1 = data;
		break;

	case 0x101:		// Control Register 2
		control2 = data;
		granuality = control2 & 2 ? 1 : 4;
		break;

	case 0x102:		// Start Address L
	case 0x103:		// Start Address H
		adpcmreg[addr - 0x102 + 0] = data;
		startaddr = (adpcmreg[1]*256+adpcmreg[0]) << 6;
		memaddr = startaddr;
		LOG1("  startaddr %.6x", startaddr);
		break;

	case 0x104:		// Stop Address L
	case 0x105:		// Stop Address H
		adpcmreg[addr - 0x104 + 2] = data;
		stopaddr = (adpcmreg[3]*256+adpcmreg[2] + 1) << 6;
		LOG1("  stopaddr %.6x", stopaddr);
		break;

	case 0x108:		// ADPCM data
		if ((control1 & 0x60) == 0x60)
		{
//			LOG2("  Wr [0x%.5x] = %.2x", memaddr, data);
			WriteRAM(data);
		}
		break;

	case 0x109:		// delta-N L
	case 0x10a:		// delta-N H
		adpcmreg[addr - 0x109 + 4] = data;
		deltan = adpcmreg[5]*256+adpcmreg[4];
		deltan = Max(256, deltan);
		adpld = deltan * adplbase >> 16;
		break;

	case 0x10b:		// Level Control
		adpcmlevel = data; 
		if (adpcmvol < 128)
			adpcmvolume = (tltable[FM_TLPOS + adpcmvol] * adpcmlevel) >> 12;
		else
			adpcmvolume = 0;
		break;

	case 0x10c:		// Limit Address L
	case 0x10d:		// Limit Address H
		adpcmreg[addr - 0x10c + 6] = data;
		limitaddr = (adpcmreg[7]*256+adpcmreg[6] + 1) << 6;
		LOG1("  limitaddr %.6x", limitaddr);
		break;

	case 0x110:		// Flag Control
		if (data & 0x80)
		{
			status = 0;
			UpdateStatus();
		}
		else
		{
			stmask = ~(data & 0x1f);
//			UpdateStatus();					//???
		}
		break;
	

	case  0: case  1: case  2: case  3: case  4: case  5: case  6: case  7:
	case  8: case  9: case 10: case 11: case 12: case 13: case 14: case 15:
		psg.SetReg(addr, data);
		break;

	// 音色 ------------------------------------------------------------------
	default:
		if (addr & 0x100)
			c += 3;
		OPNBase::SetParameter(&ch[c], addr, data);
		break;
	}
	LOG0("\n");
}

// ---------------------------------------------------------------------------
//	レジスタ取得
//
uint OPNA::GetReg(uint addr)
{
	if (addr < 0x10)
		return psg.GetReg(addr);

	if (addr == 0x108)
	{
//		LOG1("%d:reg[108] ->   ", Diag::GetCPUTick());
		
		uint data = adpcmreadbuf & 0xff;
		adpcmreadbuf >>= 8;
		if ((control1 & 0x60) == 0x20)
		{
			adpcmreadbuf |= ReadRAM() << 8;
//			LOG2("Rd [0x%.6x:%.2x] ", memaddr, adpcmreadbuf >> 8);
		}
//		LOG0("%.2x\n");
		return data;
	}
	
	if (addr == 0xff)
		return 1;
	
	return 0;
}

// ---------------------------------------------------------------------------
//	ステータスフラグ設定
//
void OPNA::SetStatus(uint8 bits)
{
	if (!(status & bits))
	{
		LOG2("SetStatus(%.2x %.2x)\n", bits, stmask);
		status |= bits & stmask;
		UpdateStatus();
	}
	else
		LOG1("SetStatus(%.2x) - ignored\n", bits);
}

void OPNA::ResetStatus(uint8 bits)
{
	status &= ~bits;
	LOG1("ResetStatus(%.2x)\n", bits);
	UpdateStatus();
}

inline void OPNA::UpdateStatus()
{
//	LOG2("%d:INT = %d\n", Diag::GetCPUTick(), (status & stmask & reg29) != 0);
	Intr((status & stmask & reg29) != 0);
}

// ---------------------------------------------------------------------------
//	ADPCM RAM への書込み操作
//
void OPNA::WriteRAM(uint data)
{
#ifndef NO_BITTYPE_EMULATION
	if (!(control2 & 2))
	{
		// 1 bit mode
		adpcmbuf[(memaddr >> 4) & 0x3ffff] = data;
		memaddr += 16;
	}
	else
	{
		// 8 bit mode
		uint8* p = &adpcmbuf[(memaddr >> 4) & 0x7fff];
		uint bank = (memaddr >> 1) & 7;
		uint8 mask = 1 << bank;
		data <<= bank;

		p[0x00000] = (p[0x00000] & ~mask) | (uint8(data) & mask); data >>= 1;
		p[0x08000] = (p[0x08000] & ~mask) | (uint8(data) & mask); data >>= 1;
		p[0x10000] = (p[0x10000] & ~mask) | (uint8(data) & mask); data >>= 1;
		p[0x18000] = (p[0x18000] & ~mask) | (uint8(data) & mask); data >>= 1;
		p[0x20000] = (p[0x20000] & ~mask) | (uint8(data) & mask); data >>= 1;
		p[0x28000] = (p[0x28000] & ~mask) | (uint8(data) & mask); data >>= 1;
		p[0x30000] = (p[0x30000] & ~mask) | (uint8(data) & mask); data >>= 1;
		p[0x38000] = (p[0x38000] & ~mask) | (uint8(data) & mask);
		memaddr += 2;
	}
#else
	adpcmbuf[(memaddr >> granuality) & 0x3ffff] = data;
	memaddr += 1 << granuality;
#endif

	if (memaddr == stopaddr)
	{
		SetStatus(4);
		statusnext = 0x04;	// EOS
		memaddr &= 0x3fffff;
	}
	if (memaddr == limitaddr)
	{
		LOG1("Limit ! (%.8x)\n", limitaddr);
		memaddr = 0;
	}
	SetStatus(8);
}

// ---------------------------------------------------------------------------
//	ADPCM RAM からの読み込み操作
//
uint OPNA::ReadRAM()
{
	uint data;
#ifndef NO_BITTYPE_EMULATION
	if (!(control2 & 2))
	{
		// 1 bit mode
		data = adpcmbuf[(memaddr >> 4) & 0x3ffff];
		memaddr += 16;
	}
	else
	{
		// 8 bit mode
		uint8* p = &adpcmbuf[(memaddr >> 4) & 0x7fff];
		uint bank = (memaddr >> 1) & 7;
		uint8 mask = 1 << bank;

		data =            (p[0x38000] & mask);
		data = data * 2 + (p[0x30000] & mask);
		data = data * 2 + (p[0x28000] & mask);
		data = data * 2 + (p[0x20000] & mask);
		data = data * 2 + (p[0x18000] & mask);
		data = data * 2 + (p[0x10000] & mask);
		data = data * 2 + (p[0x08000] & mask);
		data = data * 2 + (p[0x00000] & mask);
		data >>= bank;
		memaddr += 2;
	}
#else
	data = adpcmbuf[(memaddr >> granuality) & 0x3ffff];
	memaddr += 1 << granuality;
#endif
	if (memaddr == stopaddr)
	{
		SetStatus(4);
		statusnext = 0x04;	// EOS
		memaddr &= 0x3fffff;
	}
	if (memaddr == limitaddr)
	{
		LOG1("Limit ! (%.8x)\n", limitaddr);
		memaddr = 0;
	}
	if (memaddr < stopaddr)
		SetStatus(8);
	return data;
}


inline int OPNA::DecodeADPCMSample(uint data)
{
	static const int table1[16] =
	{
		  1,   3,   5,   7,   9,  11,  13,  15,
		 -1,  -3,  -5,  -7,  -9, -11, -13, -15,
	};
	static const int table2[16] =
	{
		 57,  57,  57,  57,  77, 102, 128, 153,
		 57,  57,  57,  57,  77, 102, 128, 153,
	};
	adpcmx = Limit(adpcmx + table1[data] * adpcmd / 8, 32767, -32768);
	adpcmd = Limit(adpcmd * table2[data] / 64, 24576, 127);
	return adpcmx;
}	


// ---------------------------------------------------------------------------
//	ADPCM RAM からの nibble 読み込み及び ADPCM 展開
//
int OPNA::ReadRAMN()
{
	uint data;
#ifndef NO_BITTYPE_EMULATION
	if (!(control2 & 2))
	{
		data = adpcmbuf[(memaddr >> 4) & 0x3ffff];
		memaddr += 8;
		if (memaddr & 8)
			return DecodeADPCMSample(data >> 4);
		data &= 0x0f;
	}
	else
	{
		uint8* p = &adpcmbuf[(memaddr >> 4) & 0x7fff] + ((~memaddr & 1) << 17);
		uint bank = (memaddr >> 1) & 7;
		uint8 mask = 1 << bank;

		data =            (p[0x18000] & mask);
		data = data * 2 + (p[0x10000] & mask);
		data = data * 2 + (p[0x08000] & mask);
		data = data * 2 + (p[0x00000] & mask);
		data >>= bank;
		memaddr ++;
		if (memaddr & 1)
			return DecodeADPCMSample(data);
	}
#else
	data = adpcmbuf[(memaddr >> granuality) & 0x3ffff];
	memaddr += 1 << (granuality-1);
	if (memaddr & (1 << (granuality-1)))
		return DecodeADPCMSample(data >> 4);
	data &= 0x0f;
#endif
	
	DecodeADPCMSample(data);
	
	// check
	if (memaddr == stopaddr)
	{
		if (control1 & 0x10)
		{
			memaddr = startaddr;
			data = adpcmx;
			adpcmx = 0, adpcmd = 127;
			return data;
		}
		else
		{
			memaddr &= 0x3fffff;
			SetStatus(4);
			adpcmplay = false;
		}
	}
	
	if (memaddr == limitaddr)
		memaddr = 0;
	
	return adpcmx;
}

// ---------------------------------------------------------------------------
//	拡張ステータスを読みこむ
//
uint OPNA::ReadStatusEx()
{
	uint r = ((status | 8) & stmask) | (adpcmplay ? 0x20 : 0);
	status |= statusnext;
	statusnext = 0;
	return r;
}

// ---------------------------------------------------------------------------
//	ADPCM 展開
//
inline void OPNA::DecodeADPCM()
{
	apout0 = apout1;
	int n = (ReadRAMN() * adpcmvolume) >> 13;
	apout1 = adpcmout + n;
	adpcmout = n;
}

// ---------------------------------------------------------------------------
//	ADPCM 合成
//	
void OPNA::ADPCMMix(Sample* dest, uint count)
{
	uint maskl = control2 & 0x80 ? -1 : 0;
	uint maskr = control2 & 0x40 ? -1 : 0;
	
	if (adpcmplay)
	{
//		LOG2("ADPCM Play: %d   DeltaN: %d\n", adpld, deltan);
		if (adpld <= 8192)		// fplay < fsamp
		{
			for (; count>0; count--)
			{
				if (adplc < 0)
				{
					adplc += 8192;
					DecodeADPCM();
					if (!adpcmplay)
						break;
				}
				int s = (adplc * apout0 + (8192-adplc) * apout1) >> 13;
				dest[0] += s & maskl; dest[1] += s & maskr; dest += 2;
				adplc -= adpld;
			}
			for (; count>0 && apout0; count--)
			{
				if (adplc < 0)
				{
					apout0 = apout1, apout1 = 0;
					adplc += 8192;
				}
				int s = (adplc * apout1) >> 13;
				dest[0] += s & maskl; dest[1] += s & maskr; dest += 2;
				adplc -= adpld;
			}
		}
		else	// fplay > fsamp	(adpld = fplay/famp*8192)
		{
			int t = (-8192*8192)/adpld;
			for (; count>0; count--)
			{
				int s = apout0 * (8192+adplc);
				while (adplc < 0)
				{
					DecodeADPCM();
					if (!adpcmplay)
						goto stop;
					s -= apout0 * Max(adplc, t);
					adplc -= t;
				}
				adplc -= 8192;
				s >>= 13;
				dest[0] += s & maskl; dest[1] += s & maskr; dest += 2;
			}
stop:
			;
		}
	}
	if (!adpcmplay)
	{
		apout0 = apout1 = adpcmout = 0;
		adplc = 0;
	}
}

// ---------------------------------------------------------------------------
//	リズム合成
//
void OPNA::RhythmMix(Sample* buffer, uint count)
{
	if (rhythmtvol < 128 && rhythm[0].sample && (rhythmkey & 0x3f))
	{
		int32* limit = buffer + count * 2;
		for (int i=0; i<6; i++)
		{
			Rhythm& r = rhythm[i];
			if ((rhythmkey & (1 << i)) && r.level < 128)
			{
				int vol;
				int db = Limit(rhythmtl+rhythmtvol+r.level+r.volume, 127, -31);
				vol = tltable[FM_TLPOS + (db << (FM_TLBITS-7))] >> 4;
				int32* dest = buffer;
				switch (r.pan)
				{
				case 0: default:
					break;
				case 1:		// -R
					for ( ; dest<limit && r.pos < r.size; dest+=2)
					{
						int sample = (r.sample[r.pos / 1024] * vol) >> 12;
						r.pos += r.step;
						dest[1] += sample;
					}
					break;
				case 2:		// L-
					for ( ; dest<limit && r.pos < r.size; dest+=2)
					{
						int sample = (r.sample[r.pos / 1024] * vol) >> 12;
						r.pos += r.step;
						dest[0] += sample;
					}
					break;
				case 3:		// LR
					for ( ; dest<limit && r.pos < r.size; dest+=2)
					{
						int sample = (r.sample[r.pos / 1024] * vol) >> 12;
						r.pos += r.step;
						dest[0] += sample, dest[1] += sample;
					}
					break;
				}
			}
		}
	}
}

// ---------------------------------------------------------------------------
//	音量設定
//
void OPNA::SetVolumeRhythmTotal(int db)
{
	db = Min(db, 20);
	rhythmtvol = -(db * 2 / 3);
}

void OPNA::SetVolumeRhythm(int index, int db)
{
	db = Min(db, 20);
	rhythm[index].volume = -(db * 2 / 3);
}

void OPNA::SetVolumeADPCM(int db)
{
	db = Min(db, 20);
	adpcmvol = -(db * 2 / 3);
	if (adpcmvol < 128)
		adpcmvolume = (tltable[FM_TLPOS + adpcmvol] * adpcmlevel) >> 12;
	else
		adpcmvolume = 0;
//	LOG2("vol = %6d   v = %8d\n", adpcmvol, tltable[FM_TLPOS + adpcmvol]);
}

// ---------------------------------------------------------------------------
//	合成
//	in:		buffer		合成先
//			nsamples	合成サンプル数
//
void OPNA::Mix(Sample* buffer, int nsamples)
{
//	LOG1("mix: %d samples  ", nsamples);
	if (fmvolume > 0)
	{
		// 準備
		// Set F-Number
		ch[0].SetFNum(fnum[0]); ch[1].SetFNum(fnum[1]);
		ch[3].SetFNum(fnum[3]); ch[4].SetFNum(fnum[4]);
		ch[5].SetFNum(fnum[5]);
		if (!(reg27 & 0xc0))
			ch[2].SetFNum(fnum[2]);
		else
		{
			// 効果音モード
			ch[2].op[0].SetFNum(fnum3[1]);	ch[2].op[1].SetFNum(fnum3[2]);
			ch[2].op[2].SetFNum(fnum3[0]);	ch[2].op[3].SetFNum(fnum[2]);
		}
		
		int activech = (ch[0].Prepare() ? 1 : 0) | (ch[1].Prepare() ? 2 : 0) | (ch[2].Prepare() ? 4 : 0);
		if (reg29 & 0x80)
			activech |= (ch[3].Prepare() ? 8 : 0) | (ch[4].Prepare() ? 16 : 0) | (ch[5].Prepare() ? 32 : 0);

		if (activech)
		{
			if (interpolation)
				Mix6I(buffer, nsamples, activech);
			else
				Mix6(buffer, nsamples, activech);
		}
		else
		{
			mixl = mixr = 0, mixdelta = 16383;
		}
	}
	
	psg.Mix(buffer, nsamples);
	ADPCMMix(buffer, nsamples);
	RhythmMix(buffer, nsamples);
}

#define IStoSample(s)	((((s) >> (FM_ISHIFT+3)) * fmvolume) >> 14)

// ---------------------------------------------------------------------------
//	合成
//
void OPNA::Mix6(Sample* buffer, int nsamples, int activech)
{
	if ((pan[0] & pan[1] & pan[2] & pan[3] & pan[4] & pan[5]) != 3)
	{
		// Mix
		ISample ibuf[4];
		ISample* idest[6];
		idest[0] = &ibuf[pan[0]];
		idest[1] = &ibuf[pan[1]];
		idest[2] = &ibuf[pan[2]];
		idest[3] = &ibuf[pan[3]];
		idest[4] = &ibuf[pan[4]];
		idest[5] = &ibuf[pan[5]];

		Sample* limit = buffer + nsamples * 2;
		for (Sample* dest = buffer; dest < limit; dest+=2)
		{
			ibuf[1] = ibuf[2] = ibuf[3] = 0;
#if FM_LIGHT
			if (activech &  1) (*idest[0]  = ch[0].Calc());
			if (activech &  2) (*idest[1] += ch[1].Calc());
			if (activech &  4) (*idest[2] += ch[2].Calc());
			if (activech &  8) (*idest[3] += ch[3].Calc());
			if (activech & 16) (*idest[4] += ch[4].Calc());
			if (activech & 32) (*idest[5] += ch[5].Calc());
#else			
			uint lforef = (lfocount >> FM_LFOCBITS) & (FM_LFOENTS-1);
			lfocount += lfodcount;
			if (activech &  1) (*idest[0]  = ch[0].Calc2(lforef));
			if (activech &  2) (*idest[1] += ch[1].Calc2(lforef));
			if (activech &  4) (*idest[2] += ch[2].Calc2(lforef));
			if (activech &  8) (*idest[3] += ch[3].Calc2(lforef));
			if (activech & 16) (*idest[4] += ch[4].Calc2(lforef));
			if (activech & 32) (*idest[5] += ch[5].Calc2(lforef));
#endif

			dest[0] += IStoSample(ibuf[2] + ibuf[3]);
			dest[1] += IStoSample(ibuf[1] + ibuf[3]);
		}
	}
	else
	{
		Sample* limit = buffer + nsamples * 2;
		for (Sample* dest = buffer; dest < limit; dest+=2)
		{
			int s = 0;
#if FM_LIGHT
			if (activech &  1) (s  = ch[0].Calc());
			if (activech &  2) (s += ch[1].Calc());
			if (activech &  4) (s += ch[2].Calc());
			if (activech &  8) (s += ch[3].Calc());
			if (activech & 16) (s += ch[4].Calc());
			if (activech & 32) (s += ch[5].Calc());
#else
			uint lforef = (lfocount >> FM_LFOCBITS) & (FM_LFOENTS-1);
			lfocount += lfodcount;
			if (activech &  1) (s  = ch[0].Calc2(lforef));
			if (activech &  2) (s += ch[1].Calc2(lforef));
			if (activech &  4) (s += ch[2].Calc2(lforef));
			if (activech &  8) (s += ch[3].Calc2(lforef));
			if (activech & 16) (s += ch[4].Calc2(lforef));
			if (activech & 32) (s += ch[5].Calc2(lforef));
#endif
			s = IStoSample(s);
			dest[0] += s;
			dest[1] += s;
		}
	}
}

// ---------------------------------------------------------------------------
//	合成 - 線形補間
//
void OPNA::Mix6I(Sample* buffer, int nsamples, int activech)
{
	if ((pan[0] & pan[1] & pan[2] & pan[3] & pan[4] & pan[5]) != 3)
	{
		// Mix
		ISample ibuf[4];
		ISample* idest[6];
		idest[0] = &ibuf[pan[0]];
		idest[1] = &ibuf[pan[1]];
		idest[2] = &ibuf[pan[2]];
		idest[3] = &ibuf[pan[3]];
		idest[4] = &ibuf[pan[4]];
		idest[5] = &ibuf[pan[5]];

		int32 delta = mixdelta;

		int32 l, r, d;
		Sample* limit = buffer + nsamples * 2;
		for (Sample* dest = buffer; dest < limit; dest+=2)
		{
			while (delta > 0)
			{
				ibuf[1] = ibuf[2] = ibuf[3] = 0;
#if FM_LIGHT
				if (activech &  1) (*idest[0]  = ch[0].Calc());
				if (activech &  2) (*idest[1] += ch[1].Calc());
				if (activech &  4) (*idest[2] += ch[2].Calc());
				if (activech &  8) (*idest[3] += ch[3].Calc());
				if (activech & 16) (*idest[4] += ch[4].Calc());
				if (activech & 32) (*idest[5] += ch[5].Calc());
#else			
				uint lforef = (lfocount >> FM_LFOCBITS) & (FM_LFOENTS-1);
				lfocount += lfodcount;
				if (activech &  1) (*idest[0]  = ch[0].Calc2(lforef));
				if (activech &  2) (*idest[1] += ch[1].Calc2(lforef));
				if (activech &  4) (*idest[2] += ch[2].Calc2(lforef));
				if (activech &  8) (*idest[3] += ch[3].Calc2(lforef));
				if (activech & 16) (*idest[4] += ch[4].Calc2(lforef));
				if (activech & 32) (*idest[5] += ch[5].Calc2(lforef));
#endif
				l = IStoSample(ibuf[2] + ibuf[3]);
				r = IStoSample(ibuf[1] + ibuf[3]);
				d = Min(mpratio, delta);
				mixl += l * d;
				mixr += r * d;
				delta -= mpratio;
			}
			dest[0] += mixl >> 14;
			dest[1] += mixr >> 14;
			mixl = l * (16384-d);
			mixr = r * (16384-d);
			delta += 16384;
		}
		mixdelta = delta;
	}
	else
	{
		int32 s, d;
		int32 m, delta;

		m = (mixl + mixr) / 2;
		delta = mixdelta;

		Sample* limit = buffer + nsamples * 2;
		
		for (Sample* dest = buffer; dest < limit; dest+=2)
		{
			while (delta > 0)
			{
				s = 0;
#if FM_LIGHT
				if (activech &  1) (s  = ch[0].Calc());
				if (activech &  2) (s += ch[1].Calc());
				if (activech &  4) (s += ch[2].Calc());
				if (activech &  8) (s += ch[3].Calc());
				if (activech & 16) (s += ch[4].Calc());
				if (activech & 32) (s += ch[5].Calc());
#else			
				uint lforef = (lfocount >> FM_LFOCBITS) & (FM_LFOENTS-1);
				lfocount += lfodcount;
				if (activech &  1) (s  = ch[0].Calc2(lforef));
				if (activech &  2) (s += ch[1].Calc2(lforef));
				if (activech &  4) (s += ch[2].Calc2(lforef));
				if (activech &  8) (s += ch[3].Calc2(lforef));
				if (activech & 16) (s += ch[4].Calc2(lforef));
				if (activech & 32) (s += ch[5].Calc2(lforef));
#endif
				s = IStoSample(s);
				d = Min(mpratio, delta);
				m += s * d;
				delta -= mpratio;
			}
			dest[0] += m >> 14;
			dest[1] += m >> 14;
			m = s * (16384-d);
			delta += 16384;
		}
		mixdelta = delta;
		mixr = mixl = m;
	}
}


#endif // BUILD_OPNA

}	// namespace FM

