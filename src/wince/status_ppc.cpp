/*===========================================================================
		M88 for PocketPC
		Original Version (c)cisc
		Pocket PC Version Programmed by Y.Taniuchi ( TAN-Y )

		StatusDisplay クラス実装
===========================================================================*/

#include "m88ce.h"
#include "status.h"


//
//	とりあえずダミー実装ね
//

StatusDisplay statusdisplay;


StatusDisplay::StatusDisplay()
{
}

StatusDisplay::~StatusDisplay()
{
}

bool StatusDisplay::Show(int priority, int duration, char* msg, ...)
{
	return true;
}

void StatusDisplay::FDAccess(uint dr, bool hd, bool active)
{
}

void StatusDisplay::UpdateDisplay()
{
}

void StatusDisplay::WaitSubSys()
{
}


