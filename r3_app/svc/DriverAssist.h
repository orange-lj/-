#pragma once
//#include<windows.h>


class DriverAssist
{
	//������������ĳ�ʼ������
public:
	static bool Initialize();
private:
	DriverAssist();
	bool InitializePortAndThreads();
	static ULONG StartDriverAsync(void* arg);
	//����������Ϣ�����߳�
	static void ThreadStub(void* parm);
	void Thread();

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

