// ---------------------------------------------------------------------------
//	M88 - PC-8801 Emulator
//	Copyright (C) cisc 1997, 1999.
// ---------------------------------------------------------------------------
//	$Id: opnif.h,v 1.14 1999/11/26 10:13:48 cisc Exp $

#ifndef PC88_OPNIF_H
#define PC88_OPNIF_H

#include "device.h"
#include "opna.h"

// ---------------------------------------------------------------------------

class PC88;
class Scheduler;

//#define USE_OPN

namespace PC8801
{
class Config;
// ---------------------------------------------------------------------------
//	88 —p‚Ì OPN Unit
//
class OPNIF : public Device, public ISoundSource
{
public:
	enum IDFunc
	{
		reset=0, setindex0, setindex1, writedata0, writedata1, setintrmask,
		readstatus=0, readstatusex, readdata0, readdata1,
	};

public:
	OPNIF(const ID& id);
	~OPNIF();
	
	bool Init(IOBus* bus, int intrport, int io, int imask, Scheduler* s);
	
	bool IFCALL Connect(ISoundControl* c);
	bool IFCALL SetRate(uint rate);
	void IFCALL Mix(int32* buffer, int nsamples);
	
	void SetVolume(const Config* config);
	void SetFMMixMode(bool);
	
	uint IFCALL GetStatusSize();
	bool IFCALL SaveStatus(uint8* status);
	bool IFCALL LoadStatus(const uint8* status);
	
	void Enable(bool en) { enable = en; }
	void SetOPNMode(bool _opna) { opnamode = _opna; }
	const uint8* GetRegs() { return regs; }
	void SetChannelMask(uint ch);
	
	void IOCALL SetIntrMask(uint, uint intrmask);
	void IOCALL Reset		(uint=0, uint=0);
	void IOCALL SetIndex0	(uint, uint data);
	void IOCALL SetIndex1	(uint, uint data);
	void IOCALL WriteData0	(uint, uint data);
	void IOCALL WriteData1	(uint, uint data);
	uint IOCALL ReadData0	(uint);
	uint IOCALL ReadData1	(uint);
	uint IOCALL ReadStatus	(uint);
	uint IOCALL ReadStatusEx(uint);
	
	const Descriptor* IFCALL GetDesc() const { return &descriptor; }

private:
	class OPNUnit : 
#ifndef USE_OPN
	public FM::OPNA
#else
	public FM::OPN
#endif
	{
	public:
		OPNUnit() : bus(0) {}
		~OPNUnit() {}
		void Intr(bool f);
		void SetIntr(IOBus* b, int p) { bus = b, pintr = p; }
		void SetIntrMask(bool e);
		uint IntrStat() { return (intrenabled ? 1 : 0) | (intrpending ? 2 : 0); }

	private:
		IOBus* bus;
		int pintr;
		bool intrenabled;
		bool intrpending;

		friend class OPNIF;
	};

	enum
	{
		ssrev = 3,
	};
	struct Status
	{
		uint8 rev;
		uint8 i0, i1, d0, d1;
		uint8 is;
		uint8 regs[0x200];
	};

private:
	void UpdateTimer();
	void IOCALL TimeEvent(uint);

	OPNUnit opn;
	ISoundControl* soundcontrol;
	IOBus* bus;
	Scheduler* scheduler;
	int32 nextcount;
	int imaskbit;
	int prevtime;
	int portio;
	uint currentrate;
	bool fmmixmode;

	bool opnamode;
	bool enable;
	
	uint index0;
	uint index1;
	uint data1;
	
	uint8 regs[0x200];

	static int prescaler;

	static const Descriptor descriptor;
	static const InFuncPtr  indef[];
	static const OutFuncPtr outdef[];
};


inline void OPNIF::SetChannelMask(uint ch)
{
	opn.SetChannelMask(ch);
}


}

#endif // PC88_OPNIF_H
