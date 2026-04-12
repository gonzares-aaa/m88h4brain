// ---------------------------------------------------------------------------
//	M88 - PC-8801 Emulator.
//	Copyright (C) cisc 1998, 1999.
// ---------------------------------------------------------------------------
//	$Id: wincore.cpp,v 1.31 2000/01/10 08:25:35 cisc Exp $

#include "headers.h"
#include <process.h>
#include "wincore.h"
#include "WinKeyIF.h"
#include "misc.h"
#include "pc88/config.h"
#include "status.h"
#include "device.h"
#include "version.h"
#include "file.h"
#include "pc88/diskmgr.h"
#include "pc88/opnif.h"
#include "pc88/beep.h"
#include "extdev.h"
#include "if/ifpc88.h"
#include "if/ifguid.h"
#include "pc88/pd8257.h"
#include "module.h"
#include "ui.h"
#include "lz77e.h"
#include "lz77d.h"

//#define LOGNAME "wincore"
#include "diag.h"

using namespace PC8801;

//					 0123456789abcdef
#define SNAPSHOT_ID	"M88 SnapshotData"

// ---------------------------------------------------------------------------
//	構築/消滅
//
WinCore::WinCore()
: mouse(DEV_ID('M', 'O', 'U', 'S')), modlist(0), hthread(0), execcount(0),
  extdev(0)
{
}

WinCore::~WinCore()
{
	Cleanup();
}

// ---------------------------------------------------------------------------
//	初期化
//
bool WinCore::Init
(WinUI* _ui, HWND hwnd, Draw* draw, DiskManager* disk, WinKeyIF* keyb, 
 IConfigPropBase* cp)
{
	ui = _ui;
	cfgprop = cp;

	if (!extdev)
	{
		extdev = new ExternalDevice[num_externals];
		if (!extdev)
			return false;
	}
	
	if (!PC88::Init(draw, disk))
		return false;

	if (!sound.Init(this, hwnd, 0, 3993600, 0))
		return false;

	if (!ConnectDevices(keyb))
		return false;

	if (!ConnectExternalDevices())
		return false;

	execute = false;
	shouldterminate = false;
	padenable = false;
	mouseenable = false;
	execcount = 0;
	guicount = 0;
	clock = 40;
	speed = 100;
	if (!hthread)
	{
		hthread = HANDLE(_beginthreadex(NULL, 0, ThreadEntry,
							reinterpret_cast<void*>(this), 0, &idthread));
		if (!hthread)
			return false;
	}
	return true;
}

// ---------------------------------------------------------------------------
//	後始末
//
bool WinCore::Cleanup()
{
	if (hthread)
	{
		shouldterminate = true;
		if (WAIT_TIMEOUT == WaitForSingleObject(hthread, 3000))
			TerminateThread(hthread, 0);
		CloseHandle(hthread);
		hthread = 0;
	}
	for (MLNode* node = modlist; node; )
	{
		MLNode* next = node->next;
		delete node->mod;
		delete node;
		node = next;
	}
	modlist = 0;

	delete[] extdev; extdev = 0;
	return true;
}

// ---------------------------------------------------------------------------
//	リセット
//
void WinCore::Reset()
{
	CriticalSection::Lock lock(cs_cpu);
	PC88::Reset();
}

// ---------------------------------------------------------------------------
//	Core Thread
//
uint WinCore::ThreadMain()
{
	time = GetMachineTime();
	effclock = 100;

	while (!shouldterminate)
	{
		bool sync = (draw->GetStatus() & Draw::flippable) != 0;
		SetUpdateMode(sync);
		
		if (execute)
		{
			if (sync)
				ExecuteSynchronus();
			else
				ExecuteAsynchronus();
		}
		else
		{
			Sleep(5);
			time = GetMachineTime();
		}
	}
	return 0;
}

// ---------------------------------------------------------------------------
//	サブスレッド開始点
//
uint __stdcall WinCore::ThreadEntry(void* arg)
{
	return reinterpret_cast<WinCore*>(arg)->ThreadMain();
}

// ---------------------------------------------------------------------------
//	ＣＰＵメインループ
//	clock	ＣＰＵのクロック(0.1MHz)
//	length	実行する時間 (0.01ms)
//
inline void WinCore::Execute(long clk, long length, long eff)
{
	CriticalSection::Lock lock(cs_cpu);
	execcount += clk * Proceed(length, clk, eff);
}

// ---------------------------------------------------------------------------
//	VSYNC 非同期
//
void WinCore::ExecuteAsynchronus()
{
	if (clock <= 0)
	{
		time = GetMachineTime();
		DWORD ms;
		int eclk = 0;
		do
		{
			if (clock)
				Execute(-clock, 500, effclock);
			else
				Execute(effclock, 500, effclock);
			eclk+=5;
			ms = GetMachineTime() - time;
		} while (ms < 10);

		// 今回の実効クロックを計算  但し上限は 10GHz? (^^;
		effclock = Min((Min(1000, eclk) * effclock / ms) + 1, 10000);
	}
	else
	{
		int dt = GetMachineTime() - time;
		int ms = Limit(dt, 50, 4);
		time += dt;								// time  終了予定時刻
		Execute(clock, ms * speed, clock * speed / 100);

		int idletime = time - GetMachineTime() + 10;	// idletime  余剰時間
//						LOG3("Time: %d  Ex: %d ms  It: %d ms\n", GetMachineTime(), ms, Max(0, idletime));
		if (idletime > 0)
			Sleep(idletime);
	}
}

// ---------------------------------------------------------------------------
//	VSYNC に同期
//
void WinCore::ExecuteSynchronus()
{
	int clk = clock > 0 ? clock : (clock < 0 ? -clock : 160);
	
	time = GetMachineTime()+1000/60;
	Execute(clk, 100000/60, clk);
	Update();
	draw->Flip();
}

// ---------------------------------------------------------------------------
//	実行クロックカウントの値を返し、カウンタをリセット
//
long WinCore::GetExecCount()
{
	long i = execcount;
	execcount = 0;
	return i;
}

// ---------------------------------------------------------------------------
//	実行を停止するか？
//
void WinCore::Wait(bool dowait)
{
	CriticalSection::Lock lock(cs_cpu);
	execute = !dowait;
}

// ---------------------------------------------------------------------------
//	時間を取得
//
inline uint32 WinCore::GetMachineTime()
{
	return timeGetTime();
}

// ---------------------------------------------------------------------------
//	設定を反映する
//
void WinCore::ApplyConfig(PC8801::Config* cfg)
{
	config = *cfg;
	clock = cfg->clock;
	bool vsync = (cfg->flag2 & PC8801::Config::synctovsync) != 0;
	
	if (cfg->flags & PC8801::Config::fullspeed)
		clock = 0, vsync = false;
	if (cfg->flags & PC8801::Config::cpuburst)
		clock = -clock, vsync = false;
	speed = cfg->speed / 10;
	if (speed != 100)
		vsync = false;

	PC88::ApplyConfig(cfg);
	sound.ApplyConfig(cfg);
	pad.ApplyConfig(cfg);
	mouse.ApplyConfig(cfg);

	EnablePad((cfg->flags & PC8801::Config::enablepad) != 0);
	if (padenable)
		cfg->flags &= ~PC8801::Config::enablemouse;
	EnableMouse((cfg->flags & PC8801::Config::enablemouse) != 0);
	draw->SetFlipMode(vsync);
}

// ---------------------------------------------------------------------------
//	GUI 使用フラグ設定
//
void WinCore::SetGUIFlag(bool usegui)
{
	if (usegui)
	{
		if (!guicount++)
			mouse.Enable(false);
	}
	else
	{
		if (!--guicount)
			mouse.Enable(mouseenable);
	}
}

// ---------------------------------------------------------------------------
//	Windows 用のデバイスを接続
//
bool WinCore::ConnectDevices(WinKeyIF* keyb)
{
	const static IOBus::Connector c_keyb[] = 
	{
		{ PC88::pres, IOBus::portout, WinKeyIF::reset },
		{ PC88::vrtc, IOBus::portout, WinKeyIF::vsync },
		{ 0x00, IOBus::portin, WinKeyIF::in },
		{ 0x01, IOBus::portin, WinKeyIF::in },
		{ 0x02, IOBus::portin, WinKeyIF::in },
		{ 0x03, IOBus::portin, WinKeyIF::in },
		{ 0x04, IOBus::portin, WinKeyIF::in },
		{ 0x05, IOBus::portin, WinKeyIF::in },
		{ 0x06, IOBus::portin, WinKeyIF::in },
		{ 0x07, IOBus::portin, WinKeyIF::in },
		{ 0x08, IOBus::portin, WinKeyIF::in },
		{ 0x09, IOBus::portin, WinKeyIF::in },
		{ 0x0a, IOBus::portin, WinKeyIF::in },
		{ 0x0b, IOBus::portin, WinKeyIF::in },
		{ 0x0c, IOBus::portin, WinKeyIF::in },
		{ 0x0d, IOBus::portin, WinKeyIF::in },
		{ 0x0e, IOBus::portin, WinKeyIF::in },
		{ 0x0f, IOBus::portin, WinKeyIF::in },
		{ 0, 0, 0 }
	};
	if (!bus1.Connect(keyb, c_keyb)) return false;

	if (FAILED(GetOPN1()->Connect(&sound))) return false;
	if (FAILED(GetOPN2()->Connect(&sound))) return false;
	if (FAILED(GetBEEP()->Connect(&sound))) return false;

	if (!mouse.Init(ui, this)) return false;
	return true;
}

// ---------------------------------------------------------------------------
//	PAD を接続
//
bool WinCore::EnablePad(bool enable)
{
	if (padenable == enable)
		return true;
	padenable = enable;

	CriticalSection::Lock lock(cs_cpu);
	if (enable)
	{
		const static IOBus::Connector c_pad[] =
		{
			{ PC88::popnio  , IOBus::portin, WinJoyPad::getdir },
			{ PC88::popnio+1, IOBus::portin, WinJoyPad::getbutton },
			{ PC88::vrtc,     IOBus::portout, WinJoyPad::vsync },
			{ 0, 0, 0 }
		};
		if (!bus1.Connect(&pad, c_pad)) return false;
		pad.Init();
	}
	else
	{
		bus1.Disconnect(&pad);
	}
	return true;
}

// ---------------------------------------------------------------------------
//	マウスを接続
//
bool WinCore::EnableMouse(bool enable)
{
	if (mouseenable == enable)
		return true;
	mouseenable = enable;

	CriticalSection::Lock lock(cs_cpu);
	if (enable)
	{
		const static IOBus::Connector c_mouse[] =
		{
			{ PC88::popnio  , IOBus::portin, WinMouse::getmove },
			{ PC88::popnio+1, IOBus::portin, WinMouse::getbutton },
			{ 0x40,           IOBus::portout, WinMouse::strobe },
			{ PC88::vrtc,     IOBus::portout, WinMouse::vsync },
			{ 0, 0, 0 }
		};
		if (!bus1.Connect(&mouse, c_mouse)) return false;
		ActivateMouse(true);
	}
	else
	{
		bus1.Disconnect(&mouse);
		ActivateMouse(false);
	}
	return true;
}

// ---------------------------------------------------------------------------
//	CPU1 の実行ログをダンプする
//
bool WinCore::EnableCPUDump(uint cpu, bool f)
{
	CriticalSection::Lock lock(cs_cpu);
	return (cpu ? GetCPU2() : GetCPU1())->EnableDump(f);
}

// ---------------------------------------------------------------------------
//	スナップショット保存
//
bool WinCore::SaveShapshot(const char* filename)
{
	CriticalSection::Lock lock(cs_cpu);

	bool compress = !!(config.flag2 & Config::compresssnapshot);

	uint size = devlist.GetStatusSize();
	uint8* buf = new uint8[compress ? size * 2 : size];
	if (!buf)
		return false;
	memset(buf, 0, size);

	if (devlist.SaveStatus(buf))
	{
		LZ77Enc enc;
		uint esize = size * 2;
		if (compress)
		{
			esize = enc.Encode(buf, size, buf+size+4, size-4) + 4;
			*(uint32*) (buf+size) = size;
		}

		SnapshotHeader ssh;
		memcpy(ssh.id, SNAPSHOT_ID, 16);

		ssh.major = ssmajor;
		ssh.minor = ssminor;
		ssh.datasize = size;
		ssh.basicmode = config.basicmode;
		ssh.clock = int16(config.clock);
		ssh.erambanks = uint16(config.erambanks);
		ssh.cpumode = int16(config.cpumode);
		ssh.mainsubratio = int16(config.mainsubratio);
		ssh.flags = config.flags | (esize < size ? 0x80000000 : 0);
		ssh.flag2 = config.flag2;
		for (uint i=0; i<2; i++)
			ssh.disk[i] = (int8) diskmgr->GetCurrentDisk(i);

		FileIO file;
		if (file.Open(filename, FileIO::create))
		{
			file.Write(&ssh, sizeof(ssh));
			if (esize < size)
				file.Write(buf+size, esize);
			else
				file.Write(buf, size);
		}
	}
	delete[] buf;
	return true;
}

// ---------------------------------------------------------------------------
//	スナップショット復元
//
bool WinCore::LoadShapshot(const char* filename, const char* diskname)
{
	CriticalSection::Lock lock(cs_cpu);

	FileIO file;
	if (!file.Open(filename, FileIO::readonly))
		return false;

	SnapshotHeader ssh;
	if (file.Read(&ssh, sizeof(ssh)) != sizeof(ssh))
		return false;
	if (memcmp(ssh.id, SNAPSHOT_ID, 16))
		return false;
	if (ssh.major != ssmajor || ssh.minor > ssminor)
		return false;

	// applyconfig
	const uint fl1a = Config::subcpucontrol | Config::fullspeed 
					| Config::enableopna	| Config::enablepcg
					| Config::fv15k 		| Config::cpuburst 
					| Config::cpuclockmode	| Config::digitalpalette
					| Config::opnona8		| Config::opnaona8
					| Config::enablewait;
	const uint fl2a = Config::disableopn44;

	config.flags = (config.flags & ~fl1a) | (ssh.flags & fl1a);
	config.flag2 = (config.flag2 & ~fl2a) | (ssh.flag2 & fl2a);
	config.basicmode = ssh.basicmode;
	config.clock = ssh.clock;
	config.erambanks = ssh.erambanks;
	config.cpumode = ssh.cpumode;
	config.mainsubratio = ssh.mainsubratio;
	ApplyConfig(&config);
	
	// Reset
	PC88::Reset();

	// 読み込み

	uint8* buf = new uint8[ssh.datasize];
	bool r = false;

	if (buf)
	{
		bool rd;
		if (ssh.flags & 0x80000000)
		{
			uint32 csize;
			file.Read(&csize, 4);
			uint8* cbuf = new uint8[csize];
			if (cbuf)
			{
				file.Read(cbuf, csize);
				LZ77Dec dec;
				rd = dec.Decode(buf, ssh.datasize, cbuf);
//				FileIO fio("test.dat", FileIO::create);
//				fio.Write(buf, ssh.datasize);
				delete[] cbuf;
			}
		}
		else
			rd = file.Read(buf, ssh.datasize) == ssh.datasize;

		if (rd)
		{
			r = devlist.LoadStatus(buf);
			if (r && diskname)
			{
				for (uint i=0; i<2; i++)
				{
					diskmgr->Unmount(i);
					diskmgr->Mount(i, diskname, false, ssh.disk[i], false);
				}
			}
			if (!r)
			{
				statusdisplay.Show(70, 3000, "バージョンが異なります");
				PC88::Reset();
			}
		}
		delete[] buf;
	}
	return r;
}

bool WinCore::ConnectExternalDevices()
{
	FileFinder ff;
	extern char m88dir[MAX_PATH];
	char buf[MAX_PATH];
	strncpy(buf, m88dir, MAX_PATH);
	strncat(buf, "*.m88", MAX_PATH);

	if (ff.FindFile(buf))
	{
		int i=0;
		while (ff.FindNext() && i < num_externals)
		{
			ExtendModule* em = ExtendModule::Create(ff.GetFileName(), this);
			if (em)
			{
				MLNode* node = new MLNode;
				if (!node)
				{
					delete em;
					break;
				}
				node->mod = em;
				node->next = modlist;
				modlist = node;
			}
			else
			{
				if (extdev[i].Init(ff.GetFileName(), this, &bus1, GetDMAC(), &sound, &mm1))
					devlist.Add(&extdev[i]), i++;
			}
		}
	}
	return true;
}

// ---------------------------------------------------------------------------
//	外部もじゅーるのためにインターフェースを提供する
//
void* WinCore::QueryIF(REFIID id)
{
	if (id == M88IID_IOBus1)			return static_cast<IIOBus*>(&bus1);
	if (id == M88IID_IOBus2)			return static_cast<IIOBus*>(&bus2);
	if (id == M88IID_IOAccess1)			return static_cast<IIOAccess*> (&bus1);
	if (id == M88IID_IOAccess2)			return static_cast<IIOAccess*> (&bus2);
	if (id == M88IID_MemoryManager1)	return static_cast<IMemoryManager*> (&mm1);
	if (id == M88IID_MemoryManager2)	return static_cast<IMemoryManager*> (&mm2);
	if (id == M88IID_MemoryAccess1)		return static_cast<IMemoryAccess*> (&mm1);
	if (id == M88IID_MemoryAccess2)		return static_cast<IMemoryAccess*> (&mm2);
	if (id == M88IID_SoundControl)		return static_cast<ISoundControl*> (&sound);
	if (id == M88IID_Scheduler)			return static_cast<IScheduler*> (this);
	if (id == M88IID_Time)				return static_cast<ITime*> (this);
	if (id == M88IID_CPUTime)			return static_cast<ICPUTime*> (this);
	if (id == M88IID_DMA)				return static_cast<IDMAAccess*> (GetDMAC());
	if (id == M88IID_ConfigPropBase)	return cfgprop;
	if (id == M88IID_WinUI)				return static_cast<IWinUI*> (ui);
	if (id == M88IID_LockCore)			return static_cast<ILockCore*> (this);
	
	return 0;
}
