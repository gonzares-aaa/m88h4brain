/*===========================================================================
		M88 for PocketPC
		Original Version (c)cisc
		Pocket PC Version Programmed by Y.Taniuchi ( TAN-Y )

		PocketPC / HandheldPC 共通プロパティページクラス実装
===========================================================================*/

#include "m88ce.h"

#define BSTATE(b) (b ? BST_CHECKED : BST_UNCHECKED)

using namespace PC8801;

/*===========================================================================
	class CPropCPU
===========================================================================*/

bool CPropCPU::Clicked(HWND hdlg, HWND hwctl, UINT id)
{
	switch (id)
	{
	case IDC_CPU_NOWAIT:
		GetConfig()->flags ^= Config::fullspeed;
		if (GetConfig()->flags & Config::fullspeed)
			GetConfig()->flags &= ~Config::cpuburst;
		Update(hdlg);
		return true;

	case IDC_CPU_BURST:
		GetConfig()->flags ^= Config::cpuburst;
		if (GetConfig()->flags & Config::cpuburst)
			GetConfig()->flags &= ~Config::fullspeed;
		Update(hdlg);
		return true;

	case IDC_CPU_CLOCKMODE:
		GetConfig()->flags ^= Config::cpuclockmode;
		return true;

	case IDC_CPU_NOSUBCPUCONTROL:
		GetConfig()->flags ^= Config::subcpucontrol;
		return true;

	case IDC_CPU_MS11:
		GetConfig()->cpumode = Config::ms11;
		return true;
		
	case IDC_CPU_MS21:
		GetConfig()->cpumode = Config::ms21;
		return true;
		
	case IDC_CPU_MSAUTO:
		GetConfig()->cpumode = Config::msauto;
		return true;
	
	case IDC_CPU_ENABLEWAIT:
		GetConfig()->flags ^= Config::enablewait;
		return true;
	}
	return false;
}

void CPropCPU::InitDialog(HWND hdlg)
{
	SendDlgItemMessage(IDC_CPU_SPEED, TBM_SETLINESIZE, 0, 1);
	SendDlgItemMessage(IDC_CPU_SPEED, TBM_SETPAGESIZE, 0, 2);
	SendDlgItemMessage(IDC_CPU_SPEED, TBM_SETRANGE, TRUE, MAKELONG(2, 20));
	SendDlgItemMessage(IDC_CPU_SPEED, TBM_SETPOS, FALSE, GetConfig()->speed / 100);
}

void CPropCPU::SetActive(HWND hdlg)
{
	::SetFocus(GetDlgItem(GetConfig()->flags & Config::fullspeed ? IDC_CPU_NOSUBCPUCONTROL : IDC_CPU_CLOCK));
	SendDlgItemMessage(IDC_CPU_CLOCK_SPIN, UDM_SETRANGE, 0, MAKELONG(100, 1));
	SendDlgItemMessage(IDC_CPU_CLOCK, EM_SETLIMITTEXT, 3, 0);
	SendDlgItemMessage(IDC_CPU_SPEED, TBM_SETRANGE, TRUE, MAKELONG(2, 20));
}

bool CPropCPU::Command(HWND hdlg, HWND hwctl, UINT nc, UINT id)
{
	if( GetInitialized() == false )
	{
		return false;
	}

	switch (id)
	{
	case IDC_CPU_CLOCK:
		if (nc == EN_CHANGE)
		{
			int clock = Limit(GetDlgItemInt(IDC_CPU_CLOCK, 0, false), 100, 1)*10; 
			//if (clock != GetConfig()->clock)
			//	base->PageChanged(hdlg);
			GetConfig()->clock = clock;
			return true;
		}
		break;
	}
	return false;
}

void CPropCPU::Update(HWND hdlg)
{
	SetDlgItemInt(IDC_CPU_CLOCK, GetConfig()->clock/10, false);
	CheckDlgButton(IDC_CPU_NOWAIT, BSTATE(GetConfig()->flags & Config::fullspeed));
	
	::EnableWindow(GetDlgItem(IDC_CPU_CLOCK), !(GetConfig()->flags & Config::fullspeed));
	
	::EnableWindow(GetDlgItem(IDC_CPU_SPEED), !(GetConfig()->flags & Config::cpuburst));
	::EnableWindow(GetDlgItem(IDC_CPU_SPEED_TEXT), !(GetConfig()->flags & Config::cpuburst));
	
	CheckDlgButton(IDC_CPU_NOSUBCPUCONTROL, BSTATE(!(GetConfig()->flags & Config::subcpucontrol)));
	CheckDlgButton(IDC_CPU_CLOCKMODE, BSTATE(GetConfig()->flags & Config::cpuclockmode));
	CheckDlgButton(IDC_CPU_BURST, BSTATE(GetConfig()->flags & Config::cpuburst));
	UpdateSlider(hdlg);

	static const int item[4] = 
	{ IDC_CPU_MS11, IDC_CPU_MS21, IDC_CPU_MSAUTO, IDC_CPU_MSAUTO };
	CheckDlgButton(item[GetConfig()->cpumode & 3], BSTATE(true));
	CheckDlgButton(IDC_CPU_ENABLEWAIT, BSTATE(GetConfig()->flags & Config::enablewait));
}

void CPropCPU::UpdateSlider(HWND hdlg)
{
	TCHAR buf[8];
	GetConfig()->speed = SendDlgItemMessage(IDC_CPU_SPEED, TBM_GETPOS, 0, 0) * 100;
	wsprintf(buf, _T("%d%%"), GetConfig()->speed/10);
	SetDlgItemText(IDC_CPU_SPEED_TEXT, buf);
}



/*===========================================================================
	class CPropDipSW1
===========================================================================*/

bool CPropDipSW1::Clicked(HWND hdlg, HWND hwctl, UINT id)
{
	if (IDC_DIPSW_1H <= id && id <= IDC_DIPSW_CL)
	{
		int n = (id - IDC_DIPSW_1H) / 2;
		int s = (id - IDC_DIPSW_1H) & 1;

		if (!s)	// ON
			GetConfig()->dipsw &= ~(1 << n);
		else
			GetConfig()->dipsw |= 1 << n;
		return true;
	}

	switch (id)
	{
	case IDC_DIPSW_DEFAULT:
		GetConfig()->dipsw = 1829;
		Update(hdlg);
		return true;
	}
	return false;
}


void CPropDipSW1::Update(HWND hdlg)
{
	for (int i=0; i<6; i++)
	{
		CheckDlgButton(IDC_DIPSW_1L + i*2, BSTATE(0 != (GetConfig()->dipsw & (1 << i))));
		CheckDlgButton(IDC_DIPSW_1H + i*2, BSTATE(0 == (GetConfig()->dipsw & (1 << i))));
	}
}



/*===========================================================================
	class CPropDipSW2
===========================================================================*/

bool CPropDipSW2::Clicked(HWND hdlg, HWND hwctl, UINT id)
{
	if (IDC_DIPSW_1H <= id && id <= IDC_DIPSW_CL)
	{
		int n = (id - IDC_DIPSW_1H) / 2;
		int s = (id - IDC_DIPSW_1H) & 1;

		if (!s)	// ON
			GetConfig()->dipsw &= ~(1 << n);
		else
			GetConfig()->dipsw |= 1 << n;
		return true;
	}
	return false;
}


void CPropDipSW2::Update(HWND hdlg)
{
	for (int i=6; i<12; i++)
	{
		CheckDlgButton(IDC_DIPSW_1L + i*2, BSTATE(0 != (GetConfig()->dipsw & (1 << i))));
		CheckDlgButton(IDC_DIPSW_1H + i*2, BSTATE(0 == (GetConfig()->dipsw & (1 << i))));
	}
}


/*===========================================================================
	class CPropSound1
===========================================================================*/

bool CPropSound1::Clicked(HWND hdlg, HWND hwctl, UINT id)
{
	switch (id)
	{
	case IDC_SOUND_NOSOUND:
		GetConfig()->sound = 0; 
		return true;

	case IDC_SOUND_11K:
		GetConfig()->sound = 1; 
		return true;

	case IDC_SOUND_22K:
		GetConfig()->sound = 2; 
		return true;

	case IDC_SOUND_44K:
		GetConfig()->sound = 3; 
		return true;
	
	case IDC_SOUND_48K:
		GetConfig()->sound = 5;		// !
		return true;
	}

	return false;
}

void CPropSound1::InitDialog(HWND hdlg)
{
}

void CPropSound1::SetActive(HWND hdlg)
{
	UDACCEL ud[2] = { { 0, 10 }, { 1, 100 } };
	SendDlgItemMessage(IDC_SOUND_BUFFERSPIN, UDM_SETRANGE, 0, MAKELONG(1000, 100));
	SendDlgItemMessage(IDC_SOUND_BUFFERSPIN, UDM_SETACCEL, 2, (LPARAM) ud);
	SendDlgItemMessage(IDC_SOUND_BUFFER, EM_SETLIMITTEXT, 4, 0);
}

bool CPropSound1::Command(HWND hdlg, HWND hwctl, UINT nc, UINT id)
{
	if( GetInitialized() == false )
	{
		return false;
	}

	switch (id)
	{
	case IDC_SOUND_BUFFER:
		if (nc == EN_CHANGE)
		{
			uint buf;
			buf = Limit(GetDlgItemInt(IDC_SOUND_BUFFER, 0, false), 1000, 100) / 10 * 10; 
			//if (buf != GetConfig()->soundbuffer)
			//	base->PageChanged(hdlg);
			GetConfig()->soundbuffer = buf;
			return TRUE;
		}
		break;
	}
	return false;
}

void CPropSound1::Update(HWND hdlg)
{
	static const int itemsr[5] =
	{ IDC_SOUND_NOSOUND, IDC_SOUND_11K, IDC_SOUND_22K, IDC_SOUND_44K, IDC_SOUND_55K, }; 

	CheckDlgButton(itemsr[Limit(GetConfig()->sound, 4, 0)], BSTATE(true));
	SetDlgItemInt(IDC_SOUND_BUFFER, GetConfig()->soundbuffer, false);
}



/*===========================================================================
	class CPropSound2
===========================================================================*/

bool CPropSound2::Clicked(HWND hdlg, HWND hwctl, UINT id)
{
	switch (id)
	{
	case IDC_SOUND44_OPN:
		GetConfig()->flags &= ~Config::enableopna;
		GetConfig()->flag2 &= ~Config::disableopn44;
		return true;

	case IDC_SOUND44_OPNA:
		GetConfig()->flags |= Config::enableopna;
		GetConfig()->flag2 &= ~Config::disableopn44;
		return true;

	case IDC_SOUND44_NONE:
		GetConfig()->flags &= ~Config::enableopna;
		GetConfig()->flag2 |= Config::disableopn44;
		return true;

	case IDC_SOUNDA8_OPN:
		GetConfig()->flags = (GetConfig()->flags & ~Config::opnaona8) | Config::opnona8;
		return true;

	case IDC_SOUNDA8_OPNA:
		GetConfig()->flags = (GetConfig()->flags & ~Config::opnona8) | Config::opnaona8;
		return true;

	case IDC_SOUNDA8_NONE:
		GetConfig()->flags = GetConfig()->flags & ~(Config::opnaona8 | Config::opnona8);
		return true;

	case IDC_SOUND_CMDSING:
		GetConfig()->flags ^= Config::disablesing;
		return true;

	case IDC_SOUND_MIXALWAYS:
		GetConfig()->flags ^= Config::mixsoundalways;
		return true;

	case IDC_SOUND_PRECISEMIX:
		GetConfig()->flags ^= Config::precisemixing;
		return true;

	case IDC_SOUND_WAVEOUT:
		GetConfig()->flag2 ^= Config::usewaveoutdrv;
		return true;

	case IDC_SOUND_FMFREQ:
		GetConfig()->flag2 ^= Config::usefmclock;
		return true;
	}

	return false;
}

void CPropSound2::InitDialog(HWND hdlg)
{
	CheckDlgButton(
		GetConfig()->flag2 & Config::disableopn44 ? IDC_SOUND44_NONE : 
		(GetConfig()->flags & Config::enableopna ? IDC_SOUND44_OPNA : IDC_SOUND44_OPN), 
		BSTATE(true));
	CheckDlgButton(
		GetConfig()->flags & Config::opnaona8 ? IDC_SOUNDA8_OPNA : 
		(GetConfig()->flags & Config::opnona8 ? IDC_SOUNDA8_OPN : IDC_SOUNDA8_NONE), 
		BSTATE(true));
}

void CPropSound2::SetActive(HWND hdlg)
{
}

bool CPropSound2::Command(HWND hdlg, HWND hwctl, UINT nc, UINT id)
{
	return false;
}

void CPropSound2::Update(HWND hdlg)
{
	CheckDlgButton(IDC_SOUND_CMDSING, BSTATE(!(GetConfig()->flags & Config::disablesing)));
	CheckDlgButton(IDC_SOUND_MIXALWAYS, BSTATE(GetConfig()->flags & Config::mixsoundalways));
	CheckDlgButton(IDC_SOUND_PRECISEMIX, BSTATE(GetConfig()->flags & Config::precisemixing));
	CheckDlgButton(IDC_SOUND_WAVEOUT, BSTATE(GetConfig()->flag2 & Config::usewaveoutdrv));
	CheckDlgButton(IDC_SOUND_FMFREQ, BSTATE(GetConfig()->flag2 & Config::usefmclock));

}


