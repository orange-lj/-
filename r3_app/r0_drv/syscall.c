#include "syscall.h"
#include"dll.h"
#include"pattern.h"
#include"conf.h"
#include"mem.h"
#include"api_defs.h"
#include"../low/lowdata.h"
#include"api.h"
static LIST Syscall_List;

static BOOLEAN Syscall_Init_List(void);
static BOOLEAN Syscall_Init_Table(void);
static BOOLEAN Syscall_Init_ServiceData(void);
static ULONG Syscall_GetIndexFromNtdll(UCHAR* code);
static BOOLEAN Syscall_GetKernelAddr(
    ULONG index, void** pKernelAddr, ULONG* pParamCount);
static NTSTATUS Syscall_DuplicateHandle(
    PROCESS* proc, SYSCALL_ENTRY* syscall_entry, ULONG_PTR* user_args);
static NTSTATUS Syscall_GetNextProcess(
    PROCESS* proc, SYSCALL_ENTRY* syscall_entry, ULONG_PTR* user_args);

static NTSTATUS Syscall_GetNextThread(
    PROCESS* proc, SYSCALL_ENTRY* syscall_entry, ULONG_PTR* user_args);
static NTSTATUS Syscall_DeviceIoControlFile(
    PROCESS* proc, SYSCALL_ENTRY* syscall_entry, ULONG_PTR* user_args);
static BOOLEAN Syscall_QuerySystemInfo_SupportProcmonStack(
    PROCESS* proc, SYSCALL_ENTRY* syscall_entry, ULONG_PTR* user_args);
static NTSTATUS Syscall_OpenHandle(
    PROCESS* proc, SYSCALL_ENTRY* syscall_entry, ULONG_PTR* user_args);
static NTSTATUS Syscall_Api_Query(PROCESS* proc, ULONG64* parms);

static NTSTATUS Syscall_Api_Invoke(PROCESS* proc, ULONG64* parms);

//����
static ULONG Syscall_MaxIndex = 0;
static SYSCALL_ENTRY** Syscall_Table = NULL;
static UCHAR* Syscall_NtdllSavedCode = NULL;
static SYSCALL_ENTRY* Syscall_SetInformationThread = NULL;


//�ṹ��
#pragma pack(push)
#pragma pack(1)


typedef struct _SERVICE_DESCRIPTOR {

    LONG* Addrs;
    ULONG* DontCare1;   // always zero?
    ULONG Limit;
    LONG DontCare2;     // always zero?
    UCHAR* DontCare3;

} SERVICE_DESCRIPTOR;


#pragma pack(pop)


void* Syscall_GetServiceTable(void)
{
    static SERVICE_DESCRIPTOR* ShadowTable = NULL;
    SERVICE_DESCRIPTOR* MasterTable;
    if (ShadowTable) 
    {
        return ShadowTable;
    }
}

BOOLEAN Syscall_Init(void)
{
	if (!Syscall_Init_List())
		return FALSE;
    if (!Syscall_Init_Table())
        return FALSE;
    if (!Syscall_Init_ServiceData())
        return FALSE;
    if (!Syscall_Set1("DuplicateObject", Syscall_DuplicateHandle))
        return FALSE;
    if (Driver_OsVersion >= DRIVER_WINDOWS_VISTA) 
    {
        if (!Syscall_Set1("GetNextProcess", Syscall_GetNextProcess))
            return FALSE;

        if (!Syscall_Set1("GetNextThread", Syscall_GetNextThread))
            return FALSE;
    }
    if (!Syscall_Set1("DeviceIoControlFile", Syscall_DeviceIoControlFile))
        return FALSE;
    if (!Syscall_Set3("QuerySystemInformation", Syscall_QuerySystemInfo_SupportProcmonStack))
        return FALSE;
    Api_SetFunction(API_QUERY_SYSCALLS, Syscall_Api_Query);
    Api_SetFunction(API_INVOKE_SYSCALL, Syscall_Api_Invoke);
}

SYSCALL_ENTRY* Syscall_GetByName(const UCHAR* name)
{
    ULONG name_len = strlen(name);
    SYSCALL_ENTRY* entry = List_Head(&Syscall_List);
    while (entry) 
    {
        if (entry->name_len == name_len
            && memcmp(entry->name, name, name_len) == 0) 
        {
            return entry;
        }
        entry = List_Next(entry);
    }
    //Syscall_ErrorForAsciiName(name);
    return NULL;
}

BOOLEAN Syscall_Set1(const UCHAR* name, P_Syscall_Handler1 handler_func)
{
    SYSCALL_ENTRY* entry = Syscall_GetByName(name);
    if (!entry)
        return FALSE;
    entry->handler1_func = handler_func;
    return TRUE;

}

BOOLEAN Syscall_Set2(const UCHAR* name, P_Syscall_Handler2 handler_func)
{
    SYSCALL_ENTRY* entry = Syscall_GetByName(name);
    if (!entry)
        return FALSE;
    entry->handler1_func = Syscall_OpenHandle;
    entry->handler2_func = handler_func;
    return TRUE;
}

BOOLEAN Syscall_Set3(const UCHAR* name, P_Syscall_Handler3_Support_Procmon_Stack handler_func)
{
    SYSCALL_ENTRY* entry = Syscall_GetByName(name);
    if (!entry)
        return FALSE;
    entry->handler3_func_support_procmon = handler_func;
    return TRUE;
}

BOOLEAN Syscall_LoadHookMap(const WCHAR* setting_name, LIST* list)
{
    ULONG index;
    const WCHAR* value;
    PATTERN* pat;
    const WCHAR* _SysCallPresets = L"SysCallPresets";
    List_Init(list);
    Conf_AdjustUseCount(TRUE);
    for (index = 0; ; ++index) 
    {
        //��ȡ��·���б����һ����������
        value = Conf_Get(_SysCallPresets, setting_name, index);
        if (!value) 
        {
            break;
        }
        //����ͼ������ӵ��б�
        pat = Pattern_Create(Driver_Pool, value, FALSE, 0);
        if (pat) 
        {
            List_Insert_After(list, NULL, pat);
        }
    }
    Conf_AdjustUseCount(FALSE);
    return TRUE;
}

int Syscall_HookMapMatch(const UCHAR* name, ULONG name_len, LIST* list)
{
    PATTERN* pat;
    int match_len = 0;
    WCHAR wname[68];
    ULONG i;
    for (i = 0; i < min(name_len, 64); i++)
        wname[i] = name[i];
    wname[i] = 0;

    pat = List_Head(list);
    while (pat) 
    {

        int cur_len = Pattern_MatchX(pat, wname, name_len);
        if (cur_len > match_len) {
            match_len = cur_len;
        }

        pat = List_Next(pat);
    }
}

VOID Syscall_FreeHookMap(LIST* list)
{
    PATTERN* pat;
    while (1) 
    {
        pat = List_Head(list);
        if (!pat)
            break;
        List_Remove(list, pat);
        Pattern_Free(pat);
    }
}

BOOLEAN Syscall_Init_List(void)
{
    BOOLEAN success = FALSE;
    UCHAR* name, * ntdll_code;
    void* ntos_addr;
    DLL_ENTRY* dll;
    SYSCALL_ENTRY* entry;
    ULONG proc_index, proc_offset, syscall_index, param_count;
    ULONG name_len, entry_len;

    List_Init(&Syscall_List);

    //׼����׼�ͽ����б�
    LIST disabled_hooks;
    Syscall_LoadHookMap(L"DisableWinNtHook", &disabled_hooks);
    LIST approved_syscalls;
    Syscall_LoadHookMap(L"ApproveWinNtSysCall", &approved_syscalls);

    //ɨ��NTDLL�е�ÿ��ZwXxx����
    dll = Dll_Load(Dll_NTDLL);
    if (!dll) 
    {
        //goto finish;
    }
    proc_offset = Dll_GetNextProc(dll, "Zw", &name, &proc_index);
    if (!proc_offset) 
    {
        //goto finish;
    }
    while (proc_offset) 
    {
        if (name[0] != 'Z' || name[1] != 'w') 
        {
            break;
        }
        //����Zwǰ׺
        name += 2;
        for (name_len = 0; (name_len < 64) && name[name_len]; ++name_len) 
        {
            ;
        }
        entry = NULL;
        //���ǲ��ṳס�����ػ���ܲ����ص����ߵ�ZwXxx����
        //������Ϊ����Syscall_API_Invokeͨ��IopXxxControlFile����NtDeviceIoControlFile���ã�
        //���������ǵ�API�豸������ļ���������ã�û�з�����ζ���ļ�����û����Ӧ�Ľ�����
        //�����ܻ����������صĸ����á�

        //zwxxx����zwcontinue��zwcallbackreturn��zwraiseexception��ntterminatejobobject��ntterminateprocess��ntterminatethread
#define IS_PROC_NAME(ln,nm) (name_len == ln && memcmp(name, nm, ln) == 0)
        if (IS_PROC_NAME(8, "Continue")
            || IS_PROC_NAME(10, "ContinueEx")
            || IS_PROC_NAME(14, "CallbackReturn")
            || IS_PROC_NAME(14, "RaiseException")
            || IS_PROC_NAME(18, "TerminateJobObject")
            || IS_PROC_NAME(16, "TerminateProcess")
            || IS_PROC_NAME(15, "TerminateThread")
            ) 
        {
            //goto next_zwxxx;
        }
        //��64λWindows�ϣ���Щϵͳ�����Ǽٵģ�Ӧ������
        if (IS_PROC_NAME(15, "QuerySystemTime")) 
        {
            //goto next_zwxxx;
        }
        //ICD-10607-McAfeeʹ�����ڶ�ջ�д����Լ������ݡ����call��������˵������Ҫ��
        //if (    IS_PROC_NAME(14, "YieldExecution")) // $Workaround$ - 3rd party fix
        //    goto next_zwxxx;

        //Google Chrome�ġ�wow_helper��������ҪNtMapViewOfSection
        //Ϊ��û�б����ϡ���Ȼ��ֻ��64λVista����Ҫ������������ϵͳ���ö�������˵���Ǻ���Ҫ������
        //Ϊ�˱���һ���ԣ�����������������ƽ̨�Ϲҽ�
        //if (    IS_PROC_NAME(16,  "MapViewOfSection")) // $Workaround$ - 3rd party fix
        //    goto next_zwxxx;

        if (Syscall_HookMapMatch(name, name_len, &disabled_hooks)) 
        {
            //goto next_zwxxx;
        }
        //����ÿ��ZwXxx�����Բ��ҷ���������
        ntos_addr = NULL;
        param_count = 0;
        ntdll_code = Dll_RvaToAddr(dll, proc_offset);

        if (ntdll_code)
        {
            syscall_index = Syscall_GetIndexFromNtdll(ntdll_code);
            if (syscall_index == -2) 
            {
                //���ZwXxx��������һ��������ϵͳ���ã���ô������
                //goto next_zwxxx;
            }
            if (syscall_index != -1)
            {
                Syscall_GetKernelAddr(syscall_index, &ntos_addr, &param_count);
            }
        }
        if (!ntos_addr) 
        {
            //Syscall_ErrorForAsciiName(name);
            //goto finish;
        }
        //�����ǵ�ϵͳ�����б��д洢ϵͳ���õ���Ŀ
        entry_len = sizeof(SYSCALL_ENTRY) + name_len + 1;
        entry = Mem_AllocEx(Driver_Pool, entry_len, TRUE);
        if (!entry) 
        {
            //goto finish;
        }
        entry->syscall_index = (USHORT)syscall_index;
        entry->param_count = (USHORT)param_count;
        entry->ntdll_offset = proc_offset;
        entry->ntos_func = ntos_addr;
        entry->handler1_func = NULL;
        entry->handler2_func = NULL;
        entry->handler3_func_support_procmon = NULL;
        entry->approved = (Syscall_HookMapMatch(name, name_len, &approved_syscalls) != 0);
        entry->name_len = (USHORT)name_len;
        memcpy(entry->name, name, name_len);
        entry->name[name_len] = '\0';
        List_Insert_After(&Syscall_List, NULL, entry);

        if (syscall_index > Syscall_MaxIndex) 
        {
            Syscall_MaxIndex = syscall_index;
        }
        //������һ�����
next_zwxxx:
        proc_offset = Dll_GetNextProc(dll, NULL, &name, &proc_index);
    }
    success = TRUE;
    //����Ҳ������������ķ����򱨸����
    if (Syscall_MaxIndex < 100) 
    {
        //Log_Msg1(MSG_1113, L"100");
        //success = FALSE;
    }
    if (Syscall_MaxIndex >= 500) {
        //Log_Msg1(MSG_1113, L"500");
        //success = FALSE;
    }
finish:
    if (!success)
        Syscall_MaxIndex = 0;
    Syscall_FreeHookMap(&disabled_hooks);
    Syscall_FreeHookMap(&approved_syscalls);
    return success;
}

BOOLEAN Syscall_Init_Table(void)
{
    SYSCALL_ENTRY* entry;
    ULONG len, i;

    //������ϵͳ����������ӳ�䵽��Ŀ�ı�
    len = sizeof(SYSCALL_ENTRY*) * (Syscall_MaxIndex + 1);
    Syscall_Table = Mem_AllocEx(Driver_Pool, len, TRUE);
    if (!Syscall_Table)
        return FALSE;
    for (i = 0; i <= Syscall_MaxIndex; ++i) 
    {
        entry = List_Head(&Syscall_List);
        while (entry) 
        {
            if (entry->syscall_index == i)
                break;
            entry = List_Next(entry);
        }
        Syscall_Table[i] = entry;
    }
    return TRUE;
}

BOOLEAN Syscall_Init_ServiceData(void)
{
    UCHAR* NtdllExports[] = NATIVE_FUNCTION_NAMES;
    SYSCALL_ENTRY* entry;
    DLL_ENTRY* dll;
    UCHAR* ntdll_code;
    ULONG i;

    //����һЩ�ռ��Ա���ntdll�еĴ���
    Syscall_NtdllSavedCode = Mem_AllocEx(Driver_Pool, (NATIVE_FUNCTION_SIZE * NATIVE_FUNCTION_COUNT), TRUE);
    if (!Syscall_NtdllSavedCode)
        return FALSE;
    dll = Dll_Load(Dll_NTDLL);
    if (!dll)
        return FALSE;
    //��ntdll�б���һЩϵͳ���ú����ĸ�����
    //���ǽ�����Щ�������ݸ�SbieSvc�����߽������Ǵ��ݸ�SbieLow
    //�������core / svc / DriverAssistInject.cpp��core / low / lowdata.h��
    for (i = 0; i < NATIVE_FUNCTION_COUNT; ++i) 
    {
        //+2����Ntǰ׺
        entry = Syscall_GetByName(NtdllExports[i] + 2);
        if (!entry)
            return FALSE;
        ntdll_code = Dll_RvaToAddr(dll, entry->ntdll_offset);
        if (!ntdll_code) 
        {
            //Syscall_ErrorForAsciiName(entry->name);
            return FALSE;
        }
        memcpy(Syscall_NtdllSavedCode + (i * NATIVE_FUNCTION_SIZE), ntdll_code, NATIVE_FUNCTION_SIZE);
    }
    //��ȡNtSetInformationThread��ϵͳ���ã������syscall_Api_Invoke��
    Syscall_SetInformationThread = Syscall_GetByName("SetInformationThread");
    return TRUE;
}

ULONG Syscall_GetIndexFromNtdll(UCHAR* code)
{
    ULONG index = -1;
    // skip mov r10, rcx
    if (code[0] == 0x4C && code[1] == 0x8B && code[2] == 0xD1) 
    {
        code += 3;
    }
    if (*code == 0xB8)                      // mov eax, syscall number
        index = *(ULONG*)(code + 1);
    if (index == -1) 
    {
        //Log_Msg1(MSG_1113, L"INDEX");
    }
    return index;
}

BOOLEAN Syscall_GetKernelAddr(ULONG index, void** pKernelAddr, ULONG* pParamCount)
{
    SERVICE_DESCRIPTOR* ShadowTable =
        (SERVICE_DESCRIPTOR*)Syscall_GetServiceTable();
    if (ShadowTable)
    {
        ULONG MaxSyscallIndexPlusOne = ShadowTable->Limit;
        if ((index < 0x1000) &&
            ((index & 0xFFF) < MaxSyscallIndexPlusOne))
        {
            LONG_PTR EntryValue = (LONG_PTR)ShadowTable->Addrs[index];
            *pKernelAddr = (UCHAR*)ShadowTable->Addrs + (EntryValue >> 4);
            *pParamCount = (ULONG)(EntryValue & 0x0F) + 4;
            return TRUE;
        }
        //Log_Msg1(MSG_1113, L"ADDRESS");
    }
    return FALSE;
}

NTSTATUS Syscall_DuplicateHandle(PROCESS* proc, SYSCALL_ENTRY* syscall_entry, ULONG_PTR* user_args)
{
    //�Ժ�ʵ��
    return STATUS_SUCCESS;
}

NTSTATUS Syscall_GetNextProcess(PROCESS* proc, SYSCALL_ENTRY* syscall_entry, ULONG_PTR* user_args)
{
    return STATUS_SUCCESS;
}

NTSTATUS Syscall_GetNextThread(PROCESS* proc, SYSCALL_ENTRY* syscall_entry, ULONG_PTR* user_args)
{
    return STATUS_SUCCESS;
}

NTSTATUS Syscall_DeviceIoControlFile(PROCESS* proc, SYSCALL_ENTRY* syscall_entry, ULONG_PTR* user_args)
{
    return STATUS_SUCCESS;
}

BOOLEAN Syscall_QuerySystemInfo_SupportProcmonStack(PROCESS* proc, SYSCALL_ENTRY* syscall_entry, ULONG_PTR* user_args)
{
    return STATUS_SUCCESS;
}

NTSTATUS Syscall_OpenHandle(PROCESS* proc, SYSCALL_ENTRY* syscall_entry, ULONG_PTR* user_args)
{
    //�Ժ�ʵ��
    return STATUS_SUCCESS;
}

NTSTATUS Syscall_Api_Query(PROCESS* proc, ULONG64* parms)
{
    ULONG buf_len;
    ULONG* user_buf;
    ULONG* ptr;
    SYSCALL_ENTRY* entry;

#ifdef HOOK_WIN32K
    if (parms[2] == 1) // 1 - win32k
    { 
        //return Syscall_Api_Query32(proc, parms);
    }
    else if (parms[2] != 0)// 0 - ntoskrnl
    { 
        //return STATUS_INVALID_PARAMETER;
    }
#endif
    BOOLEAN add_names = parms[3] != 0;
    //�����߱��������ǵķ������
    if (proc || (PsGetCurrentProcessId() != Api_ServiceProcessId))
        return STATUS_ACCESS_DENIED;
    //Ϊsyscall������û�ģʽ�ռ�
    buf_len = sizeof(ULONG)         // size of buffer
        + sizeof(ULONG)         // offset to extra data (for SbieSvc)
        + (NATIVE_FUNCTION_SIZE * NATIVE_FUNCTION_COUNT) // saved code from ntdll
        + List_Count(&Syscall_List) * ((sizeof(ULONG) * 2) + (add_names ? 64 : 0))
        + sizeof(ULONG) * 2     // ������ֹ����Ŀ
        ;
    user_buf = (ULONG*)parms[1];
    ProbeForWrite(user_buf, buf_len, sizeof(ULONG));
    //��ϵͳ����������仺������
    //���ȣ����Ǵ洢�������Ĵ�С��
    //Ȼ��ΪSbieSvcʹ�õ�ULONG���ĸ�ZwXxx��������Ĵ��������ռ�
    ptr = user_buf;
    *ptr = buf_len;
    ++ptr;

    *ptr = 0;           //ƫ�Ƶ�����ƫ�Ƶ�ռλ��
    ++ptr;
    memcpy(ptr, Syscall_NtdllSavedCode, (NATIVE_FUNCTION_SIZE * NATIVE_FUNCTION_COUNT));
    ptr += (NATIVE_FUNCTION_SIZE * NATIVE_FUNCTION_COUNT) / sizeof(ULONG);
    //�����������źͣ�����32λWindows�ϣ�ÿ��ϵͳ���õĲ��������洢��һ��ULONG�С�
    //��ntdll�е���Ӧƫ�����洢����һ��ULONG��
    entry = List_Head(&Syscall_List);
    while (entry) 
    {
        ULONG syscall_index = (ULONG)entry->syscall_index;
        *ptr = syscall_index;
        ++ptr;
        *ptr = entry->ntdll_offset;
        ++ptr;
        if (add_names) 
        {
            //memcpy(ptr, entry->name, entry->name_len);
            //((char*)ptr)[entry->name_len] = 0;
            //ptr += 16; // 16 * sizeog(ULONG) = 64
        }
        entry = List_Next(entry);
    }
    //�洢���һ������ֹ����Ŀ���ɹ�����
    *ptr = 0;
    ++ptr;
    *ptr = 0;

    return STATUS_SUCCESS;

}

NTSTATUS Syscall_Api_Invoke(PROCESS* proc, ULONG64* parms)
{
    return STATUS_SUCCESS;
}
