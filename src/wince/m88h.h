/*===========================================================================
		M88 for HandheldPC
		Original Version (c)cisc
		Pocket PC Version Programmed by Y.Taniuchi ( TAN-Y )

		HandheldPC 版基幹コード用ヘッダファイル

		TAN-Y 独自設計の部分に関しては必ずしも cisc さんの設計思想に準じて
		いるわけではないのであしからず....
===========================================================================*/

#ifndef _M88_HANDHELD
#define _M88_HANDHELD

#include "m88ce.h"

#define DRAWINDEX_X 64


/*--------------------------------------------------------------------------
	HandheldPC 用 DIBSection 版 Draw 実装クラス。
	Draw クラスを直接継承してしまう（本家 M88 とは方法が違います）。
--------------------------------------------------------------------------*/

class HpcDraw
	: public WCEDraw
{
protected:
	struct BI256		// BITMAPINFO
	{
		BITMAPINFOHEADER header;
		RGBQUAD colors[256];
	};

	HBITMAP m_hBitmap;
	BI256	m_binfo;
	int m_iDrawX;

	uint8 *m_pDst;

public:
	HpcDraw();
	~HpcDraw();

	virtual void DrawToScreen( uint8 *pImageSrc, RECT *pRect );
	uint8 *AllocVirtualDrawArea( uint width, uint height, uint bpp );
	virtual bool Init00( void );
	void CleanupSub( uint8 *pImageSrc );

};


class HpcVGADraw
	: public HpcDraw
{
public:
	virtual void DrawToScreen( uint8 *pImageSrc, RECT *pRect );
	virtual bool Init00( void );
};


class HpcVGAFullDraw
	: public HpcVGADraw
{
public:
	virtual void DrawToScreen( uint8 *pImageSrc, RECT *pRect );
};


/*--------------------------------------------------------------------------
	HandheldPC 用 キーボードデバイスクラス
	WinKeyIF からソース流用。
--------------------------------------------------------------------------*/

namespace PC8801
{


#define HPCKEYIF_LOCK_CAPS		(1)
#define HPCKEYIF_LOCK_KANA		(2)

class HpcKeyIF
	: public WCEKeyIF
{
public:
	struct KEYPUSHINFO
	{
		uint uiPort;
		uint uiSwitchBit;
		uint uiSwitchBitPattern;
	};

private:
	DWORD m_dwLockKeyFlag;

	bool m_bUseRomaji; // メニューから設定するローマ字ON/OFFフラグ
	bool m_bModifierMode; // 文字切替キーが押されているかどうかの状態フラグ

	static bool m_blUseArrowFor10;
	static KEYPUSHINFO m_aaKeyPushInfo[][4];

	KEYPUSHINFO (*m_mapA)[4]; // 通常(QWERTY)
	KEYPUSHINFO (*m_mapB)[4]; // 数字・記号 (メインキー)
	KEYPUSHINFO (*m_mapC)[4]; // 数字・記号 (テンキー化)

	// 状態に応じてマップを流し込む関数
	void UpdateKeyMap();

public:

	HpcKeyIF();
	~HpcKeyIF();

	// ローマ字モードの取得とトグル(反転)
	bool IsRomajiMode() { return m_bUseRomaji; }
	void ToggleRomajiMode() { m_bUseRomaji = !m_bUseRomaji; }

	// カナロック状態を取得
	bool IsKanaLocked() { return (m_dwLockKeyFlag & HPCKEYIF_LOCK_KANA) != 0; }

	static void SetUseArrowFor10Flag( bool blUseArrowFor10 );

	void KeyUp( uint uiVkCode, uint32 uiKeyData );
	void KeyDown( uint uiVkCode, uint32 uiKeyData );
	
	// OSのタイマーから呼ばれる関数
	void OnTimer();
};


}



/*--------------------------------------------------------------------------
	ダイアログクラス
--------------------------------------------------------------------------*/

class CDialog
{
private:
	HWND m_hDlg;
	HINSTANCE m_hInst;
	LPCTSTR m_szTemplate;

	static BOOL CALLBACK MainDlgProc( HWND hWnd, UINT uMsg,
										WPARAM wParam, LPARAM lParam );

public:
	CDialog( HINSTANCE hInstSrc, LPCTSTR szTemplateSrc );
	virtual ~CDialog();

	virtual BOOL DlgProc( UINT uMsg, WPARAM wParam, LPARAM lParam );

	static int Create( HWND hWndOwner, CDialog *pDlg );

	BOOL End( int iRet )
	{
		return ::EndDialog( m_hDlg, iRet );
	}

	LRESULT SendMessage( UINT uMsg, WPARAM wParam = 0, LPARAM lParam = 0 )
	{
		return ::SendMessage( m_hDlg, uMsg, wParam, lParam );
	}

	BOOL PostMessage( UINT uMsg, WPARAM wParam = 0, LPARAM lParam = 0 )
	{
		return ::PostMessage( m_hDlg, uMsg, wParam, lParam );
	}

	LONG SendDlgItemMessage( int iId, UINT uMsg,
								WPARAM wParam = 0, LPARAM lParam = 0 )
	{
		return ::SendDlgItemMessage( m_hDlg, iId, uMsg, wParam, lParam );
	}

	BOOL SetDlgItemInt( int iId, UINT uiValue, BOOL blSigned = TRUE )
	{
		return ::SetDlgItemInt( m_hDlg, iId, uiValue, blSigned );
	}

	BOOL SetDlgItemText( int iId, LPCTSTR szText )
	{
		return ::SetDlgItemText( m_hDlg, iId, szText );
	}

	HWND GetDlgItem( int iId )
	{
		return ::GetDlgItem( m_hDlg, iId );
	}

	UINT GetDlgItemInt( int iId, BOOL *pblTranslated, BOOL blSigned = TRUE )
	{
		return ::GetDlgItemInt( m_hDlg, iId, pblTranslated, blSigned );
	}

//	int GetDlgItemText( int iId, CTString &result );
};



/*--------------------------------------------------------------------------
	バージョン表示ダイアログクラス
--------------------------------------------------------------------------*/

class CAboutDialog
	: public CDialog
{
private:
	CDiskImageManager *m_pDiskImgMgr;

public:
	CAboutDialog( HINSTANCE hInst );
	BOOL DlgProc( UINT uMsg, WPARAM wParam, LPARAM lParam );
};



/*--------------------------------------------------------------------------
	ディスク入れ替えダイアログクラス
--------------------------------------------------------------------------*/

class CDiskChangeDialog
	: public CDialog
{
private:
	CDiskImageManager *m_pDiskImgMgr;
	int m_iDrv;

public:
	CDiskChangeDialog( HINSTANCE hInst, CDiskImageManager *pDiskImgMgr,
						int iDrv );
	BOOL DlgProc( UINT uMsg, WPARAM wParam, LPARAM lParam );

};


/*--------------------------------------------------------------------------
	設定用ダイアログクラス
--------------------------------------------------------------------------*/

#define CONFIGDIALOG_PAGEMAX 6

//
//	各ページクラス
//
class CPropFunction
	: public CConfigPropSheetImpl< CPropFunction >
{
private:
	CM88DefFile *m_pDefFile;

public:

	CPropFunction( PC8801::Config *pConfig, CM88DefFile *pDefFile )
	 	: CConfigPropSheetImpl< CPropFunction >( pConfig )
	{
		m_pDefFile = pDefFile;
	}

	enum { IDD = MAKEINTRESOURCE(IDD_CONFIG_FUNCTION_H) };

	void SetActive(HWND hdlg);
	bool Command(HWND hdlg, HWND hwctl, UINT nc, UINT id);
};


//
//	ダイアログ本体
//
class HpcConfigDialog
	: public CConfigDialog
{
private:
	CPropCPU *m_pPropCPU;
	CPropSound1 *m_pPropSound1;
	CPropSound2 *m_pPropSound2;
	CPropDipSW1 *m_pPropDipSW1;
	CPropDipSW2 *m_pPropDipSW2;
	CPropFunction *m_pPropFunction;

	CM88DefFile *m_pDefFile;

protected:
	void InitPSPHeader( PROPSHEETHEADER *pPSPHeader );
	bool InitPSP( HPROPSHEETPAGE *phPSP );

public:
	HpcConfigDialog( PC8801::Config *pConfig,
						CDiskImageManager *pDiskImageManager,
						CM88DefFile *pDefFile );
	~HpcConfigDialog();

};



/*--------------------------------------------------------------------------
	UI クラス。WinUI に相当。
	InstanceThunk 未使用のため、インスタンスは一つしか作れない。
--------------------------------------------------------------------------*/

class HpcUI
	: public WCEUI
{
private:
	HpcDraw m_drawHVGA;
	HpcVGADraw m_drawVGA;
	HpcVGAFullDraw m_drawFullVGA;

	PC8801::HpcKeyIF m_keyif;
	HWND m_hWndCB;
	HMENU m_hMenu;
	int m_iPrevBasicModeMenu;

	// Mute Sound
	DWORD mute_before_vol;	// Mute前のボリューム
	bool isMute;			// Muteしていればtrue

	CDiskImageManager *m_pDiskImgMgr;

	void OnCreate( HWND hWnd );
	void OnCommand( HWND hWnd, WPARAM wParam, LPARAM lParam );

	void CheckBasicModeRadioItem( void );
	bool CreateDiskMenu( uint drive );


public:
	HpcUI( HINSTANCE hInst );
	~HpcUI();

	virtual bool InitM88Sub( void );
	virtual void CleanupM88Sub( void );
	virtual HWND CreateMainWindow( void );
	virtual void InitWindowClass( WNDCLASS *pWcl );

	virtual LRESULT WndProc( HWND hWnd, UINT uMsg,
										WPARAM wParam, LPARAM lParam );

};

#endif
