#include "process.h"
#include"mem.h"
#include"api.h"
#include"api_defs.h"
#include"syscall.h"
#include"../common/map.h"
#include"token.h"

HASH_MAP Process_Map;
HASH_MAP Process_MapDfp;
PERESOURCE Process_ListLock = NULL;
static KEVENT* Process_Low_Event = NULL;
static BOOLEAN Process_NotifyImageInstalled = FALSE;
static BOOLEAN Process_NotifyProcessInstalled = FALSE;
volatile BOOLEAN Process_ReadyToSandbox = FALSE;

static NTSTATUS Process_Low_Api_InjectComplete(
	PROCESS* proc, ULONG64* parms);
static void Process_NotifyProcessEx(
	PEPROCESS ParentId, HANDLE ProcessId, PPS_CREATE_NOTIFY_INFO CreateInfo);
static void Process_NotifyImage(
	const UNICODE_STRING* FullImageName,
	HANDLE ProcessId, IMAGE_INFO* ImageInfo);
static NTSTATUS Process_CreateUserProcess(
	PROCESS* proc, SYSCALL_ENTRY* syscall_entry, ULONG_PTR* user_args);

#ifdef ALLOC_PRAGMA
#pragma alloc_text (INIT, Process_Init)
#ifdef XP_SUPPORT
#ifndef _WIN64
#pragma alloc_text (INIT, Process_HookProcessNotify)
#endif _WIN64
#endif
#endif // ALLOC_PRAGMA

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

NTSTATUS Process_GetSidStringAndSessionId(HANDLE ProcessHandle, HANDLE ProcessId, UNICODE_STRING* SidString, ULONG* SessionId)
{
	NTSTATUS status;
	PEPROCESS ProcessObject = NULL;
	PACCESS_TOKEN TokenObject;

	if (ProcessHandle == NtCurrentProcess()) {

		ProcessObject = PsGetCurrentProcess();
		ObReferenceObject(ProcessObject);
		status = STATUS_SUCCESS;

	}
	else if (ProcessHandle) {

		const KPROCESSOR_MODE AccessMode =
			((ProcessHandle == NtCurrentProcess()) ? KernelMode : UserMode);

		status = ObReferenceObjectByHandle(ProcessHandle, 0, *PsProcessType,
			AccessMode, &ProcessObject, NULL);

	}
	else if (ProcessId) {

		status = PsLookupProcessByProcessId(ProcessId, &ProcessObject);

	}
	else {

		status = STATUS_INVALID_PARAMETER;
	}

	if (NT_SUCCESS(status)) {

		*SessionId = PsGetProcessSessionId(ProcessObject);

		TokenObject = PsReferencePrimaryToken(ProcessObject);
		status = Token_QuerySidString(TokenObject, SidString);
		PsDereferencePrimaryToken(TokenObject);

		ObDereferenceObject(ProcessObject);
	}

	if (!NT_SUCCESS(status)) {

		SidString->Buffer = NULL;
		*SessionId = -1;
	}

	return status;
}

void Process_GetProcessName(POOL* pool, ULONG_PTR idProcess, void** out_buf, ULONG* out_len, WCHAR** out_ptr)
{
	NTSTATUS status;
	OBJECT_ATTRIBUTES objattrs;
	CLIENT_ID cid;
	HANDLE handle;
	ULONG len;

	*out_buf = NULL;
	*out_len = 0;
	*out_ptr = NULL;

	if (!idProcess)
		return;

	InitializeObjectAttributes(&objattrs,
		NULL, OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE, NULL, NULL);
	cid.UniqueProcess = (HANDLE)idProcess;
	cid.UniqueThread = 0;

	status = ZwOpenProcess(
		&handle, PROCESS_QUERY_INFORMATION, &objattrs, &cid);

	if (!NT_SUCCESS(status))
		return;

	status = ZwQueryInformationProcess(
		handle, ProcessImageFileName, NULL, 0, &len);

	if (status == STATUS_INFO_LENGTH_MISMATCH) {

		ULONG uni_len = len + 8 + 8;
		UNICODE_STRING* uni = Mem_Alloc(pool, uni_len);
		if (uni) {

			uni->Buffer = NULL;

			status = ZwQueryInformationProcess(
				handle, ProcessImageFileName, uni, len + 8, &len);

			if (NT_SUCCESS(status) && uni->Buffer) {

				WCHAR* ptr;
				uni->Buffer[uni->Length / sizeof(WCHAR)] = L'\0';
				if (!uni->Buffer[0]) {
					uni->Buffer[0] = L'?';
					uni->Buffer[1] = L'\0';
				}
				ptr = wcsrchr(uni->Buffer, L'\\');
				if (ptr) {
					++ptr;
					if (!*ptr)
						ptr = uni->Buffer;
				}
				else
					ptr = uni->Buffer;
				*out_buf = uni;
				*out_len = uni_len;
				*out_ptr = ptr;

			}
			else
				Mem_Free(uni, uni_len);
		}
	}

	ZwClose(handle);
}

PROCESS* Process_Find(HANDLE ProcessId, KIRQL* out_irql)
{
	PROCESS* proc;
	KIRQL irql;
	BOOLEAN check_terminated;

	//如果我们正在查找当前进程，那么请检查执行模式，以确保它不是系统进程或内核模式调用者
	if (!ProcessId) 
	{
	
	}
	else 
	{
		check_terminated = FALSE;
	}
	//查找与当前ProcessId匹配的PROCESS块
	KeRaiseIrql(APC_LEVEL, &irql);
	ExAcquireResourceSharedLite(Process_ListLock, TRUE);
	proc = map_get(&Process_Map, ProcessId);
	if (proc) 
	{
	
	}
	if (out_irql) 
	{
		*out_irql = irql;
	}
	else 
	{
	
	}
	return proc;
}

NTSTATUS Process_Low_Api_InjectComplete(PROCESS* proc, ULONG64* parms)
{
	//以后实现
	return STATUS_SUCCESS;
}

void Process_NotifyProcessEx(PEPROCESS ParentId, HANDLE ProcessId, PPS_CREATE_NOTIFY_INFO CreateInfo)
{
	//在init的主驱动程序说可以之前不要做任何事情
	if (!Process_ReadyToSandbox) 
	{
		return;
	}
	//处理进程的创建和删除。请注意，我们在任意线程上下文中运行

}

void Process_NotifyImage(const UNICODE_STRING* FullImageName, HANDLE ProcessId, IMAGE_INFO* ImageInfo)
{
	static const WCHAR* _Ntdll32 = L"\\syswow64\\ntdll.dll";    // 19 chars
	PROCESS* proc;
	ULONG fail = 0;

	//对于为任何目的映射的任何图像，都会调用Notify例程。我们不关心这里的系统映像
	if ((!ProcessId) || ImageInfo->SystemModeImage)
		return;
	//如果进程是由process_NotifyProcess_Create分配的，但尚未完全初始化，则立即初始化它
	proc = Process_Find(ProcessId, NULL);

	if ((!proc) || proc->initialized) 
	{
		if (proc && (!proc->ntdll32_base)) 
		{
		
		}
		return;
	}
}

NTSTATUS Process_CreateUserProcess(PROCESS* proc, SYSCALL_ENTRY* syscall_entry, ULONG_PTR* user_args)
{
	//以后实现
	return STATUS_SUCCESS;
}
