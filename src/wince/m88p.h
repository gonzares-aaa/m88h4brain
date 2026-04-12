/*===========================================================================
		M88 for PocketPC
		Original Version (c)cisc
		Pocket PC Version Programmed by Y.Taniuchi ( TAN-Y )

		PocketPC 版基幹コード用ヘッダファイル

		TAN-Y 独自設計の部分に関しては必ずしも cisc さんの設計思想に準じて
		いるわけではないのであしからず....
===========================================================================*/

#ifndef _M88_POCKET
#define _M88_POCKET

#include "m88ce.h"
#include <gx.h>
#include <aygshell.h>
#include "CGAPISoftKeyb.h"

//	有効キーコードの下限
#define KEYCODE_MIN 0x01

//	有効キーコードの上限
#define KEYCODE_MAX 0x62

#define M88P_KEYDEFFILENAME	"M88p_key.def" 



/*--------------------------------------------------------------------------
	キー定義ファイルクラス
--------------------------------------------------------------------------*/

class PpcKeyDefFile
{
private:
	int m_aiKeyCode[256];
	char *m_szFileName;
	GXKeyList *m_pGXkl;

public:
	PpcKeyDefFile( const char *szFileName, GXKeyList *pGXkl );
	~PpcKeyDefFile();

	void SetFileName( const char *szFileName );

	int GetKeyCode( int iIndex ){ return m_aiKeyCode[iIndex]; }
	void SetKeyCode( int iIndex, int iKey )
	{
		m_aiKeyCode[ iIndex ] = iKey;
	}

	GXKeyList *GetGXKeyList( void ){ return m_pGXkl; }

	bool Load();
	bool Save();

	void InitKey( void );
};


/*--------------------------------------------------------------------------
	PocketPC GAPI用 Draw 実装クラス。
	Draw クラスを直接継承してしまう（本家 M88 とは方法が違います）。
--------------------------------------------------------------------------*/

class GAPIDraw
	: public WCEDraw
{
public:
	enum DrawKeybStatus {
		kbnormal, kbdraw, kbredraw
	};

private:

	CGAPISoftKeyb *m_pKeyb;
	DrawKeybStatus m_dks;
	volatile bool m_blActiveGAPI;

	GXDisplayProperties m_gxdp;

	long m_lDstXXH;				//	横向き用右方向へのインデックス加算値

	//
	//	実描画関数
	//
	void DrawWindowH555( void *pDst, const uint8 *pSrc, RECT *pRect );
	void DrawWindowH565( void *pDst, const uint8 *pSrc, RECT *pRect );

public:
	GAPIDraw();
	~GAPIDraw();

	CGAPISoftKeyb *GetKeyb( void ){ return m_pKeyb; }

	void SetDrawKeybStatus( DrawKeybStatus s ){ m_dks = s; }

	bool ActivateGAPI( bool blActive );


	void RequestPaint();	//	再オーバーライド

	void DrawToScreen( uint8 *pImageSrc, RECT *pRect );
	uint8 *AllocVirtualDrawArea( uint width, uint height, uint bpp );
	bool Init00( void );
	void CleanupSub( uint8 *pImageSrc );

};


/*--------------------------------------------------------------------------
	PocketPC 用 キーボードデバイスクラス
--------------------------------------------------------------------------*/

namespace PC8801
{

class PpcKeyIF
	: public WCEKeyIF
{
public:

	PpcKeyIF();
	~PpcKeyIF();

	void KeyUp( int iKeyCode );
	void KeyDown( int iKeyCode );
};


}


/*--------------------------------------------------------------------------
	PocketPC 用 Core クラス
--------------------------------------------------------------------------*/

class PpcUI;

class PpcCore
	: public PC88, public tl4::IRunnable
{
private:
	CriticalSection m_cscpu;
	PpcUI *m_pUI;
	HWND m_hWnd;
	tl4::CThread *m_pThread;
	volatile bool m_blThreadLoop;
	volatile bool m_blActive;

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

	bool ConnectDevices(PC8801::PpcKeyIF* keyb);


public:
	PpcCore();
	virtual ~PpcCore();

	bool Init( PpcUI *pUI, HWND hWnd, Draw *pDraw, DiskManager *pDiskmgr,
				PC8801::PpcKeyIF *pKeyb ); 
	bool Cleanup();

	void Reset();
	void ApplyConfig(PC8801::Config *pConfig);

	DWORD Run();
	void Activate( bool blActive );

};



/*--------------------------------------------------------------------------
	設定用ダイアログクラス
--------------------------------------------------------------------------*/

#define CONFIGDIALOG_PAGEMAX 9

//
//	各ページクラス
//
class CPropControl
	: public CConfigPropSheetImpl< CPropControl >
{
public:

	CPropControl( PC8801::Config *pConfig )
	 	: CConfigPropSheetImpl< CPropControl >( pConfig )
	{
	}

	enum { IDD = MAKEINTRESOURCE(IDD_CONFIG_CONTROL) };

	void SetActive(HWND hdlg);
	bool Command(HWND hdlg, HWND hwctl, UINT nc, UINT id);
};


class CPropDisk
	: public CConfigPropSheetImpl< CPropDisk >
{
private:
	CDiskImageManager *m_pDiskImageManager;
	void SetDiskImageList( HWND hWnd, uint uiDrive );

public:

	CPropDisk( PC8801::Config *pConfig, CDiskImageManager *pDiskImageManager )
	 	: CConfigPropSheetImpl< CPropDisk >( pConfig )
	{
		m_pDiskImageManager = pDiskImageManager;
	}

	enum { IDD = MAKEINTRESOURCE(IDD_CONFIG_DISK) };

	void SetActive(HWND hdlg);
	bool Command(HWND hdlg, HWND hwctl, UINT nc, UINT id);
};

class CPropKeyMap;

class CPropKeyMapKeySel
	: public CWindowImpl< CPropKeyMapKeySel >
{
private:
	BEGIN_MSG_MAP( CPropKeyMapKeySel )
		MESSAGE_HANDLER( WM_KEYUP, OnKeyUp )
		MESSAGE_HANDLER( WM_KEYDOWN, OnKeyDown )
	END_MSG_MAP()

	LRESULT OnKeyUp( UINT uMsg, WPARAM wParam,
						LPARAM lParam, BOOL &blHandled);
	LRESULT OnKeyDown( UINT uMsg, WPARAM wParam,
						LPARAM lParam, BOOL &blHandled);

	CPropKeyMap *m_pPropKeyMap;


public:
	CPropKeyMapKeySel( CPropKeyMap *pPropKeyMap )
		: CWindowImpl< CPropKeyMapKeySel >()
	{
		m_pPropKeyMap = pPropKeyMap;
	}
};


class CPropKeyMap
	: public CConfigPropSheetImpl< CPropKeyMap >
{
friend CPropKeyMapKeySel;

private:
	PpcKeyDefFile *m_pKeyDef;

	void UpdateConfigKeyList( void );

	CPropKeyMapKeySel *m_pKeySelSubClass;

public:

	CPropKeyMap( PC8801::Config *pConfig, PpcKeyDefFile *pKeyDef )
	 	: CConfigPropSheetImpl< CPropKeyMap >( pConfig )
	{
		m_pKeySelSubClass = new CPropKeyMapKeySel( this );
		m_pKeyDef = pKeyDef;
	}

	~CPropKeyMap()
	{
		RELEASE_OBJ( m_pKeySelSubClass );
	}

	enum { IDD = MAKEINTRESOURCE(IDD_CONFIG_KEYMAP_P) };

	void SetActive(HWND hdlg);
	void InitDialog(HWND hdlg);
	bool Command(HWND hdlg, HWND hwctl, UINT nc, UINT id);


};


class CPropAbout
	: public CConfigPropSheetImpl< CPropAbout >
{
public:

	CPropAbout( PC8801::Config *pConfig )
	 	: CConfigPropSheetImpl< CPropAbout >( pConfig )
	{
	}

	enum { IDD = MAKEINTRESOURCE(IDD_ABOUT) };

	void InitDialog(HWND hdlg);
};


//
//	ダイアログ本体
//
class PpcConfigDialog
	: public CConfigDialog
{
private:
	CPropControl *m_pPropControl;
	CPropDisk *m_pPropDisk;
	CPropCPU *m_pPropCPU;
	CPropSound1 *m_pPropSound1;
	CPropSound2 *m_pPropSound2;
	CPropDipSW1 *m_pPropDipSW1;
	CPropDipSW2 *m_pPropDipSW2;
	CPropKeyMap *m_pPropKeyMap;
	CPropAbout *m_pPropAbout;

	PpcKeyDefFile *m_pKeyDef;

protected:
	void InitPSPHeader( PROPSHEETHEADER *pPSPHeader );
	bool InitPSP( HPROPSHEETPAGE *phPSP );
	DWORD GetDialogFlag();

public:
	PpcConfigDialog( PC8801::Config *pConfig,
						CDiskImageManager *pDiskImageManager,
						PpcKeyDefFile *pKeyDefFile );
	~PpcConfigDialog();

};




/*--------------------------------------------------------------------------
	UI クラス。WinUI に相当。
	InstanceThunk 未使用のため、インスタンスは一つしか作れない。
--------------------------------------------------------------------------*/


class PpcUI
	: public WCEUI
{
private:

//	CPpcConfig *m_pPpcConfig;
	GAPIDraw m_draw;
	PC8801::PpcKeyIF m_keyif;
	GXKeyList m_gxkl;
	PpcKeyDefFile *m_pKeyDef;

public:
	PpcUI( HINSTANCE hInst );
	~PpcUI();

	virtual bool InitM88Sub( void );
	virtual void CleanupM88Sub( void );
	virtual HWND CreateMainWindow( void );

	virtual LRESULT WndProc( HWND hWnd, UINT uMsg,
										WPARAM wParam, LPARAM lParam );

};


#endif
