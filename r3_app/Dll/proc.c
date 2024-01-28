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
    //ARM64上的Sandboxie要求x86应用程序不要使用CHPE二进制文件。
    //此挂钩导致CreateProcessInternalW将PsAttributeChpe设置为0，从而使内核将ntdll的常规非混合版本加载到新进程中。
    //有关更多详细信息，请参阅HookImageOptionsEx core/low/init.c中的注释。
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
    //获取沙盒目录中“pgm”的完整路径“pathtopgmName” pgmArgs”
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
    //如果未指定 PROCESS_INFORMATION *pi，则只需将命令行返回给调用方即可
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