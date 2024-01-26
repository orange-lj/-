#pragma once
//#include"pool.h"
#include"main.h"


BOOLEAN Token_Init(void);

NTSTATUS Token_Api_Filter(PROCESS* proc, ULONG64* parms);

NTSTATUS Token_QuerySidString(void* TokenObject, UNICODE_STRING* SidString);
