#pragma once


#define NATIVE_FUNCTION_NAMES   { "NtDelayExecution", "NtDeviceIoControlFile", "NtFlushInstructionCache", "NtProtectVirtualMemory" }
#define NATIVE_FUNCTION_COUNT   4
#define NATIVE_FUNCTION_SIZE    32

//SBIELOW_DATA����λ�ڵײ�ġ�zzzz�����֣��ò���
//ָ����벿���е�λ�á�
//Ӳ����ɾ��������ƫ�������
#define SBIELOW_INJECTION_SECTION ".text"
#define SBIELOW_SYMBOL_SECTION     "zzzz"
