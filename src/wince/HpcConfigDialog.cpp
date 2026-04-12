/*===========================================================================
		M88 for HandheldPC
		Original Version (c)cisc
		Pocket PC Version Programmed by Y.Taniuchi ( TAN-Y )

		プロパティページ関係クラス実装
===========================================================================*/

#include "m88h.h"

#define BSTATE(b) (b ? BST_CHECKED : BST_UNCHECKED)

LPCWSTR aszKeyNames[];

using namespace PC8801;



/*---------------------------------------------------------------------------
		コンストラクタ
---------------------------------------------------------------------------*/

HpcConfigDialog::HpcConfigDialog( PC8801::Config *pConfig,
									CDiskImageManager *pDiskImageManager,
									CM88DefFile *pDefFile )
	: CConfigDialog( CONFIGDIALOG_PAGEMAX, pConfig, pDiskImageManager )
{
	m_pDefFile = pDefFile;
}


/*---------------------------------------------------------------------------
		デストラクタ
---------------------------------------------------------------------------*/

HpcConfigDialog::~HpcConfigDialog()
{
	RELEASE_OBJ( m_pPropFunction );
	RELEASE_OBJ( m_pPropDipSW2 );
	RELEASE_OBJ( m_pPropDipSW1 );
	RELEASE_OBJ( m_pPropSound2 );
	RELEASE_OBJ( m_pPropSound1 );
	RELEASE_OBJ( m_pPropCPU );
}


/*---------------------------------------------------------------------------
		PSP生成
---------------------------------------------------------------------------*/

bool HpcConfigDialog::InitPSP( HPROPSHEETPAGE *phPSP )
{
	m_pPropCPU = new CPropCPU( GetConfig() );
	phPSP[0] = m_pPropCPU->Create();

	m_pPropSound1 = new CPropSound1( GetConfig() );
	phPSP[1] = m_pPropSound1->Create();

	m_pPropSound2 = new CPropSound2( GetConfig() );
	phPSP[2] = m_pPropSound2->Create();

	m_pPropDipSW1 = new CPropDipSW1( GetConfig() );
	phPSP[3] = m_pPropDipSW1->Create();

	m_pPropDipSW2 = new CPropDipSW2( GetConfig() );
	phPSP[4] = m_pPropDipSW2->Create();

	m_pPropFunction = new CPropFunction( GetConfig(), m_pDefFile );
	phPSP[5] = m_pPropFunction->Create();

	return true;
}


/*---------------------------------------------------------------------------
		PROPSHEETHEADER の初期化
---------------------------------------------------------------------------*/

void HpcConfigDialog::InitPSPHeader( PROPSHEETHEADER *pPSPHeader )
{
}


/*===========================================================================
		CPropFunction
===========================================================================*/

void CPropFunction::SetActive(HWND hdlg)
{
	if( GetConfig()->flags & PC8801::Config::usearrowfor10 )
	{
		::SendDlgItemMessage( hdlg, IDC_FUNCTION_USEARROWFOR10,
								BM_SETCHECK, BST_CHECKED, 0 );
	} else {
		::SendDlgItemMessage( hdlg, IDC_FUNCTION_USEARROWFOR10,
								BM_SETCHECK, BST_UNCHECKED, 0 );
	}

	static const int item[4] = 
	{ IDC_DRAWTYPE_HVGA, IDC_DRAWTYPE_VGA, IDC_DRAWTYPE_VGA2, IDC_DRAWTYPE_HVGA };
	CheckDlgButton(item[m_pDefFile->GetValue(CM88DefFile::DrawType) & 3], BSTATE(true));


	::SetWindowLong( hdlg, DWL_MSGRESULT, 0 );
}


bool CPropFunction::Command(HWND hdlg, HWND hwctl, UINT nc, UINT id)
{

	switch( id )
	{
	case IDC_FUNCTION_USEARROWFOR10:
		{
			GetConfig()->flags ^= PC8801::Config::usearrowfor10;
		}
		break;

	case IDC_DRAWTYPE_HVGA:
		{
			m_pDefFile->SetValue(CM88DefFile::DrawType, 0);
		}
		break;

	case IDC_DRAWTYPE_VGA:
		{
			m_pDefFile->SetValue(CM88DefFile::DrawType, 1);
		}
		break;

	case IDC_DRAWTYPE_VGA2:
		{
			m_pDefFile->SetValue(CM88DefFile::DrawType, 2);
		}
		break;
	}

	return false;
}


