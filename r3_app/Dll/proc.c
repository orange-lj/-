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