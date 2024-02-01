#pragma once
#include"PipeServer.h"

class SbieIniServer
{
public:
	SbieIniServer(PipeServer* pipeServer);
protected:
	void LockConf(WCHAR* IniPath);
	static MSG_HEADER* Handler(void* _this, MSG_HEADER* msg);
	MSG_HEADER* Handler2(MSG_HEADER* msg);
protected:

	CRITICAL_SECTION m_critsec;
	static SbieIniServer* m_instance;
	struct SConfigIni* m_pConfigIni;
	HANDLE m_hLockFile;
	ULONG m_session_id;
};

