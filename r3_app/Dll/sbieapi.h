#pragma once
#include"dll.h"

#ifdef __cplusplus
extern "C" {
#endif


#ifndef SBIEAPI_EXPORT
#define SBIEAPI_EXPORT __declspec(dllexport)
#endif

SBIEAPI_EXPORT
long SbieApi_Ioctl(ULONG64* parms);

SBIEAPI_EXPORT
LONG SbieApi_GetHomePath(
	WCHAR* NtPath, ULONG NtPathMaxLen,
	WCHAR* DosPath, ULONG DosPathMaxLen
);

SBIEAPI_EXPORT
LONG SbieApi_QueryConf(
	const WCHAR* section_name,      // WCHAR [66]
	const WCHAR* setting_name,      // WCHAR [66]
	ULONG setting_index,
	WCHAR* out_buffer,
	ULONG buffer_len);

#define SbieApi_QueryConfAsIs(bx, st, idx, buf, buflen) \
    SbieApi_QueryConf((bx), (st), ((idx) | CONF_GET_NO_EXPAND), buf, buflen)

SBIEAPI_EXPORT
BOOLEAN SbieApi_QueryConfBool(
	const WCHAR* section_name,      // WCHAR [66]
	const WCHAR* setting_name,      // WCHAR [66]
	BOOLEAN def);

LONG SbieApi_QuerySymbolicLink(
	WCHAR* NameBuf,
	ULONG NameLen);

SBIEAPI_EXPORT
LONG SbieApi_CheckInternetAccess(
	HANDLE ProcessId,
	const WCHAR* DeviceName32,
	BOOLEAN IssueMessage);

SBIEAPI_EXPORT
LONG SbieApi_MonitorPut(
	ULONG Type,
	const WCHAR* Name);

SBIEAPI_EXPORT
LONG SbieApi_MonitorPut2(
	ULONG Type,
	const WCHAR* Name,
	BOOLEAN bCheckObjectExists);

SBIEAPI_EXPORT
LONG SbieApi_MonitorPut2Ex(
	ULONG Type,
	ULONG NameLen,
	const WCHAR* Name,
	BOOLEAN bCheckObjectExists,
	BOOLEAN bIsMessage);

SBIEAPI_EXPORT LONG SbieApi_GetVersionEx(
	WCHAR* version_string,          // WCHAR [16]
	ULONG* abi_version);

SBIEAPI_EXPORT
LONG SbieApi_Call(ULONG api_code, LONG arg_num, ...);

SBIEAPI_EXPORT
ULONG SbieApi_GetMessage(
	ULONG* MessageNum,
	ULONG SessionId,
	ULONG* MessageId,
	ULONG* Pid,
	wchar_t* Buffer,
	ULONG Length);

#ifdef __cplusplus
}
#endif
