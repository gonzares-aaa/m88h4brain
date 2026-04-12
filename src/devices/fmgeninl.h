// ---------------------------------------------------------------------------
//	FM Sound Generator
//	Copyright (C) cisc 1998, 1999.
// ---------------------------------------------------------------------------
//	$Id: fmgeninl.h,v 1.8 1999/12/12 12:41:15 cisc Exp $

#ifndef FM_GEN_INL_H
#define FM_GEN_INL_H

// ---------------------------------------------------------------------------
//	乗算の代わりにシフトを多用したエンジンを使用する
//	Pentium 系では若干高速になるかもしれない
//
#define FM_LIGHT		0

// ---------------------------------------------------------------------------
//	定数その２
//	
#define FM_PI			3.14159265358979323846

#define FM_CLBITS		10
#define FM_CLENTS		(1 << FM_CLBITS)

#define FM_EGCBITS		18			// eg の count のシフト値
#define FM_LFOCBITS		20
#define FM_PGBITS		9

#define FM_ISHIFT		12

#define FM_RATIOBITS	8

// ---------------------------------------------------------------------------
//	Operator
//
inline FM::Operator::Operator()
{
	// EG Part
	ar = dr = sr = rr = ksr = 0;

	// PG Part
	multiple = 0;
	detune = 0;
	
	Reset();
}

//	初期化
inline void FM::Operator::Reset()
{
	// EG part
	tllinear = 0;
	tl = tll = 127;
	ShiftPhase(off);

	// PG part
	pgcount = 0;

	// OP part
	out = out2 = 0;

	paramchanged = true;
}

//	フィードバックバッファをクリア
inline void FM::Operator::ResetFB()
{
	out = out2 = 0;
}

//	キーオン
inline void FM::Operator::KeyOn()
{
	if (!key)
	{
		key = true;
		if (phase == release || phase == off)
		{
			ShiftPhase(attack);
			out = out2 = 0;
			pgcount = 0;
		}
	}
}

//	キーオフ
inline void	FM::Operator::KeyOff()
{
	if (key)
	{
		key = false;
		ShiftPhase((rr + ksr) < 62 ? release : off);
	}
}

//	オペレータは稼働中か？
inline int FM::Operator::IsOn()
{
	return phase - off;
}

//	Detune (0-7)
inline void FM::Operator::SetDT(uint dt)
{
	detune = dt * 0x20, paramchanged = true;
}

//	Multiple (0-15)
inline void FM::Operator::SetMULTI(uint mul)	
{ 
	multiple = mul, paramchanged = true;
}

//	Total Level (0-127) (0.75dB step)
inline void FM::Operator::SetTL(uint _tl, bool csm)
{
	if (!csm)
		tl = _tl, paramchanged = true;
	tll = _tl;
}

//	Attack Rate (0-63)
inline void FM::Operator::SetAR(uint _ar)
{
	ar = _ar; paramchanged = true;
}

//	Decay Rate (0-63)
inline void FM::Operator::SetDR(uint _dr)
{ 
	dr = _dr; paramchanged = true;
}

//	Sustain Rate (0-63)
inline void FM::Operator::SetSR(uint _sr)		
{ 
	sr = _sr; paramchanged = true;
}

//	Sustain Level (0-127)
inline void FM::Operator::SetSL(uint _sl)		
{ 
	sl = _sl; paramchanged = true;
}

//	Release Rate (0-63)
inline void FM::Operator::SetRR(uint _rr)		
{ 
	rr = _rr; paramchanged = true;
}

//	Keyscale (0-3)
inline void FM::Operator::SetKS(uint _ks)		
{ 
	ks = _ks; paramchanged = true; 
}

//	SSG-type Envelop (0-15)
inline void FM::Operator::SetSSGEC(uint ssgec)	
{ 
	ssgtype = ssgec; 
}

inline void FM::Operator::SetDPBN(uint _dp, uint _bn)
{
	dp = _dp, bn = _bn; paramchanged = true; 
}

inline void FM::Operator::SetAMON(bool on)		
{ 
	amon = on;  
}

inline void FM::Operator::Mute(bool m)
{
	mute = m;
	paramchanged = true;
}

// ---------------------------------------------------------------------------
//	4-op Channel

//	セルフ・フィードバックレートの設定 (0-7)
inline void FM::Channel4::SetFB(uint feedback)
{
	fb = fbtable[feedback];
}

//	OPNA 系 LFO の設定
inline void FM::Channel4::SetMS(uint ms)
{
	ams = amtable[(ms >> 4) & 3];
	pms = pmtable[ms & 7];
}

//	チャンネル・マスク
inline void FM::Channel4::Mute(bool m)
{
	for (int i=0; i<4; i++)
		op[i].Mute(m);
}

#endif // FM_GEN_INL_H
