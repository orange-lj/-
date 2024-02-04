#pragma once
//#include"pool.h"
#include"main.h"

//在64位Windows中运行的32位应用程序出现问题。在32位应用程序中，通过API_ARGS结构之一传递的进程句柄是handle，它是32位的VOID*。64位驱动程序会将其与NtCurrentProcess（）进行比较，后者是64位-1。所以它永远不会匹配。这个宏解决了这个问题。
#define IS_ARG_CURRENT_PROCESS(h) ((ULONG)h == 0xffffffff)      // -1


typedef NTSTATUS(*P_Api_Function)(PROCESS*, ULONG64* parms);

void Api_SetFunction(ULONG func_code, P_Api_Function func_ptr);

BOOLEAN Api_Init(void);

//将'len'字节从'str'处的内核模式缓冲区复制到'uni'指定的用户模式缓冲区，并更新uni->Length。
//可能引发STATUS_BUFFER_TOO_SMALL或STATUS_ACCESS_VIOLATION
void Api_CopyStringToUser(
    UNICODE_STRING64* uni, WCHAR* str, size_t len);

extern volatile HANDLE Api_ServiceProcessId;

//从用户复制boxname参数
BOOLEAN Api_CopyBoxNameFromUser(
    WCHAR* boxname34, const WCHAR* user_boxname);