#pragma once
#include"../common/win32_ntddk.h"
#include<windows.h>

extern HMODULE _Ntdll;
extern HMODULE _Kernel32;
extern SYSTEM_INFO _SystemInfo;
#define NUMBER_OF_PROCESSORS (_SystemInfo.dwNumberOfProcessors)
#define MINIMUM_NUMBER_OF_THREADS 8
#define NUMBER_OF_THREADS   \
            ((NUMBER_OF_PROCESSORS * 2) <= MINIMUM_NUMBER_OF_THREADS \
                    ? MINIMUM_NUMBER_OF_THREADS : (NUMBER_OF_PROCESSORS * 2))