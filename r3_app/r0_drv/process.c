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
	//׼��һЩˮͰ���������
	map_resize(&Process_Map, 128);

	map_init(&Process_MapDfp, Driver_Pool);
	map_resize(&Process_MapDfp, 128);

	if (!Mem_GetLockResource(&Process_ListLock, TRUE))
		return FALSE;
	if (!Process_Low_Init())
		return FALSE;
	//��װ����֪ͨ����
	if (Driver_OsVersion >= DRIVER_WINDOWS_7)
	{
		status = PsSetCreateProcessNotifyRoutineEx(Process_NotifyProcessEx, FALSE);
	}
	if (NT_SUCCESS(status)) {

		Process_NotifyProcessInstalled = TRUE;
	}
	else {

		//ϵͳ���Ѿ���װ��̫��֪ͨ����
		//Log_Status(MSG_PROCESS_NOTIFY, 0x11, status);
		return FALSE;
	}
	//��װӳ��֪ͨ����
	status = PsSetLoadImageNotifyRoutine(Process_NotifyImage);
	if (NT_SUCCESS(status))
		Process_NotifyImageInstalled = TRUE;
	else {
		//Log_Status(MSG_PROCESS_NOTIFY, 0x22, status);
		return FALSE;
	}
	//����������Vista�����߰汾��ϵͳ���ô������
	//ע����ʹ��NtCreateProcess/NtCreateProcessEx�ӷ�
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

	//����������ڲ��ҵ�ǰ���̣���ô����ִ��ģʽ����ȷ��������ϵͳ���̻��ں�ģʽ������
	if (!ProcessId) 
	{
	
	}
	else 
	{
		check_terminated = FALSE;
	}
	//�����뵱ǰProcessIdƥ���PROCESS��
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
	//�Ժ�ʵ��
	return STATUS_SUCCESS;
}

void Process_NotifyProcessEx(PEPROCESS ParentId, HANDLE ProcessId, PPS_CREATE_NOTIFY_INFO CreateInfo)
{
	//��init������������˵����֮ǰ��Ҫ���κ�����
	if (!Process_ReadyToSandbox) 
	{
		return;
	}
	//������̵Ĵ�����ɾ������ע�⣬�����������߳�������������

}

void Process_NotifyImage(const UNICODE_STRING* FullImageName, HANDLE ProcessId, IMAGE_INFO* ImageInfo)
{
	static const WCHAR* _Ntdll32 = L"\\syswow64\\ntdll.dll";    // 19 chars
	PROCESS* proc;
	ULONG fail = 0;

	//����Ϊ�κ�Ŀ��ӳ����κ�ͼ�񣬶������Notify���̡����ǲ����������ϵͳӳ��
	if ((!ProcessId) || ImageInfo->SystemModeImage)
		return;
	//�����������process_NotifyProcess_Create����ģ�����δ��ȫ��ʼ������������ʼ����
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
	//�Ժ�ʵ��
	return STATUS_SUCCESS;
}
