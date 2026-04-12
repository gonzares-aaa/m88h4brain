// ---------------------------------------------------------------------------
//	OPN-ish Sound Generator
//	Copyright (C) cisc 1998, 1999.
// ---------------------------------------------------------------------------
//	$Id: opna.h,v 1.13 1999/12/07 00:14:44 cisc Exp $

#ifndef FM_OPNA_H
#define FM_OPNA_H

#include "fmgen.h"
#include "psg.h"

// ---------------------------------------------------------------------------
//	class OPN/OPNA
//	OPN/OPNA に良く似た音を生成する音源ユニット
//	
//	interface:
//	bool Init(uint clock, uint rate, bool interpolation, const char* path);
//		初期化．このクラスを使用する前にかならず呼んでおくこと．
//		OPNA の場合はこの関数でリズムサンプルを読み込む
//
//		clock:	OPN の動作クロック (OPNA の場合は実際の 1/2 の数値を与える)
//
//		rate:	生成する PCM のレート
//
//		inter.:	線形補完モード (OPNA のみ有効)
//				true にすると，FM 音源の合成は音源本来のレートで行うように
//				なる．最終的に生成される PCM は rate で指定されたレートになる
//				よう線形補完される
//				
//		path:	リズムサンプルのパス(OPNA のみ有効)
//				省略時はカレントディレクトリから読み込む
//				文字列の末尾には '\' や '/' などをつけること
//
//		返り値	初期化に成功すれば true
//
//	bool LoadRhythmSample(const char* path)
//		(OPNA ONLY)
//		Rhythm サンプルを読み直す．
//		path は Init の path と同じ．
//		
//	bool SetRate(uint clock, uint rate, bool interpolation)
//		クロックや PCM レートを変更する
//		引数等は Init を参照のこと．
//	
//	void Mix(Sample* dest, int nsamples)
//		Stereo PCM データを nsamples 分合成し， dest で始まる配列に
//		加える(加算する)
//		・dest には sample*2 個分の領域が必要
//		・格納形式は L, R, L, R... となる．
//		・あくまで加算なので，あらかじめ配列をゼロクリアする必要がある
//		・Sample は 32bit int を想定しているので，この関数ではクリッピングは
//		  行われない．
//		・この関数は音源内部のタイマーとは無干渉である．
//		  Timer はあくまで Count と GetNextEvent で操作する必要がある．
//	
//	void Reset()
//		音源をリセット(初期化)する
//
//	void SetReg(uint reg, uint data)
//		音源のレジスタ reg に data を書き込む
//	
//	uint GetReg(uint reg)
//		音源のレジスタ reg の内容を読み出す
//		読み込むことが出来るレジスタは PSG, ADPCM の一部，ID(0xff) とか
//	
//	uint ReadStatus()/ReadStatusEx()
//		音源のステータスレジスタを読み出す
//		ReadStatusEx は拡張ステータスレジスタの読み出し(OPNA)
//		busy フラグはエミュレートしていない
//	
//	bool Count(uint32 t)
//		音源のタイマーを t [μ秒] 進める．
//		音源の内部状態に変化があった時(timer オーバーフロー)
//		true を返す
//
//	uint32 GetNextEvent()
//		音源のタイマーのどちらかがオーバーフローするまでに必要な
//		時間[μ秒]を返す
//		タイマーが停止している場合は ULONG_MAX を返す… と思う
//	
//	void SetVolumeFM(int db)/SetVolumePSG(int db) ...
//		各音源の音量を調節する
//		単位は約 1/2 dB，有効範囲の上限は 20 (10dB)
//
namespace FM
{
	//	OPN Base -------------------------------------------------------
	class OPNBase
	{
	public:
		OPNBase();
		
		bool	Init(uint c, uint rf, uint rp);
		virtual void Reset();
		
		bool	Count(int32 us);
		int32	GetNextEvent();
		
		void	SetVolumeFM(int db);
		void	SetVolumePSG(int db);
	
	protected:
		virtual void SetStatus(uint8 bit) = 0;
		virtual void ResetStatus(uint8 bit) = 0;
		void	SetParameter(Channel4* ch, uint addr, uint data);
		void	SetTimerA(uint addr, uint data);
		void	SetTimerB(uint data);
		void	SetTimerControl(uint data);
		void	SetPrescaler(uint p);
		void	RebuildTimeTable();
		
		Channel4* csmch;
		int		fmvolume;
		
		uint	clock;
		uint	rate;
		uint	psgrate;
		uint8	status;
		uint8	reg27;
	
	private:
		uint8	prescale;
		uint8	reg24[2];
		
		int32	timera;
		int32	timera_count;
		int32	timerb;
		int32	timerb_count;
		int32	timer_count;
		int32	timer_step;

	protected:
		PSG		psg;
	};

	//	YM2203(OPN) ----------------------------------------------------
	class OPN : public OPNBase
	{
	public:
		OPN();
		virtual ~OPN() {}
		
		bool	Init(uint c, uint r, bool, const char*);
		bool	SetRate(uint c, uint r, bool);
		
		void	Reset();
		void 	Mix(Sample* buffer, int nsamples);
		void 	SetReg(uint addr, uint data);
		uint	GetReg(uint addr);
		uint	ReadStatus() { return status & 0x03; }
		uint	ReadStatusEx() { return 0xff; }
		
	private:
		virtual void Intr(bool) {}
		
		void	SetStatus(uint8 bit);
		void	ResetStatus(uint8 bit);
		
		uint	fnum[3];
		uint	fnum3[3];
		uint8	fnum2[6];
		
		Channel4 ch[3];
	};

	//	YM2608(OPNA) ---------------------------------------------------
	class OPNA : public OPNBase
	{
	public:
		OPNA();
		virtual ~OPNA();
		
		bool	Init(uint c, uint r, bool ipflag = false, const char* rhythmpath=0);
		bool	LoadRhythmSample(const char*);
	
		bool	SetRate(uint c, uint r, bool ipflag = false);
		void 	Mix(Sample* buffer, int nsamples);

		void	Reset();
		void 	SetReg(uint addr, uint data);
		uint	GetReg(uint addr);
		uint	ReadStatus() { return status & 0x03; }
		uint	ReadStatusEx();

		void	SetVolumeADPCM(int db);
		void	SetVolumeRhythmTotal(int db);
		void	SetVolumeRhythm(int index, int db);

		uint8*	GetADPCMBuffer() { return adpcmbuf; }
		void	SetChannelMask(uint mask);
		
	private:
		virtual void Intr(bool) {}
	
	private:
		struct Rhythm
		{
			uint8	pan;		// ぱん
			int8	level;		// おんりょう
			int		volume;		// おんりょうせってい
			int16*	sample;		// さんぷる
			uint	size;		// さいず
			uint	pos;		// いち
			uint	step;		// すてっぷち
			uint	rate;		// さんぷるのれーと
		};
	
		void 	Mix3(Sample* buffer, int nsamples);
		void 	Mix3I(Sample* buffer, int nsamples);
		void 	Mix6(Sample* buffer, int nsamples, int activech);
		void 	Mix6I(Sample* buffer, int nsamples, int activech);
		
		void	SetStatus(uint8 bit);
		void	ResetStatus(uint8 bit);
		void	UpdateStatus();

		void	DecodeADPCM();
		void	ADPCMMix(Sample* dest, uint count);
		void	RhythmMix(Sample* buffer, uint count);

		void	WriteRAM(uint data);
		uint	ReadRAM();
		int		ReadRAMN();
		int		DecodeADPCMSample(uint);
		
	// 線形補間用ワーク
		int32	mixl, mixr;		
		int32	mixdelta;
		int		mpratio;
		bool	interpolation;
		
	// FM 音源関係
		uint8	reg29;
		uint8	reg22;
		uint8	stmask;
		uint8	statusnext;

		uint32	lfocount;
		uint32	lfodcount;
		
		uint	fnum[6];
		uint	fnum3[6];
		
		uint8	pan[6];
		uint8	fnum2[9];

	// ADPCM 関係
		uint8*	adpcmbuf;		// ADPCM RAM
		uint	startaddr;		// Start address
		uint	stopaddr;		// Stop address
		uint	memaddr;		// 再生中アドレス
		uint	limitaddr;		// Limit address
		int		adpcmlevel;		// ADPCM 音量
		int		adpcmvolume;
		int		adpcmvol;
		uint	deltan;			// ⊿N
		int		adplc;			// 周波数変換用変数
		int		adpld;			// 周波数変換用変数差分値
		uint	adplbase;		// adpld の元
		int		adpcmx;			// ADPCM 合成用 x
		int		adpcmd;			// ADPCM 合成用 ⊿
		int		adpcmout;		// ADPCM 合成後の出力
		int		apout0;			// out(t-2)+out(t-1)
		int		apout1;			// out(t-1)+out(t)

		uint	adpcmreadbuf;	// ADPCM リード用バッファ
		bool	adpcmplay;		// ADPCM 再生中
		uint8	granuality;		

		uint8	control1;		// ADPCM コントロールレジスタ１
		uint8	control2;		// ADPCM コントロールレジスタ２
		uint8	adpcmreg[8];	// ADPCM レジスタの一部分

	// リズム音源関係
		
		Rhythm	rhythm[6];
		int8	rhythmtl;		// リズム全体の音量
		int		rhythmtvol;		
		uint8	rhythmkey;		// リズムのキー
 
		Channel4 ch[6];
	};
}

// ---------------------------------------------------------------------------

inline void FM::OPNBase::RebuildTimeTable()
{
	int p = prescale;
	prescale = -1;
	SetPrescaler(p);
}

inline void FM::OPNBase::SetVolumePSG(int db)
{
	psg.SetVolume(db);
}

#endif // FM_OPNA_H
