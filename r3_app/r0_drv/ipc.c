#include "ipc.h"
#include"mem.h"
#include"syscall.h"
#include"conf.h"
#include"obj.h"

static BOOLEAN Ipc_Init_Type(
    const WCHAR* TypeName, P_Syscall_Handler2 handler, ULONG createEx);
static NTSTATUS Ipc_CheckGenericObject(
    PROCESS* proc, void* Object, UNICODE_STRING* Name,
    ULONG Operation, ACCESS_MASK GrantedAccess);
static NTSTATUS Ipc_CheckPortObject(
    PROCESS* proc, void* Object, UNICODE_STRING* Name,
    ULONG Operation, ACCESS_MASK GrantedAccess);
static NTSTATUS Ipc_CheckJobObject(
    PROCESS* proc, void* Object, UNICODE_STRING* Name,
    ULONG Operation, ACCESS_MASK GrantedAccess);

#ifdef ALLOC_PRAGMA
#pragma alloc_text (INIT, Ipc_Init)
#pragma alloc_text (INIT, Ipc_Init_Type)
#endif // ALLOC_PRAGMA

static const WCHAR* Ipc_Event_TypeName = L"Event";
static const WCHAR* Ipc_EventPair_TypeName = L"EventPair";
static const WCHAR* Ipc_KeyedEvent_TypeName = L"KeyedEvent";
static const WCHAR* Ipc_Mutant_TypeName = L"Mutant";
static const WCHAR* Ipc_Semaphore_TypeName = L"Semaphore";
static const WCHAR* Ipc_Section_TypeName = L"Section";
static const WCHAR* Ipc_JobObject_TypeName = L"JobObject";
static const WCHAR* Ipc_SymLink_TypeName = L"SymbolicLinkObject";
static const WCHAR* Ipc_Directory_TypeName = L"DirectoryObject";
static PERESOURCE Ipc_DirLock = NULL;
static LIST Ipc_ObjDirs;
IPC_DYNAMIC_PORTS Ipc_Dynamic_Ports;



BOOLEAN Ipc_Init(void)
{
    const UCHAR* _PortSyscallNames[] = {
        "ConnectPort", "SecureConnectPort", "CreatePort",
        "AlpcConnectPort", "AlpcCreatePort", NULL
    };
    const UCHAR** NamePtr;

    List_Init(&Ipc_ObjDirs);
    if (!Mem_GetLockResource(&Ipc_DirLock, TRUE))
        return FALSE;

    //为泛型对象设置对象打开处理程序
#define Ipc_Init_Type_Generic(TypeName, ex)                     \
    if (! Ipc_Init_Type(TypeName, Ipc_CheckGenericObject, ex))  \
        return FALSE;
    Ipc_Init_Type_Generic(Ipc_Event_TypeName, 0);
    Ipc_Init_Type_Generic(Ipc_EventPair_TypeName, 0);
    Ipc_Init_Type_Generic(Ipc_KeyedEvent_TypeName, 0);
    Ipc_Init_Type_Generic(Ipc_Mutant_TypeName, 0);
    Ipc_Init_Type_Generic(Ipc_Semaphore_TypeName, 0);
    //NtCreateSectionEx在windows 10 1809中引入
    Ipc_Init_Type_Generic(Ipc_Section_TypeName, 17763); 
    Ipc_Init_Type_Generic(Ipc_SymLink_TypeName, 0);
    //NtCreateDirectoryObjectEx引入了windows 8
    Ipc_Init_Type_Generic(Ipc_Directory_TypeName, 9200);
    if (!Ipc_Init_Type(Ipc_JobObject_TypeName, Ipc_CheckJobObject, 0))
        return FALSE;
    //为端口对象设置对象打开处理程序
    for (NamePtr = _PortSyscallNames; *NamePtr; ++NamePtr) 
    {

        if (Driver_OsVersion >= DRIVER_WINDOWS_VISTA ||
            memcmp(*NamePtr, "Alpc", 4) != 0) 
        {

            if (!Syscall_Set2(*NamePtr, Ipc_CheckPortObject))
                return FALSE;
        }
    }
    if (Driver_OsVersion >= DRIVER_WINDOWS_8) 
    {
        if (!Syscall_Set2("AlpcConnectPortEx", Ipc_CheckPortObject))
            return FALSE;
    }
    //在Vista SP1及更高版本上注册对象筛选器回调
    if (Driver_OsVersion > DRIVER_WINDOWS_VISTA) 
    {
        if (Conf_Get_Boolean(NULL, L"EnableObjectFiltering", 0, TRUE)) {
            if (!Obj_Load_Filter())
                return FALSE;
        }
    }
    //设置端口请求过滤器处理程序
    //if (!Syscall_Set1("ImpersonateClientOfPort", Ipc_ImpersonatePort))
    //    return FALSE;
    //if (Driver_OsVersion >= DRIVER_WINDOWS_VISTA) {

    //    if (!Syscall_Set1(
    //        "AlpcImpersonateClientOfPort", Ipc_ImpersonatePort))
    //        return FALSE;

    //    //
    //    // protect use of NtRequestPort, NtRequestWaitReplyPort, and
    //    // NtAlpcSendWaitReceivePort on Windows Vista to prevent use
    //    // of EndTask and NetUserChangePassword, when called through our
    //    // syscall interface.
    //    //
    //    // note that if a malicious program calls NtAlpcSendWaitReceivePort
    //    // directly (without using our syscall interface to elevate first)
    //    // to emulate NetUserChangePassword, then it gets status code
    //    // STATUS_PRIVILEGE_NOT_HELD because the restricted process token
    //    // does not include the change notify privilege.
    //    //
    //    // on Windows Vista, direct use of NtAlpcSendWaitReceivePort to
    //    // emulate EndTask (without using our syscall interface to elevate
    //    // first) will be blocked by UIPI.  (But note that applications in
    //    // other sandboxed will still be killable.)
    //    //
    //    // on Windows XP, the real NtRequestPort and NtRequestWaitReplyPort
    //    // in the kernel are already hooked by the gui_xp module.
    //    //

    //    if (!Syscall_Set1("RequestPort", Ipc_RequestPort))
    //        return FALSE;

    //    if (!Syscall_Set1("RequestWaitReplyPort", Ipc_RequestPort))
    //        return FALSE;

    //    if (!Syscall_Set1(
    //        "AlpcSendWaitReceivePort", Ipc_AlpcSendWaitReceivePort))
    //        return FALSE;
    //}
    
    // set API handlers
    //Api_SetFunction(API_DUPLICATE_OBJECT, Ipc_Api_DuplicateObject);
    //Api_SetFunction(API_CREATE_DIR_OR_LINK, Ipc_Api_CreateDirOrLink);
    //Api_SetFunction(API_OPEN_DEVICE_MAP, Ipc_Api_OpenDeviceMap);
    //Api_SetFunction(API_QUERY_SYMBOLIC_LINK, Ipc_Api_QuerySymbolicLink);
    //Api_SetFunction(API_GET_DYNAMIC_PORT_FROM_PID, Ipc_Api_GetDynamicPortFromPid);
    //Api_SetFunction(API_OPEN_DYNAMIC_PORT, Ipc_Api_OpenDynamicPort);

    //
    // prepare dynamic ports
    //

    if (!Mem_GetLockResource(&Ipc_Dynamic_Ports.pPortLock, TRUE))
        return FALSE;
    List_Init(&Ipc_Dynamic_Ports.Ports);

    //
    // finish
    //

    return TRUE;
}

BOOLEAN Ipc_Init_Type(const WCHAR* TypeName, P_Syscall_Handler2 handler, ULONG createEx)
{
    WCHAR nameW[64];
    UCHAR nameA[64];
    ULONG i, n;
    wcscpy(nameW, L"Open");
    wcscat(nameW, TypeName);
    n = wcslen(nameW);
    for (i = 0; i <= n; ++i)
        nameA[i] = (UCHAR)nameW[i];
    if (!Syscall_Set2(nameA, handler))
        return FALSE;
    wcscpy(nameW, L"Create");
    wcscat(nameW, TypeName);
    n = wcslen(nameW);
    for (i = 0; i <= n; ++i)
        nameA[i] = (UCHAR)nameW[i];

    if (!Syscall_Set2(nameA, handler))
        return FALSE;
    if (createEx && Driver_OsBuild >= createEx) 
    {

        strcat(nameA, "Ex");

        if (!Syscall_Set2(nameA, handler))
            return FALSE;
    }

    return TRUE;
}

NTSTATUS Ipc_CheckGenericObject(PROCESS* proc, void* Object, UNICODE_STRING* Name, ULONG Operation, ACCESS_MASK GrantedAccess)
{
    //以后实现
    return STATUS_SUCCESS;
}

NTSTATUS Ipc_CheckPortObject(PROCESS* proc, void* Object, UNICODE_STRING* Name, ULONG Operation, ACCESS_MASK GrantedAccess)
{
    //以后实现
    return STATUS_SUCCESS;
}

NTSTATUS Ipc_CheckJobObject(PROCESS* proc, void* Object, UNICODE_STRING* Name, ULONG Operation, ACCESS_MASK GrantedAccess)
{
    //以后实现
    return STATUS_SUCCESS;
}
