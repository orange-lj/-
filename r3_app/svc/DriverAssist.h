#pragma once
//#include<windows.h>


class DriverAssist
{
	//������������ĳ�ʼ������
public:
	static bool Initialize();
	static bool IsDriverReady();
private:
	DriverAssist();
	bool InitializePortAndThreads();
	static ULONG StartDriverAsync(void* arg);
	static void InitClipboard();
	//����������Ϣ�����߳�
	static void ThreadStub(void* parm);
	static DWORD MsgWorkerThreadStub(void* msg);
	void MsgWorkerThread(void* msg);
	void Thread();
	//����Ϣ��¼���ļ�
	void LogMessage();
	void LogMessage_Single(ULONG code, wchar_t* data, ULONG pid);
	//���½���ע��Ͳ�����ĺ���
	bool InjectLow_Init();
private:
	static DriverAssist* m_instance;
	volatile HANDLE m_PortHandle;
	HANDLE* m_Threads;
	volatile bool m_DriverReady;
	ULONG m_last_message_number;
	//�ٽ���
	CRITICAL_SECTION m_LogMessage_CritSec;
	CRITICAL_SECTION m_critSecHostInjectedSvcs;
};

