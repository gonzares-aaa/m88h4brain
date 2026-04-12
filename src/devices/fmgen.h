// ---------------------------------------------------------------------------
//	FM Sound Generator
//	Copyright (C) cisc 1998, 1999.
// ---------------------------------------------------------------------------
//	$Id: fmgen.h,v 1.15 1999/12/12 12:41:15 cisc Exp $

#ifndef FM_GEN_H
#define FM_GEN_H

#include "types.h"

// ---------------------------------------------------------------------------
//	定数その１
//	静的テーブルのサイズ

#define FM_EGBITS		20
#define FM_OPSINBITS	10
#define FM_LFOBITS		8
#define FM_TLBITS		7

// ---------------------------------------------------------------------------

#define FM_EGENTS		(1 << FM_EGBITS)
#define FM_OPSINENTS	(1 << FM_OPSINBITS)
#define FM_TLENTS		(1 << FM_TLBITS)
#define FM_LFOENTS		(1 << FM_LFOBITS)
#define FM_TLPOS		(FM_TLENTS/4)

// ---------------------------------------------------------------------------

namespace FM
{
	//	Types ----------------------------------------------------------------
	typedef int32	ISample;
	typedef int32 	Sample;

	enum { ARCSIZE = 74 };
	
	//	Tables (グローバルなものや asm から参照されるもの等) -----------------
	void MakeTable();
	void MakeTimeTable(uint ratio);
	extern uint32 tltable[];
	extern uint32 cltable[];
	extern uint32 dltable[];
	extern int32 sinetable[];
	extern uint32 pmtable[][FM_LFOENTS];
	extern uint32 amtable[][FM_LFOENTS];
	extern uint32 lfotable[8];

	//	Operator -------------------------------------------------------------
	class Operator
	{
	public:
		Operator();
		static void MakeTable();
		static void	MakeTimeTable(uint ratio);
		
		ISample	Calc2(ISample in);
		ISample	CalcFB2(ISample in);
		ISample	Calc(ISample in);
		ISample CalcFB(uint fb);
		void	Prepare();
		void	KeyOn();
		void	KeyOff();
		void	Reset();
		void	ResetFB();
		int		IsOn();
		
		void	SetDT(uint dt);
		void	SetMULTI(uint multi);
		void	SetTL(uint tl, bool csm);
		void	SetKS(uint ks);
		void	SetAR(uint ar);
		void	SetDR(uint dr);
		void	SetSR(uint sr);
		void	SetRR(uint rr);
		void	SetSL(uint sl);
		void	SetSSGEC(uint ssgec);
		void	SetFNum(uint fnum);
		void	SetDPBN(uint dp, uint bn);
		void	SetMode(bool modulator);
		void	SetAMON(bool on);
		void	Mute(bool);
	
	private:
		typedef uint32 Counter;
		
		ISample	out, out2;
		uint	dp;
		uint	bn;

	//	Phase Generator ------------------------------------------------------
		
		uint32	PGCalc();

		uint	detune;		// Detune
		uint	multiple;	// Multiple
		uint32	pgcount;
		uint32	pgdcount;

	//	Envelope Generator ---------------------------------------------------
		
		enum	EGPhase { next, attack, decay, sustain, release, off };
		
		int32	EGCalc();
		void	ShiftPhase(EGPhase nextphase);
		
		Counter	egcount;	// カウント値
		Counter	egdcount;	// カウント差分
		Counter eglimit;	// カウント上限
		uint32	tllinear;	// Total Level (linear)
		uint	ksr;		// key scale rate
		EGPhase	phase;
		bool	key;		// current key state
		
		uint8	tl;			// Total Level	 (0-127)
		uint8	tll;		// Total Level Latch (for CSM mode)
		uint8	ar;			// Attack Rate   (0-63)
		uint8	dr;			// Decay Rate    (0-63)
		uint8	sr;			// Sustain Rate  (0-63)
		uint8	sl;			// Sustain Level (0-127)
		uint8	rr;			// Release Rate  (0-63)
		uint8	ks;			// Keyscale      (0-3)
		uint8	ssgtype;	// SSG-Type Envelop Control

		bool	amon;		// enable Amplitude Modulation
		bool	paramchanged;	// パラメータが更新された
		bool	mute;

	//	Tables ---------------------------------------------------------------
		
		enum TableIndex { dldecay = 0, dlattack = 0x400, };
		
		static Counter d2atable[0x400];
		static Counter a2dtable[ARCSIZE];
		static Counter ratetable[64];
		static int32 dttable[256];
		static uint32 multable[16];

		friend class Channel4;
	};
	
	//	4-op Channel ----------------------------------------------------------
	class Channel4
	{
	public:
		Channel4() { SetAlgorithm(0); }
		
		ISample Calc();				// 1 sample 分計算
		ISample Calc2(uint lfo);	// 1 sample 分計算
		void SetFNum(uint fnum);
		void SetFB(uint fb);
		void SetAlgorithm(uint algo);
		int Prepare();
		void KeyControl(uint key);
		void Reset();
		void SetMS(uint ms);
		void Mute(bool);

	private:
		static const uint8 fbtable[8];
		uint	fb;
		int		buf[4];
		int*	in[3];			// 各 OP の入力ポインタ
		int*	out[3];			// 各 OP の出力ポインタ
		uint32	pmc;			// Phase Modulator (1.0 = 10000h)
		uint32	amc;			// Amplitude Modulator
		uint32*	ams;
		uint32*	pms;

	public:
		Operator op[4];
	};
}

#endif // FM_GEN_H
