// ---------------------------------------------------------------------------
//	M88 - PC-8801 Series Emulator
//	Copyright (C) cisc 1999.
// ---------------------------------------------------------------------------
//	Implementation of USART(uPD8251AF)
// ---------------------------------------------------------------------------
//	$Id: sio.h,v 1.3 1999/10/10 01:47:11 cisc Exp $

#ifndef pc88_sio_h
#define pc88_sio_h

#include "device.h"

class Scheduler;

namespace PC8801
{

class SIO : public Device
{
public:
	enum
	{
		reset=0, setcontrol, setdata,
		getstatus=0, getdata,
	};

public:
	SIO(const ID& id);
	~SIO();
	bool Init(IOBus* bus, uint prxrdy, Scheduler* scheduler);

	void IOCALL Reset(uint=0, uint=0);
	void IOCALL SetControl(uint, uint d);
	void IOCALL SetData(uint, uint d);
	uint IOCALL GetStatus(uint=0);
	uint IOCALL GetData(uint=0);

	const Descriptor* IFCALL GetDesc() const { return &descriptor; }

private:
	struct Thunk
	{
		int id;
		int size;
		Thunk* next;
		uint8* data;
	};

	enum Mode { clear=0, async, sync1, sync2, sync };
	enum Parity { none='N', odd='O', even='E' };

	void GetNextData();
	void ThunkStart();
	Thunk* tape;
	Thunk* thunk;
	int pos;
	
	IOBus* bus;
	uint prxrdy;
	Scheduler* scheduler;
	uint baseclock;
	uint clock;
	uint datalen;
	uint stop;
	uint status;
	uint data;
	Mode mode;
	Parity parity;
	bool rxen;
	bool txen;

private:
	enum Status
	{
		TXRDY	= 0x01,
		RXRDY	= 0x02,
		TXE		= 0x04,
		PE		= 0x08,
		OE		= 0x10,
		FE		= 0x20,
		SYNDET	= 0x40,
		DSR		= 0x80,
	};

private:
	static const Descriptor descriptor;
	static const InFuncPtr  indef[];
	static const OutFuncPtr outdef[];
};

}

#endif // pc88_sio_h

