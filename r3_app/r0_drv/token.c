#include "token.h"
#include"api.h"
#include"mem.h"
#include"../common/defines.h"
#include"api_defs.h"
//变量
ULONG Token_RestrictedSidCount_offset = 0;
ULONG Token_RestrictedSids_offset = 0;
ULONG Token_UserAndGroups_offset = 0;
ULONG Token_UserAndGroupCount_offset = 0;
static TOKEN_PRIVILEGES* Token_FilterPrivileges = NULL;
static TOKEN_GROUPS* Token_FilterGroups = NULL;
static UCHAR Token_AdministratorsSid[16] = {
	1,                                      // Revision
	2,                                      // SubAuthorityCount
	0,0,0,0,0,5, // SECURITY_NT_AUTHORITY   // IdentifierAuthority
	SECURITY_BUILTIN_DOMAIN_RID,0,0,0,      // SubAuthority 1
	(DOMAIN_ALIAS_RID_ADMINS & 0xFF),       // SubAuthority 2
		((DOMAIN_ALIAS_RID_ADMINS & 0xFF00) >> 8),0,0
};

static UCHAR Token_PowerUsersSid[16] = {
	1,                                      // Revision
	2,                                      // SubAuthorityCount
	0,0,0,0,0,5, // SECURITY_NT_AUTHORITY   // IdentifierAuthority
	SECURITY_BUILTIN_DOMAIN_RID,0,0,0,      // SubAuthority 1
	(DOMAIN_ALIAS_RID_POWER_USERS & 0xFF),  // SubAuthority 2
		((DOMAIN_ALIAS_RID_POWER_USERS & 0xFF00) >> 8),0,0
};

//函数指针
typedef NTSTATUS(*P_SepFilterToken)(
	void* TokenObject,
	ULONG_PTR Zero2,
	ULONG_PTR Zero3,
	ULONG_PTR Zero4,
	ULONG_PTR Zero5,
	ULONG_PTR Zero6,
	ULONG_PTR Zero7,
	ULONG_PTR RestrictedSidCount,
	ULONG_PTR RestrictedSidPtr,
	ULONG_PTR VariableLengthIncrease,
	void** NewTokenObject);
P_SepFilterToken Token_SepFilterToken = NULL;

static BOOLEAN Token_Init_SepFilterToken(void);

BOOLEAN Token_Init(void)
{
	const ULONG NumBasePrivs = 7;
	const ULONG NumVistaPrivs = 1;

	//$Offset$-硬偏移依赖项
	if (Driver_OsVersion <= DRIVER_WINDOWS_7) {

		//Token_RestrictedSidCount_offset = 0x7C;
		//Token_RestrictedSids_offset = 0x98;
		//Token_UserAndGroups_offset = 0x90;

	}
	else if (Driver_OsVersion >= DRIVER_WINDOWS_8
		&& Driver_OsVersion <= DRIVER_WINDOWS_10)
	{
		Token_RestrictedSidCount_offset = 0x80;
		Token_RestrictedSids_offset = 0xA0;
		//适用于windows 10-10041
		Token_UserAndGroups_offset = 0x98;  
		Token_UserAndGroupCount_offset = 0x7c;  
	}
	//创建要筛选的权限列表
		//用于TOKEN_PRIVILEGES。特权数量和空余房间
	ULONG len = (NumBasePrivs + NumVistaPrivs) * sizeof(LUID_AND_ATTRIBUTES)
		+ 32;
	Token_FilterPrivileges = Mem_AllocEx(Driver_Pool, len, TRUE);
	if (!Token_FilterPrivileges)
		return FALSE;
	memzero(Token_FilterPrivileges, len);
	if (Driver_OsVersion >= DRIVER_WINDOWS_VISTA)
		Token_FilterPrivileges->PrivilegeCount += NumVistaPrivs;
#define MySetPrivilege(i) Token_FilterPrivileges->Privileges[i].Luid.LowPart
	MySetPrivilege(0) = SE_RESTORE_PRIVILEGE;
	MySetPrivilege(1) = SE_BACKUP_PRIVILEGE;
	MySetPrivilege(2) = SE_LOAD_DRIVER_PRIVILEGE;
	MySetPrivilege(3) = SE_SHUTDOWN_PRIVILEGE;
	MySetPrivilege(4) = SE_DEBUG_PRIVILEGE;
	MySetPrivilege(5) = SE_SYSTEMTIME_PRIVILEGE;
	MySetPrivilege(6) = SE_MANAGE_VOLUME_PRIVILEGE;
	MySetPrivilege(7) = SE_TIME_ZONE_PRIVILEGE; // vista
//创建组列表以在使用“删除权限”时进行筛选
	len = 2 * sizeof(SID_AND_ATTRIBUTES)
		+ 32; // for TOKEN_GROUPS.GroupCount, and spare room

	Token_FilterGroups = Mem_AllocEx(Driver_Pool, len, TRUE);
	if (!Token_FilterGroups)
		return FALSE;
	memzero(Token_FilterGroups, len);

	Token_FilterGroups->GroupCount = 2;
#define MySetGroup(i) Token_FilterGroups->Groups[i].Sid

	MySetGroup(0) = Token_AdministratorsSid;
	MySetGroup(1) = Token_PowerUsersSid;
	//查找Token_RestrictHelper1的SepFilterToken
	if (!Token_Init_SepFilterToken())
		return FALSE;
	Api_SetFunction(API_FILTER_TOKEN, Token_Api_Filter);

	return TRUE;

}

NTSTATUS Token_Api_Filter(PROCESS* proc, ULONG64* parms)
{
	//以后实现
	return STATUS_SUCCESS;
}

NTSTATUS Token_QuerySidString(void* TokenObject, UNICODE_STRING* SidString)
{
	TOKEN_USER* TokenUserData = NULL;
	NTSTATUS status = SeQueryInformationToken(
		TokenObject, TokenUser, &TokenUserData);
	if (NT_SUCCESS(status)) {

		status = RtlConvertSidToUnicodeString(
			SidString, TokenUserData->User.Sid, TRUE);

		ExFreePool(TokenUserData);
	}

	return status;
}

BOOLEAN Token_Init_SepFilterToken(void)
{
	UNICODE_STRING uni;
	UCHAR* ptr;
	//nt！SeFilterToken是未导出nt的包装器！SepFilterToken。
	//在函数Token_RestrictHelper1中，我们需要底层的未导出函数，所以分析nt！SeFilterToken以确定其地址
	RtlInitUnicodeString(&uni, L"SeFilterToken");
	ptr = MmGetSystemRoutineAddress(&uni);
	if (ptr) 
	{
		ULONG i;
		for (i = 0; i < 256; ++i) 
		{
			//64位：查找“and dword ptr[rsp + 48h]，0”之后的“call nt！SepFilterToken”
			if (*(ULONG*)ptr == 0x48246483 && ptr[4] == 0) 
			{

				for (; i < 256; ++i) 
				{

					if (*ptr == 0xE8) 
					{

						ptr += *(LONG*)(ptr + 1) + 5;
						Token_SepFilterToken = (P_SepFilterToken)ptr;

						break;
					}

					++ptr;
				}
			}
			++ptr;
		}
	}
	if (!Token_SepFilterToken) {
		//Log_Msg1(MSG_1108, uni.Buffer);
		return FALSE;
	}
	return TRUE;
}
