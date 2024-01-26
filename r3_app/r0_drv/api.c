#include"api.h"
#include"api_defs.h"
#include"mem.h"
#include"log.h"
#include"util.h"
#include"alpc.h"
#include"../common/defines.h"
static const WCHAR* Api_ParamPath = L"\\REGISTRY\\MACHINE\\SECURITY\\SBIE";
static P_Api_Function* Api_Functions = NULL;
static LOG_BUFFER* Api_LogBuffer = NULL;
static PERESOURCE Api_LockResource = NULL;
static BOOLEAN Api_WorkListInitialized = FALSE;
static FAST_IO_DISPATCH* Api_FastIoDispatch = NULL;
DEVICE_OBJECT* Api_DeviceObject = NULL;
static volatile LONG Api_UseCount = -1;
static void* Api_ServicePortObject = NULL;

volatile HANDLE Api_ServiceProcessId = NULL;

static NTSTATUS Api_Irp_CREATE(DEVICE_OBJECT* device_object, IRP* irp);
static NTSTATUS Api_SetServicePort(PROCESS* proc, ULONG64* parms);

static NTSTATUS Api_Irp_CLEANUP(DEVICE_OBJECT* device_object, IRP* irp);
static NTSTATUS Api_GetVersion(PROCESS* proc, ULONG64* parms);
static KIRQL Api_EnterCriticalSection(void);
static void Api_LeaveCriticalSection(KIRQL oldirql);
NTSTATUS Api_GetSecureParam(PROCESS* proc, ULONG64* parms);
NTSTATUS KphVerifyBuffer(PUCHAR Buffer, ULONG BufferSize, PUCHAR Signature, ULONG SignatureSize);

void Api_SetFunction(ULONG func_code, P_Api_Function func_ptr)
{
	if (!Api_Functions) 
	{
		ULONG len = (API_LAST - API_FIRST - 1) * sizeof(P_Api_Function);
		Api_Functions = Mem_AllocEx(Driver_Pool, len, TRUE);

		if (Api_Functions)
			memzero(Api_Functions, len);
		else
			Api_Functions = (void*)-1;
	}
	if ((Api_Functions != (void*)-1) &&
		(func_code > API_FIRST) && (func_code < API_LAST)) 
	{
		Api_Functions[func_code - API_FIRST - 1] = func_ptr;
	}
}

BOOLEAN Api_Init(void)
{
	NTSTATUS status;
	UNICODE_STRING uni;
	//初始化日志缓冲区
	Api_LogBuffer = log_buffer_init(8 * 8 * 1024);
	//初始化工作列表
	if (!Mem_GetLockResource(&Api_LockResource, TRUE))
		return FALSE;

	Api_WorkListInitialized = TRUE;
	//初始化快速IO调度指针
	Api_FastIoDispatch = ExAllocatePoolWithTag(NonPagedPool, sizeof(FAST_IO_DISPATCH), tzuk);
	if (!Api_FastIoDispatch) 
	{
		//Log_Status(MSG_API_DEVICE, 0, STATUS_INSUFFICIENT_RESOURCES);
		return FALSE;
	}
	memzero(Api_FastIoDispatch, sizeof(FAST_IO_DISPATCH));
	Api_FastIoDispatch->SizeOfFastIoDispatch = sizeof(FAST_IO_DISPATCH);
	Api_FastIoDispatch->FastIoDeviceControl = Api_FastIo_DEVICE_CONTROL;
	Driver_Object->FastIoDispatch = Api_FastIoDispatch;
	//初始化IRP调度指针
	Driver_Object->MajorFunction[IRP_MJ_CREATE] = Api_Irp_CREATE;
	Driver_Object->MajorFunction[IRP_MJ_CLEANUP] = Api_Irp_CLEANUP;
	//创建设备对象
	RtlInitUnicodeString(&uni, API_DEVICE_NAME);
	status = IoCreateDevice(
		Driver_Object, 0, &uni,
		FILE_DEVICE_UNKNOWN, FILE_DEVICE_SECURE_OPEN, FALSE,
		&Api_DeviceObject);
	if (!NT_SUCCESS(status)) {
		Api_DeviceObject = NULL;
		//Log_Status(MSG_API_DEVICE, 0, status);
		return FALSE;
	}

	Api_DeviceObject->Flags &= ~DO_DEVICE_INITIALIZING;
	// set API functions
	Api_SetFunction(API_GET_VERSION, Api_GetVersion);
	////Api_SetFunction(API_GET_WORK,           Api_GetWork);
	//Api_SetFunction(API_LOG_MESSAGE, Api_LogMessage);
	//Api_SetFunction(API_GET_MESSAGE, Api_GetMessage);
	//Api_SetFunction(API_GET_HOME_PATH, Api_GetHomePath);
	//Api_SetFunction(API_SET_SERVICE_PORT, Api_SetServicePort);
	//
	//Api_SetFunction(API_UNLOAD_DRIVER, Driver_Api_Unload);
	//
	////Api_SetFunction(API_HOOK_TRAMP,         Hook_Api_Tramp);
	//
	//Api_SetFunction(API_PROCESS_EXEMPTION_CONTROL, Api_ProcessExemptionControl);
	//
	//Api_SetFunction(API_QUERY_DRIVER_INFO, Api_QueryDriverInfo);
	//
	//Api_SetFunction(API_SET_SECURE_PARAM, Api_SetSecureParam);
	Api_SetFunction(API_GET_SECURE_PARAM, Api_GetSecureParam);
	if ((!Api_Functions) || (Api_Functions == (void*)-1))
		return FALSE;
	//表明API已准备好使用
	InterlockedExchange(&Api_UseCount, 0);

	return TRUE;
}

BOOLEAN Api_FastIo_DEVICE_CONTROL(FILE_OBJECT* FileObject, BOOLEAN Wait, void* InputBuffer, ULONG InputBufferLength, void* OutputBuffer, ULONG OutputBufferLength, ULONG IoControlCode, IO_STATUS_BLOCK* IoStatus, DEVICE_OBJECT* DeviceObject)
{
	//以后完善
	NTSTATUS status;
	ULONG buf_len, func_code;
	ULONG64* buf;
	ULONG64 user_args[API_NUM_ARGS];
	PROCESS* proc;
	P_Api_Function func_ptr;
	BOOLEAN ApcsDisabled;
	return STATUS_SUCCESS;

}

NTSTATUS Api_Irp_CREATE(DEVICE_OBJECT* device_object, IRP* irp)
{
	return STATUS_SUCCESS;
}

NTSTATUS Api_Irp_CLEANUP(DEVICE_OBJECT* device_object, IRP* irp)
{
	return STATUS_SUCCESS;
}

NTSTATUS Api_GetVersion(PROCESS* proc, ULONG64* parms)
{
	API_GET_VERSION_ARGS* args = (API_GET_VERSION_ARGS*)parms;
	if (args->string.val != NULL) {
		//size_t len = (wcslen(Driver_Version) + 1) * sizeof(WCHAR);
		//ProbeForWrite(args->string.val, len, sizeof(WCHAR));
		//memcpy(args->string.val, Driver_Version, len);
	}

	if (args->abi_ver.val != NULL) {
		ProbeForWrite(args->abi_ver.val, sizeof(ULONG), sizeof(ULONG));
		*args->abi_ver.val = MY_ABI_VERSION;
	}

	return STATUS_SUCCESS;
}


NTSTATUS Api_GetSecureParam(PROCESS* proc, ULONG64* parms) 
{
	NTSTATUS status = STATUS_SUCCESS;
	API_SECURE_PARAM_ARGS* args = (API_SECURE_PARAM_ARGS*)parms;
	HANDLE handle = NULL;
	WCHAR* name = NULL;
	ULONG  name_len = 0;
	UCHAR* data = NULL;
	ULONG  data_len = 0;

	if (proc) {
		status = STATUS_NOT_IMPLEMENTED;
		goto finish;
	}
	__try {

		UNICODE_STRING KeyPath;
		RtlInitUnicodeString(&KeyPath, Api_ParamPath);

		name_len = (wcslen(args->param_name.val) + 3 + 1) * sizeof(WCHAR);
		name = Mem_Alloc(Driver_Pool, name_len);
		wcscpy(name, args->param_name.val);
		UNICODE_STRING ValueName;
		RtlInitUnicodeString(&ValueName, name);

		OBJECT_ATTRIBUTES objattrs;
		InitializeObjectAttributes(&objattrs, &KeyPath, OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE, NULL, NULL);
		status = ZwOpenKey(&handle, KEY_WRITE, &objattrs);
		if (status == STATUS_SUCCESS) {

			data_len = args->param_size.val + sizeof(KEY_VALUE_PARTIAL_INFORMATION);
			data = Mem_Alloc(Driver_Pool, data_len);

			ULONG length;
			status = ZwQueryValueKey(handle, &ValueName, KeyValuePartialInformation, data, data_len, &length);

			if (NT_SUCCESS(status) && args->param_verify.val)
			{
				wcscat(name, L"Sig");
				RtlInitUnicodeString(&ValueName, name);

				UCHAR data_sig[128];
				ULONG length_sig;
				status = ZwQueryValueKey(handle, &ValueName, KeyValuePartialInformation, data_sig, sizeof(data_sig), &length_sig);

				if (NT_SUCCESS(status))
				{
					PKEY_VALUE_PARTIAL_INFORMATION info = (PKEY_VALUE_PARTIAL_INFORMATION)data;
					PKEY_VALUE_PARTIAL_INFORMATION info_sig = (PKEY_VALUE_PARTIAL_INFORMATION)data_sig;
					status = KphVerifyBuffer(info->Data, info->DataLength, info_sig->Data, info_sig->DataLength);
				}
			}

			if (NT_SUCCESS(status))
			{
				PKEY_VALUE_PARTIAL_INFORMATION info = (PKEY_VALUE_PARTIAL_INFORMATION)data;
				if (info->DataLength <= args->param_size.val)
				{
					memcpy(args->param_data.val, info->Data, info->DataLength);

					if (args->param_size_out.val)
						*args->param_size_out.val = info->DataLength;
				}
				else
					status = STATUS_BUFFER_TOO_SMALL;
			}
		}

	}
	__except (EXCEPTION_EXECUTE_HANDLER) {
		status = GetExceptionCode();
	}

	if (name)
		Mem_Free(name, name_len);
	if (data)
		Mem_Free(data, data_len);

	if (handle)
		ZwClose(handle);

finish:
	return status;
}


NTSTATUS Api_SetServicePort(PROCESS* proc, ULONG64* parms) 
{
	//确认调用方是我们安装文件夹中的一个未装箱的系统进程，即Sandboxie服务
	NTSTATUS status = STATUS_ACCESS_DENIED;
	if ((!proc) && MyIsCallerMyServiceProcess()) {

		status = STATUS_SUCCESS;
	}

	if (NT_SUCCESS(status) && !MyIsCallerSigned()) {

		status = STATUS_INVALID_SIGNATURE;
	}

	//
	//对指定的LPC端口对象进行引用
	//

	if (NT_SUCCESS(status)) {

		void* PortObject, * OldObject;

		HANDLE PortHandle = (HANDLE)(ULONG_PTR)parms[1];
		if (PortHandle) {

			status = ObReferenceObjectByHandle(
				PortHandle, 0, *LpcPortObjectType, KernelMode,
				&PortObject, NULL);
		}
		else {

			PortObject = NULL;
			status = STATUS_SUCCESS;
		}

		//
		//替换所存储的端口对象引用，
		//释放对旧存储的端口对象的引用
		//

		if (NT_SUCCESS(status)) {

			KIRQL irql = Api_EnterCriticalSection();

			OldObject = InterlockedExchangePointer(
				&Api_ServicePortObject, PortObject);

			InterlockedExchangePointer(
				&Api_ServiceProcessId, PsGetCurrentProcessId());

			Api_LeaveCriticalSection(irql);

			if (OldObject)
				ObDereferenceObject(OldObject);
		}
	}

	return status;
}


KIRQL Api_EnterCriticalSection(void)
{
	KIRQL irql;

	KeRaiseIrql(APC_LEVEL, &irql);
	ExAcquireResourceExclusiveLite(Api_LockResource, TRUE);

	return irql;
}

void Api_LeaveCriticalSection(KIRQL oldirql)
{
	ExReleaseResourceLite(Api_LockResource);
	KeLowerIrql(oldirql);
}
