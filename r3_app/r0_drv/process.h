#pragma once
//#include"pool.h"
#include"main.h"


BOOLEAN Process_Init(void);

//低层系统接口（process_low.c）
BOOLEAN Process_Low_Init(void);

//返回进程句柄或进程ID号指定的进程的SID字符串和会话ID。
//若要释放返回的SidString，请使用RtlFreeUnicodeString
NTSTATUS Process_GetSidStringAndSessionId(
    HANDLE ProcessHandle, HANDLE ProcessId,
    UNICODE_STRING* SidString, ULONG* SessionId);

//返回从指定池分配的idProcess的ProcessName.exe。返回时，* out_buf指向UNICODE_STRING结构，该结构指向紧挨在该结构后面的以NULL终止的进程名称。
//*out_len包含*out_buf的长度。
//*out_ptr接收out_buf->Buffer。
//使用Mem_Free（*out_buf，*out_len）释放缓冲区。
//出错时，out_buf、out_len、out_ptr接收NULL
void Process_GetProcessName(
    POOL* pool, ULONG_PTR idProcess,
    void** out_buf, ULONG* out_len, WCHAR** out_ptr);