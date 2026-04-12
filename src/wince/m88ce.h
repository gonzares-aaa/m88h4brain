/*===========================================================================
		M88 for PocketPC / for HandheldPC
		Original Version (c)cisc
		WindowsCE Version Programmed by Y.Taniuchi ( TAN-Y )

		WindowsCE 版基幹コード用ヘッダファイル
		PocketPC / HandlheldPC 共用部

		TAN-Y 独自設計の部分に関しては必ずしも cisc さんの設計思想に準じて
		いるわけではないのであしからず....
===========================================================================*/

#include "headers.h"
#include "CThread.h"
#include "critsect.h"
#include "draw.h"
#include "pc88.h"
#include "beep.h"
#include "opnif.h"
#include "config.h"
#include "diskmgr.h"
#include "wcesound.h"

#include <atlbase.h>
extern CComModule _Module;
#include <atlwin.h>

#define M88P_REL			"0.06"
#define M88CE_DEFFILENAME	"M88ce.def" 

#define RELEASE_OBJ(x) if( x!=NULL ){ delete x; x=NULL; }
#define RELEASE_COMOBJ(x) if( x!=NULL ){ x->Release(); x=NULL; }
#define RELEASE_ARRAY(x) if( x!=NULL ){ delete[] x; x=NULL; }
#define RELEASE_STR(x) RELEASE_ARRAY(x)


#include "resource.h"

/*--------------------------------------------------------------------------
	ATL ベースの設定プロパティーシートクラス
--------------------------------------------------------------------------*/

template <class T, class TBase = CWindow>
class ATL_NO_VTABLE CConfigPropSheetImpl
	: public CDialogImpl< TBase >
{
private:
	PC8801::Config *m_pConfig;
	HPROPSHEETPAGE m_hPSP;
	bool m_blInitialized;

public:
	CConfigPropSheetImpl( PC8801::Config *pConfig )
		: CDialogImpl< TBase >()
	{
		m_pConfig = pConfig;
		m_hPSP = NULL;
		m_blInitialized = false;
	}

	~CConfigPropSheetImpl()
	{
		if( m_hPSP != NULL )
		{
			::CloseHandle( m_hPSP );
		}
	}


	BEGIN_MSG_MAP( CConfigPropSheetImpl )
		MESSAGE_HANDLER( WM_INITDIALOG, OnInitDialog )
		MESSAGE_HANDLER( WM_COMMAND, OnCommand )
		MESSAGE_HANDLER( WM_HSCROLL, OnHScroll )

		NOTIFY_CODE_HANDLER( PSN_SETACTIVE, OnSetActive )
		NOTIFY_CODE_HANDLER( PSN_APPLY, OnApply )
//		NOTIFY_CODE_HANDLER( PSN_QUERYCANCEL, OnApply )
	END_MSG_MAP()

	LRESULT OnInitDialog( UINT uMsg, WPARAM wParam, 
							LPARAM lParam, BOOL &blHandled)
	{
		InitDialog( m_hWnd );
		Update( m_hWnd );
		m_blInitialized = true;
		return TRUE;
	}

	LRESULT OnCommand( UINT uMsg, WPARAM wParam, 
							LPARAM lParam, BOOL &blHandled)
	{
		if( HIWORD(wParam) == BN_CLICKED )
		{
			if( Clicked( m_hWnd, HWND(lParam), LOWORD(wParam) ) )
			{
				return TRUE;
			}
		}
		return Command( m_hWnd, HWND(lParam), HIWORD(wParam), LOWORD(wParam) );
	}


	LRESULT OnHScroll( UINT uMsg, WPARAM wParam, 
							LPARAM lParam, BOOL &blHandled)
	{
		UpdateSlider( m_hWnd );
		return TRUE;
	}

	LRESULT OnSetActive( WPARAM wParam, 
							LPNMHDR pNMHDR, BOOL& blHandled )
	{
		SetActive( m_hWnd );
		Update( m_hWnd );
		return TRUE;
	}

	LRESULT OnApply( WPARAM wParam, 
							LPNMHDR pNMHDR, BOOL& blHandled )
	{
		Apply( m_hWnd );
		return PSNRET_NOERROR;
	}

	virtual void InitDialog(HWND hdlg) {}
	virtual void Update(HWND hdlg) {}
	virtual void UpdateSlider(HWND hdlg) {}
	virtual void SetActive(HWND hdlg) {}
	virtual bool Clicked(HWND hdlg, HWND hwctl, UINT id) { return false; }
	virtual bool Command(HWND hdlg, HWND hwctl, UINT nc, UINT id) { return false; }
	virtual void Apply(HWND hdlg) {}


	HPROPSHEETPAGE Create( void )
	{
	PROPSHEETPAGE psp;

		memset( &psp, 0, sizeof(PROPSHEETPAGE) );
		psp.dwSize = sizeof(PROPSHEETPAGE);
		psp.dwFlags = PSP_USECALLBACK;
		psp.hInstance = _Module.GetResourceInstance();
		psp.pszTemplate = MAKEINTRESOURCE(T::IDD);
		psp.pszIcon = NULL;
		psp.pszTitle = NULL;
		psp.pfnDlgProc = (DLGPROC)T::StartDialogProc;
		psp.lParam = (LPARAM)this;
		psp.pfnCallback = T::PropPageCallbackStatic;
		psp.pcRefParent = NULL;

		m_hPSP = ::CreatePropertySheetPage( &psp );
		return m_hPSP;
	}

	PC8801::Config *GetConfig( void ){ return m_pConfig; }

	BOOL GetInitialized( void ){ return m_blInitialized; }

protected:

	static UINT CALLBACK PropPageCallbackStatic( HWND hWnd, UINT uMsg,
													LPPROPSHEETPAGE ppsp)
	{
		ATLASSERT( hWnd == NULL );
		if( uMsg == PSPCB_CREATE )
		{
			CDialogImplBase* pPage = (CDialogImplBase*)ppsp->lParam;
			_Module.AddCreateWndData(&pPage->m_thunk.cd, pPage);
		}

		return 1;
	}
};


/*--------------------------------------------------------------------------
	定義ファイルクラス
--------------------------------------------------------------------------*/

class CM88DefFile
{
public:
	enum Keys {
		Flags = 0,
		CPUClock,
		Speed,
		RefreshTiming,
		BASICMode,
		Sound,
		OPNClock,
		Switches,
		SoundBuffer,
		MouseSensibility,
		CPUMode,
		KeyboardType,
		VolumeFM,
		VolumeSSG,
		VolumeADPCM,
		VolumeRhythm,
		VolumeBD,
		VolumeSD,
		VolumeTOP,
		VolumeHH,
		VolumeTOM,
		VolumeRIM,
		Flag2,
		ERAMBank,
		DrawType,
		END_KEY
	};

private:
	char *m_szFileName;
	int m_aiValues[END_KEY];

public:
	CM88DefFile( const char *szFileName );
	~CM88DefFile();

	int GetValue( CM88DefFile::Keys key )
	{
		return m_aiValues[key];
	}

	void SetValue( CM88DefFile::Keys key, int iVal )
	{
		m_aiValues[key] = iVal;
	}

	bool Load();
	bool Save();

};


/*--------------------------------------------------------------------------
	CE 用 Draw 基底クラス。
	Draw クラスを直接継承してしまう（本家 M88 とは方法が違います）。
--------------------------------------------------------------------------*/

class WCEDraw
	: public Draw, public tl4::IRunnable
{
private:
	HWND m_hWnd;
	HANDLE m_hEventDraw;
	int m_iBPL;
	bool m_blLocked;
	uint m_uiStatus;
	volatile bool m_blActive;
	bool m_blDrawing;
	bool m_blDrawAll;
	volatile bool m_blThreadLoop;
	RECT m_rectDrawArea;

	uint8 *m_pImageSrc;

	uint m_uiWidth;
	uint m_uiHeight;
	uint m_uiBPP;

	Palette m_aPal[256];

	tl4::CThread *m_pThread;

	void PaintWindow();

protected:
	bool m_blPalChanged;

	CriticalSection m_csdraw;
	uint GetWidth( void ){ return m_uiWidth; }
	uint GetHeight( void ){ return m_uiHeight; }
	uint GetBPP( void ){ return m_uiBPP; }
	Palette *GetPalette( void ){ return m_aPal; }

public:
	WCEDraw();
	virtual ~WCEDraw();

	bool Init0( HWND hWnd );

	bool Init(uint width, uint height, uint bpp);
	bool Cleanup();

	bool Lock(uint8** pimage, int* pbpl);
	bool Unlock();

	uint GetStatus();
	void Resize(uint width, uint height);
	void DrawScreen(const Region& region);
	void SetPalette(uint index, uint nents, const Palette* pal);
	//void Flip();
	bool SetFlipMode(bool);
	HWND GetHWnd( void ){ return m_hWnd; }

	DWORD Run();
	void Activate( bool blActive );
	void RequestPaint();


	virtual void DrawToScreen( uint8 *pImageSrc, RECT *pRect ) = 0;
	virtual uint8 *AllocVirtualDrawArea( uint width, uint height, uint bpp )
				= 0;
	virtual bool Init00( void ) = 0;
	virtual void CleanupSub( uint8 *pImageSrc ) = 0;

};


/*--------------------------------------------------------------------------
	CE 用キーボードインターフェースクラス
--------------------------------------------------------------------------*/

namespace PC8801
{

class WCEKeyIF
	: public Device
{
private:
	bool m_blActive;

	static const Descriptor descriptor;
	static const InFuncPtr  indef[];
	static const OutFuncPtr outdef[];

	uint m_auiKeyTbl[16];
	uint m_auiKeyTblBackup[16];
	uint m_aauiKeyPushedCntTbl[16][8];

public:
	enum
	{
		reset = 0, vsync,
		in = 0,
	};

	WCEKeyIF();
	~WCEKeyIF();

	void Activate(bool blActive) { m_blActive = blActive; }

	uint IOCALL In(uint port);
	void IOCALL VSync(uint, uint data);
	void IOCALL Reset(uint=0, uint=0);
	const Descriptor* IFCALL GetDesc() const { return &descriptor; }

	void KeyUp( int iPort, uint uiBit );
	void KeyDown( int iPort, uint uiBit );

};

}


/*--------------------------------------------------------------------------
		ディスクイメージ管理クラス
--------------------------------------------------------------------------*/


class CDiskImageManager
{
public:
	struct DiskInfo
	{
		HMENU hmenu;
		int currentdisk;
		int idchgdisk;
		bool readonly;
		char filename[MAX_PATH];
	};

private:
	DiskInfo m_aDiskInfo[DiskManager::max_drives];
	DiskManager *m_pDiskMgr;


public:
	CDiskImageManager( DiskManager *pDiskMgr );

	bool OpenDiskImage(int drive, const char* filename, bool readonly,
						int id, bool create);
	void OpenDiskImage(const char* filename);

	const char *GetDiskImageName( int iIndex )
	{
		return m_aDiskInfo[iIndex].filename;
	}

	DiskInfo *GetDiskInfo( int iDrive ){ return &m_aDiskInfo[iDrive]; }

	DiskManager *GetDiskManager( void ){ return m_pDiskMgr; }
};


/*--------------------------------------------------------------------------
	設定用ダイアログクラス
--------------------------------------------------------------------------*/

class CPropCPU
	: public CConfigPropSheetImpl< CPropCPU >
{
public:

	CPropCPU( PC8801::Config *pConfig )
	 	: CConfigPropSheetImpl< CPropCPU >( pConfig )
	{
	}

#ifdef _M88_POCKET
	enum { IDD = MAKEINTRESOURCE(IDD_CONFIG_CPU_P) };
#else
	enum { IDD = MAKEINTRESOURCE(IDD_CONFIG_CPU_H) };
#endif

	void InitDialog(HWND hdlg);
	void Update(HWND hdlg);
	void UpdateSlider(HWND hdlg);
	void SetActive(HWND hdlg);
	bool Clicked(HWND hdlg, HWND hwctl, UINT id);
	bool Command(HWND hdlg, HWND hwctl, UINT nc, UINT id);
};


class CPropDipSW1
	: public CConfigPropSheetImpl< CPropDipSW1 >
{
public:

	CPropDipSW1( PC8801::Config *pConfig )
	 	: CConfigPropSheetImpl< CPropDipSW1 >( pConfig )
	{
	}

#ifdef _M88_POCKET
	enum { IDD = MAKEINTRESOURCE(IDD_CONFIG_SWITCHES_P1) };
#else
	enum { IDD = MAKEINTRESOURCE(IDD_CONFIG_SWITCHES_H1) };
#endif

	bool Clicked(HWND hdlg, HWND hwctl, UINT id);
	void Update(HWND hdlg);

};


class CPropDipSW2
	: public CConfigPropSheetImpl< CPropDipSW2 >
{
public:

	CPropDipSW2( PC8801::Config *pConfig )
	 	: CConfigPropSheetImpl< CPropDipSW2 >( pConfig )
	{
	}

#ifdef _M88_POCKET
	enum { IDD = MAKEINTRESOURCE(IDD_CONFIG_SWITCHES_P2) };
#else
	enum { IDD = MAKEINTRESOURCE(IDD_CONFIG_SWITCHES_H2) };
#endif

	bool Clicked(HWND hdlg, HWND hwctl, UINT id);
	void Update(HWND hdlg);

};


class CPropSound1
	: public CConfigPropSheetImpl< CPropSound1 >
{
public:

	CPropSound1( PC8801::Config *pConfig )
	 	: CConfigPropSheetImpl< CPropSound1 >( pConfig )
	{
	}

#ifdef _M88_POCKET
	enum { IDD = MAKEINTRESOURCE(IDD_CONFIG_SOUND_P1) };
#else
	enum { IDD = MAKEINTRESOURCE(IDD_CONFIG_SOUND_H1) };
#endif

	void InitDialog(HWND hdlg);
	void Update(HWND hdlg);
	void SetActive(HWND hdlg);
	bool Clicked(HWND hdlg, HWND hwctl, UINT id);
	bool Command(HWND hdlg, HWND hwctl, UINT nc, UINT id);
};


class CPropSound2
	: public CConfigPropSheetImpl< CPropSound2 >
{
public:

	CPropSound2( PC8801::Config *pConfig )
	 	: CConfigPropSheetImpl< CPropSound2 >( pConfig )
	{
	}

#ifdef _M88_POCKET
	enum { IDD = MAKEINTRESOURCE(IDD_CONFIG_SOUND_P2) };
#else
	enum { IDD = MAKEINTRESOURCE(IDD_CONFIG_SOUND_H2) };
#endif

	void InitDialog(HWND hdlg);
	void Update(HWND hdlg);
	void SetActive(HWND hdlg);
	bool Clicked(HWND hdlg, HWND hwctl, UINT id);
	bool Command(HWND hdlg, HWND hwctl, UINT nc, UINT id);
};


class CConfigDialog
{
private:
	HPROPSHEETPAGE *m_phPSPs;
	int m_iPageCnt;
	PC8801::Config *m_pConfig;
	CDiskImageManager *m_pDiskImageManager;

	static int CALLBACK PropSheetCallback(HWND hWnd, UINT uMsg, LPARAM lParam);

protected:
	virtual void InitPSPHeader( PROPSHEETHEADER *pPSPHeader ) = 0;
	virtual bool InitPSP( HPROPSHEETPAGE *phPSP ) = 0;

	PC8801::Config *GetConfig( void ){ return m_pConfig; }
	CDiskImageManager *GetDiskImageManager( void )
	{
		return m_pDiskImageManager;
	}

	virtual DWORD GetDialogFlag();

public:

	CConfigDialog( int iPageCnt, PC8801::Config *pConfig,
					CDiskImageManager *pDiskImageManager );
	virtual ~CConfigDialog();
	int Show( HWND hWndParent );

};


/*--------------------------------------------------------------------------
	CE 用 Core クラス
--------------------------------------------------------------------------*/

class WCECore
	: public PC88, public tl4::IRunnable
{
private:
	CriticalSection m_cscpu;
	HWND m_hWnd;
	tl4::CThread *m_pThread;
	volatile bool m_blThreadLoop;
	volatile bool m_blActive;

	PC8801::WceSound sound;

	int m_iClock;
	int m_iExeccount;
	int m_iSpeed;
	int m_iEffclock;
	int m_iTime;


	uint32 GetMachineTime()
	{
		//return ::timeGetTime();
		return ::GetTickCount();
	}

	void ExecuteAsynchronus();
	void Execute( long lClock, long lLength, long lEffclock );

	bool ConnectDevices(PC8801::WCEKeyIF* keyb);


public:
	WCECore();
	virtual ~WCECore();

	bool Init( HWND hWnd, Draw *pDraw, DiskManager *pDiskmgr,
				PC8801::WCEKeyIF *pKeyb ); 
	bool Cleanup();

	void Reset();
	void ApplyConfig(PC8801::Config *pConfig);

	DWORD Run();
	void Activate( bool blActive );

};


/*--------------------------------------------------------------------------
	CE 用 UI クラス
	InstanceThunk 未使用のため、インスタンスは一つしか作れない。
	....ていうか、これ抽象クラスだし(笑)。

	m_pDraw, m_pKeyIF には継承クラスのコンストラクタでインスタンス
	設定をしてください。
--------------------------------------------------------------------------*/

class WCEUI
{
private:
	static LPCTSTR m_szClassName;
	static LPCTSTR m_szTitle;

	static WCEUI *m_pThis;

	HWND m_hWnd;
	HINSTANCE m_hInstance;

	CDiskImageManager *m_pDiskImageManager;
	CM88DefFile *m_pDefFile;
	PC8801::Config m_config;

	WCECore m_core;


	static LRESULT CALLBACK WndProcStatic( HWND hWnd, UINT uMsg,
											WPARAM wParam, LPARAM lParam );

	bool InitM88();
	void CleanupM88();


protected:
	WCEDraw *m_pDraw;
	PC8801::WCEKeyIF *m_pKeyIF;

public:
	WCEUI( HINSTANCE hInst );
	virtual ~WCEUI();

	int Main();

	static LPCTSTR GetWindowClassName( void ){ return m_szClassName; }
	static LPCTSTR GetWindowTitle( void ){ return m_szTitle; }

	void ApplyConfig();
	void Activate( bool blActive );

	HWND GetHWnd( void ){ return m_hWnd; }
	HINSTANCE GetInstance( void ){ return m_hInstance; }
	DiskManager *GetDiskManager( void )
	{
		return m_pDiskImageManager->GetDiskManager();
	}
	CDiskImageManager *GetDiskImageManager( void )
	{
		return m_pDiskImageManager;
	}
	PC8801::Config *GetM88Config( void ){ return &m_config; }
	WCECore *GetCore( void ){ return &m_core; }
	CM88DefFile *GetM88DefFile( void ){ return m_pDefFile; }


	bool InitWindow( int nWinMode );

	virtual LRESULT WndProc( HWND hWnd, UINT uMsg,
										WPARAM wParam, LPARAM lParam ) = 0;

	virtual bool InitM88Sub( void ) = 0;
	virtual void CleanupM88Sub( void ) = 0;
	virtual HWND CreateMainWindow( void ) = 0;

	virtual void InitWindowClass( WNDCLASS *pWcl );

};



/*--------------------------------------------------------------------------
	その他グローバル
--------------------------------------------------------------------------*/

extern int M88ceMain( WCEUI *pUI, int nwinmode );
extern const char *ReadLine(FileIO *pFile);

