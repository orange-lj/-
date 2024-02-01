#pragma once
//#include<windows.h>


class DriverAssist
{
	//主和驱动程序的初始化功能
public:
	static bool Initialize();
	static bool IsDriverReady();
private:
	DriverAssist();
	bool InitializePortAndThreads();
	static ULONG StartDriverAsync(void* arg);
	static void InitClipboard();
	//主侦听和消息工作线程
	static void ThreadStub(void* parm);
	static DWORD MsgWorkerThreadStub(void* msg);
	void MsgWorkerThread(void* msg);
	void Thread();
	//将消息记录到文件
	void LogMessage();
	void LogMessage_Single(ULONG code, wchar_t* data, ULONG pid);
	//向新进程注入低层代码层的函数
	bool InjectLow_Init();
private:
	static DriverAssist* m_instance;
	volatile HANDLE m_PortHandle;
	HANDLE* m_Threads;
	volatile bool m_DriverReady;
	ULONG m_last_message_number;
	//临界区
	CRITICAL_SECTION m_LogMessage_CritSec;
	CRITICAL_SECTION m_critSecHostInjectedSvcs;
};

