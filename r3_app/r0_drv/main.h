#pragma once
#include"my_winnt.h"
#include"../common/pool.h"
#include"../common/list.h"
#include<ntstrsafe.h>
#include<ntimage.h>
#include"../common/defines.h"
#include<ntifs.h>
#include<windef.h>

#define KERNEL_MODE

extern const ULONG tzuk;
extern ULONG Driver_OsVersion;
extern POOL* Driver_Pool;
extern WCHAR* Driver_HomePathDos;
extern WCHAR* Driver_RegistryPath;
extern DRIVER_OBJECT* Driver_Object;
extern UNICODE_STRING Driver_Altitude;
extern ULONG Driver_OsBuild;

typedef struct _PROCESS             PROCESS;


#define DRIVER_WINDOWS_10       8
#define DRIVER_WINDOWS_VISTA    4
#define DRIVER_WINDOWS_7        5
#define DRIVER_WINDOWS_8        6

#define DEVICE_NAME L"\\Device\\devhyDrv"  
#define LINK_NAME   L"\\DosDevices\\hyDrv"

void* Driver_FindMissingService(const char* ProcName, int prmcnt);
