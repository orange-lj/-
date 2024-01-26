#pragma once


#define NATIVE_FUNCTION_NAMES   { "NtDelayExecution", "NtDeviceIoControlFile", "NtFlushInstructionCache", "NtProtectVirtualMemory" }
#define NATIVE_FUNCTION_COUNT   4
#define NATIVE_FUNCTION_SIZE    32

//SBIELOW_DATA符号位于底层的“zzzz”部分，该部分
//指向代码部分中的位置。
//硬编码删除了数据偏移相关性
#define SBIELOW_INJECTION_SECTION ".text"
#define SBIELOW_SYMBOL_SECTION     "zzzz"

//UNISIDE_STRING兼容32位和64位API
typedef struct _UNIVERSAL_STRING {
    USHORT  Length;
    USHORT  MaxLen;
    ULONG   Buf32;
    ULONG64 Buf64;
} UNIVERSAL_STRING;

//在系统调用数据区域中传递用于注入SbieDll的附加字符串。
//系统调用数据区域中的第二个ULONG指定该额外数据结构的偏移量
typedef struct _SBIELOW_EXTRA_DATA 
{
    ULONG LdrLoadDll_offset;
    ULONG LdrGetProcAddr_offset;
    ULONG NtProtectVirtualMemory_offset;
    ULONG NtRaiseHardError_offset;
    ULONG NtDeviceIoControlFile_offset;
    ULONG RtlFindActCtx_offset;
    ULONG KernelDll_offset;
    ULONG KernelDll_length;

    ULONG NativeSbieDll_offset;
    ULONG NativeSbieDll_length;
    ULONG Wow64SbieDll_offset;
    ULONG Wow64SbieDll_length;

    ULONG InjectData_offset;
} SBIELOW_EXTRA_DATA;


//Detour Code使用的临时数据对此结构的任何更改都必须与entry_asm.asm和entry_arm.asm中的所有3个版本同步
typedef struct _INJECT_DATA {

    ULONG64 sbielow_data;           // 0
    ULONG64 RtlFindActCtx;          // 8
    ULONG RtlFindActCtx_Protect;
    UCHAR RtlFindActCtx_Bytes[20];
    ULONG64 LdrLoadDll;
    ULONG64 LdrGetProcAddr;
    ULONG64 NtProtectVirtualMemory;
    ULONG64 NtRaiseHardError;
    ULONG64 NtDeviceIoControlFile;
    ULONG64 api_device_handle;
    UNIVERSAL_STRING KernelDll;
    UNIVERSAL_STRING SbieDll;

} INJECT_DATA;