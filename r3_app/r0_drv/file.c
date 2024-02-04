#include "file.h"
#include"mem.h"
#include"my_fltkernel.h"
#include"../common/list.h"

static FLT_PREOP_CALLBACK_STATUS File_PreOperation(
    PFLT_CALLBACK_DATA Data,
    PCFLT_RELATED_OBJECTS FltObjects,
    void** CompletionContext);
static NTSTATUS File_QueryTeardown(
    PCFLT_RELATED_OBJECTS FltObjects,
    FLT_INSTANCE_QUERY_TEARDOWN_FLAGS Flags);

static BOOLEAN File_Init_Filter(void);

#ifdef ALLOC_DATA_PRAGMA
#pragma data_seg("INITDATA")
#pragma const_seg("INITDATA")
#endif

#define FILE_CALLBACK(irp) { irp, 0, File_PreOperation, NULL, NULL },

//����
static LIST File_ProtectedRoots;
static PERESOURCE File_ProtectedRootsLock;
static const FLT_CONTEXT_REGISTRATION File_Contexts[] = {

    { FLT_CONTEXT_END }
};
static const FLT_OPERATION_REGISTRATION File_Callbacks[] = {

    FILE_CALLBACK(IRP_MJ_CREATE)
    FILE_CALLBACK(IRP_MJ_CREATE_NAMED_PIPE)
    FILE_CALLBACK(IRP_MJ_CREATE_MAILSLOT)
    FILE_CALLBACK(IRP_MJ_SET_INFORMATION)

    /*
    FILE_CALLBACK(IRP_MJ_CLOSE)
    FILE_CALLBACK(IRP_MJ_READ)
    FILE_CALLBACK(IRP_MJ_WRITE)
    FILE_CALLBACK(IRP_MJ_QUERY_INFORMATION)
    FILE_CALLBACK(IRP_MJ_QUERY_EA)
    FILE_CALLBACK(IRP_MJ_SET_EA)
    FILE_CALLBACK(IRP_MJ_FLUSH_BUFFERS)
    FILE_CALLBACK(IRP_MJ_QUERY_VOLUME_INFORMATION)
    FILE_CALLBACK(IRP_MJ_SET_VOLUME_INFORMATION)
    FILE_CALLBACK(IRP_MJ_DIRECTORY_CONTROL)
    FILE_CALLBACK(IRP_MJ_FILE_SYSTEM_CONTROL)
    FILE_CALLBACK(IRP_MJ_DEVICE_CONTROL)
    FILE_CALLBACK(IRP_MJ_INTERNAL_DEVICE_CONTROL)
    FILE_CALLBACK(IRP_MJ_SHUTDOWN)
    FILE_CALLBACK(IRP_MJ_LOCK_CONTROL)
    FILE_CALLBACK(IRP_MJ_CLEANUP)
    FILE_CALLBACK(IRP_MJ_QUERY_SECURITY)
    FILE_CALLBACK(IRP_MJ_SET_SECURITY)
    FILE_CALLBACK(IRP_MJ_QUERY_QUOTA)
    FILE_CALLBACK(IRP_MJ_SET_QUOTA)
    FILE_CALLBACK(IRP_MJ_PNP)

    */
    FILE_CALLBACK(IRP_MJ_ACQUIRE_FOR_SECTION_SYNCHRONIZATION)
    /*
    FILE_CALLBACK(IRP_MJ_RELEASE_FOR_SECTION_SYNCHRONIZATION)
    FILE_CALLBACK(IRP_MJ_ACQUIRE_FOR_MOD_WRITE)
    FILE_CALLBACK(IRP_MJ_RELEASE_FOR_MOD_WRITE)
    FILE_CALLBACK(IRP_MJ_ACQUIRE_FOR_CC_FLUSH)
    FILE_CALLBACK(IRP_MJ_RELEASE_FOR_CC_FLUSH)

    FILE_CALLBACK(IRP_MJ_FAST_IO_CHECK_IF_POSSIBLE)
    FILE_CALLBACK(IRP_MJ_NETWORK_QUERY_OPEN)
    FILE_CALLBACK(IRP_MJ_MDL_READ)
    FILE_CALLBACK(IRP_MJ_MDL_READ_COMPLETE)
    FILE_CALLBACK(IRP_MJ_PREPARE_MDL_WRITE)
    FILE_CALLBACK(IRP_MJ_MDL_WRITE_COMPLETE)
    FILE_CALLBACK(IRP_MJ_VOLUME_MOUNT)
    FILE_CALLBACK(IRP_MJ_VOLUME_DISMOUNT)
    */

    {IRP_MJ_OPERATION_END}
};

static const FLT_REGISTRATION File_Registration = {

    // Size, Version, Flags

    sizeof(FLT_REGISTRATION),

    FLT_REGISTRATION_VERSION_0202,

    FLTFL_REGISTRATION_DO_NOT_SUPPORT_SERVICE_STOP,

    // ContextRegistration, OperationRegistration

    File_Contexts,
    File_Callbacks,

    // Callbacks

    NULL,                                   //  FilterUnload
    NULL,                                   //  InstanceSetup
    File_QueryTeardown,                     //  InstanceQueryTeardown
    NULL,                                   //  InstanceTeardownStart
    NULL,                                   //  InstanceTeardownComplete
    NULL,                                   //  GenerateFileName
    NULL,                                   //  GenerateDestinationFileName
    NULL                                    //  NormalizeNameComponent

};


#ifdef ALLOC_DATA_PRAGMA
#pragma data_seg()
#pragma const_seg()
#endif

#ifdef ALLOC_PRAGMA
#pragma alloc_text (INIT, File_Init)
#pragma alloc_text (INIT, File_Init_Filter)
#endif // ALLOC_PRAGMA

PFLT_FILTER File_FilterCookie = NULL;
static LIST File_ReparsePointsList;
static PERESOURCE File_ReparsePointsLock = NULL;


extern void File_InitReparsePoints(BOOLEAN init);




BOOLEAN File_Init(void)
{
	//��Windows XP/2003�ϰ�װ�������̹ҹ�
	//��Vista�����߰汾��ע��Ϊ�ļ�ϵͳ΢ɸѡ��
	typedef BOOLEAN(*P_File_Init_2)(void);
	P_File_Init_2 p_File_Init_2 = File_Init_Filter;
    if (!p_File_Init_2())
        return FALSE;
    //��ʼ���ط�����
    File_InitReparsePoints(TRUE);

    //����������XP��Vista��ϵͳ���ô������
    //if (!Syscall_Set1("CreatePagingFile", File_CreatePagingFile))
    //    return FALSE;
     
    // set API functions

    //Api_SetFunction(API_RENAME_FILE, File_Api_Rename);
    //Api_SetFunction(API_GET_FILE_NAME, File_Api_GetName);
    //Api_SetFunction(API_REFRESH_FILE_PATH_LIST, File_Api_RefreshPathList);
    //Api_SetFunction(API_OPEN_FILE, File_Api_Open);
    //Api_SetFunction(API_CHECK_INTERNET_ACCESS, File_Api_CheckInternetAccess);
    //Api_SetFunction(API_GET_BLOCKED_DLL, File_Api_GetBlockedDll);

    return TRUE;

}

BOOLEAN File_Init_Filter(void)
{
	static const WCHAR* _MiniFilter = L"MiniFilter";
	NTSTATUS status;

	List_Init(&File_ProtectedRoots);
	if (!Mem_GetLockResource(&File_ProtectedRootsLock, TRUE))
		return FALSE;
	//ע��Ϊ΢ɸѡ����������

	//��ע�⣬΢ɸѡ����֧��ɸѡ��ʵ�ļ���������ǻ���Ϊ�ļ����ϵͳ�������ô������
	//ִ��΢ɸѡ��ɸѡ�кô�����Ϊϵͳ���ô��������ڶ���򿪺�������أ��������ļ�������ܻὫ�־��ļ�����ɳ��֮�⡣
	// ���⣬�ļ�·���ܸ��ӣ����ܰ������ӣ���Щ���ӽ����ں�IO��ϵͳΪ��������׼��
	status = FltRegisterFilter(
		Driver_Object, &File_Registration, &File_FilterCookie);
    if (!NT_SUCCESS(status)) {
        //Log_Status_Ex(MSG_OBJ_HOOK_ANY_PROC, 0x81, status, _MiniFilter);
        return FALSE;
    }

    status = FltStartFiltering(File_FilterCookie);
    if (!NT_SUCCESS(status)) {
        //Log_Status_Ex(MSG_OBJ_HOOK_ANY_PROC, 0x82, status, _MiniFilter);
        return FALSE;
    }
    //Ϊ�����ܵ����ʼ����ļ��������ö���򿪴������
    //if (!Syscall_Set2("CreateFile", File_CheckFileObject))
    //    return FALSE;
    //
    //if (!Syscall_Set2("CreateMailslotFile", File_CheckFileObject))
    //    return FALSE;
    //
    //if (!Syscall_Set2("CreateNamedPipeFile", File_CheckFileObject))
    //    return FALSE;
    //
    //if (!Syscall_Set2("OpenFile", File_CheckFileObject))
    //    return FALSE;
    // 
    // set API functions

    //Api_SetFunction(API_PROTECT_ROOT, File_Api_ProtectRoot);
    //Api_SetFunction(API_UNPROTECT_ROOT, File_Api_UnprotectRoot);
    return TRUE;
}

void File_InitReparsePoints(BOOLEAN init)
{
    if (init) {

        List_Init(&File_ReparsePointsList);
        Mem_GetLockResource(&File_ReparsePointsLock, TRUE);

    }
    else {

        //Mem_FreeLockResource(&File_ReparsePointsLock);
    }
}

FLT_PREOP_CALLBACK_STATUS File_PreOperation(PFLT_CALLBACK_DATA Data, PCFLT_RELATED_OBJECTS FltObjects, void** CompletionContext)
{
    return STATUS_SUCCESS;
}

NTSTATUS File_QueryTeardown(PCFLT_RELATED_OBJECTS FltObjects, FLT_INSTANCE_QUERY_TEARDOWN_FLAGS Flags)
{
    return STATUS_SUCCESS;
}
