#include "SbieIniServer.h"
#include"../common/ini.h"
#include"../Dll/sbiedll.h"
#include"../Dll/sbieapi.h"
SbieIniServer* SbieIniServer::m_instance = NULL;

SbieIniServer::SbieIniServer(PipeServer* pipeServer)
{
	InitializeCriticalSection(&m_critsec);
	m_instance = this;

	m_hLockFile = INVALID_HANDLE_VALUE;
	LockConf(NULL);

	pipeServer->Register(MSGID_SBIE_INI, this, Handler);
}

void SbieIniServer::LockConf(WCHAR* IniPath)
{
	WCHAR buf[80];
	LONG rc;
	if (m_hLockFile != INVALID_HANDLE_VALUE)
		return;
	rc = SbieApi_QueryConfAsIs(NULL, L"EditPassword", 0, buf, sizeof(buf));
	if (rc != 0) {
		m_hLockFile = INVALID_HANDLE_VALUE;
		return;
	}
}

MSG_HEADER* SbieIniServer::Handler(void* _this, MSG_HEADER* msg)
{
	SbieIniServer* pThis = (SbieIniServer*)_this;
	EnterCriticalSection(&pThis->m_critsec);
	MSG_HEADER* rpl = pThis->Handler2(msg);
	LeaveCriticalSection(&pThis->m_critsec);
	return rpl;
}

MSG_HEADER* SbieIniServer::Handler2(MSG_HEADER* msg)
{
	//���������κν��̵�����sbie-ctrl���󣬷�������߲��ܱ�ɳ�л�������ģ�������
	/*HANDLE idProcess = (HANDLE)(ULONG_PTR)PipeServer::GetCallerProcessId();
	m_session_id = PipeServer::GetCallerSessionId();
	NTSTATUS status =
		SbieApi_QueryProcess(idProcess, NULL, NULL, NULL, NULL);*/
	return msg;
}
