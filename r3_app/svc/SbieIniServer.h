#pragma once
#include"PipeServer.h"

class SbieIniServer
{
public:
	SbieIniServer(PipeServer* pipeServer);
	static bool TokenIsAdmin(HANDLE hToken, bool OnlyFull = false);
protected:
	void LockConf(WCHAR* IniPath);
	static MSG_HEADER* Handler(void* _this, MSG_HEADER* msg);
	MSG_HEADER* Handler2(MSG_HEADER* msg);
	MSG_HEADER* GetVersion(MSG_HEADER* msg);
	MSG_HEADER* GetUser(MSG_HEADER* msg);
	bool SetUserSettingsSectionName(HANDLE hToken);
protected:

	CRITICAL_SECTION m_critsec;
	static SbieIniServer* m_instance;
	WCHAR m_username[256];
	WCHAR m_sectionname[128];
	struct SConfigIni* m_pConfigIni;
	HANDLE m_hLockFile;
	ULONG m_session_id;
	BOOLEAN m_admin;
};

