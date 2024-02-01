#pragma once
#include <ntstatus.h>
#define WIN32_NO_STATUS
typedef long NTSTATUS;

#include <windows.h>
#include "../common/win32_ntddk.h"

#include "sbiedll.h"
#include"../common/list.h"
#include"../common/defines.h"
#include"../r0_drv/api_flags.h"

#define TRUE_NAME_BUFFER        0
#define MISC_NAME_BUFFER        4
#define NAME_BUFFER_COUNT       12
#define NAME_BUFFER_DEPTH       16

extern const ULONG tzuk;
extern HINSTANCE Dll_Instance;
extern HMODULE Dll_Ntdll;
extern BOOLEAN Dll_SbieTrace;
extern const WCHAR* Dll_BoxName;
extern BOOLEAN Dll_AppContainerToken;
extern const WCHAR* DllName_advapi32;
extern ULONG Dll_Windows;
extern const WCHAR* Dll_HomeDosPath;

extern __declspec(dllexport) int __CRTDECL Sbie_snwprintf(wchar_t* _Buffer, size_t Count, const wchar_t* const _Format, ...);
extern __declspec(dllexport) int __CRTDECL Sbie_snprintf(char* _Buffer, size_t Count, const char* const _Format, ...);

//指向64位PEB_LDR_DATA的指针位于64位PEB的偏移量0x0018处
#define GET_ADDR_OF_PEB NtCurrentPeb()
#define GET_PEB_IMAGE_BUILD (*(USHORT *)(GET_ADDR_OF_PEB + 0x120))

typedef struct _THREAD_DATA 
{
    // name buffers：第一个索引是真实名称，第二个索引是拷贝名称
    WCHAR* name_buffer[NAME_BUFFER_COUNT][NAME_BUFFER_DEPTH];
    ULONG name_buffer_len[NAME_BUFFER_COUNT][NAME_BUFFER_DEPTH];
    int name_buffer_count[NAME_BUFFER_DEPTH];
    int name_buffer_depth;

    ////
    //// locks
    ////

    //BOOLEAN key_NtCreateKey_lock;

    //BOOLEAN file_NtCreateFile_lock;
    //BOOLEAN file_NtClose_lock;
    //BOOLEAN file_GetCurDir_lock;

    //BOOLEAN ipc_KnownDlls_lock;

    //BOOLEAN obj_NtQueryObject_lock;

    ////
    //// file module
    ////

    //ULONG file_dont_strip_write_access;

    ////
    //// misc modules
    ////

    //HANDLE  scm_last_own_token;

    ////
    //// proc module:  image path for a child process being started
    ////

    //ULONG           proc_create_process;
    //BOOLEAN         proc_create_process_capture_image;
    //BOOLEAN         proc_create_process_force_elevate;
    //BOOLEAN         proc_create_process_as_invoker;
    //BOOLEAN         proc_image_is_copy;
    //WCHAR* proc_image_path;
    //WCHAR* proc_command_line;

    //ULONG           sh32_shell_execute;

    ////
    //// gui module
    ////

    //ULONG_PTR       gui_himc;

    //HWND            gui_dde_client_hwnd;
    //HWND            gui_dde_proxy_hwnd;
    //WPARAM          gui_dde_post_wparam;
    //LPARAM          gui_dde_post_lparam;

    //ULONG           gui_create_window;

    //BOOLEAN         gui_hooks_installed;

    //BOOL            gui_should_suppress_msgbox;

    // sbieapi:  SbieSvc port handle

    HANDLE          PortHandle;
    ULONG           MaxDataLen;
    ULONG           SizeofPortMsg;
    //BOOLEAN         bOperaFileDlgThread;

    ////
    //// rpc module
    ////

    ULONG_PTR       rpc_caller;

} THREAD_DATA;


THREAD_DATA* Dll_GetTlsData(ULONG* pLastError);
WCHAR* Dll_GetTlsNameBuffer(THREAD_DATA* data, ULONG which, ULONG size);
void* Dll_Alloc(ULONG size);
void Dll_Free(void* ptr);
void Dll_PushTlsNameBuffer(THREAD_DATA* data);
void* Dll_AllocTemp(ULONG size);
void Dll_PopTlsNameBuffer(THREAD_DATA* data);