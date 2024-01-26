#include "SbieIniServer.h"
#include"../common/ini.h"
#include"../Dll/sbiedll.h"
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
	return nullptr;
}
