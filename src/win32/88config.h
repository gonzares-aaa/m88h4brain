// ---------------------------------------------------------------------------
//  M88 - PC-8801 emulator
//	Copyright (C) cisc 1998, 1999.
// ---------------------------------------------------------------------------
//	$Id: 88config.h,v 1.8 1999/10/10 15:59:54 cisc Exp $

#if !defined(win32_88config_h)
#define win32_88config_h

#include "if/ifcommon.h"
#include "pc88/config.h"
#include "instthnk.h"

namespace PC8801
{
	void SaveConfig(Config* cfg, const char* inifile, bool writedefault);
	void LoadConfig(Config* cfg, const char* inifile, bool applydefault);
	void LoadConfigDirectory(Config* cfg, const char* inifile, const char* entry);
}

#endif // !defined(win32_88config_h)
