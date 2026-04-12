// ---------------------------------------------------------------------------
//	FM Sound Generator - Core Unit
//	Copyright (C) cisc 1998, 1999.
// ---------------------------------------------------------------------------
//	$Id: fmgen.cpp,v 1.17 1999/12/28 10:34:03 cisc Exp $
// ---------------------------------------------------------------------------
//	参考:
//	FM sound generator for M.A.M.E., written by Tatsuyuki Satoh.
//
//	未実装:
//		SSG-Type Envelop
//
//	制限:
//		サンプリングレート比の設定はグローバル
//
#include "headers.h"
#include "misc.h"
#include "fmgen.h"
#include "fmgeninl.h"

#define LOGNAME "fmgen"
#include "diag.h"

#if !FM_LIGHT && !defined(_DEBUG) && (_MSC_VER >= 1200)
//	#define FM_USE_X86_CODE			// LFO は x86 版のみ有効
#endif

// ---------------------------------------------------------------------------
//	Global Table/etc
//
namespace FM
{
	const uint8 notetable[128] =
	{
		 0,  0,  0,  0,  0,  0,  0,  1,  2,  3,  3,  3,  3,  3,  3,  3, 
		 4,  4,  4,  4,  4,  4,  4,  5,  6,  7,  7,  7,  7,  7,  7,  7, 
		 8,  8,  8,  8,  8,  8,  8,  9, 10, 11, 11, 11, 11, 11, 11, 11, 
		12, 12, 12, 12, 12, 12, 12, 13, 14, 15, 15, 15, 15, 15, 15, 15, 
		16, 16, 16, 16, 16, 16, 16, 17, 18, 19, 19, 19, 19, 19, 19, 19, 
		20, 20, 20, 20, 20, 20, 20, 21, 22, 23, 23, 23, 23, 23, 23, 23, 
		24, 24, 24, 24, 24, 24, 24, 25, 26, 27, 27, 27, 27, 27, 27, 27, 
		28, 28, 28, 28, 28, 28, 28, 29, 30, 31, 31, 31, 31, 31, 31, 31, 
	};

	uint32 tltable[FM_TLPOS + FM_TLENTS];
#if FM_LIGHT
	uint32 cltable[FM_CLENTS*2];
#endif
	
	uint32 dltable[0x400 + ARCSIZE];
	int32 sinetable[FM_OPSINENTS];
	uint32 pmtable[8][FM_LFOENTS];
	uint32 amtable[4][FM_LFOENTS];
	uint32 lfotable[8];
	
	static bool tablemade = false;
	static uint currentratio = ~0;
}

namespace FM
{

// ---------------------------------------------------------------------------
//	テーブル作成
//
void MakeTable()
{
	if (tablemade)
		return;
	int i;
	Operator::MakeTable();
	tablemade = true;
	for (i=-FM_TLPOS; i<FM_TLENTS; i++)
	{
		tltable[FM_TLPOS + i] = uint(65536. * pow(2.0, i * -16. / FM_TLENTS))-1;
		LOG2("tltable[%4d] = 0x%.4x\n", i, tltable[FM_TLPOS+i]);
	}
#if FM_LIGHT
	for (i=0; i<FM_CLENTS; i++)
	{
		int c = int(((1 << (16 + FM_ISHIFT)) - 1) * pow(2.0, -i / 64.));
		cltable[i*2  ] = c;
		cltable[i*2+1] = -c;
		LOG2("cltable[%4d*2] = %6d\n", i, cltable[i*2]);
	}
#endif

	static const double pms[8] = { 0, 3.4, 6.7, 10, 14, 20, 40, 80 };
	static const double ams[4] = { 0, 1.4, 5.9, 11.8 };

	for (i=0; i<8; i++)
	{
		double pmb = pms[i] / 2400.0;
		for (int j=0; j<FM_LFOENTS; j++)
		{
			pmtable[i][j] =
				uint32(0x10000 * pow(2.0, pmb * sin(2 * FM_PI * j / FM_LFOENTS)));
//			LOG3("PM[%d][%3d] = %8x\n", i, j, pmtable[i][j]);
		}
	}
	for (i=0; i<4; i++)
	{
		double amb = -ams[i] / 40.0;
		for (int j=0; j<FM_LFOENTS; j++)
		{
			amtable[i][j] =
				uint32(0x10000 * pow(10.0, amb * (1 + sin(2 * FM_PI * j / FM_LFOENTS))));
//			LOG3("AM[%d][%3d] = %8x\n", i, j, amtable[i][j]);
		}
	}
}

//	チップのサンプリングレートと生成する音のサンプリングレートとの比を設定
void MakeTimeTable(uint ratio)
{
	if (ratio != currentratio)
	{
		currentratio = ratio;
		Operator::MakeTimeTable(ratio);

		static const int table[8] =
		{
			109,  78,  72,  68,  63,  45,  9,  6
		};

		for (int i=0; i<8; i++)
		{
			lfotable[i] = uint(78.0 / 10000.0 / table[i] * ratio * (1 << (FM_LFOBITS + FM_LFOCBITS - FM_RATIOBITS))); 
			LOG3("lfotable[%d] = %8x(%f)\n", i, lfotable[i], lfotable[i]*1.0/(1<<(FM_LFOBITS + FM_LFOCBITS)) );
		}
	}
}

// ---------------------------------------------------------------------------
//	Envelope Generator
//
//  n = .75dB step
#define EGINDEX(n)			((n) << (FM_EGBITS))

#if FM_LIGHT
	#define TOTLL(l)		(l << (FM_CLBITS-7+1))
	#define SILENT			((FM_CLENTS-1)*2)
#else
	#define TOTLL(l)		tltable[FM_TLPOS + (l << (FM_TLBITS-7))]
	#define SILENT			0
#endif

//	準備
void Operator::Prepare()
{
	if (paramchanged)
	{
		paramchanged = false;
		//	PG Part
		pgdcount = (dp + dttable[detune + bn]) * multable[multiple];

		// EG Part
		tllinear = !mute ? TOTLL(tl) : SILENT;
		ksr = bn >> (3-ks);
		
		switch (phase)
		{
		case attack:
			egdcount = ar ? ratetable[Min(63, ar+ksr)] : 0;
			break;
		case decay:
			egdcount = dr ? ratetable [Min(63, dr+ksr)] : 0;
			eglimit = EGINDEX(dldecay + sl*8);
			break;
		case sustain:
			egdcount = sr ? ratetable [Min(63, sr+ksr)] : 0;
			break;
		case release:
			egdcount =      ratetable [Min(63, rr+ksr)];
			break;
		}
	}
}

//	envelope の phase 変更
void Operator::ShiftPhase(EGPhase nextphase)
{
	if (nextphase == next)
		nextphase = EGPhase(phase + 1);
		
	switch (nextphase)
	{
	case attack:		// Attack Phase
		tl = tll;
		tllinear = !mute ? TOTLL(tl) : SILENT;
		if ((ar+ksr) < 62)
		{
			if (phase == release)
				egcount = d2atable[(egcount >> FM_EGBITS) - dldecay];
			else
				egcount = EGINDEX(dlattack);
			
			eglimit = EGINDEX(dlattack + ARCSIZE);
			egdcount = ar ? ratetable[Min(63, ar+ksr)] : 0;
			phase = attack;
			LOG2("ar=%2d  dcount=%8x\n", ar+ksr, egdcount);
			break;
		}
	case decay:			// Decay Phase
		if (sl)
		{
			egcount = EGINDEX(dldecay);
			eglimit = EGINDEX(dldecay + sl*8);
			egdcount = dr ? ratetable[Min(63, dr+ksr)] : 0;
			phase = decay;
			break;
		}
	case sustain:		// Sustain Phase
		egcount = EGINDEX(dldecay + sl*8);
		eglimit = EGINDEX(dldecay + 0x400);
		egdcount = sr ? ratetable[Min(63, sr+ksr)] : 0;
		phase = sustain;
		break;
	
	case release:		// Release Phase
		if (phase == attack || (egcount < EGINDEX(dldecay + 0x400) && phase != off))
		{
			if (phase == attack)
				egcount = a2dtable[(egcount >> FM_EGBITS) - dlattack];
			egdcount = ratetable[Min(63, rr+ksr)];
			eglimit = EGINDEX(dldecay + 0x400);
			phase = release;
			break;
		}
	case off:			// off
	default:
		egcount = EGINDEX(dldecay + 0x400) - 1;
		eglimit = EGINDEX(dldecay + 0x400);
		egdcount = 0;
		phase = off;
		break;
	}
}

#if FM_LIGHT
	#define ToTLE(a)	(Counter(a * 2))
#else
	#define ToTLE(a)	Counter(0xffff * pow(2., -a / 64.))
#endif

//	テーブルを作成
void Operator::MakeTable()
{
	//	EG Part
	int i;

	//  make decay curve
	for (i=0; i<0x400; i++)
	{
		dltable[dldecay + i] = ToTLE(i);
		LOG2("decay[%3d] = %8x\n", i, i);
	}

	//  make attack curve
	int y = 0x3ff;
	int j = 0x3ff;
	for (i=0; i<ARCSIZE; i++)
	{
		dltable[dlattack + i] = ToTLE(y);
		LOG2("attack[%3d] = %8x\n", i, y);

		a2dtable[i] = y << FM_EGBITS;
		while (j > y) 
		{
			d2atable[j--] = (dlattack + i - 1) << FM_EGBITS;
			LOG2("d2atable[%d] = %8x\n", j+1, d2atable[j+1]);
		}
		y = y - 1 - (y / 16);		// <- 多分これが正解．OPM も同じ?
	}
	while (j>=0)
	{
		d2atable[j--] = (dlattack + 0x45) << FM_EGBITS;
		LOG2("d2atable[%d] = %8x\n", j+1, d2atable[j+1]);
	}
	
	//	PG Part
	static const uint8 table2[128] =
	{
		 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, //FD=0
		 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
		 0,  0,  0,  0,  1,  1,  1,  1,  1,  1,  1,  1,  2,  2,  2,  2, //FD=1
		 2,  3,  3,  3,  4,  4,  4,  5,  5,  6,  6,  7,  8,  8,  8,  8,
		 1,  1,  1,  1,  2,  2,  2,  2,  2,  3,  3,  3,  4,  4,  4,  5, //FD=2
		 5,  6,  6,  7,  8,  8,  9, 10, 11, 12, 13, 14, 16, 16, 16, 16,
		 2,  2,  2,  2,  2,  3,  3,  3,  4,  4,  4,  5,  5,  6,  6,  7, //FD=3
		 8,  8,  9, 10, 11, 12, 13, 14, 16, 17, 19, 20, 22, 22, 22, 22,
	};
	
	for (i=0; i<128; i++)
	{
		int val = table2[i] << 1;
		dttable[i    ] =  val, dttable[i+128] = -val;
	}

#if FM_LIGHT
	double il2 = 64. / log(2.);
	for (i=1; i<FM_OPSINENTS/2; i++)
	{
		double s = -log(sin(2. * FM_PI * i / FM_OPSINENTS)) * il2;
		
		int x = Min(int(s), 1023);
		sinetable[i             ] = x * 2;
		sinetable[FM_OPSINENTS-i] = x * 2 + 1;
		
		LOG2("sin[%4d] = %8d\n", i, sinetable[i]);
	}
	sinetable[0] = sinetable[FM_OPSINENTS / 2] = 1023 * 2;
#else
	for (i=0; i<FM_OPSINENTS; i++)
		sinetable[i] = int32(0xff4 * sin(2. * FM_PI * i / FM_OPSINENTS));
#endif
}

//	クロック・サンプリングレート比に依存するテーブルを作成
void Operator::MakeTimeTable(uint ratio)
{
	int i;

	//	EG Part
	//	YM2608 の EG 分解能は 1/2048 step
	//	実際は 3 サンプルごとに 1 回計算されているような雰囲気．
	double r = double(ratio) / (1 << FM_RATIOBITS) * (1 << (FM_EGBITS - 11));
	
	Counter* p = ratetable;
	for (i=0; i<16; i++)
	{
		int b = (1 << i) / 2;
		for (int j=0; j<4; j++)
		{
			int c = Counter( r * b * (j+4) / 4 / 3 );
			*p++ = c;
			LOG3("%2d-%d  %8x\n", i, j, c);
		}
	}
	
	// PG Part
	for (i=0; i<16; i++)
	{
		int mul = i ? i * 2 : 1;
#if ((2 + FM_RATIOBITS) > FM_PGBITS)
		multable[i] = (mul * ratio) >> (2 + FM_RATIOBITS - FM_PGBITS);
#else
		multable[i] = (mul * ratio) << (FM_PGBITS - FM_RATIOBITS - 2));
#endif
	}
}

//	Block/F-Num
void Operator::SetFNum(uint f)
{
	dp = (f & 2047) << ((f >> 11) & 7);
	bn = notetable[(f >> 7) & 127];
	paramchanged = true;
	LOG1("dp = %.8x\n", dp);
}

//	static tables
uint32 Operator::multable[16];
int32  Operator::dttable[256];
Operator::Counter Operator::ratetable[64];
Operator::Counter Operator::a2dtable[ARCSIZE];
Operator::Counter Operator::d2atable[0x400];


// ---------------------------------------------------------------------------
//	4-op Channel
//
const uint8 Channel4::fbtable[8] = { 31, 7, 6, 5, 4, 3, 2, 1 };

// リセット
void Channel4::Reset()
{
	op[0].Reset();
	op[1].Reset();
	op[2].Reset();
	op[3].Reset();
}

//	Calc の用意
int Channel4::Prepare()
{
	op[0].Prepare();
	op[1].Prepare();
	op[2].Prepare();
	op[3].Prepare();
	
	static int i=3;
	if (!--i)
		i=3, LOG0("\n");
	
	return op[0].IsOn() | op[1].IsOn()
		 | op[2].IsOn() | op[3].IsOn();
}

//	F-Number/BLOCK を設定
void Channel4::SetFNum(uint f)
{
	uint dp = (f & 2047) << ((f >> 11) & 7);
	uint bn = notetable[(f >> 7) & 127];
	op[0].SetDPBN(dp, bn);
	op[1].SetDPBN(dp, bn);
	op[2].SetDPBN(dp, bn);
	op[3].SetDPBN(dp, bn);
}

//	キー制御
void Channel4::KeyControl(uint key)
{
	if (key & 0x10) op[0].KeyOn(); else op[0].KeyOff();
	if (key & 0x20) op[1].KeyOn(); else op[1].KeyOff();
	if (key & 0x40) op[2].KeyOn(); else op[2].KeyOff();
	if (key & 0x80) op[3].KeyOn(); else op[3].KeyOff();
}

//	アルゴリズムを設定
void Channel4::SetAlgorithm(uint algo)
{
	static const uint8 table1[8][6] = 
	{
		{ 0, 1, 1, 2, 2, 3 },	{ 1, 0, 0, 1, 1, 2 },
		{ 1, 1, 1, 0, 0, 2 },	{ 0, 1, 2, 1, 1, 2 },
		{ 0, 1, 2, 2, 2, 1 },	{ 0, 1, 0, 1, 0, 1 },
		{ 0, 1, 2, 1, 2, 1 },	{ 1, 0, 1, 0, 1, 0 },
	};
	
	in [0] = &buf[table1[algo][0]];
	out[0] = &buf[table1[algo][1]];
	in [1] = &buf[table1[algo][2]];
	out[1] = &buf[table1[algo][3]];
	in [2] = &buf[table1[algo][4]];
	out[2] = &buf[table1[algo][5]];
	op[0].ResetFB();
}

// ---------------------------------------------------------------------------
//	4-op Channel 
//	１サンプル合成

//	ISample を envelop count (2π) に変換するシフト量
#define IS2EC_SHIFT		((20 + FM_PGBITS) - (16 + FM_ISHIFT))

#ifdef FM_USE_X86_CODE
	#include "fmgenx86.h"
#else

// 入力: s = 20+FM_PGBITS = 29
#define Sine(s)	sinetable[((s) >> (20+FM_PGBITS-FM_OPSINBITS))&(FM_OPSINENTS-1)]

#if FM_LIGHT
	#define CMUL(a, b)			(a + b)
	static inline FM::ISample OPMUL(uint32 a, uint32 b)
	{
		return ((a + b) < FM_CLENTS * 2) ? FM::cltable[a + b] : 0;
	}
#else
	#define CMUL(a, b)			((a * b) >> 16)
	#define OPMUL(a, b)			(a * b)
#endif

//	EG 計算
//	計算式が割り出せたんだから，本当はテーブル使わずに
//	直に計算するのが真っ当なやり方なんだろうけど，面倒なのでこのまま．
inline int32 FM::Operator::EGCalc()
{
	egcount += egdcount;
	if (egcount >= eglimit)
		ShiftPhase(next);
	return CMUL(tllinear, dltable[egcount >> FM_EGBITS]);
}

//	PG 計算
//	ret:2^(20+PGBITS) / cycle
inline uint32 FM::Operator::PGCalc(/* int32 lfoa */)
{
	return pgcount += pgdcount;		// + dpms * lfoa;
}

//	OP 計算
//	in: ISample (最大 8π)
inline FM::ISample FM::Operator::Calc(ISample in)
{
	return OPMUL(EGCalc(), Sine(PGCalc() + (in << (2 + IS2EC_SHIFT))));
}

//	OP (FB) 計算
//	Self Feedback の変調最大 = 4π
inline FM::ISample FM::Operator::CalcFB(uint fb)
{
	ISample in = (out + out2)/2;
	out2 = out;
	out = OPMUL(EGCalc(), Sine(PGCalc() + ((in << (2 + IS2EC_SHIFT)) >> fb)));
	return out;
}

#undef Sine

//  合成
ISample Channel4::Calc()
{
	buf[1] = buf[2] = buf[3] = 0;

	buf[0] = op[0].CalcFB(fb);
	*out[0] += op[1].Calc(*in[0]);
	*out[1] += op[2].Calc(*in[1]);
	return *out[2] + op[3].Calc(*in[2]);
}

//	C++ 版では LFO 未サポート故に Calc と同じ
ISample Channel4::Calc2(uint)
{
	buf[1] = buf[2] = buf[3] = 0;
	
	buf[0] = op[0].CalcFB(fb);
	*out[0] += op[1].Calc(*in[0]);
	*out[1] += op[2].Calc(*in[1]);
	return *out[2] + op[3].Calc(*in[2]);
}
#endif // FM_USE_X86_CODE

}	// namespace FM
