// ---------------------------------------------------------------------------
//	M88 - PC-8801 Emulator.
//	Copyright (C) cisc 1998, 1999.
// ---------------------------------------------------------------------------
//  カレンダ時計(μPD4990A) のエミュレーション
// ---------------------------------------------------------------------------
//	$Id: calender.cpp,v 1.4 1999/10/10 01:47:04 cisc Exp $
//	・TP, 1Hz 機能が未実装

//	Win32API ベースに変更(by TAN-Y)

#include "headers.h"
#include "calender.h"
#include "misc.h"

//#define LOGNAME "calender"
#include "diag.h"

using namespace PC8801;

//
//	FILETIME で現在の時刻を取得
//
static void GetLocalFileTime( LPFILETIME pFT )
{
SYSTEMTIME st;

	::GetLocalTime( &st );
	::SystemTimeToFileTime( &st, pFT );
}


// ---------------------------------------------------------------------------
//	Construct/Destruct
//
Calender::Calender(const ID& id)
: Device(id)
{
	((ULARGE_INTEGER *)&diff)->QuadPart = 0;
	Reset();
}

Calender::~Calender()
{
}

// ---------------------------------------------------------------------------
//	入・出力
//
void IOCALL Calender::Reset(uint, uint)
{
	datain = 0;
	dataoutmode = 0;
	strobe = 0;
	cmd = 0x80;
	scmd = 0;
	for (int i=0; i<6; i++)
		reg[i] = 0;
}

uint IOCALL Calender::In40(uint)
{
	if (dataoutmode)
		return IOBus::Active((reg[0] & 1) << 4, 0x10);
	else
	{
//		SYSTEMTIME st;
//		GetLocalTime(&st);
//		return (st.wSecond & 1) << 4;
		return IOBus::Active(0, 0x10);
	}
}

void IOCALL Calender::Out10(uint, uint data)
{
	pcmd = data & 7;
	datain = (data >> 3) & 1;
}

void IOCALL Calender::Out40(uint, uint data)
{
	uint modified;
	modified = strobe ^ data;
	strobe = data;
	if (modified & data & 2)
		Command();
	if (modified & data & 4)
		ShiftData();
}

// ---------------------------------------------------------------------------
//	制御
//
void Calender::Command()
{
	if (pcmd == 7)
		cmd = scmd | 0x80;
	else
		cmd = pcmd;

	LOG1("Command = %.2x\n", cmd);
	switch (cmd & 15)
	{
	case 0x00:			// register hold
		hold = true;
		dataoutmode = false;
		break;

	case 0x01:			// register shift
		hold = false;
		dataoutmode = true;
		break;
	
	case 0x02:			// time set
		SetTime();
		hold = true;
		dataoutmode = true;
		break;

	case 0x03:			// time read
		GetTime();
		hold = true;
		dataoutmode = false;
		break;
	}
}

// ---------------------------------------------------------------------------
//	データシフト
//
void Calender::ShiftData()
{
	if (hold)
	{
		if (cmd & 0x80)
		{
			// shift sreg only
			LOG1("Shift HS %d\n", datain);
			scmd = (scmd >> 1) | (datain << 3);
		}
		else
		{
			LOG1("Shift HP -\n", datain);
		}
	}
	else
	{
		if (cmd & 0x80)
		{
			reg[0] = (reg[0] >> 1) | (reg[1] << 7);
			reg[1] = (reg[1] >> 1) | (reg[2] << 7);
			reg[2] = (reg[2] >> 1) | (reg[3] << 7);
			reg[3] = (reg[3] >> 1) | (reg[4] << 7);
			reg[4] = (reg[4] >> 1) | (reg[5] << 7);
			reg[5] = (reg[5] >> 1) | (scmd   << 7);
			scmd   = (scmd   >> 1) | (datain << 3);
			LOG1("Shift -S %d\n", datain);
		}
		else
		{
			reg[0] = (reg[0] >> 1) | (reg[1] << 7);
			reg[1] = (reg[1] >> 1) | (reg[2] << 7);
			reg[2] = (reg[2] >> 1) | (reg[3] << 7);
			reg[3] = (reg[3] >> 1) | (reg[4] << 7);
			reg[4] = (reg[4] >> 1) | (datain << 7);
			LOG1("Shift -P %d\n", datain);
		}
	}
}

// ---------------------------------------------------------------------------
//	時間取得
//
void Calender::GetTime()
{
	FILETIME ct;
	SYSTEMTIME lt;

	GetLocalFileTime( &ct );
	((ULARGE_INTEGER *)&ct)->QuadPart += ((ULARGE_INTEGER *)&diff)->QuadPart;

	::FileTimeToSystemTime( &ct, &lt );

	reg[5] = NtoBCD(lt.wYear % 100);
	reg[4] = lt.wMonth * 16 + lt.wDayOfWeek;
	reg[3] = NtoBCD(lt.wDay);
	reg[2] = NtoBCD(lt.wHour);
	reg[1] = NtoBCD(lt.wMinute);
	reg[0] = NtoBCD(lt.wSecond);
}

// ---------------------------------------------------------------------------
//	時間設定
//
void Calender::SetTime()
{
	FILETIME ct, at;
	SYSTEMTIME lt, nt;

	::GetLocalTime( &lt );
	::SystemTimeToFileTime( &lt, &ct );

	nt.wYear = (cmd & 0x80) ? ( BCDtoN(reg[5]) + 1900 ): lt.wYear;
	if (nt.wYear < 1970) nt.wYear += 100;
	nt.wMonth  = (reg[4]-1) >> 4;
	nt.wDay = BCDtoN(reg[3]);
	nt.wHour = BCDtoN(reg[2]);
	nt.wMinute  = BCDtoN(reg[1]);
	nt.wSecond  = BCDtoN(reg[0]);

	::SystemTimeToFileTime( &nt, &at );

	((ULARGE_INTEGER *)&diff)->QuadPart = ((ULARGE_INTEGER *)&at)->QuadPart -
											((ULARGE_INTEGER *)&ct)->QuadPart;
}

// ---------------------------------------------------------------------------
//	状態保存
//
uint IFCALL Calender::GetStatusSize()
{
	return sizeof(Status);
}

bool IFCALL Calender::SaveStatus(uint8* s)
{
	Status* st = (Status*) s;
	st->rev = ssrev;
	GetLocalFileTime( &st-> t);
	((ULARGE_INTEGER *)&(st->t))->QuadPart +=
									((ULARGE_INTEGER *)&diff)->QuadPart;
	st->dataoutmode = dataoutmode;
	st->hold = hold;
	st->datain = datain;
	st->strobe = strobe;
	st->cmd = cmd;
	st->scmd = scmd;
	st->pcmd = pcmd;
	memcpy(st->reg, reg, 6);
	return true;
}

bool IFCALL Calender::LoadStatus(const uint8* s)
{
	const Status* st = (const Status*) s;
	if (st->rev != ssrev)
		return false;
	FILETIME ct;
	GetLocalFileTime( &ct );
	((ULARGE_INTEGER *)&diff)->QuadPart =
						((ULARGE_INTEGER *)&(st->t))->QuadPart -
						((ULARGE_INTEGER *)&ct)->QuadPart;
	dataoutmode = st->dataoutmode;
	hold = st->hold;
	datain = st->datain;
	strobe = st->strobe;
	cmd = st->cmd;
	scmd = st->scmd;
	pcmd = st->pcmd;
	memcpy(reg, st->reg, 6);
	return true;
}

// ---------------------------------------------------------------------------
//	device description
//
const Device::Descriptor Calender::descriptor = { indef, outdef };

const Device::OutFuncPtr Calender::outdef[] = 
{
	STATIC_CAST(Device::OutFuncPtr, Reset),
	STATIC_CAST(Device::OutFuncPtr, Out10),
	STATIC_CAST(Device::OutFuncPtr, Out40),
};

const Device::InFuncPtr Calender::indef[] = 
{
	STATIC_CAST(Device::InFuncPtr, In40),
};

