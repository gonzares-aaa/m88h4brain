// ---------------------------------------------------------------------------
//  M88 - PC-8801 emulator
//  Copyright (C) cisc 2000.
// ---------------------------------------------------------------------------
//	$Id: lz77e.h,v 1.1 2000/01/10 08:25:05 cisc Exp $

#ifndef incl_lz77e_h
#define incl_lz77e_h

#include "types.h"

class LZ77Enc
{
public:
	LZ77Enc();
	~LZ77Enc();
	int Encode(const void* src, int len, uint8* dest, uint destsize);
	
private:
	enum
	{
		dictbits = 12,
		hashshift = 5,
		hashbits = hashshift * 3,
		dictsize = 1 << dictbits,
		hashsize = 1 << hashbits,
		threshold = 3,
	};

	void AddHashTable(const uint8*, uint);
	uint GetMatchLen(const uint8*, const uint8*, int);
	uint GetMatchLen2(const uint8*, const uint8*, int, int);
	uint Hash(const uint8* s);
	void HashAdd(uint& h, uint v);
	void EmitChar(uint c);
	void EmitLZ(uint l, uint p);
	void Flush();
	void EmitBit(uint);
	
	const uint8* src;
	const uint8** prev;
	const uint8** hash;

	uint8* buf;
	uint8* dest;
	uint32* key;
	int bufsize;
	uint32 obit;
};

#endif // incl_lz77e_h
