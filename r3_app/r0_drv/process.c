#include "process.h"
#include"mem.h"
#include"api.h"
#include"api_defs.h"
#include"syscall.h"
#include"../common/map.h"
#include"token.h"
#include"api_defs.h"
#include"util.h"

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
	Api_SetFunction(API_QUERY_PROCESS, Process_Api_Query);
	//Api_SetFunction(API_QUERY_PROCESS_INFO, Process_Api_QueryInfo);
	//Api_SetFunction(API_QUERY_BOX_PATH, Process_Api_QueryBoxPath);
	//Api_SetFunction(API_QUERY_PROCESS_PATH, Process_Api_QueryProcessPath);
	//Api_SetFunction(API_QUERY_PATH_LIST, Process_Api_QueryPathList);
	Api_SetFunction(API_ENUM_PROCESSES, Process_Api_Enum);

	return TRUE;
}

NTSTATUS Process_Api_Query(PROCESS* proc, ULONG64* parms)
{
	API_QUERY_PROCESS_ARGS* args = (API_QUERY_PROCESS_ARGS*)parms;
	NTSTATUS status;
	HANDLE ProcessId;
	ULONG* num32;
	ULONG64* num64;
	KIRQL irql;

	//����SbieDll��һ�ε���SbieApi
	if (proc && !proc->sbiedll_loaded) 
	{
	
	}
	//���ָ����ProcessId����λ������ƥ��Ľ��̡�������÷�����ɳ�У������ָ��ProcessId
	ProcessId = args->process_id.val;
	if (proc) {
		if (ProcessId == proc->pid || IS_ARG_CURRENT_PROCESS(ProcessId))
			ProcessId = 0;  // don't have to search for the current pid
	}
	else {
		if ((!ProcessId) || IS_ARG_CURRENT_PROCESS(ProcessId))
			return STATUS_INVALID_CID;
	}
	if (ProcessId) 
	{
		proc = Process_Find(ProcessId, &irql);
		if (!proc) {
			ExReleaseResourceLite(Process_ListLock);
			KeLowerIrql(irql);
			return STATUS_INVALID_CID;
		}
	}

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

NTSTATUS Process_Enumerate(const WCHAR* boxname, BOOLEAN all_sessions, ULONG session_id, ULONG* pids, ULONG* count)
{
	NTSTATUS status;
	PROCESS* proc1;
	ULONG num;
	KIRQL irql;

	if (count == NULL)
		return STATUS_INVALID_PARAMETER;

	//�����ص��÷��û������¼�Ự�еĽ���
	if ((!all_sessions) && (session_id == -1)) 
	{
		status = MyGetSessionId(&session_id);
		if (!NT_SUCCESS(status))
			return status;
	}
	KeRaiseIrql(APC_LEVEL, &irql);
	ExAcquireResourceSharedLite(Process_ListLock, TRUE);
	__try 
	{
		num = 0;
		//ȫ�ּ��������Ŀ��ٿ�ݷ�ʽ
		if (pids == NULL && (!boxname[0]) && all_sessions) 
		{ // no pids, all boxes, all sessions
			num = Process_Map.nnodes;
			goto done;
		}
		map_iter_t iter = map_iter();
		while (map_next(&Process_Map, &iter)) 
		{
			proc1 = iter.value;
			BOX* box1 = proc1->box;
			if (box1 && !proc1->bHostInject) 
			{
				BOOLEAN same_box =
					(!boxname[0]) || (_wcsicmp(box1->name, boxname) == 0);
				BOOLEAN same_session =
					(all_sessions || box1->session_id == session_id);
				if (same_box && same_session) {
					if (pids) {
						if (num >= *count)
							break;
						pids[num] = (ULONG)(ULONG_PTR)proc1->pid;
					}
					++num;
				}
			}
		}
done:
		*count = num;
		status = STATUS_SUCCESS;
	}
	__except (EXCEPTION_EXECUTE_HANDLER) 
	{
		status = GetExceptionCode();
	}
	ExReleaseResourceLite(Process_ListLock);
	KeLowerIrql(irql);

	return status;
}

NTSTATUS Process_Api_Enum(PROCESS* proc, ULONG64* parms)
{
	NTSTATUS status;
	ULONG count;
	ULONG* user_pids;                   // user mode ULONG [512]
	WCHAR* user_boxname;                // user mode WCHAR [BOXNAME_COUNT]
	BOOLEAN all_sessions;
	ULONG session_id;
	WCHAR boxname[BOXNAME_COUNT];
	ULONG* user_count;
	//�ӵڶ���������ȡboxname
	memzero(boxname, sizeof(boxname));
	if (proc)
		wcscpy(boxname, proc->box->name);
	user_boxname = (WCHAR*)parms[2];
	if ((!boxname[0]) && user_boxname) 
	{
		ProbeForRead(user_boxname, sizeof(WCHAR) * (BOXNAME_COUNT - 2), sizeof(UCHAR));
		if (user_boxname[0])
			wcsncpy(boxname, user_boxname, (BOXNAME_COUNT - 2));
	}
	//�ӵ�����������ȡ�������û�/����ǰ�û�����־
	all_sessions = FALSE;
	if (parms[3])
		all_sessions = TRUE;

	session_id = (ULONG)parms[4];
	
	//�ӵ�һ��������ȡ�û�pid������
	user_count = (ULONG*)parms[5];
	user_pids = (ULONG*)parms[1];
	if (user_count) 
	{
	
	}
	else //��������
	{
		if (!user_pids)
			return STATUS_INVALID_PARAMETER;
		count = API_MAX_PIDS - 1;
		user_count = user_pids;
		user_pids += 1;
	}
	ProbeForWrite(user_count, sizeof(ULONG), sizeof(ULONG));
	if (user_pids) {
		ProbeForWrite(user_pids, sizeof(ULONG) * count, sizeof(ULONG));
	}
	status = Process_Enumerate(boxname, all_sessions, session_id,
		user_pids, &count);
	if (!NT_SUCCESS(status))
		return status;

	*user_count = count;

	return status;

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
