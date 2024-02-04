#include "SbieIniServer.h"
#include"../common/my_version.h"
#include"../common/ini.h"
#include"../Dll/sbiedll.h"
#include"../Dll/sbieapi.h"
#include"sbieiniwire.h"
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
	//处理来自任何进程的运行sbie-ctrl请求，否则调用者不能被沙盒化，我们模拟调用者
	HANDLE idProcess = (HANDLE)(ULONG_PTR)PipeServer::GetCallerProcessId();
	m_session_id = PipeServer::GetCallerSessionId();
	NTSTATUS status =
		SbieApi_QueryProcess(idProcess, NULL, NULL, NULL, NULL);
	if (msg->msgid == MSGID_SBIE_INI_RUN_SBIE_CTRL) 
	{
		//return RunSbieCtrl(msg, idProcess, NT_SUCCESS(status));
	}
	if (NT_SUCCESS(status))     // if sandboxed
		return SHORT_REPLY(STATUS_NOT_SUPPORTED);
	if (PipeServer::ImpersonateCaller(&msg) != 0)
		return msg;
	//处理获取版本请求
	if (msg->msgid == MSGID_SBIE_INI_GET_VERSION) 
	{
		return GetVersion(msg);
	}
	return msg;
}

MSG_HEADER* SbieIniServer::GetVersion(MSG_HEADER* msg)
{
	WCHAR ver_str[16];
	wsprintf(ver_str, L"%S", MY_VERSION_STRING);
	ULONG ver_len = wcslen(ver_str);
	ULONG rpl_len = sizeof(SBIE_INI_GET_USER_RPL)
		+ (ver_len + 1) * sizeof(WCHAR);
	SBIE_INI_GET_VERSION_RPL* rpl =
		(SBIE_INI_GET_VERSION_RPL*)LONG_REPLY(rpl_len);
	if (!rpl)
		return SHORT_REPLY(STATUS_INSUFFICIENT_RESOURCES);
	wcscpy(rpl->version, ver_str);
	rpl->abi_ver = MY_ABI_VERSION;

	return &rpl->h;
}
