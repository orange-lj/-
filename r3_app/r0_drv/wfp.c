//#include"wfp.h"
//#include"../common/map.h"
//#include"conf.h"
//#include"api.h"
//#include<fwpmtypes.h>
//
//extern DEVICE_OBJECT* Api_DeviceObject;
//
//BOOLEAN WFP_Enabled = FALSE;
//static PERESOURCE WFP_InitLock = NULL;
//static HANDLE WFP_state_handle = NULL;
//static BOOLEAN WPF_MapInitialized = FALSE;
//static map_base_t WFP_Processes;
//static KSPIN_LOCK WFP_MapLock;
//
//void WFP_state_changed(
//	_Inout_ void* context,
//	_In_ FWPM_SERVICE_STATE newState);
//
//
//VOID* WFP_Alloc(void* pool, size_t size)
//{
//	return ExAllocatePoolWithTag(NonPagedPool, size, tzuk);
//}
//
//
//VOID WFP_Free(void* pool, void* ptr)
//{
//	ExFreePoolWithTag(ptr, tzuk);
//}
//
//BOOLEAN WFP_Init(void)
//{
//	map_init(&WFP_Processes, NULL);
//	WFP_Processes.func_malloc = &WFP_Alloc;
//	WFP_Processes.func_free = &WFP_Free;
//
//	KeInitializeSpinLock(&WFP_MapLock);
//
//	WPF_MapInitialized = TRUE;
//
//	if (!Conf_Get_Boolean(NULL, L"NetworkEnableWFP", 0, FALSE))
//		return TRUE;
//
//	return WFP_Load();
//}
//
//
//
//BOOLEAN WFP_Load(void)
//{
//	if (WFP_Enabled)
//		return TRUE;
//
//	WFP_Enabled = TRUE;
//
//	map_resize(&WFP_Processes, 128); // prepare some buckets for better performance
//
//	DbgPrint("Sbie WFP enabled\r\n");
//
//	if (!Mem_GetLockResource(&WFP_InitLock, TRUE))
//		return FALSE;
//
//	NTSTATUS status = FwpmBfeStateSubscribeChanges((void*)Api_DeviceObject, WFP_state_changed, NULL, &WFP_state_handle);
//	if (!NT_SUCCESS(status)) {
//		DbgPrint("Sbie WFP failed to install state change callback\r\n");
//		Mem_FreeLockResource(&WFP_InitLock);
//		WFP_InitLock = NULL;
//		return FALSE;
//	}
//
//	if (FwpmBfeStateGet() == FWPM_SERVICE_RUNNING) {
//
//		KeEnterCriticalRegion();
//
//		ExAcquireResourceSharedLite(WFP_InitLock, TRUE);
//
//		WFP_Install_Callbacks();
//
//		ExReleaseResourceLite(WFP_InitLock);
//
//		KeLeaveCriticalRegion();
//	}
//	else
//		DbgPrint("Sbie WFP is not ready\r\n");
//
//	return TRUE;
//}
//
//void WFP_state_changed(_Inout_ void* context, _In_ FWPM_SERVICE_STATE newState)
//{
//	KeEnterCriticalRegion();
//
//	ExAcquireResourceSharedLite(WFP_InitLock, TRUE);
//
//	if (newState == FWPM_SERVICE_STOP_PENDING)
//		WFP_Uninstall_Callbacks();
//	else if (newState == FWPM_SERVICE_RUNNING)
//		WFP_Install_Callbacks();
//
//	ExReleaseResourceLite(WFP_InitLock);
//
//	KeLeaveCriticalRegion();
//}