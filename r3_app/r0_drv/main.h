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
#define POOL_TAG tzuk
extern const WCHAR* Driver_S_1_5_18;
extern ULONG Driver_OsVersion;
extern POOL* Driver_Pool;
extern WCHAR* Driver_Version;
extern WCHAR* Driver_HomePathDos;
extern WCHAR* Driver_RegistryPath;
extern DRIVER_OBJECT* Driver_Object;
extern UNICODE_STRING Driver_Altitude;
extern ULONG Driver_OsBuild;
extern WCHAR* Driver_HomePathNt;
extern ULONG  Driver_HomePathNt_Len;

typedef struct _BOX                 BOX;
typedef struct _CONF_EXPAND_ARGS    CONF_EXPAND_ARGS;
typedef struct _THREAD              THREAD;
typedef struct _PROCESS             PROCESS;


#define DRIVER_WINDOWS_10       8
#define DRIVER_WINDOWS_VISTA    4
#define DRIVER_WINDOWS_7        5
#define DRIVER_WINDOWS_8        6
#define HOOK_WIN32K
#define DEVICE_NAME L"\\Device\\devhyDrv"  
#define LINK_NAME   L"\\DosDevices\\hyDrv"

void* Driver_FindMissingService(const char* ProcName, int prmcnt);
