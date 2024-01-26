#pragma once
//#include"pool.h"
#include"main.h"
typedef NTSTATUS(*P_Api_Function)(PROCESS*, ULONG64* parms);

void Api_SetFunction(ULONG func_code, P_Api_Function func_ptr);

BOOLEAN Api_Init(void);

static BOOLEAN Api_FastIo_DEVICE_CONTROL(
    FILE_OBJECT* FileObject, BOOLEAN Wait,
    void* InputBuffer, ULONG InputBufferLength,
    void* OutputBuffer, ULONG OutputBufferLength,
    ULONG IoControlCode, IO_STATUS_BLOCK* IoStatus,
    DEVICE_OBJECT* DeviceObject);

extern volatile HANDLE Api_ServiceProcessId;