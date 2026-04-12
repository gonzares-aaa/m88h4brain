// ---------------------------------------------------------------------------
//	M88 - PC-88 Emulator.
//	Copyright (C) cisc 1998, 1999.
// ---------------------------------------------------------------------------
//  画面制御とグラフィックス画面合成
// ---------------------------------------------------------------------------
//	$Id: screen.cpp,v 1.23 1999/11/26 10:13:49 cisc Exp $

#include "headers.h"
#include "pc88/screen.h"
#include "pc88/memory.h"
#include "pc88/config.h"
#include "pc88/crtc.h"
#include "status.h"

//#define LOGNAME	"screen"
#include "diag.h"

using namespace PC8801;

#define GVRAMC_BIT	0xf0
#define GVRAMC_CLR	0xc0
#define GVRAM0_SET	0x10
#define GVRAM0_RES	0x00
#define GVRAM1_SET	0x20
#define GVRAM1_RES	0x00
#define GVRAM2_SET	0x80
#define GVRAM2_RES	0x40

#define GVRAMM_BIT	0x20		// 1110
#define GVRAMM_BITF	0xe0		// 1110
#define GVRAMM_SET	0x20
#define GVRAMM_ODD	0x40
#define GVRAMM_EVEN	0x80

// ---------------------------------------------------------------------------
//	原色パレット
//	RGB
const Draw::Palette Screen::palcolor[8] =
{
	{   0,  0,  0 }, {   0,  0,255 }, { 255,  0,  0 }, { 255,  0,255 },
	{   0,255,  0 }, {   0,255,255 }, { 255,255,  0 }, { 255,255,255 },
};

const uint8 Screen::palextable[2][8] =
{
	{	0,  36,  73, 109, 146, 182, 219, 255 },
	{   0, 255, 255, 255, 255, 255, 255, 255 },
};

// ---------------------------------------------------------------------------
// 構築/消滅
//
Screen::Screen(const ID& id)
: Device(id)
{
	CreateTable();
	line400 = false;
	line320 = false;
}

Screen::~Screen()
{
}

// ---------------------------------------------------------------------------
//	初期化
//
bool Screen::Init(IOBus* _bus, Memory* mem, CRTC* _crtc)
{
	bus = _bus;
	memory = mem;
	crtc = _crtc;

	palettechanged = true;
	modechanged = true;
	fv15k = false;
	texttp = false;
	pex = palextable[0];

	bgpal.red = 0;
	bgpal.green = 0;
	bgpal.blue = 0;
	gmask = 0;
	for (int c=0; c<8; c++)
	{
		pal[c].green = c & 4 ? 255 : 0;
		pal[c].red   = c & 2 ? 255 : 0;
		pal[c].blue  = c & 1 ? 255 : 0;
	}
	return true;
}

void IOCALL Screen::Reset(uint,uint)
{
	n80mode = (newmode & 2) != 0;
	palettechanged = true;
	displaygraphics = false;
	port31 = ~port31;
	Out31(0x31, ~port31);
	modechanged = true;
}

static inline Draw::Palette Avg(Draw::Palette a, Draw::Palette b)
{
	Draw::Palette c;
	c.red   = (a.red   + b.red)   / 2;
	c.green = (a.green + b.green) / 2;
	c.blue  = (a.blue  + b.blue)  / 2;
	return c;
}

// ---------------------------------------------------------------------------
//	パレットを更新
//
bool Screen::UpdatePalette(Draw* draw)
{
	int pmode;
	
	// 32 31 80  CM 53 30 53 53 53 30 dg
	pmode = displaygraphics ? 1 : 0;
	pmode |= (port53 & 1) << 6;
	pmode |= port30 & 0x22;
	pmode |= n80mode ? 0x100 : 0;
	pmode |= line320 ? 0x400 : 0;
	if (!color)
		pmode |= 0x80 | ((port53 & 14) << 1);
//	statusdisplay.Show(10, 0, "SCRN: %.3x", pmode);

	if (pmode != prevpmode || modechanged)
	{
		LOG1("p:%.2x ", pmode);
		palettechanged = true;
		prevpmode = pmode;
	}

	if (palettechanged)
	{
		palettechanged = false;
		// palette parameter is
		//	palette
		//	-textcolor(port30 & 2)
		//	-displaygraphics
		//	port32 & 0x20
		//	-port53 & 1
		//	^port53 & 6 (if not color)

		Draw::Palette xpal[10];
		if (!texttp)
		{
			for (int i=0; i<8; i++)
			{
				xpal[i].red   = pex[pal[i].red];
				xpal[i].green = pex[pal[i].green];
				xpal[i].blue  = pex[pal[i].blue];
			}
		}
		else
		{
			for (int i=0; i<8; i++)
			{
				xpal[i].red   = (pex[pal[i].red]   * 3 + ((i << 7) & 0x100)) / 4;
				xpal[i].green = (pex[pal[i].green] * 3 + ((i << 6) & 0x100)) / 4;
				xpal[i].blue  = (pex[pal[i].blue]  * 3 + ((i << 8) & 0x100)) / 4;
			}
		}
		if (gmask)
		{
			for (int i=0; i<8; i++)
			{
				if (i & ~gmask)
				{
					xpal[i].green = (xpal[i].green / 8) + 0xe0;
					xpal[i].red   = (xpal[i].red   / 8) + 0xe0;
					xpal[i].blue  = (xpal[i].blue  / 8) + 0xe0;
				}
				else
				{
					xpal[i].green = (xpal[i].green / 6) + 0;
					xpal[i].red   = (xpal[i].red   / 6) + 0;
					xpal[i].blue  = (xpal[i].blue  / 6) + 0;
				}
			}
		}

		xpal[8].red = xpal[8].green = xpal[8].blue = 0;
		xpal[9].red   = pex[bgpal.red];
		xpal[9].green = pex[bgpal.green];
		xpal[9].blue  = pex[bgpal.blue];
		
		Draw::Palette palette[0x90];
		Draw::Palette* p = palette;

		int textcolor = port30 & 2 ? 7 : 0;

		if (color)
		{
			LOG2("\ncolor  port53 = %.2x  port32 = %.2x\n", port53, port32);
			//	color mode		GG GG GR GB TE TG TR TB	
			if (port53 & 1)		// hide text plane ?
			{
				for (int gc=0; gc<9; gc++)
				{
					Draw::Palette c = displaygraphics || texttp ? xpal[gc] : xpal[8];

					for (int i=0; i<16; i++)
						*p++ = c;
				}
			}
			else
			{
				for (int gc=0; gc<9; gc++)
				{
					Draw::Palette c = displaygraphics || texttp ? xpal[gc] : xpal[8];
					
					for (int i=0; i<8; i++)
						*p++ = c;
					if (texttp)
					{
						for (int tc=0; tc<8; tc++)
							*p++ = Avg(c, palcolor[tc]);
					}
					else
					{
						*p++ = palcolor[0]; 
						for (int tc=1; tc<8; tc++)
							*p++ = palcolor[tc | textcolor];
					}
				}

				if (fv15k)
				{
					for (int i=0x80; i<0x90; i++)
						palette[i] = palcolor[0];
				}
			}
		}
		else
		{
			//	b/w mode	0  1  G  RE TE   TG TB TR

			const Draw::Palette* tpal = port32 & 0x20 ? xpal : palcolor;
			
			LOG3("\nb/w  port53 = %.2x  port32 = %.2x  port30 = %.2x\n", port53, port32, port30);
			if (port53 & 1)		// hidetext
			{
				int m = texttp || displaygraphics ? ~0 : ~1;
				for (int gc=0; gc<4; gc++)
				{
					int x = gc & m;
					if (((~x>>1) & x & 1))
					{
						for (int i=0; i<32; i++)
							*p++ = tpal[(i&7) | textcolor];
					}
					else
					{
						for (int i=0; i<32; i++)
							*p++ = xpal[9];
					}
				}
			}
			else
			{
				int m = texttp || displaygraphics ? ~0 : ~4;
				for (int gc=0; gc<16; gc++)
				{
					int x = gc & m;
					if (((((~x>>3) & (x>>2)) | x) ^ (x>>1)) & 1)
					{
						if ((x & 8) && fv15k)
							for (int i=0; i<8; i++)
								*p++ = xpal[8];
						else
							for (int i=0; i<8; i++)
								*p++ = tpal[i | textcolor];
					}
					else
					{
						for (int i=0; i<8; i++)
							*p++ = xpal[9];
					}
				}
			}
		}

//		for (int gc=0; gc<0x90; gc++)
//			LOG4("P[%.2x] = %.2x %.2x %.2x\n", gc, palette[gc].green, palette[gc].red, palette[gc].blue);
		draw->SetPalette(0x40, 0x90, palette);
		return true;
	}
	return false;
}

// ---------------------------------------------------------------------------
//	画面イメージの更新
//	arg:	region		更新領域
//
void Screen::UpdateScreen(uint8* image, int bpl, Draw::Region& region, bool refresh)
{
	// 32 80 CL 53 53 53 L4
	int gmode = line400 ? 1 : 0;
	gmode |= color ? 0x10 : (port53 & 0x0e);
	gmode |= n80mode ? 0x20 : 0;
	gmode |= line320 ? 0x40 : 0;
	if (gmode != prevgmode)
	{
		LOG1("g:%.2x ", gmode);
		prevgmode = gmode;
		modechanged = true;
	}
	
	if (modechanged || refresh)
	{
		LOG0("<modechange> ");
		modechanged = false;
		palettechanged = true;
		ClearScreen(image, bpl);
		memset(memory->GetDirtyFlag(), 1, 0x400);
	}
	if (!n80mode)
	{
		if (color)
			UpdateScreen200c(image, bpl, region);
		else
		{
			if (line400)
				UpdateScreen400b(image, bpl, region);
			else
				UpdateScreen200b(image, bpl, region);
		}
	}
	else
	{
		if (line320)
			UpdateScreen80c(image, bpl, region);
		else
		{
			if (color)
				UpdateScreen200c(image, bpl, region);
			else
				UpdateScreen80b(image, bpl, region);
		}
	}
}


// ---------------------------------------------------------------------------
//	画面更新
//
#define WRITEC0(d, a)	d = (d & ~PACK(GVRAMC_BIT)) \
			| BETable0[(a>>4)&15] | BETable1[(a>>12)&15] | BETable2[(a>>20)&15]

#define WRITEC1(d, a)	d = (d & ~PACK(GVRAMC_BIT)) \
			| BETable0[ a    &15] | BETable1[(a>> 8)&15] | BETable2[(a>>16)&15]

#define WRITEC0F(o, a)	*((packed*)(((uint8*)(d+o))+bpl)) = d[o] = (d[o] & ~PACK(GVRAMC_BIT)) \
			| BETable0[(a>>4)&15] | BETable1[(a>>12)&15] | BETable2[(a>>20)&15]

#define WRITEC1F(o, a)	*((packed*)(((uint8*)(d+o))+bpl)) = d[o] = (d[o] & ~PACK(GVRAMC_BIT)) \
			| BETable0[ a    &15] | BETable1[(a>> 8)&15] | BETable2[(a>>16)&15]


void Screen::UpdateScreen200c(uint8* image, int bpl, Draw::Region& region)
{
	uint8* dirty = memory->GetDirtyFlag();
	int y;
	for (y=0; y<1000; y+=sizeof(packed))
	{
		if (*(packed*)(&dirty[y]))
			break;
	}
	if (y < 1000)
	{
		y /= 5;
		
		int begin = y, end = y;
		
		image += 2 * bpl * y;
		dirty += 5 * y;

		Memory::quadbyte* src = memory->GetGVRAM() + y * 80;

		if (!fullline)
		{
			for (; y<200; y++, image += 2*bpl)
			{
				packed* dest = (packed*) image;

				for (int x=0; x<5; x++, dirty++, src += 16, dest += 32)
				{
					if (*dirty)
					{
						*dirty = 0;
						end = y;
						
						Memory::quadbyte* s = src;
						packed* d = (packed*) dest;
						for (int j=0; j<4; j++)
						{
							WRITEC0(d[0], s[0].pack); WRITEC1(d[1], s[0].pack);
							WRITEC0(d[2], s[1].pack); WRITEC1(d[3], s[1].pack);
							WRITEC0(d[4], s[2].pack); WRITEC1(d[5], s[2].pack);
							WRITEC0(d[6], s[3].pack); WRITEC1(d[7], s[3].pack);
							d += 8, s += 4;
						}
					}
				}
			}
		}
		else
		{
			for (; y<200; y++, image += 2*bpl)
			{
				packed* dest = (packed*) image;

				for (int x=0; x<5; x++, dirty++, src += 16, dest += 32)
				{
					if (*dirty)
					{
						*dirty = 0;
						end = y;
						
						Memory::quadbyte* s = src;
						packed* d = (packed*) dest;
						for (int j=0; j<4; j++)
						{
							WRITEC0F(0, s[0].pack); WRITEC1F(1, s[0].pack);
							WRITEC0F(2, s[1].pack); WRITEC1F(3, s[1].pack);
							WRITEC0F(4, s[2].pack); WRITEC1F(5, s[2].pack);
							WRITEC0F(6, s[3].pack); WRITEC1F(7, s[3].pack);
							d += 8, s += 4;
						}
					}
				}
			}
		}
		region.Update(2 * begin, 2 * end + 1);
	}
}

// ---------------------------------------------------------------------------
//	画面更新 (200 lines  b/w)
//
#define WRITEB0(d, a)	d = (d & ~PACK(GVRAMM_BIT)) \
			| BETable1[((a>>4) | (a>>12) | (a>>20)) & 15]

#define WRITEB1(d, a)	d = (d & ~PACK(GVRAMM_BIT)) \
			| BETable1[(a    | (a>> 8) | (a>>16)) & 15]

#define WRITEB0F(o, a)	*((packed*)(((uint8*)(d+o))+bpl)) = d[o] = (d[o] & ~PACK(GVRAMM_BIT)) \
			| BETable1[((a>>4) | (a>>12) | (a>>20)) & 15]

#define WRITEB1F(o, a)	*((packed*)(((uint8*)(d+o))+bpl)) = d[o] = (d[o] & ~PACK(GVRAMM_BIT)) \
			| BETable1[(a    | (a>> 8) | (a>>16)) & 15]


void Screen::UpdateScreen200b(uint8* image, int bpl, Draw::Region& region)
{
	uint8* dirty = memory->GetDirtyFlag();
	int y;
	for (y=0; y<1000; y+=sizeof(packed))
	{
		if (*(packed*)(&dirty[y]))
			break;
	}
	if (y < 1000)
	{
		y /= 5;
		
		int begin = y, end = y;
		
		image += 2 * bpl * y;
		dirty += 5 * y;

		Memory::quadbyte* src = memory->GetGVRAM() + y * 80;

		Memory::quadbyte mask;
		mask.byte[0] = port53 & 2 ? 0x00 : 0xff;
		mask.byte[1] = port53 & 4 ? 0x00 : 0xff;
		mask.byte[2] = port53 & 8 ? 0x00 : 0xff;
		mask.byte[3] = 0;

		if (!fullline)
		{
			for (; y<200; y++, image += 2*bpl)
			{
				packed* dest = (packed*) image;

				for (int x=0; x<5; x++, dirty++, src += 16, dest += 32)
				{
					if (*dirty)
					{
						*dirty = 0;
						end = y;
						
						Memory::quadbyte* s = src;
						packed* d = (packed*) dest;
						for (int j=0; j<4; j++)
						{
							uint32 x;
							x = s[0].pack & mask.pack; WRITEB0(d[0], x); WRITEB1(d[1], x);
							x = s[1].pack & mask.pack; WRITEB0(d[2], x); WRITEB1(d[3], x);
							x = s[2].pack & mask.pack; WRITEB0(d[4], x); WRITEB1(d[5], x);
							x = s[3].pack & mask.pack; WRITEB0(d[6], x); WRITEB1(d[7], x);
							d += 8, s += 4;
						}
					}
				}
			}
		}
		else
		{
			for (; y<200; y++, image += 2*bpl)
			{
				packed* dest = (packed*) image;

				for (int x=0; x<5; x++, dirty++, src += 16, dest += 32)
				{
					if (*dirty)
					{
						*dirty = 0;
						end = y;
						
						Memory::quadbyte* s = src;
						packed* d = (packed*) dest;
						for (int j=0; j<4; j++)
						{
							uint32 x;
							x = s[0].pack & mask.pack; WRITEB0F(0, x); WRITEB1F(1, x);
							x = s[1].pack & mask.pack; WRITEB0F(2, x); WRITEB1F(3, x);
							x = s[2].pack & mask.pack; WRITEB0F(4, x); WRITEB1F(5, x);
							x = s[3].pack & mask.pack; WRITEB0F(6, x); WRITEB1F(7, x);
							d += 8, s += 4;
						}
					}
				}
			}
		}
		region.Update(2 * begin, 2 * end + 1);
	}
}

// ---------------------------------------------------------------------------
//	画面更新 (400 lines  b/w)
//
#define WRITE400B(d, a)	(d)[0] = ((d)[0] & ~PACK(GVRAMM_BIT)) | BETable1[(a >> 4) & 15], \
						(d)[1] = ((d)[1] & ~PACK(GVRAMM_BIT)) | BETable1[(a >> 0) & 15]

void Screen::UpdateScreen400b(uint8* image, int bpl, Draw::Region& region)
{
	uint8* dirty = memory->GetDirtyFlag();
	int y;
	for (y=0; y<1000; y+=sizeof(packed))
	{
		if (*(packed*)(&dirty[y]))
			break;
	}
	if (y < 1000)
	{
		y /= 5;
		
		int begin = y, end = y;
		
		image += bpl * y;
		dirty += 5 * y;

		Memory::quadbyte* src = memory->GetGVRAM() + y * 80;

		Memory::quadbyte mask;
		mask.byte[0] = port53 & 2 ? 0x00 : 0xff;
		mask.byte[1] = port53 & 4 ? 0x00 : 0xff;
		mask.byte[2] = port53 & 8 ? 0x00 : 0xff;
		mask.byte[3] = 0;

		for (; y<200; y++, image += bpl)
		{
			uint8* dest0 = image;
			uint8* dest1 = image + 200*bpl;

			for (int x=0; x<5; x++, dirty++, src += 16, dest0 += 128, dest1 += 128)
			{
				if (*dirty)
				{
					*dirty = 0;
					end = y;
					
					Memory::quadbyte* s = src;
					packed* d0 = (packed*) dest0;
					packed* d1 = (packed*) dest1;
					for (int j=0; j<4; j++)
					{
						WRITE400B(d0  , s[0].byte[0]); WRITE400B(d1  , s[0].byte[1]);
						WRITE400B(d0+2, s[1].byte[0]); WRITE400B(d1+2, s[1].byte[1]);
						WRITE400B(d0+4, s[2].byte[0]); WRITE400B(d1+4, s[2].byte[1]);
						WRITE400B(d0+6, s[3].byte[0]); WRITE400B(d1+6, s[3].byte[1]);
						d0 += 8, d1 += 8, s += 4;
					}
				}
			}
		}
		region.Update(begin, 200 + end);
	}
}

// ---------------------------------------------------------------------------
//	画面更新
//
#define WRITE80C0(d, a)		d = (d & ~PACK(GVRAMC_BIT)) | E80Table[(a >> 4) &15]

#define WRITE80C1(d, a)		d = (d & ~PACK(GVRAMC_BIT)) | E80Table[a & 15]

#define WRITE80C0F(o, a)	*((packed*)(((uint8*)(d+o))+bpl)) = d[o] = (d[o] & ~PACK(GVRAMC_BIT)) \
							 | E80Table[(a >> 4) & 15]
#define WRITE80C1F(o, a)	*((packed*)(((uint8*)(d+o))+bpl)) = d[o] = (d[o] & ~PACK(GVRAMC_BIT)) \
							 | E80Table[a & 15]

void Screen::UpdateScreen80c(uint8* image, int bpl, Draw::Region& region)
{
	uint8* dirty = memory->GetDirtyFlag();
	int y;
	for (y=0; y<1000; y+=sizeof(packed))
	{
		if (*(packed*)(&dirty[y]))
			break;
	}
	if (y < 1000)
	{
		y /= 5;
		
		int begin = y, end = y;
		
		image += 2 * bpl * y;
		dirty += 5 * y;

		Memory::quadbyte* src = memory->GetGVRAM() + y * 80;

		if (!fullline)
		{
			for (; y<200; y++, image += 2*bpl)
			{
				packed* dest = (packed*) image;

				for (int x=0; x<5; x++, dirty++, src += 16, dest += 32)
				{
					if (*dirty)
					{
						*dirty = 0;
						end = y;
						
						Memory::quadbyte* s = src;
						packed* d = (packed*) dest;
						for (int j=0; j<4; j++)
						{
							WRITE80C0(d[0], s[0].byte[0]); WRITE80C1(d[1], s[0].byte[0]);
							WRITE80C0(d[2], s[1].byte[0]); WRITE80C1(d[3], s[1].byte[0]);
							WRITE80C0(d[4], s[2].byte[0]); WRITE80C1(d[5], s[2].byte[0]);
							WRITE80C0(d[6], s[3].byte[0]); WRITE80C1(d[7], s[3].byte[0]);
							d += 8, s += 4;
						}
					}
				}
			}
		}
		else
		{
			for (; y<200; y++, image += 2*bpl)
			{
				packed* dest = (packed*) image;

				for (int x=0; x<5; x++, dirty++, src += 16, dest += 32)
				{
					if (*dirty)
					{
						*dirty = 0;
						end = y;
						
						Memory::quadbyte* s = src;
						packed* d = (packed*) dest;
						for (int j=0; j<4; j++)
						{
							WRITE80C0F(0, s[0].byte[0]); WRITE80C1F(1, s[0].byte[0]);
							WRITE80C0F(2, s[1].byte[0]); WRITE80C1F(3, s[1].byte[0]);
							WRITE80C0F(4, s[2].byte[0]); WRITE80C1F(5, s[2].byte[0]);
							WRITE80C0F(6, s[3].byte[0]); WRITE80C1F(7, s[3].byte[0]);
							d += 8, s += 4;
						}
					}
				}
			}
		}
		region.Update(2 * begin, 2 * end + 1);
	}
}

// ---------------------------------------------------------------------------
//	画面更新 (200 lines  b/w)
//
#define WRITE80B0(d, a)	d = (d & ~PACK(GVRAMM_BIT)) | BETable1[(a>>4) & 15]

#define WRITE80B1(d, a)	d = (d & ~PACK(GVRAMM_BIT)) | BETable1[(a   ) & 15]

#define WRITE80B0F(o, a)	*((packed*)(((uint8*)(d+o))+bpl)) = d[o] = (d[o] & ~PACK(GVRAMM_BIT)) \
			| BETable1[(a>>4) & 15]

#define WRITE80B1F(o, a)	*((packed*)(((uint8*)(d+o))+bpl)) = d[o] = (d[o] & ~PACK(GVRAMM_BIT)) \
			| BETable1[(a   ) & 15]


void Screen::UpdateScreen80b(uint8* image, int bpl, Draw::Region& region)
{
	uint8* dirty = memory->GetDirtyFlag();
	int y;
	for (y=0; y<1000; y+=sizeof(packed))
	{
		if (*(packed*)(&dirty[y]))
			break;
	}
	if (y < 1000)
	{
		y /= 5;
		
		int begin = y, end = y;
		
		image += 2 * bpl * y;
		dirty += 5 * y;

		Memory::quadbyte* src = memory->GetGVRAM() + y * 80;

		Memory::quadbyte mask;
		if (!gmask)
		{
			mask.byte[0] = port53 & 2 ? 0x00 : 0xff;
			mask.byte[1] = port53 & 4 ? 0x00 : 0xff;
			mask.byte[2] = port53 & 8 ? 0x00 : 0xff;
		}
		else
		{
			mask.byte[0] = gmask & 1 ? 0x00 : 0xff;
			mask.byte[1] = gmask & 2 ? 0x00 : 0xff;
			mask.byte[2] = gmask & 4 ? 0x00 : 0xff;
		}
		mask.byte[3] = 0;

		if (!fullline)
		{
			for (; y<200; y++, image += 2*bpl)
			{
				packed* dest = (packed*) image;

				for (int x=0; x<5; x++, dirty++, src += 16, dest += 32)
				{
					if (*dirty)
					{
						*dirty = 0;
						end = y;
						
						Memory::quadbyte* s = src;
						packed* d = (packed*) dest;
						for (int j=0; j<4; j++)
						{
							WRITE80B0(d[0], s[0].byte[0]); WRITE80B1(d[1], s[0].byte[0]);
							WRITE80B0(d[2], s[1].byte[0]); WRITE80B1(d[3], s[1].byte[0]);
							WRITE80B0(d[4], s[2].byte[0]); WRITE80B1(d[5], s[2].byte[0]);
							WRITE80B0(d[6], s[3].byte[0]); WRITE80B1(d[7], s[3].byte[0]);
							d += 8, s += 4;
						}
					}
				}
			}
		}
		else
		{
			for (; y<200; y++, image += 2*bpl)
			{
				packed* dest = (packed*) image;

				for (int x=0; x<5; x++, dirty++, src += 16, dest += 32)
				{
					if (*dirty)
					{
						*dirty = 0;
						end = y;
						
						Memory::quadbyte* s = src;
						packed* d = (packed*) dest;
						for (int j=0; j<4; j++)
						{
							WRITE80B0F(0, s[0].byte[0]); WRITE80B1F(1, s[0].byte[0]);
							WRITE80B0F(2, s[1].byte[0]); WRITE80B1F(3, s[1].byte[0]);
							WRITE80B0F(4, s[2].byte[0]); WRITE80B1F(5, s[2].byte[0]);
							WRITE80B0F(6, s[3].byte[0]); WRITE80B1F(7, s[3].byte[0]);
							d += 8, s += 4;
						}
					}
				}
			}
		}
		region.Update(2 * begin, 2 * end + 1);
	}
}

// ---------------------------------------------------------------------------
//	Out 30
//	b1	CRT モードコントロール
//
void IOCALL Screen::Out30(uint, uint data)
{
//	uint i = port30 ^ data;
	port30 = data;
	crtc->SetTextSize(!(data & 0x01));
}

// ---------------------------------------------------------------------------
//	Out 31
//	b4	color / ~b/w
//	b3	show graphic plane
//	b0	200line / ~400line
//	
void IOCALL Screen::Out31(uint, uint data)
{
	int i = port31 ^ data;
	
	if (!n80mode)
	{
		if (i & 0x19)
		{
			port31 = data;
			displaygraphics = (data & 8) != 0;

			if (i & 0x11)
			{
				color = (data & 0x10) != 0;
				line400 = !(data & 0x01) && !color;
				crtc->SetTextMode(color);
			}
		}
	}
	else
	{
		line320 = (data & 0x10) != 0;
		color = line320 || (data & 0x04) != 0;
		crtc->SetTextMode(color);
		displaygraphics = (data & 8) != 0;
		
		if (i & 0xf4)
		{
			port31 = data;
			palettechanged = true;

			Pal col;
			col.green = data & 0x80 ? 7 : 0;
			col.red   = data & 0x40 ? 7 : 0;
			col.blue  = data & 0x20 ? 7 : 0;

			if (!line320)
			{
				pal[0].green = pal[0].red = pal[0].blue = 0;
				for (int j=1; j<8; j++)
					pal[j] = col;
			}
			else
			{
				if (data & 0x04)
				{
					pal[0].blue = 7; pal[0].red = 0; pal[0].green = 0;
					pal[1].blue = 7; pal[1].red = 7; pal[1].green = 0;
					pal[2].blue = 7; pal[2].red = 0; pal[2].green = 7;
				}
				else
				{
					pal[0].blue = 0; pal[0].red = 0; pal[0].green = 0;
					pal[1].blue = 0; pal[1].red = 7; pal[1].green = 0;
					pal[2].blue = 0; pal[2].red = 0; pal[2].green = 7;
				}
				pal[3] = col;
				for (int j=0; j<4; j++)
					pal[4+j] = pal[j];
			}
		}
	}
}

// ---------------------------------------------------------------------------
//	Out 32
//	b5	パレットモード
//	
void IOCALL Screen::Out32(uint, uint data)
{
	uint i = port32 ^ data;
	if (i & 0x20)
	{
		port32 = data;
		if (!color)
			palettechanged = true;
	}
}

// ---------------------------------------------------------------------------
//	Out 52
//	バックグラウンドカラー(デジタル)の指定
//
void IOCALL Screen::Out52(uint, uint data)
{
	if (!(port32 & 0x20))
	{
		bgpal.blue  = (data & 0x08) ? 255 : 0;
		bgpal.red   = (data & 0x10) ? 255 : 0;
		bgpal.green = (data & 0x20) ? 255 : 0;
		LOG1("bgpalette(d) = %6x\n", bgpal.green*0x10000+bgpal.red*0x100+bgpal.blue);
		if (!color)
			palettechanged = true;
	}
}

// ---------------------------------------------------------------------------
//	Out 53
//	画面重ねあわせの制御
//	
void IOCALL Screen::Out53(uint, uint data)
{
	LOG4("show plane(53) : %c%c%c %c\n",
		data & 8 ? '-' : '2', data & 4 ? '-' : '1', 
		data & 2 ? '-' : '0', data & 1 ? '-' : 'T');

	if ((port53 ^ data) & (color ? 0x01 : 0x0f))
	{
		port53 = data;
	}
}

// ---------------------------------------------------------------------------
//	Out 54
//	set palette #0 / BG Color	
//
void IOCALL Screen::Out54(uint, uint data)
{ 
	if (port32 & 0x20)		// is analog palette mode ? 
	{
		Pal& p = data & 0x80 ? bgpal : pal[0];

		if (data & 0x40)
			p.green = data & 7;
		else
			p.blue = data & 7, p.red = (data >> 3) & 7;
		
		LOG2("palette(a) %c = %3x\n", 
			data & 0x80 ? 'b' : '0', pal[0].green*0x100+pal[0].red*0x10+pal[0].blue);
	}
	else
	{
		pal[0].green = data & 4 ? 7 : 0;
		pal[0].red   = data & 2 ? 7 : 0;
		pal[0].blue  = data & 1 ? 7 : 0;
		LOG1("palette(d) 0 = %.3x\n", pal[0].green*0x100+pal[0].red*0x10+pal[0].blue);
	}
	palettechanged = true;
}

// ---------------------------------------------------------------------------
//	Out 55 - 5b
//	Set palette #1 to #7
//	
void IOCALL Screen::Out55to5b(uint port, uint data)
{
	Pal& p = pal[port - 0x54];
	
	if (port32 & 0x20)		// is analog palette mode?
	{
		if (data & 0x40)
			p.green = data & 7;
		else
			p.blue = data & 7, p.red = (data >> 3) & 7;
	}
	else
	{
		p.green = data & 4 ? 7 : 0;
		p.red   = data & 2 ? 7 : 0;
		p.blue  = data & 1 ? 7 : 0;
	}
	
	LOG2("palette    %d = %.3x\n", port-0x54, p.green*0x100+p.red*0x10+p.blue);
	palettechanged = true;
}


// ---------------------------------------------------------------------------
//	画面消去
//
void Screen::ClearScreen(uint8* image, int bpl)
{
	// COLOR
	
	if (color)
	{
		for (int y=0; y<400; y++, image+=bpl)
		{
			packed* ptr = (packed*) image;

			for (int v=0; v<640/sizeof(packed)/4; v++, ptr+=4)
			{
				ptr[0] = (ptr[0] & ~PACK(GVRAMC_BIT)) | PACK(GVRAMC_CLR);
				ptr[1] = (ptr[1] & ~PACK(GVRAMC_BIT)) | PACK(GVRAMC_CLR);
				ptr[2] = (ptr[2] & ~PACK(GVRAMC_BIT)) | PACK(GVRAMC_CLR);
				ptr[3] = (ptr[3] & ~PACK(GVRAMC_BIT)) | PACK(GVRAMC_CLR);
			}
		}
	}
	else
	{
		int d = line400 ? bpl : 2*bpl;
		for (int y=(line400 ? 400 : 200); y>0; y--, image+=d)
		{
			int v;
			packed* ptr = (packed*) image;

			for (v=0; v<640/sizeof(packed)/4; v++, ptr+=4)
			{
				ptr[0] = (ptr[0] & ~PACK(GVRAMM_BITF)) | PACK(GVRAMM_ODD);
				ptr[1] = (ptr[1] & ~PACK(GVRAMM_BITF)) | PACK(GVRAMM_ODD);
				ptr[2] = (ptr[2] & ~PACK(GVRAMM_BITF)) | PACK(GVRAMM_ODD);
				ptr[3] = (ptr[3] & ~PACK(GVRAMM_BITF)) | PACK(GVRAMM_ODD);
			}
			
			if (!line400)
			{
				ptr = (packed*)(image+bpl);
				for (v=0; v<640/sizeof(packed)/4; v++, ptr+=4)
				{
					ptr[0] = (ptr[0] & ~PACK(GVRAMM_BITF)) | PACK(GVRAMM_EVEN);
					ptr[1] = (ptr[1] & ~PACK(GVRAMM_BITF)) | PACK(GVRAMM_EVEN);
					ptr[2] = (ptr[2] & ~PACK(GVRAMM_BITF)) | PACK(GVRAMM_EVEN);
					ptr[3] = (ptr[3] & ~PACK(GVRAMM_BITF)) | PACK(GVRAMM_EVEN);
				}
			}
		}
	}
}

// ---------------------------------------------------------------------------
//	設定更新
//
void Screen::ApplyConfig(const Config* config)
{
	fv15k = config->IsFV15k();
	pex = palextable[(config->flags & Config::digitalpalette) ? 1 : 0];
	texttp = (config->flags & Config::specialpalette) != 0;
	bool flp = fullline;
	fullline = (config->flags & Config::fullline) != 0;
	if (fullline != flp)
		modechanged = true;
	palettechanged = true;
	newmode = config->basicmode;
	gmask = (config->flag2 / Config::mask0) & 7;
}

// ---------------------------------------------------------------------------
//	Table 作成
//
packed Screen::BETable0[1 << sizeof(packed)] = { -1 };
packed Screen::BETable1[1 << sizeof(packed)];
packed Screen::BETable2[1 << sizeof(packed)];
packed Screen::E80Table[1 << sizeof(packed)];

#ifdef ENDIAN_IS_BIG
	#define CHKBIT(i, j)	((1 << (sizeof(packed)-j)) & i)
#else
	#define CHKBIT(i, j)	((1 << j) & i)
#endif

void Screen::CreateTable()
{
	if (BETable0[0] == -1)
	{
		int i;
		for (i=0; i<(1 << sizeof(packed)); i++)
		{
			int j;
			packed p=0, q=0, r=0;

			for (j=0; j<sizeof(packed); j++)
			{
				bool chkbit = CHKBIT(i,j) != 0;
				p = (p << 8) | (chkbit ? GVRAM0_SET : GVRAM0_RES);
				q = (q << 8) | (chkbit ? GVRAM1_SET : GVRAM1_RES);
				r = (r << 8) | (chkbit ? GVRAM2_SET : GVRAM2_RES);
			}
			BETable0[i] = p;
			BETable1[i] = q;
			BETable2[i] = r;
		}

		for (i=0; i<(1 << sizeof(packed)); i++)
		{
			E80Table[i] = BETable0[(i & 0x05) | ((i & 0x05) << 1)]
						| BETable1[(i & 0x0a) | ((i & 0x0a) >> 1)]
						| PACK(GVRAM2_RES);
		}
	}
}

// ---------------------------------------------------------------------------
//	状態保存
//
uint IFCALL Screen::GetStatusSize()
{
	return sizeof(Status);
}

bool IFCALL Screen::SaveStatus(uint8* s)
{
	Status* st = (Status*) s;
	st->rev = ssrev;
	st->p30 = port30;
	st->p31 = port31;
	st->p32 = port32;
	st->p53 = port53;
	st->bgpal = bgpal;
	for (int i=0; i<8; i++)
		st->pal[i] = pal[i];
	return true;
}

bool IFCALL Screen::LoadStatus(const uint8* s)
{
	const Status* st = (const Status*) s;
	if (st->rev != ssrev)
		return false;
	Out30(0x30, st->p30);
	Out31(0x31, st->p31);
	Out32(0x32, st->p32);
	Out53(0x53, st->p53);
	bgpal = st->bgpal;
	for (int i=0; i<8; i++)
		pal[i] = st->pal[i];
	modechanged = true;
	return true;
}


// ---------------------------------------------------------------------------
//	device description
//
const Device::Descriptor Screen::descriptor =
{
	0, Screen::outdef
};

const Device::OutFuncPtr Screen::outdef[] = 
{
	STATIC_CAST(Device::OutFuncPtr, Reset),
	STATIC_CAST(Device::OutFuncPtr, Out30),
	STATIC_CAST(Device::OutFuncPtr, Out31),
	STATIC_CAST(Device::OutFuncPtr, Out32),
	STATIC_CAST(Device::OutFuncPtr, Out52),
	STATIC_CAST(Device::OutFuncPtr, Out53),
	STATIC_CAST(Device::OutFuncPtr, Out54),
	STATIC_CAST(Device::OutFuncPtr, Out55to5b),
};

