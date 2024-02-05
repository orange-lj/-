#include "SbieIniServer.h"
#include"../common/my_version.h"
#include"../common/ini.h"
#include"../Dll/sbiedll.h"
#include"../Dll/sbieapi.h"
#include"sbieiniwire.h"
#include"crc.h"
SbieIniServer* SbieIniServer::m_instance = NULL;

SbieIniServer::SbieIniServer(PipeServer* pipeServer)
{
	InitializeCriticalSection(&m_critsec);
	m_instance = this;

	m_hLockFile = INVALID_HANDLE_VALUE;
	LockConf(NULL);

	pipeServer->Register(MSGID_SBIE_INI, this, Handler);
}

bool SbieIniServer::TokenIsAdmin(HANDLE hToken, bool OnlyFull)
{
	//检查令牌是否为Administrators组成员
	SID_IDENTIFIER_AUTHORITY NtAuthority = SECURITY_NT_AUTHORITY;
	PSID AdministratorsGroup;
	BOOL b = AllocateAndInitializeSid(
		&NtAuthority,
		2,
		SECURITY_BUILTIN_DOMAIN_RID,
		DOMAIN_ALIAS_RID_ADMINS,
		0, 0, 0, 0, 0, 0,
		&AdministratorsGroup);
	if (b) 
	{
		if (!CheckTokenMembership(NULL, AdministratorsGroup, &b))
			b = FALSE;
		FreeSid(AdministratorsGroup);

		//在Windows Vista上，检查UAC拆分令牌
		if (!b || OnlyFull) 
		{
		
		}
	}
	return b ? true : false;
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
	//句柄获取用户请求
	if (msg->msgid == MSGID_SBIE_INI_GET_USER) 
	{
		return GetUser(msg);
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

MSG_HEADER* SbieIniServer::GetUser(MSG_HEADER* msg)
{
	HANDLE hToken;
	BOOL ok1 = OpenThreadToken(GetCurrentThread(), TOKEN_QUERY, FALSE, &hToken);
	if (!ok1) 
	{
		return SHORT_REPLY(STATUS_NO_TOKEN);
	}
	bool ok2 = SetUserSettingsSectionName(hToken);
	BOOLEAN admin = FALSE;
	if (ok2 && TokenIsAdmin(hToken)) 
	{
		admin = TRUE;
	}
	CloseHandle(hToken);
	if (!ok2) 
	{
		return SHORT_REPLY(STATUS_NO_TOKEN);
	}
	ULONG name_len = wcslen(m_username);
	ULONG rpl_len = sizeof(SBIE_INI_GET_USER_RPL)
		+ (name_len + 1) * sizeof(WCHAR);
	SBIE_INI_GET_USER_RPL* rpl =
		(SBIE_INI_GET_USER_RPL*)LONG_REPLY(rpl_len);
	if (!rpl)
		return SHORT_REPLY(STATUS_INSUFFICIENT_RESOURCES);
	rpl->admin = admin;
	wcscpy(rpl->section, m_sectionname);
	wcscpy(rpl->name, m_username);
	rpl->name_len = name_len;

	return &rpl->h;
}

bool SbieIniServer::SetUserSettingsSectionName(HANDLE hToken)
{
	union {
		TOKEN_USER user;
		UCHAR space[128];
		WCHAR value[4];
	} info;
	m_admin = FALSE;
	//如果存在UserSettings_Portable部分，则使用
	const WCHAR* _portable = L"UserSettings_Portable";

	NTSTATUS status = SbieApi_QueryConfAsIs(
		_portable, NULL, 0, info.value, sizeof(info.value));
	if (status == STATUS_SUCCESS || status == STATUS_BUFFER_TOO_SMALL) {

		wcscpy(m_sectionname, _portable);
		wcscpy(m_username, _portable + 13);  // L"Portable"

		return true;
	}
	//获取调用方的用户名
	ULONG info_len = sizeof(info);
	BOOL ok = GetTokenInformation(
		hToken, TokenUser, &info, info_len, &info_len);
	if (!ok)
		return false;
	ULONG username_len = sizeof(m_username) / sizeof(WCHAR) - 4;
	WCHAR domain[256];
	ULONG domain_len = sizeof(domain) / sizeof(WCHAR) - 4;
	SID_NAME_USE use;

	m_username[0] = L'\0';

	ok = LookupAccountSid(NULL, info.user.User.Sid,
		m_username, &username_len, domain, &domain_len, &use);
	if (!ok || !m_username[0])
		return false;
	m_username[sizeof(m_username) / sizeof(WCHAR) - 4] = L'\0';
	_wcslwr(m_username);

	//计算crc
	ULONG crc = CRC_Adler32(
		(UCHAR*)m_username, wcslen(m_username) * sizeof(WCHAR));
	wsprintf(m_sectionname, L"UserSettings_%08X", crc);

	//将用户名中的空格和反斜杠转换为下划线
	while (1) {
		WCHAR* ptr = wcschr(m_username, L'\\');
		if (!ptr)
			ptr = wcschr(m_username, L' ');
		if (!ptr)
			break;
		*ptr = L'_';
	}

	return true;
}
