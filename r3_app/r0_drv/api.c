#include"api.h"
#include"api_defs.h"
#include"mem.h"
#include"log.h"
#include"../common/defines.h"
static P_Api_Function* Api_Functions = NULL;
static LOG_BUFFER* Api_LogBuffer = NULL;
static PERESOURCE Api_LockResource = NULL;
static BOOLEAN Api_WorkListInitialized = FALSE;
static FAST_IO_DISPATCH* Api_FastIoDispatch = NULL;
DEVICE_OBJECT* Api_DeviceObject = NULL;
static volatile LONG Api_UseCount = -1;

static NTSTATUS Api_Irp_CREATE(DEVICE_OBJECT* device_object, IRP* irp);

static NTSTATUS Api_Irp_CLEANUP(DEVICE_OBJECT* device_object, IRP* irp);

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
	//Api_SetFunction(API_GET_VERSION, Api_GetVersion);
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
	//Api_SetFunction(API_GET_SECURE_PARAM, Api_GetSecureParam);
	if ((!Api_Functions) || (Api_Functions == (void*)-1))
		return FALSE;
	//表明API已准备好使用
	InterlockedExchange(&Api_UseCount, 0);

	return TRUE;
}

BOOLEAN Api_FastIo_DEVICE_CONTROL(FILE_OBJECT* FileObject, BOOLEAN Wait, void* InputBuffer, ULONG InputBufferLength, void* OutputBuffer, ULONG OutputBufferLength, ULONG IoControlCode, IO_STATUS_BLOCK* IoStatus, DEVICE_OBJECT* DeviceObject)
{
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
