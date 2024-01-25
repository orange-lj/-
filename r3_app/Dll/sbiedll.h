#pragma once

#include "sbieapi.h"

#ifdef __cplusplus
extern "C" {
#endif


#ifndef SBIEDLL_EXPORT
#define SBIEDLL_EXPORT  __declspec(dllexport)
#endif
SBIEDLL_EXPORT  const WCHAR* SbieDll_PortName(void);
SBIEDLL_EXPORT  const WCHAR* SbieDll_GetDrivePath(ULONG DriveIndex);
SBIEDLL_EXPORT  struct _MSG_HEADER* SbieDll_CallServer(
    struct _MSG_HEADER* req);
SBIEDLL_EXPORT  BOOLEAN SbieDll_StartSbieSvc(BOOLEAN retry);
SBIEDLL_EXPORT  ULONG SbieDll_InjectLow_InitHelper();

#ifdef __cplusplus
}
#endif