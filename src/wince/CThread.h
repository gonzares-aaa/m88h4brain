/*============================================================================
		TAN-Y Library 4 for Visual C++ Ver.6.0 (Win32)
		システムコアライブラリ　よりスレッドクラスだけ抜粋

		ヘッダファイル
		Copyright & Programmed by Y.Taniuchi( TAN-Y )
============================================================================*/

#ifndef _INC_TANYLIB4_SYS
#define _INC_TANYLIB4_SYS


namespace tl4
{	//	start namespace

/*---------------------------------------------------------------------------
		スレッドクラス
---------------------------------------------------------------------------*/

class IRunnable
{
public:
	virtual	DWORD Run() = 0;
};


class CThread :
	public IRunnable
{
private:
	IRunnable *m_pRunnable;

	static DWORD WINAPI ThreadHook( IRunnable *lpth )
	{
		return lpth->Run();
	}

protected:
	HANDLE m_hThread;
	DWORD m_dwThreadId;

	DWORD Run();

	void Exit( DWORD dwExitCode )
	{
		::ExitThread( dwExitCode );
	}

public:

	CThread( IRunnable *pRunnable = NULL );

	virtual ~CThread();

	operator HANDLE()
	{
		return GetHandle();
	}

	operator DWORD()
	{
		return GetId();
	}

	HANDLE GetHandle( void )
	{
		return	m_hThread;
	}

	DWORD GetId( void )
	{
		return	m_dwThreadId;
	}

	BOOL Suspend()
	{
		return ::SuspendThread( m_hThread );
	}

	BOOL Resume()
	{
		return ::ResumeThread( m_hThread );
	}

	BOOL SetPriority( int nPriority )
	{
		return ::SetThreadPriority( m_hThread, nPriority );
	}

	BOOL GetExitCode( DWORD *dwExitCode )
	{
		return GetExitCodeThread( m_hThread, dwExitCode );
	}

	BOOL IsRunning();

	BOOL Terminate( DWORD dwExitCode );

	BOOL Start( DWORD dwCreationFlags = 0 );
	BOOL Close();
};

}	//	end namespace

#endif


