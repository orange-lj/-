#pragma once
//#include"pool.h"
#include"main.h"
typedef NTSTATUS(*P_Api_Function)(PROCESS*, ULONG64* parms);

void Api_SetFunction(ULONG func_code, P_Api_Function func_ptr);

BOOLEAN Api_Init(void);


extern volatile HANDLE Api_ServiceProcessId;