#include"dll.h"
#include <psapi.h>

typedef BOOL(WINAPI* P_GetModuleInformation)(
    _In_ HANDLE hProcess,
    _In_ HMODULE hModule,
    _Out_ LPMODULEINFO lpmodinfo,
    _In_ DWORD cb);

P_GetModuleInformation __sys_GetModuleInformation = NULL;