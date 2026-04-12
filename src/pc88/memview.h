// ----------------------------------------------------------------------------
//	M88 - PC-8801 series emulator
//	Copyright (C) cisc 1999.
// ----------------------------------------------------------------------------
//	メモリ監視インターフェース
// ----------------------------------------------------------------------------
//	$Id: memview.h,v 1.1 1999/06/20 05:23:54 cisc Exp $

#ifndef pc88_memview_h
#define pc88_memview_h

#include "device.h"
#include "memory.h"
#include "pc88.h"

namespace PC8801
{
// ----------------------------------------------------------------------------
//	0	N88 N80 RAM ERAM
//	60	N88 N80 RAM ERAM E0 E1 E2 E3
//	80  RAM
//	C0	RAM GV0 GV1 GV2
//	F0	RAM TV
//	
class MemoryViewer
{
public:
	enum Type
	{
		mainram, eram, n88rom, nrom, n88e0, n88e1, n88e2, n88e3,
		gvram0, gvram1, gvram2, tvram
	};
	
	MemoryViewer();
	~MemoryViewer();

	bool Init(PC88* pc);
	MemoryBus* GetBus() { return &bus; }
	void SelectBank(Type a0, Type a6, Type a8, Type ac, Type af);

private:
	Memory* mem1;
	MemoryBus bus;
};

};

#endif // pc88_memview_h
