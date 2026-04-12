/*===========================================================================
		M88 for PocketPC / HandheldPC
		Original Version (c)cisc
		Pocket PC Version Programmed by Y.Taniuchi ( TAN-Y )

		ディスクイメージ管理クラス
===========================================================================*/

#include "m88ce.h"


/*---------------------------------------------------------------------------
		コンストラクタ
---------------------------------------------------------------------------*/

CDiskImageManager::CDiskImageManager( DiskManager *pDiskMgr )
{
	m_pDiskMgr = pDiskMgr;
}


/*---------------------------------------------------------------------------
		ui.cpp から持ってきたもの
---------------------------------------------------------------------------*/
// ---------------------------------------------------------------------------
//	適当にディスクイメージを開く
//
void CDiskImageManager::OpenDiskImage(const char* path)
{
	// ディスクイメージをマウントする
	OpenDiskImage(0, path, 0, 0, false);
	if (m_pDiskMgr->GetNumDisks(0) > 1)
	{
		OpenDiskImage(1, path, 0, 1, false);
	}
	else
	{
		m_pDiskMgr->Unmount(1);
		OpenDiskImage(1, 0, 0, 0, false);
	}
}


// ---------------------------------------------------------------------------
//	OpenDiskImage
//	ディスクイメージを開く
//
bool CDiskImageManager::OpenDiskImage
(int drive, const char* name, bool readonly, int id, bool create)
{
	DiskInfo& dinfo = m_aDiskInfo[drive];

	bool result = false;
	if (name)
	{
		if( *name != 0 )
		{
			//	空文字列ではなければ新しいファイル名を設定
			strcpy(dinfo.filename, name);
		}
		m_pDiskMgr->Unmount(drive);
		result = m_pDiskMgr->Mount(drive, dinfo.filename,
									readonly, id, create);
		dinfo.readonly = readonly;
		dinfo.currentdisk = m_pDiskMgr->GetCurrentDisk(0);
	}

	if (!result)
		dinfo.filename[0] = 0;

//	CreateDiskMenu(drive);
	return true;
}


