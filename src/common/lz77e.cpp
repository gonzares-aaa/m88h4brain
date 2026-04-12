// ---------------------------------------------------------------------------
//  M88 - PC-8801 emulator
//  Copyright (C) cisc 2000.
// ---------------------------------------------------------------------------
//	$Id: lz77e.cpp,v 1.1 2000/01/10 08:25:05 cisc Exp $

#include "headers.h"
#include "lz77e.h"

LZ77Enc::LZ77Enc()
{
	prev = new const uint8* [dictsize];
	hash = new const uint8* [hashsize];
}

LZ77Enc::~LZ77Enc()
{
	delete[] prev;
	delete[] hash;
}

inline void LZ77Enc::AddHashTable(const uint8* at, uint hval)
{
	assert(hash[hval] != at);
	prev[(at-src) & (dictsize-1)] = hash[hval];
	hash[hval] = at;
}

inline uint LZ77Enc::GetMatchLen(const uint8* so, const uint8* d, int match)
{
	const uint8* s;
	const uint8* me = so + match;
	uint32 m;
	for (s=so; !(m = *(const uint32*)s ^ *(const uint32*) d) && s < me; s+=4, d+=4)
		;
	if (!m)
		return s+4-so;
	if (!(m & 0xffffff))
		return s+3-so;
	if (!(m & 0xffff))
		return s+2-so;
	if (!(m & 0xff))
		return s+1-so;
	return s-so;
}

inline uint LZ77Enc::GetMatchLen2(const uint8* so, const uint8* d, int minmatch, int maxmatch)
{
	const uint8* s;
	const uint8* me;
	
	for (s=so, me = so + (minmatch & ~3); s < me; s+=4, d+=4)
	{
		if (*(const uint32*)s ^ *(const uint32*) d)
			return 0;
	}

	uint32 m;
	for (me = so+maxmatch; !(m = *(const uint32*)s ^ *(const uint32*) d) && s < me; s+=4, d+=4)
		;
	
	if (!m)
		return s+4-so;
	if (!(m & 0xffffff))
		return s+3-so;
	if (!(m & 0xffff))
		return s+2-so;
	if (!(m & 0xff))
		return s+1-so;
	return s-so;
}

inline uint LZ77Enc::Hash(const uint8* s)
{
	return ((((s[0] << hashshift) ^ s[1]) << hashshift) ^ s[2]) & (hashsize-1);
}

inline void LZ77Enc::HashAdd(uint& h, uint v)
{
	h = ((h << hashshift) ^ v) & (hashsize-1);
}

inline void LZ77Enc::EmitBit(uint b)
{
	if (b)
		*key |= obit;
	if (!(obit >>= 1))
		Flush();
}

inline void LZ77Enc::EmitChar(uint c)
{
	EmitBit(1);
	*dest++ = c;
}

/*
	1 [xx]			無圧縮
	0 [xx] 0		near (3 bytes)
	0 [00]			end
	0 [xx] 1x1		200h
	0 [xx] 1x01		400h
	0 [xx] 1x00xxx	1000h

		- 1			3 bytes
		- 01		4
		  001		5
		  0001		6
		  000010	7
		  000011	8
		  000000 xxx 9 - 16
		  000001[xx] 272
*/

void LZ77Enc::EmitLZ(uint len, uint pos)
{
	int r = -(int)pos;
	EmitBit(0);
	*dest++ = r & 0xff;

	if (len == 3 && pos <= 0xff)
	{
		EmitBit(0);
		return;
	}
	EmitBit(1);
	if (pos <= 0x200)
	{
		EmitBit(r & 0x100);
		EmitBit(1);
	}
	else
	{
		if (pos <= 0x400)
		{
			EmitBit(r & 0x100);
			EmitBit(0);
			EmitBit(1);
		}
		else
		{
			EmitBit(r & 0x800);
			EmitBit(0);
			EmitBit(0);
			EmitBit(r & 0x400);
			EmitBit(r & 0x200);
			EmitBit(r & 0x100);
		}
	}
	for (uint i=3; i<=6; i++)
	{
		if (len == i) 
		{ 
			EmitBit(1); 
			return; 
		} 
		else 
		{ 
			EmitBit(0);
		}
	}
	if (len <= 8)
	{
		EmitBit(1); EmitBit(len-7);
		return;
	}
	EmitBit(0);
	if (len <= 16)
	{
		EmitBit(0); 
		EmitBit((len-9) & 4); EmitBit((len-9) & 2); EmitBit((len-9) & 1);
	}
	else
	{
		EmitBit(1);
		*dest++ = len - 17;
	}
}

void LZ77Enc::Flush()
{
	if (dest - buf > bufsize - 0x20)
	{
		//	構造化例外に変更(by TAN-Y)
		//throw (void*) 0;
		::RaiseException( 0, 0, 0, NULL );
	}
	obit = 0x80000000; key = (uint32*) dest; *key = 0; dest += 4;
}

int  LZ77Enc::Encode(const void* _s, int len, uint8* _d, uint dsize)
{
	//	構造化例外に変更(by TAN-Y)
//	try
	__try
	{
		const int searchlen = 272;
			
		src = (const uint8*) _s;
		dest = buf = _d;
		bufsize = dsize;

		obit = 0x80000000; key = (uint32*) dest; *key = 0; dest += 4;
		
		int i;
		const uint8* srctop = src + len;
		for (i=0; i<dictsize; i++)	prev[i] = src - dictsize;
		for (i=0; i<hashsize; i++)	hash[i] = src - dictsize;
		
		const uint8* lastmpos;
		int lastbest = threshold - 1;
		
		uint h = Hash(src);
		AddHashTable(src, h);
		
		const uint8* ss = src;
		for (const uint8* s = src+1; s < srctop-2; )
		{
			int best = threshold - 1;
			const uint8* matchpos;
			int maxmatch = (srctop - s < searchlen) ? srctop - s : searchlen;
			
			HashAdd(h, s[2]);
			assert(h == Hash(s));
			// get match len
			for (const uint8* p = hash[h]; s - p < dictsize; p = prev[(p-src) & (dictsize-1)])
			{
				if (p[best+1] == s[best+1])
				{
					int match = GetMatchLen2(s, p, best, maxmatch);
					if (match > best)
					{
						best = match, matchpos = p;
						if (best >= maxmatch)
						{
							best = maxmatch;
							break;
						}
					}
				}
			}
			AddHashTable(s, h);
			
			if (best < lastbest || lastbest == maxmatch)
			{
				EmitLZ(lastbest, s - lastmpos - 1);
				s++;
				for (int l = lastbest-1; l>0; l--)
				{
					HashAdd(h, s[2]);
					assert(h == Hash(s));
					
					AddHashTable(s, h);
					s++;
				}
				lastbest = threshold - 1;
			}
			else
			{
				EmitChar(s[-1]);
				lastbest = best, lastmpos = matchpos;
				s++;
			}
		}
		if (lastbest >= threshold)
		{
			EmitLZ(lastbest, s - lastmpos - 1);
			s += lastbest;
		}
		for (; s <= srctop; s++)
			EmitChar(s[-1]);

		EmitBit(0);
		*dest++ = 0;
		EmitBit(0);

		return dest - buf;
	}
//	catch (void*)
	__except( GetExceptionCode() == 0 )
	{
		return len * 2;
	}
}
