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

//变量
static ULONG Syscall_MaxIndex = 0;
static SYSCALL_ENTRY** Syscall_Table = NULL;
static UCHAR* Syscall_NtdllSavedCode = NULL;
static SYSCALL_ENTRY* Syscall_SetInformationThread = NULL;


//结构体
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
        //获取此路径列表的下一个配置设置
        value = Conf_Get(_SysCallPresets, setting_name, index);
        if (!value) 
        {
            break;
        }
        //创建图案并添加到列表
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

    //准备批准和禁用列表
    LIST disabled_hooks;
    Syscall_LoadHookMap(L"DisableWinNtHook", &disabled_hooks);
    LIST approved_syscalls;
    Syscall_LoadHookMap(L"ApproveWinNtSysCall", &approved_syscalls);

    //扫描NTDLL中的每个ZwXxx导出
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
        //跳过Zw前缀
        name += 2;
        for (name_len = 0; (name_len < 64) && name[name_len]; ++name_len) 
        {
            ;
        }
        entry = NULL;
        //我们不会钩住不返回或可能不返回调用者的ZwXxx服务。
        //这是因为调用Syscall_API_Invoke通过IopXxxControlFile（由NtDeviceIoControlFile调用）
        //它接受我们的API设备对象的文件对象的引用，没有返回意味着文件对象没有相应的解引用
        //。可能还有其他严重的副作用。

        //zwxxx服务：zwcontinue、zwcallbackreturn、zwraiseexception、ntterminatejobobject、ntterminateprocess、ntterminatethread
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
        //在64位Windows上，有些系统调用是假的，应该跳过
        if (IS_PROC_NAME(15, "QuerySystemTime")) 
        {
            //goto next_zwxxx;
        }
        //ICD-10607-McAfee使用它在堆栈中传递自己的数据。这个call对我们来说并不重要。
        //if (    IS_PROC_NAME(14, "YieldExecution")) // $Workaround$ - 3rd party fix
        //    goto next_zwxxx;

        //Google Chrome的“wow_helper”进程需要NtMapViewOfSection
        //为还没有被迷上。虽然这只在64位Vista上需要，但这个特殊的系统调用对我们来说不是很重要，所以
        //为了保持一致性，我们跳过了在所有平台上挂接
        //if (    IS_PROC_NAME(16,  "MapViewOfSection")) // $Workaround$ - 3rd party fix
        //    goto next_zwxxx;

        if (Syscall_HookMapMatch(name, name_len, &disabled_hooks)) 
        {
            //goto next_zwxxx;
        }
        //分析每个ZwXxx导出以查找服务索引号
        ntos_addr = NULL;
        param_count = 0;
        ntdll_code = Dll_RvaToAddr(dll, proc_offset);

        if (ntdll_code)
        {
            syscall_index = Syscall_GetIndexFromNtdll(ntdll_code);
            if (syscall_index == -2) 
            {
                //如果ZwXxx导出不是一个真正的系统调用，那么跳过它
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
        //在我们的系统调用列表中存储系统调用的条目
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
        //进程下一个输出
next_zwxxx:
        proc_offset = Dll_GetNextProc(dll, NULL, &name, &proc_index);
    }
    success = TRUE;
    //如果找不到合理数量的服务，则报告错误
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

    //构建将系统调用索引号映射到条目的表
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

    //分配一些空间以保存ntdll中的代码
    Syscall_NtdllSavedCode = Mem_AllocEx(Driver_Pool, (NATIVE_FUNCTION_SIZE * NATIVE_FUNCTION_COUNT), TRUE);
    if (!Syscall_NtdllSavedCode)
        return FALSE;
    dll = Dll_Load(Dll_NTDLL);
    if (!dll)
        return FALSE;
    //在ntdll中保存一些系统调用函数的副本，
    //我们将把这些函数传递给SbieSvc，后者将把它们传递给SbieLow
    //（请参阅core / svc / DriverAssistInject.cpp和core / low / lowdata.h）
    for (i = 0; i < NATIVE_FUNCTION_COUNT; ++i) 
    {
        //+2跳过Nt前缀
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
    //获取NtSetInformationThread的系统调用（请参阅syscall_Api_Invoke）
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
    //以后实现
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
    //以后实现
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
    //调用者必须是我们的服务进程
    if (proc || (PsGetCurrentProcessId() != Api_ServiceProcessId))
        return STATUS_ACCESS_DENIED;
    //为syscall表分配用户模式空间
    buf_len = sizeof(ULONG)         // size of buffer
        + sizeof(ULONG)         // offset to extra data (for SbieSvc)
        + (NATIVE_FUNCTION_SIZE * NATIVE_FUNCTION_COUNT) // saved code from ntdll
        + List_Count(&Syscall_List) * ((sizeof(ULONG) * 2) + (add_names ? 64 : 0))
        + sizeof(ULONG) * 2     // 最终终止符条目
        ;
    user_buf = (ULONG*)parms[1];
    ProbeForWrite(user_buf, buf_len, sizeof(ULONG));
    //用系统调用数据填充缓冲区。
    //首先，我们存储缓冲区的大小，
    //然后为SbieSvc使用的ULONG和四个ZwXxx存根函数的代码留出空间
    ptr = user_buf;
    *ptr = buf_len;
    ++ptr;

    *ptr = 0;           //偏移到额外偏移的占位符
    ++ptr;
    memcpy(ptr, Syscall_NtdllSavedCode, (NATIVE_FUNCTION_SIZE * NATIVE_FUNCTION_COUNT));
    ptr += (NATIVE_FUNCTION_SIZE * NATIVE_FUNCTION_COUNT) / sizeof(ULONG);
    //将服务索引号和（仅在32位Windows上）每个系统调用的参数计数存储到一个ULONG中。
    //将ntdll中的相应偏移量存储到另一个ULONG中
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
    //存储最后一个零终止符条目并成功返回
    *ptr = 0;
    ++ptr;
    *ptr = 0;

    return STATUS_SUCCESS;

}

NTSTATUS Syscall_Api_Invoke(PROCESS* proc, ULONG64* parms)
{
    return STATUS_SUCCESS;
}
