// ---------------------------------------------------------------------------
//	Scheduling class
//	Copyright (C) cisc 1999.
// ---------------------------------------------------------------------------
//	$Id: schedule.cpp,v 1.13 1999/11/26 10:13:25 cisc Exp $

#include "headers.h"
#include "schedule.h"
#include "misc.h"

//using namespace std;

//#define LOGNAME "schedule"
#include "diag.h"

/*
#define EVITEM	\
	{for(int i=evlast;i>=0;i--){if(events[i].inst) \
	{uint id=BSwap(events[i].inst->GetID());LOG1("<%.4s>",(char*)&id);}}LOG0("\n");}
*/

// ---------------------------------------------------------------------------

Scheduler::Scheduler()
{
	events = 0;
	evmax = evlast = 0;
}

Scheduler::~Scheduler()
{
}

// ---------------------------------------------------------------------------

bool Scheduler::Init(int mevents)
{
	delete[] events;
	
	evmax = mevents;
	evlast = -1;
	events = new Event[mevents];
	
	time = 0;
	return events != 0;
}

// ---------------------------------------------------------------------------
//	時間イベントを追加
//	
Scheduler::Event* IFCALL Scheduler::AddEvent
(int count, IDevice* inst, IDevice::TimeFunc func, int arg, bool repeat)
{
	assert(inst && func);
	assert(count > 0);
	
	int i;
	// 空いてる Event を探す
	for (i=0; i<=evlast; i++)
		if (!events[i].inst)
			break;
	if (i>=evmax)
		return 0;
	if (i>evlast)
		evlast = i;
	
	Event& ev = events[i];
	ev.count = GetTime() + count;
	ev.inst = inst, ev.func = func, ev.arg = arg;
	ev.time = repeat ? count : 0;
	
//	uint id = BSwap(inst->GetID());
//	LOG4("%.8d: %.8d %.4s count=%.8d\n", ev.count - count, ev.count, (char*)&id, count);
//	LOG0("Addevent:");
//	EVITEM;

	// 最短イベント発生時刻を更新する？
	if ((etime - ev.count) > 0)
	{
		Shorten(etime - ev.count);
		etime = ev.count;
	}
	return &ev;
}

// ---------------------------------------------------------------------------
//	時間イベントの属性変更
//	
void IFCALL Scheduler::SetEvent
(Event* ev, int count, IDevice* inst, IDevice::TimeFunc func, int arg, bool repeat)
{
	assert(inst && func);
	assert(count > 0);
	
	ev->count = GetTime() + count;
	ev->inst = inst, ev->func = func, ev->arg = arg;
	ev->time = repeat ? count : 0;
	
	// 最短イベント発生時刻を更新する？
	if ((etime - ev->count) > 0)
	{
		Shorten(etime - ev->count);
		etime = ev->count;
	}
}


// ---------------------------------------------------------------------------
//	時間イベントを削除
//	
bool IFCALL Scheduler::DelEvent(IDevice* inst)
{
	Event* ev = &events[evlast];
	for (int i=evlast; i>=0; i--, ev--)
	{
		if (ev->inst == inst)
		{
			ev->inst = 0;
			if (evlast == i)
				evlast--;
		}
	}
	return true;
}

bool IFCALL Scheduler::DelEvent(Event* ev)
{
	if (ev)
	{
		ev->inst = 0;
		if (ev - events == evlast)
			evlast--;
	}
	return true;
}

// ---------------------------------------------------------------------------
//	時間を進める
//
int Scheduler::Proceed(int ticks)
{
	int t;
	for (t=ticks; t>0; )
	{
		int i;
		int ptime = t;
		for (i=0; i<=evlast; i++)
		{
			Event& ev = events[i];
			if (ev.inst)
			{
				int l = ev.count - time;
				if (l < ptime)
					ptime = l;
			}
		}
		
		etime = time + ptime;
		
		executing = true;
		int xtime = Execute(ptime);
		etime = time += xtime;
		t -= xtime;
		executing = false;

		// イベントを駆動
//		LOG0("Proceed1:");
//		EVITEM;
		for (i=evlast; i>=0; i--)
		{
			Event& ev = events[i];

			if (ev.inst && (ev.count - time <= 0))
			{
//				uint id = BSwap(events[i].inst->GetID());
//				LOG1("%.4s: timer\n", (char*)&id);
				IDevice* inst = ev.inst;
				if (ev.time)
					ev.count += ev.time;
				else
				{
					ev.inst = 0;
					if (evlast == i)
						evlast--;
				}
				(inst->*ev.func)(ev.arg);
			}
		}
//		LOG0("Proceed2:");
//		EVITEM;
	}
	return ticks - t;
}
