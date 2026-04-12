/*============================================================================
		TAN-Y Library 4 for Visual C++ Ver.6.0 (Win32)
		コアライブラリ

		スレッドクラス実装部
		Copyright & Programmed by Y.Taniuchi( TAN-Y )
============================================================================*/

//#include "tl4sys.h"
#include "headers.h"
#include "CThread.h"

namespace tl4
{	//	start namespace


/*----------------------------------------------------------------------------
		コンストラクタ
		引数に IRunnable 実装クラスオブジェクトへのポインタを設定すると、
		スレッドの処理はそちらで行う。
		（スレッドを操作するオブジェクトと実装部を切り離せる）
----------------------------------------------------------------------------*/

CThread::CThread( IRunnable *pRunnable )
{
	m_dwThreadId = 0;
	m_hThread = NULL;
	m_pRunnable = pRunnable;
}


/*----------------------------------------------------------------------------
		デストラクタ
----------------------------------------------------------------------------*/

CThread::~CThread()
{
	Close();
}


/*----------------------------------------------------------------------------
		スレッドを開始する
		引数	dwCreationFlags		スレッド生成時のフラグ設定
		戻り値	BOOL				FALSE はすでにスレッドが動作中か生成
									できない場合
----------------------------------------------------------------------------*/

BOOL CThread::Start( DWORD dwCreationFlags )
{
LPVOID pObj = (LPVOID)this;

	if( IsRunning() )
	{
		return FALSE;
	}

	if( m_pRunnable != NULL )
	{
		pObj = (LPVOID)m_pRunnable;
	}

	m_hThread = CreateThread( NULL, 0,
							(LPTHREAD_START_ROUTINE)ThreadHook,
							pObj, dwCreationFlags,
							&m_dwThreadId );
	if( m_hThread == FALSE )
	{
		m_hThread = NULL;
		m_dwThreadId = 0;
		return	FALSE;
	}
	return	TRUE;
}


/*----------------------------------------------------------------------------
		スレッドハンドルを閉じる
		戻り値	BOOL			TRUE で正常終了。
								FALSE はハンドルが NULL 。
----------------------------------------------------------------------------*/

BOOL CThread::Close()
{
	if( m_hThread == NULL )
	{
		return	FALSE;
	}
	CloseHandle( m_hThread );
	m_hThread = NULL;
	m_dwThreadId = 0;

	return	TRUE;
}


/*----------------------------------------------------------------------------
		スレッドを停止する
		引数	dwExitCode		スレッド終了コード
		戻り値	BOOL			TRUE で正常終了。
								FALSE はスレッドが動作していない。
----------------------------------------------------------------------------*/

BOOL CThread::Terminate( DWORD dwExitCode )
{

	if( IsRunning() )
	{
		TerminateThread( m_hThread, dwExitCode );
		Close();
		return	TRUE;
	}
	return	FALSE;
}


/*----------------------------------------------------------------------------
		スレッドの動作確認
		戻り値	BOOL			TRUE でスレッド動作中
----------------------------------------------------------------------------*/

BOOL CThread::IsRunning()
{
DWORD	dwGetExitCode;
BOOL	bResult;

	bResult = GetExitCodeThread( m_hThread, &dwGetExitCode );
	if( bResult && dwGetExitCode == STILL_ACTIVE )
	{
		return	TRUE;
	}

	Close();
	return	FALSE;
}


/*----------------------------------------------------------------------------
		IRunnable の空実装
		CThread のコンストラクタの引数を省略するか NULL を設定した場合は
		ちゃんとした実装をする必要がある。
----------------------------------------------------------------------------*/

DWORD CThread::Run()
{
	return 0;
}


}	//	end namespace
