#include "process.h"
#include"mem.h"
#include"api.h"
#include"api_defs.h"
#include"syscall.h"
#include"../common/map.h"

HASH_MAP Process_Map;
HASH_MAP Process_MapDfp;
PERESOURCE Process_ListLock = NULL;
static KEVENT* Process_Low_Event = NULL;
static BOOLEAN Process_NotifyImageInstalled = FALSE;
static BOOLEAN Process_NotifyProcessInstalled = FALSE;

static NTSTATUS Process_Low_Api_InjectComplete(
	PROCESS* proc, ULONG64* parms);
static void Process_NotifyProcessEx(
	PEPROCESS ParentId, HANDLE ProcessId, PPS_CREATE_NOTIFY_INFO CreateInfo);
static void Process_NotifyImage(
	const UNICODE_STRING* FullImageName,
	HANDLE ProcessId, IMAGE_INFO* ImageInfo);
static NTSTATUS Process_CreateUserProcess(
	PROCESS* proc, SYSCALL_ENTRY* syscall_entry, ULONG_PTR* user_args);

BOOLEAN Process_Init(void)
{
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	map_init(&Process_Map, Driver_Pool);
	//准备一些水桶以提高性能
	map_resize(&Process_Map, 128);

	map_init(&Process_MapDfp, Driver_Pool);
	map_resize(&Process_MapDfp, 128);

	if (!Mem_GetLockResource(&Process_ListLock, TRUE))
		return FALSE;
	if (!Process_Low_Init())
		return FALSE;
	//安装进程通知程序
	if (Driver_OsVersion >= DRIVER_WINDOWS_7)
	{
		status = PsSetCreateProcessNotifyRoutineEx(Process_NotifyProcessEx, FALSE);
	}
	if (NT_SUCCESS(status)) {

		Process_NotifyProcessInstalled = TRUE;
	}
	else {

		//系统中已经安装了太多通知例程
		//Log_Status(MSG_PROCESS_NOTIFY, 0x11, status);
		return FALSE;
	}
	//安装映像通知例程
	status = PsSetLoadImageNotifyRoutine(Process_NotifyImage);
	if (NT_SUCCESS(status))
		Process_NotifyImageInstalled = TRUE;
	else {
		//Log_Status(MSG_PROCESS_NOTIFY, 0x22, status);
		return FALSE;
	}
	//设置适用于Vista及更高版本的系统调用处理程序
	//注：不使用NtCreateProcess/NtCreateProcessEx接缝
	if (!Syscall_Set1("CreateUserProcess", Process_CreateUserProcess))
		return FALSE;

	//Api_SetFunction(API_START_PROCESS, Process_Api_Start);
	//Api_SetFunction(API_QUERY_PROCESS, Process_Api_Query);
	//Api_SetFunction(API_QUERY_PROCESS_INFO, Process_Api_QueryInfo);
	//Api_SetFunction(API_QUERY_BOX_PATH, Process_Api_QueryBoxPath);
	//Api_SetFunction(API_QUERY_PROCESS_PATH, Process_Api_QueryProcessPath);
	//Api_SetFunction(API_QUERY_PATH_LIST, Process_Api_QueryPathList);
	//Api_SetFunction(API_ENUM_PROCESSES, Process_Api_Enum);

	return TRUE;
}

BOOLEAN Process_Low_Init(void)
{
	Process_Low_Event = ExAllocatePoolWithTag(NonPagedPool, sizeof(KEVENT), tzuk);
	if (!Process_Low_Event) {
		//Log_Msg0(MSG_1104);
		return FALSE;
	}
	KeInitializeEvent(Process_Low_Event, SynchronizationEvent, FALSE);

	Api_SetFunction(API_INJECT_COMPLETE, Process_Low_Api_InjectComplete);

	return TRUE;
}

NTSTATUS Process_Low_Api_InjectComplete(PROCESS* proc, ULONG64* parms)
{
	//以后实现
	return STATUS_SUCCESS;
}

void Process_NotifyProcessEx(PEPROCESS ParentId, HANDLE ProcessId, PPS_CREATE_NOTIFY_INFO CreateInfo)
{
	//以后实现
}

void Process_NotifyImage(const UNICODE_STRING* FullImageName, HANDLE ProcessId, IMAGE_INFO* ImageInfo)
{
	//以后实现
}

NTSTATUS Process_CreateUserProcess(PROCESS* proc, SYSCALL_ENTRY* syscall_entry, ULONG_PTR* user_args)
{
	//以后实现
	return STATUS_SUCCESS;
}
