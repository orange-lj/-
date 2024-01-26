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