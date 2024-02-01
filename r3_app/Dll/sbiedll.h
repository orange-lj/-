#pragma once

#include "sbieapi.h"

#ifdef __cplusplus
extern "C" {
#endif


#ifndef SBIEDLL_EXPORT
#define SBIEDLL_EXPORT  __declspec(dllexport)
#endif

SBIEDLL_EXPORT  void SbieDll_HookInit();
SBIEDLL_EXPORT  void* SbieDll_Hook(
        const char* SourceFuncName, void* SourceFunc, void* DetourFunc, HMODULE module);

#define SBIEDLL_HOOK(pfx,proc)                  \
    *(ULONG_PTR *)&__sys_##proc = (ULONG_PTR)   \
        SbieDll_Hook(#proc, proc, pfx##proc, module);   \
    if (! __sys_##proc) return FALSE;

SBIEDLL_EXPORT  const WCHAR* SbieDll_PortName(void);
SBIEDLL_EXPORT  const WCHAR* SbieDll_GetDrivePath(ULONG DriveIndex);
SBIEDLL_EXPORT  struct _MSG_HEADER* SbieDll_CallServer(struct _MSG_HEADER* req);
SBIEDLL_EXPORT  BOOLEAN SbieDll_StartSbieSvc(BOOLEAN retry);
SBIEDLL_EXPORT  ULONG SbieDll_InjectLow_InitHelper();
SBIEDLL_EXPORT  ULONG SbieDll_InjectLow_InitSyscalls(BOOLEAN drv_init);
SBIEDLL_EXPORT  BOOLEAN SbieDll_GetServiceRegistryValue(
    const WCHAR* name, void* kvpi, ULONG sizeof_kvpi);
SBIEDLL_EXPORT  BOOLEAN SbieDll_DisableCHPE(void);
SBIEDLL_EXPORT  WCHAR* SbieDll_FormatMessage0(ULONG code);
SBIEDLL_EXPORT  const WCHAR* SbieDll_GetStartError(void);
SBIEDLL_EXPORT  WCHAR* SbieDll_FormatMessage(ULONG code, const WCHAR** ins);
SBIEDLL_EXPORT  BOOLEAN SbieDll_RunFromHome(
    const WCHAR* pgmName, const WCHAR* pgmArgs,
    STARTUPINFOW* si, PROCESS_INFORMATION* pi);
SBIEDLL_EXPORT  ULONG SbieDll_GetLanguage(BOOLEAN* rtl);
SBIEDLL_EXPORT  ULONG SbieDll_FormatMessage_2(WCHAR** text_ptr, const WCHAR** ins);
SBIEDLL_EXPORT  void SbieDll_FreeMem(void* data);
#ifdef __cplusplus
}
#endif