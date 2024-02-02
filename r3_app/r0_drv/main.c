#pragma once



#include"mem.h"
#include"main.h"
#include"obj.h"
#include"conf.h"
#include"dll.h"
#include"syscall.h"
#include"session.h"
#include"hook.h"
#include"token.h"
#include"process.h"
#include"thread.h"
#include"file.h"
#include"key.h"
#include"ipc.h"
#include"gui.h"
#include"api.h"
#include"log.h"
#include"../common/my_version.h"
#include"../common/ntproto.h"
const ULONG tzuk = 'xobs';
const WCHAR* Driver_S_1_5_18 = L"S-1-5-18"; //	System

DRIVER_OBJECT* Driver_Object;

UNICODE_STRING Driver_Altitude;
const WCHAR* Altitude_Str = FILTER_ALTITUDE;

ULONG Driver_OsVersion = 0;
ULONG Driver_OsBuild = 0;

POOL* Driver_Pool = NULL;

PACL Driver_PublicAcl = NULL;
PSECURITY_DESCRIPTOR Driver_PublicSd = NULL;
PSECURITY_DESCRIPTOR Driver_LowLabelSd = NULL;

WCHAR* Driver_RegistryPath;
WCHAR* Driver_HomePathDos = NULL;
WCHAR* Driver_HomePathNt = NULL;
ULONG  Driver_HomePathNt_Len = 0;

BOOLEAN Driver_FullUnload = TRUE;
WCHAR* Driver_Version = TEXT(MY_VERSION_STRING);

P_NtCreateToken                 ZwCreateToken = NULL;
P_NtCreateTokenEx               ZwCreateTokenEx = NULL;

NTSTATUS
DriverEntry(
	PDRIVER_OBJECT DriverObject,
	PUNICODE_STRING RegistryPath
);
static BOOLEAN Driver_CheckOsVersion(void);
static BOOLEAN Driver_InitPublicSecurity(void);
static BOOLEAN Driver_FindHomePath(UNICODE_STRING* RegistryPath);
static BOOLEAN Driver_FindMissingServices(void);

#ifdef ALLOC_PRAGMA
#pragma alloc_text (INIT, DriverEntry)
#pragma alloc_text (INIT, Driver_CheckOsVersion)
#pragma alloc_text (INIT, Driver_FindHomePath)
#pragma alloc_text (INIT, Driver_FindMissingServices)
#endif // ALLOC_PRAGMA


VOID
DriverUnload(
	PDRIVER_OBJECT pDriverObj
);


NTSTATUS
DriverEntry(
	PDRIVER_OBJECT DriverObject,
	PUNICODE_STRING RegistryPath
)
{
	DbgBreakPoint();
	BOOLEAN ok = TRUE;
	//DrvRtPoolNxOptIn 可能是其中的一个标志，表示开启了对于 Non-Executive (NX) 启用的池的支持。
	//NX 是一种硬件技术，用于防止缓冲区溢出攻击
	ExInitializeDriverRuntime(DrvRtPoolNxOptIn);

	//初始化全局驱动变量
	Driver_Object = DriverObject;
	Driver_Object->DriverUnload = NULL;

	RtlInitUnicodeString(&Driver_Altitude, Altitude_Str);

	if (ok)
	{
		ok = Driver_CheckOsVersion();
	}

	if (ok)
	{
		Driver_Pool = Pool_Create();
		if (!Driver_Pool)
		{
			//打印失败日志
		}
	}

	if (ok)
	{
		ok = Driver_InitPublicSecurity();
	}
	if (ok)
	{
		Driver_RegistryPath =
			Mem_AllocStringEx(Driver_Pool, RegistryPath->Buffer, TRUE);
		if (!Driver_RegistryPath)
			ok = FALSE;
	}
	if (ok)
	{
		ok = Driver_FindHomePath(RegistryPath);
	}
	//MyValidateCertificate();

	//初始化简单的实用程序模块。这些不能钩住任何东西
	if (ok)
	{
		ok = Obj_Init();
	}
	if (ok)
	{
		ok = Conf_Init();
	}
	if (ok)
	{
		ok = Dll_Init();
	}
	if (ok)
	{
		ok = Syscall_Init();
	}
	if (ok) 
	{
		ok = Session_Init();
	}
	if (ok) 
	{
		ok = Driver_FindMissingServices();
	}
	if (ok) 
	{
		ok = Token_Init();
	}
	//初始化模块。这些地方与系统相连。钩子
	//在安装完毕后立即激活。Process模块必须首先初始化
	//，因为它初始化进程列表
	if (ok) 
	{
		ok = Process_Init();
	}
	if (ok) 
	{
		ok = Thread_Init();
	}
	if (ok) 
	{
		ok = File_Init();
	}
	if (ok) 
	{
		ok = Key_Init();
	}
	if (ok) 
	{
		ok = Ipc_Init();
	}
	if (ok) 
	{
		ok = Gui_Init();
	}
	//创建用于服务用户模式请求的驱动程序设备
	if (ok) 
	{
		ok = Api_Init();
	}
	//初始化Windows筛选平台调用
	if (ok) 
	{
		//ok = WFP_Init();
	}	
	//完成驱动程序初始化
		//初始化期间释放的dll加载
	Dll_Unload();
	if (!ok) 
	{
		//Log_Msg1(MSG_DRIVER_ENTRY_FAILED, Driver_Version);
		//SbieDrv_DriverUnload(Driver_Object);
		return STATUS_UNSUCCESSFUL;
	}
	Driver_FullUnload = FALSE;
	Log_Msg1(MSG_DRIVER_ENTRY_OK, Driver_Version);
	return STATUS_SUCCESS;
}


unsigned int g_TrapFrameOffset = 0;
BOOLEAN Driver_CheckOsVersion(void)
{
	ULONG MajorVersion, MinorVersion;
	WCHAR str[64];

	//确保我们在Windows XP（v5.1）或更高版本（32位）
	//或Windows 7（v6.1）或更高版本（64位）上运行
	const ULONG MajorVersionMin = 6;
	const ULONG MinorVersionMin = 1;
	PsGetVersion(&MajorVersion, &MinorVersion, &Driver_OsBuild, NULL);
	if (MajorVersion > MajorVersionMin || (MajorVersion == MajorVersionMin && MinorVersion >= MinorVersionMin)) 
	{
		if (MajorVersion == 10) 
		{
			Driver_OsVersion = DRIVER_WINDOWS_10;
			g_TrapFrameOffset = 0x90;
		}
		else if (MajorVersion == 6)
		{
		
		}
		else 
		{
		
		}
		if (Driver_OsVersion) 
		{
			return TRUE;
		}
	}
}

BOOLEAN Driver_InitPublicSecurity(void)
{
#define MyAddAccessAllowedAce(pAcl,pSid)                                \
    RtlAddAccessAllowedAceEx(pAcl, ACL_REVISION,                        \
        CONTAINER_INHERIT_ACE | OBJECT_INHERIT_ACE | INHERITED_ACE,     \
        GENERIC_ALL, pSid);
	//使用允许的DACL创建安全描述符
	//通过身份验证的用户和Everyone SID访问
	static UCHAR AuthSid[12] =
	{
		//修订
		1,
		//子机构计数
		1,
		//身份验证机构
		0,0,0,0,0,5,
		//子机构
		SECURITY_AUTHENTICATED_USER_RID
	};
	static UCHAR WorldSid[12] = {
		1,                                      // Revision
		1,                                      // SubAuthorityCount
		0,0,0,0,0,1, // SECURITY_WORLD_SID_AUTHORITY // IdentifierAuthority
		SECURITY_WORLD_RID                      // SubAuthority
	};
	static UCHAR RestrSid[12] = {
		1,                                      // Revision
		1,                                      // SubAuthorityCount
		0,0,0,0,0,5, // SECURITY_NT_AUTHORITY   // IdentifierAuthority
		SECURITY_RESTRICTED_CODE_RID            // SubAuthority
	};
	Driver_PublicAcl = Mem_AllocEx(Driver_Pool, 128, TRUE);
	if (!Driver_PublicAcl) 
	{
		return FALSE;
	}
	RtlCreateAcl(Driver_PublicAcl, 128, ACL_REVISION);
	MyAddAccessAllowedAce(Driver_PublicAcl, &AuthSid);
	MyAddAccessAllowedAce(Driver_PublicAcl, &WorldSid);
	Driver_PublicSd = Mem_AllocEx(Driver_Pool, 64, TRUE);
	if (!Driver_PublicSd)
		return FALSE;
	RtlCreateSecurityDescriptor(
		Driver_PublicSd, SECURITY_DESCRIPTOR_REVISION);
	RtlSetDaclSecurityDescriptor(
		Driver_PublicSd, TRUE, Driver_PublicAcl, FALSE);
	//在Windows Vista上，创建一个安全描述符，该描述符
	//允许低完整性进程访问
	if (Driver_OsVersion >= DRIVER_WINDOWS_VISTA) 
	{
		typedef struct _ACE_HEADER {
			UCHAR  AceType;
			UCHAR  AceFlags;
			USHORT AceSize;
		} ACE_HEADER;
		typedef struct _SYSTEM_MANDATORY_LABEL_ACE {
			ACE_HEADER  Header;
			ACCESS_MASK Mask;
			ULONG       SidStart;
		} SYSTEM_MANDATORY_LABEL_ACE, * PSYSTEM_MANDATORY_LABEL_ACE;
		PACL LowLabelAcl1, LowLabelAcl2;
		UCHAR ace_space[32];
		SYSTEM_MANDATORY_LABEL_ACE* pAce =
			(SYSTEM_MANDATORY_LABEL_ACE*)ace_space;
		ULONG* pSid = &pAce->SidStart;
		pAce->Header.AceType = SYSTEM_MANDATORY_LABEL_ACE_TYPE;
		pAce->Header.AceFlags = 0;
		pAce->Header.AceSize = 5 * sizeof(ULONG);
		pAce->Mask = SYSTEM_MANDATORY_LABEL_NO_WRITE_UP;
		pSid[0] = 0x00000101;
		pSid[1] = 0x10000000;
		pSid[2] = SECURITY_MANDATORY_LOW_RID;

		LowLabelAcl1 = Mem_AllocEx(Driver_Pool, 128, TRUE);
		if (!LowLabelAcl1)
			return FALSE;
		RtlCreateAcl(LowLabelAcl1, 128, ACL_REVISION);
		RtlAddAce(LowLabelAcl1, ACL_REVISION, 0, pAce, pAce->Header.AceSize);

		LowLabelAcl2 = Mem_AllocEx(Driver_Pool, 128, TRUE);
		if (!LowLabelAcl2)
			return FALSE;
		RtlCreateAcl(LowLabelAcl2, 128, ACL_REVISION);
		MyAddAccessAllowedAce(LowLabelAcl2, &AuthSid);
		MyAddAccessAllowedAce(LowLabelAcl2, &WorldSid);
		MyAddAccessAllowedAce(LowLabelAcl2, &RestrSid);

		Driver_LowLabelSd = Mem_AllocEx(Driver_Pool, 128, TRUE);
		if (!Driver_LowLabelSd)
			return FALSE;
		RtlCreateSecurityDescriptor(
			Driver_LowLabelSd, SECURITY_DESCRIPTOR_REVISION);
		RtlSetDaclSecurityDescriptor(
			Driver_LowLabelSd, TRUE, LowLabelAcl2, FALSE);
		RtlSetSaclSecurityDescriptor(
			Driver_LowLabelSd, TRUE, LowLabelAcl1, FALSE);
	}
	return TRUE;
}

BOOLEAN Driver_FindHomePath(UNICODE_STRING* RegistryPath)
{
	NTSTATUS status;
	OBJECT_ATTRIBUTES objattrs;
	UNICODE_STRING uni;
	HANDLE handle;
	union 
	{
		KEY_VALUE_PARTIAL_INFORMATION part;
		WCHAR info_space[256];
	}info;
	WCHAR path[384];
	WCHAR* ptr;
	ULONG len;
	IO_STATUS_BLOCK MyIoStatusBlock;
	FILE_OBJECT* file_object;
	OBJECT_NAME_INFORMATION* Name = NULL;
	ULONG NameLength = 0;

	//找到SbieDrv.sys的路径
	InitializeObjectAttributes(&objattrs,
		RegistryPath, OBJ_CASE_INSENSITIVE, NULL, NULL);
	status = ZwOpenKey(&handle, KEY_READ, &objattrs);
	if (!NT_SUCCESS(status)) {
		//Log_Status_Ex(MSG_DRIVER_FIND_HOME_PATH, 0x11, status, SBIEDRV);
		return FALSE;
	}
	InitializeObjectAttributes(&objattrs, &uni, OBJ_CASE_INSENSITIVE, NULL, NULL);
	RtlInitUnicodeString(&uni, L"ImagePath");
	len = sizeof(info);
	status = ZwQueryValueKey(handle, &uni, KeyValuePartialInformation, &info, len, &len);
	ZwClose(handle);
	if (!NT_SUCCESS(status)) {
		//Log_Status_Ex(MSG_DRIVER_FIND_HOME_PATH, 0x22, status, uni.Buffer);
		return FALSE;
	}
	if ((info.part.Type != REG_SZ && info.part.Type != REG_EXPAND_SZ)
		|| info.part.DataLength < 4) {
		//Log_Status_Ex(MSG_DRIVER_FIND_HOME_PATH, 0x33, status, uni.Buffer);
		return FALSE;
	}
	//路径应该是\？\<home>\SbieDrv.sys，其中<home>是-
	//Sandboxie安装目录。我们需要删除Sandbox.sys，-//和前缀\？
	ptr = (WCHAR*)info.part.Data;
	ptr[info.part.DataLength / sizeof(WCHAR) - 1] = L'\0';
	if (*ptr != L'\\') {
		wcscpy(path, L"\\??\\");
		wcscat(path, ptr);
	}
	else
		wcscpy(path, ptr);

	ptr = wcsrchr(path, L'\\');
	if (ptr)
		*ptr = L'\0';
	Driver_HomePathDos = Mem_AllocStringEx(Driver_Pool, path, TRUE);
	if (!Driver_HomePathDos) {
		status = STATUS_INSUFFICIENT_RESOURCES;
		//Log_Status(MSG_DRIVER_FIND_HOME_PATH, 0x44, status);
		return FALSE;
	}
	//尝试打开路径，这样我们就可以得到它的FILE_OBJECT
	RtlInitUnicodeString(&uni, path);

	InitializeObjectAttributes(&objattrs,
		&uni, OBJ_CASE_INSENSITIVE, NULL, NULL);
	status = ZwCreateFile(
		&handle,
		FILE_GENERIC_READ,      // DesiredAccess
		&objattrs,
		&MyIoStatusBlock,
		NULL,                   // AllocationSize
		0,                      // FileAttributes
		FILE_SHARE_READ,        // ShareAccess
		FILE_OPEN,              // CreateDisposition
		FILE_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT,
		NULL, 0);
	if (!NT_SUCCESS(status)) {
		//Log_Status_Ex(MSG_DRIVER_FIND_HOME_PATH, 0x55, status, uni.Buffer);
		return FALSE;
	}
	//从文件对象获取规范路径名
	status = ObReferenceObjectByHandle(handle, 0, NULL, KernelMode, &file_object, NULL);
	if (!NT_SUCCESS(status)) 
	{
		ZwClose(handle);
		//Log_Status_Ex(MSG_DRIVER_FIND_HOME_PATH, 0x66, status, uni.Buffer);
		return FALSE;
	}
	status = Obj_GetName(Driver_Pool, file_object, &Name, &NameLength);
	ObDereferenceObject(file_object);
	ZwClose(handle);
	if (!NT_SUCCESS(status)) {
		//Log_Status_Ex(MSG_DRIVER_FIND_HOME_PATH, 0x77, status, uni.Buffer);
		return FALSE;
	}

	Driver_HomePathNt = Name->Name.Buffer;
	Driver_HomePathNt_Len = wcslen(Driver_HomePathNt);

	return TRUE;
}

BOOLEAN Driver_FindMissingServices(void)
{
	ZwCreateToken = (P_NtCreateToken)Driver_FindMissingService("ZwCreateToken", 13);
	if (Driver_OsVersion >= DRIVER_WINDOWS_8) {
		ZwCreateTokenEx = (P_NtCreateTokenEx)Driver_FindMissingService("ZwCreateTokenEx", 17);
		//DbgPrint("ZwCreateTokenEx: %p\r\n", ZwCreateTokenEx);
	}
}



VOID
DriverUnload(
	PDRIVER_OBJECT pDriverObj
)
{
	
}

void* Driver_FindMissingService(const char* ProcName, int prmcnt)
{
	void* ptr = Dll_GetProc(Dll_NTDLL, ProcName, FALSE);
	if (!ptr) 
	{
		return NULL;
	}
	void* svc = NULL;
	if (!Hook_GetService(ptr, NULL, prmcnt, NULL, &svc))
		return NULL;
	return svc;
}
