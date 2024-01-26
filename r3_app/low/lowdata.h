#pragma once


#define NATIVE_FUNCTION_NAMES   { "NtDelayExecution", "NtDeviceIoControlFile", "NtFlushInstructionCache", "NtProtectVirtualMemory" }
#define NATIVE_FUNCTION_COUNT   4
#define NATIVE_FUNCTION_SIZE    32

//SBIELOW_DATA����λ�ڵײ�ġ�zzzz�����֣��ò���
//ָ����벿���е�λ�á�
//Ӳ����ɾ��������ƫ�������
#define SBIELOW_INJECTION_SECTION ".text"
#define SBIELOW_SYMBOL_SECTION     "zzzz"

//UNISIDE_STRING����32λ��64λAPI
typedef struct _UNIVERSAL_STRING {
    USHORT  Length;
    USHORT  MaxLen;
    ULONG   Buf32;
    ULONG64 Buf64;
} UNIVERSAL_STRING;

//��ϵͳ�������������д�������ע��SbieDll�ĸ����ַ�����
//ϵͳ�������������еĵڶ���ULONGָ���ö������ݽṹ��ƫ����
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


//Detour Codeʹ�õ���ʱ���ݶԴ˽ṹ���κθ��Ķ�������entry_asm.asm��entry_arm.asm�е�����3���汾ͬ��
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