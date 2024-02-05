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

SBIEAPI_EXPORT
LONG SbieApi_QueryProcess(
	HANDLE ProcessId,
	WCHAR* out_box_name_wchar34,        // WCHAR [34]
	WCHAR* out_image_name_wchar96,      // WCHAR [96]
	WCHAR* out_sid_wchar96,             // WCHAR [96]
	ULONG* out_session_id);             // ULONG

SBIEAPI_EXPORT
LONG SbieApi_QueryProcessEx2(
	HANDLE ProcessId,
	ULONG image_name_len_in_wchars,
	WCHAR* out_box_name_wchar34,        // WCHAR [34]
	WCHAR* out_image_name_wcharXXX,     // WCHAR [?]
	WCHAR* out_sid_wchar96,             // WCHAR [96]
	ULONG* out_session_id,              // ULONG
	ULONG64* out_create_time);          // ULONG64

SBIEAPI_EXPORT
LONG SbieApi_SessionLeader(
	HANDLE TokenHandle,
	HANDLE* ProcessId);


SBIEAPI_EXPORT
LONG SbieApi_EnumBoxesEx(
	LONG index,                     // initialize to -1
	WCHAR* box_name,                // WCHAR [34]
	BOOLEAN ignore_hidden);


SBIEAPI_EXPORT
LONG SbieApi_IsBoxEnabled(
	const WCHAR* box_name);


SBIEAPI_EXPORT
LONG SbieApi_EnumProcessEx(
	const WCHAR* box_name,          // WCHAR [34]
	BOOLEAN all_sessions,
	ULONG which_session,            // -1 for current session
	ULONG* boxed_pids,             // ULONG [512]
	ULONG* boxed_count);

#define SbieApi_EnumProcess(box_name,boxed_pids) \
    SbieApi_EnumProcessEx(box_name,FALSE,-1,boxed_pids, NULL)

#ifdef __cplusplus
}
#endif
