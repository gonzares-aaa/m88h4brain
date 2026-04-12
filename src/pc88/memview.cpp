// ----------------------------------------------------------------------------
//	M88 - PC-8801 series emulator
//	Copyright (C) cisc 1999.
// ----------------------------------------------------------------------------
//	$Id: memview.cpp,v 1.3 1999/10/10 01:47:08 cisc Exp $

#include "headers.h"
#include "device.h"
#include "device_i.h"
#include "pc88/memory.h"
#include "memview.h"

using namespace PC8801;

// ----------------------------------------------------------------------------
//
//
MemoryViewer::MemoryViewer()
{
	mem1 = 0;
}

MemoryViewer::~MemoryViewer()
{
}

// ----------------------------------------------------------------------------
//
//
bool MemoryViewer::Init(PC88* pc)
{
	if (!bus.Init(0x10000 >> MemoryBus::pagebits))
		return false;
	mem1 = pc->GetMem1();
	SelectBank(mainram, mainram, mainram, mainram, mainram); 
	return true;
}

// ----------------------------------------------------------------------------
//
//
void MemoryViewer::SelectBank(Type a0, Type a6, Type a8, Type ac, Type af)
{
	uint8* p;
	// a0
	switch (a0)
	{
	case n88rom: p = mem1->GetROM()+Memory::n88; break;
	case nrom:   p = mem1->GetROM()+Memory::n80; break;
	default:	 p = mem1->GetRAM(); break;
	}
	bus.SetMemorys(0x0000, 0x6000, p);
	// a6
	switch (a6)
	{
	case n88rom: p = mem1->GetROM()+Memory::n88+0x6000; break;
	case nrom:   p = mem1->GetROM()+Memory::n80+0x6000; break;
	case n88e0: case n88e1: case n88e2: case n88e3:
		p = mem1->GetROM()+Memory::n88e+(a6-n88e0)*0x2000; break;
	default:	 p = mem1->GetRAM()+0x6000; break;
	}
	bus.SetMemorys(0x6000, 0x2000, p);
	bus.SetMemorys(0x8000, 0x7000, mem1->GetRAM()+0x8000);
//	bus.SetMemorys(0xc000, 0x3000, mem1->GetRAM()+0xc000);
	// af
	switch (af)
	{
	case tvram: p = mem1->GetTVRAM(); break;
	default:	p = mem1->GetRAM() + 0xf000; break;
	}
	bus.SetMemorys(0xf000, 0x1000, p);
}

