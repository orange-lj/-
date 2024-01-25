#pragma once


#define NATIVE_FUNCTION_NAMES   { "NtDelayExecution", "NtDeviceIoControlFile", "NtFlushInstructionCache", "NtProtectVirtualMemory" }
#define NATIVE_FUNCTION_COUNT   4
#define NATIVE_FUNCTION_SIZE    32

//SBIELOW_DATA符号位于底层的“zzzz”部分，该部分
//指向代码部分中的位置。
//硬编码删除了数据偏移相关性
#define SBIELOW_INJECTION_SECTION ".text"
#define SBIELOW_SYMBOL_SECTION     "zzzz"
