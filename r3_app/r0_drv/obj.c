#include "obj.h"
#include"mem.h"
#include"../common/defines.h"

#define PAD_LEN         (4 * sizeof(WCHAR))

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
    NTSTATUS status;
    UCHAR small_info_space[80];
    OBJECT_NAME_INFORMATION* small_info;
    ULONG len;
    OBJECT_NAME_INFORMATION* info;
    ULONG info_len;

    *Name = NULL;
    *NameLength = 0;

    //��С�������ϵ���ObQueryNameString����ʹ��ȥPAD_LEN�ֽڣ�С������Ҳ�������sizeof��OBJECT_NAME_INFORMATION����
    //���仰˵������С������������32�ֽ����ң�ȡ����PAD_LEN��

    //��ʱObQueryNameString�������������STATUS_OBJECT_PATH_INVALID������������£�����ֻʹ����С�Ļ��������е���
    memzero(small_info_space, sizeof(small_info_space));
    small_info = (OBJECT_NAME_INFORMATION*)small_info_space;

    len = 0;        // must be initialized
    status = ObQueryNameString(
        Object, small_info, sizeof(small_info_space) - PAD_LEN, &len);
    if (status == STATUS_OBJECT_PATH_INVALID) 
    {
        len = 0;        // must be initialized
        status = ObQueryNameString(
            Object, small_info,
            sizeof(small_info_space) - PAD_LEN * 2, &len);
    }
    if (NT_SUCCESS(status)) 
    {
        //���ǽ�������ȫ����С�������У�������Ƿ���һ���ػ����������������ַ���
    }
    if (status != STATUS_INFO_LENGTH_MISMATCH &&
        status != STATUS_BUFFER_OVERFLOW) 
    {
        return status;
    }
    //��Windows 2000�ϣ����ǿ��ܻ�õ�STATUS_INFO_LENGTH_MISMATCH�����������Ϊ�㣬����������£����Ǳ���ʹ�ø���Ļ���������
    info = NULL;
    info_len = 0;

    while (!len) 
    {
    
    }
    //��Windows XP�У�����Ӧ�õõ�һ��������ȣ�������
    //���������ѭ��������info��Ȼ��NULL��������Ҫ
    //�����������ٴβ�ѯ����
    if (!info) 
    {
        info_len = len + PAD_LEN * 2;
        info = (OBJECT_NAME_INFORMATION*)Mem_Alloc(pool, info_len);
        if (!info)
            return STATUS_INSUFFICIENT_RESOURCES;

        memzero(info, info_len);
        len = 0;        // must be initialized
        status = ObQueryNameString(
            Object, info, info_len - PAD_LEN, &len);

        if (!NT_SUCCESS(status)) {
            Mem_Free(info, info_len);
            return status;
        }
    }
    //�������ֻ��Ҫȷ�����Ʋ��ǿյ�
finish:

    if (info && info->Name.Length && info->Name.Buffer) {

        //
        // On Windows 7, we may get two leading backslashes
        //

        if (Driver_OsVersion >= DRIVER_WINDOWS_7 &&
            info->Name.Length >= 2 * sizeof(WCHAR) &&
            info->Name.Buffer[0] == L'\\' &&
            info->Name.Buffer[1] == L'\\') {

            WCHAR* Buffer = info->Name.Buffer;
            USHORT Length = info->Name.Length;

            Length = Length / sizeof(WCHAR) - 1;
            wmemmove(Buffer, Buffer + 1, Length);
            Buffer[Length] = L'\0';

            info->Name.Length -= sizeof(WCHAR);
            info->Name.MaximumLength -= sizeof(WCHAR);
        }

        *Name = info;
        *NameLength = info_len;

    }
    else {

        //if (info)
        //    Mem_Free(info, info_len);
        //
        //*Name = (OBJECT_NAME_INFORMATION*)&Obj_Unnamed;
        //*NameLength = 0;
    }

    /*if (0) {
        OBJECT_NAME_INFORMATION *xname = *Name;
        WCHAR *xbuf = xname->Name.Buffer;
        ULONG xlen  = xname->Name.Length / sizeof(WCHAR);
        DbgPrint("Object Name: %*.*S\n", xlen, xlen, xbuf);
    }*/

    return STATUS_SUCCESS;
}

BOOLEAN Obj_Init(void)
{
	if (Driver_OsVersion >= DRIVER_WINDOWS_7) 
	{
		//��ȡObQueryNameInfo��ObGetObjectType
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
    //��ʼ��һ���ʶ��Ķ�������
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
    //��Vista SP1�����߰汾��׼������ɸѡ���ص�ע��
    if (Driver_OsVersion > DRIVER_WINDOWS_VISTA) {

        if (!Obj_Init_Filter())
            return FALSE;
    }
}

POBJECT_TYPE Obj_GetTypeObjectType(void)
{
    static POBJECT_TYPE _TypeObjectType = NULL;
    //��Windows7�ϣ�������Ҫ�ҵ�ObpTypeObjectType��
    //�����ᱻ��������һ���µ�Windows 7��������ObGetObjectTypeʹ������ָ��������һ���������ͱ�
        //mov eax��˫��ptr[nt��ObTypeIndexTable+eax*4]
    //ObpTypeObjectType�Ǳ��еĵ�����ָ��
    if ((Driver_OsVersion >= DRIVER_WINDOWS_7) && (!_TypeObjectType)) 
    {
        const ULONG status = STATUS_UNSUCCESSFUL;
        static const WCHAR* TypeName = L"ObjectType";

        POBJECT_TYPE pType;
        UNICODE_STRING* Name;

        //����ObGetObjectType�Բ���ObTypeIndexTable
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
        //ȷ��obtypeindextable[2]ָ�����͡��������͵ĵ�
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
    //����Syscall_DuplicateHandle_2��
    //��ע�⣬�ļ���ע�����[��]��֧���ڸ����ڼ��ھ���������Ȩ�ޡ��������κ�ָ��SecurityProcedure�Ķ������Ͷ�����ˡ���
    //�ļ��Ĵ� / �����ɰ�װ��FltRegisterFilter��΢ɸѡ������ע�����Ĵ� / �½���CmRegisterCallbackEx����
    //��Syscall_DuplicateHandle�������������
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

    //Windows 7Ҫ������������ĵڶ��������д���ObjectType��
    //�����ڰ汾��Windows����Ҫ��������
    //Obj_GetTypeObjectType������Windows 7�Ϸ���ObjectType�������ڰ汾��Windows�Ϸ���NULL
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
    //ע�⣺Obj_Load_Filter��Ҫ�ȳ�ʼ��һЩ�������ݣ����������Ipc_Init����
    return TRUE;
}

OB_PREOP_CALLBACK_STATUS Obj_PreOperationCallback(PVOID RegistrationContext, POB_PRE_OPERATION_INFORMATION PreInfo)
{
    return STATUS_SUCCESS;
}
