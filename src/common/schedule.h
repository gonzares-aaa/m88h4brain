// ---------------------------------------------------------------------------
//	Scheduling class
//	Copyright (C) cisc 1999.
// ---------------------------------------------------------------------------
//	$Id: schedule.h,v 1.11 1999/10/10 01:46:18 cisc Exp $

#ifndef common_schedule_h
#define common_schedule_h

#include "device.h"

// ---------------------------------------------------------------------------

struct SchedulerEvent
{
	int count;			// ŽžŠÔŽc‚è
	IDevice* inst;
	IDevice::TimeFunc func;
	int arg;
	int time;			// ŽžŠÔ
};

class Scheduler : public IScheduler, public ITime
{
public:
	typedef SchedulerEvent Event;

public:
	Scheduler();
	virtual ~Scheduler();

	bool Init(int maxevents = 16);
	int Proceed(int ticks);

	Event* IFCALL AddEvent(int count, IDevice* dev, IDevice::TimeFunc func, int arg=0, bool repeat=false);
	void IFCALL SetEvent(Event* ev, int count, IDevice* dev, IDevice::TimeFunc func, int arg=0, bool repeat=false);
	bool IFCALL DelEvent(IDevice* dev);
	bool IFCALL DelEvent(Event* ev);

	int IFCALL GetTime();

private:
	virtual int Execute(int ticks) = 0;
	virtual void Shorten(int ticks) = 0;
	virtual int GetTicks() = 0;

private:
	Event* events;
	int evmax;
	int evlast;
	int time;		// ?
	int etime;		// ?
	bool executing;
};

// ---------------------------------------------------------------------------

inline int IFCALL Scheduler::GetTime()
{
	return time + GetTicks();
}


#endif // common_schedule_h
