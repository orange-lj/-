#include "obj.h"
#include"mem.h"
#include"../common/defines.h"
POBJECT_TYPE* Obj_ObjectTypes = NULL;
BOOLEAN Obj_CallbackInstalled = FALSE;
P_ObGetObjectType pObGetObjectType = NULL;
P_ObQueryNameInfo pObQueryNameInfo = NULL;
static PVOID Obj_FilterCookie = NULL;
static OB_CALLBACK_REGISTRATION  Obj_CallbackRegistration = { 0 };
static OB_OPERATION_REGISTRATION Obj_OperationRegistrations[2] = { { 0 }, { 0 } };
static P_ObRegisterCallbacks pObRegisterCallbacks = NULL;
static P_ObUnRegisterCallbacks pObUnRegisterCallbacks = NULL;


static OBJECT_TYPE* Obj_GetObjectType(const WCHAR* TypeName);
static BOOLEAN Obj_AddObjectType(const WCHAR* TypeName);
static BOOLEAN Obj_Init_Filter(void);
static OB_PREOP_CALLBACK_STATUS Obj_PreOperationCallback(
    _In_ PVOID RegistrationContext, _Inout_ POB_PRE_OPERATION_INFORMATION PreInfo);

NTSTATUS Obj_GetName(POOL* pool, void* Object, OBJECT_NAME_INFORMATION** Name, ULONG* NameLength)
{
    //具体实现下次补上
	return STATUS_SUCCESS;
}

BOOLEAN Obj_Init(void)
{
	if (Driver_OsVersion >= DRIVER_WINDOWS_7) 
	{
		//获取ObQueryNameInfo和ObGetObjectType
		UNICODE_STRING uni;
		void* ptr;
        RtlInitUnicodeString(&uni, L"ObQueryNameInfo");
        ptr = MmGetSystemRoutineAddress(&uni);
        if (!ptr) {
            //Log_Msg1(MSG_DLL_GET_PROC, uni.Buffer);
            return FALSE;
        }

        pObQueryNameInfo = (P_ObQueryNameInfo)ptr;

        RtlInitUnicodeString(&uni, L"ObGetObjectType");
        ptr = MmGetSystemRoutineAddress(&uni);
        if (!ptr) {
            //Log_Msg1(MSG_DLL_GET_PROC, uni.Buffer);
            return FALSE;
        }

        pObGetObjectType = (P_ObGetObjectType)ptr;
    }
    //初始化一组可识别的对象类型
    Obj_ObjectTypes = Mem_AllocEx(
        Driver_Pool, sizeof(POBJECT_TYPE) * 10, TRUE);
    if (!Obj_ObjectTypes)
        return FALSE;
    memzero(Obj_ObjectTypes, sizeof(POBJECT_TYPE) * 10);

    if (!Obj_AddObjectType(L"Job")) // PsJobType
        return FALSE;
    if (!Obj_AddObjectType(L"Event")) // ExEventObjectType
        return FALSE;
    if (!Obj_AddObjectType(L"Mutant")) // ExMutantObjectType - not exported
        return FALSE;
    if (!Obj_AddObjectType(L"Semaphore")) // ExSemaphoreObjectType
        return FALSE;
    if (!Obj_AddObjectType(L"Section")) // MmSectionObjectType
        return FALSE;
    if (!Obj_AddObjectType(L"ALPC Port"))  // AlpcPortObjectType - not exported
        return FALSE;
    if (!Obj_AddObjectType(L"SymbolicLink")) // ObpSymbolicLinkObjectType - not exported
        return FALSE;
    //在Vista SP1及更高版本上准备对象筛选器回调注册
    if (Driver_OsVersion > DRIVER_WINDOWS_VISTA) {

        if (!Obj_Init_Filter())
            return FALSE;
    }
}

POBJECT_TYPE Obj_GetTypeObjectType(void)
{
    static POBJECT_TYPE _TypeObjectType = NULL;
    //在Windows7上，我们需要找到ObpTypeObjectType。
    //它不会被导出，但一个新的Windows 7导出函数ObGetObjectType使用以下指令引用了一个对象类型表：
        //mov eax，双字ptr[nt！ObTypeIndexTable+eax*4]
    //ObpTypeObjectType是表中的第三个指针
    if ((Driver_OsVersion >= DRIVER_WINDOWS_7) && (!_TypeObjectType)) 
    {
        const ULONG status = STATUS_UNSUCCESSFUL;
        static const WCHAR* TypeName = L"ObjectType";

        POBJECT_TYPE pType;
        UNICODE_STRING* Name;

        //分析ObGetObjectType以查找ObTypeIndexTable
        POBJECT_TYPE* pObTypeIndexTable = NULL;

        UCHAR* ptr = (UCHAR*)pObGetObjectType;
        if (ptr) 
        {
            ULONG i;
            if (Driver_OsVersion < DRIVER_WINDOWS_10) 
            {
            
            }
            else 
            {
                for (i = 0x1c; i < 0x2c; ++i) 
                {
                    if (ptr[i + 0] == 0x48 &&       // lea rcx,nt!ObTypeIndexTable
                        ptr[i + 1] == 0x8D &&
                        ptr[i + 2] == 0x0D)
                    {
                        LONG offset = *(LONG*)(ptr + i + 3);
                        pObTypeIndexTable =
                            (POBJECT_TYPE*)(ptr + i + 7 + offset);
                    }
                }
            }
        }
        if (!pObTypeIndexTable) {
            //Log_Status_Ex(MSG_OBJ_HOOK_ANY_PROC, 0x91, status, TypeName);
            return FALSE;
        }
        //确保obtypeindextable[2]指向“类型”对象类型的点
        pType = pObTypeIndexTable[2];
        if ((!pType) || ((ULONG_PTR)pType >= 0xFFFFFFFF00000000)) {
            //Log_Status_Ex(MSG_OBJ_HOOK_ANY_PROC, 0x92, status, TypeName);
            return FALSE;
        }
        Name = &((OBJECT_TYPE_VISTA_SP1*)pType)->Name;
        if (Name->Length != 8 ||
            Name->Buffer[0] != L'T' || Name->Buffer[1] != L'y' ||
            Name->Buffer[2] != L'p' || Name->Buffer[3] != L'e') {
            //Log_Status_Ex(MSG_OBJ_HOOK_ANY_PROC, 0x93, status, TypeName);
            return FALSE;
        }

        _TypeObjectType = pType;
    }
    return _TypeObjectType;
}

BOOLEAN Obj_Load_Filter(void)
{
    NTSTATUS status;

    if (Obj_CallbackInstalled)
        return TRUE;
    //来自Syscall_DuplicateHandle_2：
    //请注意，文件和注册表项[…]不支持在复制期间在句柄上添加新权限。（对于任何指定SecurityProcedure的对象类型都是如此。）
    //文件的打开 / 创建由安装了FltRegisterFilter的微筛选器处理，注册表项的打开 / 新建由CmRegisterCallbackEx处理
    //由Syscall_DuplicateHandle处理的类型如下
    // "Process"    -> Thread_CheckProcessObject        <- PsProcessType
    // "Thread"     -> Thread_CheckThreadObject         <- PsThreadType
    // 
    // "File"       -> File_CheckFileObject             <- IoFileObjectType // given the the note above why do we double filter for files ???
    // 
    // "Event"      -> Ipc_CheckGenericObject
    // "EventPair"  -> Ipc_CheckGenericObject           <- ExEventPairObjectType not exported
    // "KeyedEvent" -> Ipc_CheckGenericObject           <- ExpKeyedEventObjectType not exported
    // "Mutant"     -> Ipc_CheckGenericObject           <- ExMutantObjectType not exported
    // "Semaphore"  -> Ipc_CheckGenericObject           <- ExSemaphoreObjectType
    // "Section"    -> Ipc_CheckGenericObject           <- MmSectionObjectType
    // 
    // "JobObject"  -> Ipc_CheckJobObject               <- PsJobType
    // 
    // "Port" / "ALPC Port" -> Ipc_CheckPortObject      <- AlpcPortObjectType and LpcWaitablePortObjectType not exported, LpcPortObjectType exported
    //      Note: proper  IPC isolation requires filtering of NtRequestPort, NtRequestWaitReplyPort, and NtAlpcSendWaitReceivePort calls
    // 
    // "Token"      -> Thread_CheckTokenObject          <- SeTokenObjectType
    if (!pObRegisterCallbacks || !pObUnRegisterCallbacks)
        status = STATUS_PROCEDURE_NOT_FOUND;
    else {

        Obj_OperationRegistrations[0].ObjectType = PsProcessType;
        Obj_OperationRegistrations[0].Operations = OB_OPERATION_HANDLE_CREATE | OB_OPERATION_HANDLE_DUPLICATE;
        Obj_OperationRegistrations[0].PreOperation = Obj_PreOperationCallback;
        Obj_OperationRegistrations[0].PostOperation = NULL;

        Obj_OperationRegistrations[1].ObjectType = PsThreadType;
        Obj_OperationRegistrations[1].Operations = OB_OPERATION_HANDLE_CREATE | OB_OPERATION_HANDLE_DUPLICATE;
        Obj_OperationRegistrations[1].PreOperation = Obj_PreOperationCallback;
        Obj_OperationRegistrations[1].PostOperation = NULL;


        Obj_CallbackRegistration.Version = OB_FLT_REGISTRATION_VERSION;
        Obj_CallbackRegistration.OperationRegistrationCount = 2;
        Obj_CallbackRegistration.Altitude = Driver_Altitude;
        Obj_CallbackRegistration.RegistrationContext = NULL;
        Obj_CallbackRegistration.OperationRegistration = Obj_OperationRegistrations;

        status = pObRegisterCallbacks(&Obj_CallbackRegistration, &Obj_FilterCookie);
    }

    if (!NT_SUCCESS(status)) {
        //Log_Status_Ex(MSG_OBJ_HOOK_ANY_PROC, 0x81, status, L"Objects");
        //Obj_FilterCookie = NULL;
        return FALSE;
    }

    Obj_CallbackInstalled = TRUE;

    return TRUE;
}

OBJECT_TYPE* Obj_GetObjectType(const WCHAR* TypeName)
{
    NTSTATUS status;
    WCHAR ObjectName[64];
    UNICODE_STRING uni;
    OBJECT_ATTRIBUTES objattrs;
    HANDLE handle;
    OBJECT_TYPE* object;

    wcscpy(ObjectName, L"\\ObjectTypes\\");
    wcscat(ObjectName, TypeName);
    RtlInitUnicodeString(&uni, ObjectName);
    InitializeObjectAttributes(&objattrs,
        &uni, OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE, NULL, NULL);

    //Windows 7要求我们在下面的第二个参数中传递ObjectType，
    //而早期版本的Windows不需要这样做。
    //Obj_GetTypeObjectType（）在Windows 7上返回ObjectType，在早期版本的Windows上返回NULL
    status = ObOpenObjectByName(&objattrs, Obj_GetTypeObjectType(), KernelMode,
        NULL, 0, NULL, &handle);
    if (!NT_SUCCESS(status)) 
    {
        return NULL;
    }
    status = ObReferenceObjectByHandle(handle, 0, NULL, KernelMode, &object, NULL);
    ZwClose(handle);

    if (!NT_SUCCESS(status)) {
        //Log_Status_Ex(MSG_OBJ_HOOK_ANY_PROC, 0x55, status, TypeName);
        return NULL;
    }

    ObDereferenceObject(object);

    return object;
}

BOOLEAN Obj_AddObjectType(const WCHAR* TypeName)
{
    OBJECT_TYPE* object;
    ULONG i;

    object = Obj_GetObjectType(TypeName);
    if (object == NULL) 
    {
        return FALSE;
    }
    for (i = 0; Obj_ObjectTypes[i]; ++i) 
    {
        ;
    }
    Obj_ObjectTypes[i] = object;
    return TRUE;
}

BOOLEAN Obj_Init_Filter(void)
{
    UNICODE_STRING uni;
    RtlInitUnicodeString(&uni, L"ObRegisterCallbacks");
    pObRegisterCallbacks = (P_ObRegisterCallbacks)MmGetSystemRoutineAddress(&uni);
    RtlInitUnicodeString(&uni, L"ObUnRegisterCallbacks");
    pObUnRegisterCallbacks = (P_ObUnRegisterCallbacks)MmGetSystemRoutineAddress(&uni);
    //注意：Obj_Load_Filter需要先初始化一些其他内容，因此它将由Ipc_Init调用
    return TRUE;
}

OB_PREOP_CALLBACK_STATUS Obj_PreOperationCallback(PVOID RegistrationContext, POB_PRE_OPERATION_INFORMATION PreInfo)
{
    return STATUS_SUCCESS;
}
