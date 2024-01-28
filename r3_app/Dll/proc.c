#include "dll.h"

typedef NTSTATUS(*P_LdrQueryImageFileExecutionOptionsEx)(
    PUNICODE_STRING SubKey,
    PCWSTR ValueName,
    ULONG Type,
    PVOID Buffer,
    ULONG BufferSize,
    PULONG ReturnedLength,
    BOOLEAN Wow64);
static P_LdrQueryImageFileExecutionOptionsEx
__sys_LdrQueryImageFileExecutionOptionsEx = NULL;
static NTSTATUS Proc_LdrQueryImageFileExecutionOptionsEx(
    PUNICODE_STRING SubKey,
    PCWSTR ValueName,
    ULONG Type,
    PVOID Buffer,
    ULONG BufferSize,
    PULONG ReturnedLength,
    BOOLEAN Wow64);

BOOLEAN SbieDll_DisableCHPE(void)
{
    HMODULE module = Dll_Ntdll;

    P_LdrQueryImageFileExecutionOptionsEx LdrQueryImageFileExecutionOptionsEx =
        (P_LdrQueryImageFileExecutionOptionsEx)GetProcAddress(
            Dll_Ntdll, "LdrQueryImageFileExecutionOptionsEx");
    SBIEDLL_HOOK(Proc_, LdrQueryImageFileExecutionOptionsEx);

    return TRUE;
}


NTSTATUS Proc_LdrQueryImageFileExecutionOptionsEx(
    PUNICODE_STRING SubKey,
    PCWSTR ValueName,
    ULONG Type,
    PVOID Buffer,
    ULONG BufferSize,
    PULONG ReturnedLength,
    BOOLEAN Wow64) 
{
    //ARM64�ϵ�SandboxieҪ��x86Ӧ�ó���Ҫʹ��CHPE�������ļ���
    //�˹ҹ�����CreateProcessInternalW��PsAttributeChpe����Ϊ0���Ӷ�ʹ�ں˽�ntdll�ĳ���ǻ�ϰ汾���ص��½����С�
    //�йظ�����ϸ��Ϣ�������HookImageOptionsEx core/low/init.c�е�ע�͡�
    if (_wcsicmp(ValueName, L"LoadCHPEBinaries") == 0) {
        *(ULONG*)Buffer = 0;
        return STATUS_SUCCESS;
    }

    return __sys_LdrQueryImageFileExecutionOptionsEx(SubKey, ValueName, Type, Buffer, BufferSize, ReturnedLength, Wow64);
}

BOOLEAN SbieDll_RunFromHome(
    const WCHAR* pgmName, const WCHAR* pgmArgs,
    STARTUPINFOW* si, PROCESS_INFORMATION* pi) 
{
    ULONG len, i, err;
    WCHAR* path, * ptr;
    BOOL ok, inherit;
    ULONG creation_flags;
    //��ȡɳ��Ŀ¼�С�pgm��������·����pathtopgmName�� pgmArgs��
    len = MAX_PATH * 2 + wcslen(pgmName);
    if (pgmArgs)
        len += 1 + wcslen(pgmArgs);
    path = Dll_AllocTemp(len * sizeof(WCHAR));

    ptr = wcsrchr(pgmName, L'.');
    if (ptr && _wcsicmp(ptr, L".exe") == 0) {
        path[0] = L'\"';
        i = 1;
    }
    else
        i = 0;

    if (Dll_HomeDosPath) {
        wcscpy(&path[i], Dll_HomeDosPath);
        wcscat(path, L"\\");
    }
    else {
        GetModuleFileName(NULL, &path[i], MAX_PATH);
        ptr = wcsrchr(path, L'\\');
        if (ptr)
            ptr[1] = L'\0';
    }
    wcscat(path, pgmName);
    if (i) {
        if (pgmArgs) {
            wcscat(path, L"\" ");
            wcscat(path, pgmArgs);
        }
        else
            wcscat(path, L"\"");
    }
    //���δָ�� PROCESS_INFORMATION *pi����ֻ�轫�����з��ظ����÷�����
    if (!pi) 
    {
        len = (wcslen(path) + 1) * sizeof(WCHAR);
        si->lpReserved = HeapAlloc(GetProcessHeap(), 0, len);
        if (si->lpReserved) 
        {
            memcpy(si->lpReserved, path, len);
            Dll_Free(path);
            return TRUE;
        }
        else 
        {
            Dll_Free(path);
            return FALSE;
        }
    }
}