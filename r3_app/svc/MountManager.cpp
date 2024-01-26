#include "MountManager.h"

typedef int (*P_SHCreateDirectoryExW)(HWND hwnd, LPCWSTR pszPath, const SECURITY_ATTRIBUTES* psa);

P_SHCreateDirectoryExW __sys_SHCreateDirectoryExW = NULL;
MountManager* MountManager::m_instance = NULL;
MountManager::MountManager(PipeServer* pipeServer)
{
    InitializeCriticalSection(&m_CritSec);

    HMODULE shell32 = LoadLibrary(L"shell32.dll");
    __sys_SHCreateDirectoryExW = (P_SHCreateDirectoryExW)GetProcAddress(shell32, "SHCreateDirectoryExW");

    //m_hCleanUpThread = INVALID_HANDLE_VALUE;

    pipeServer->Register(MSGID_IMBOX, this, Handler);

    // todo: find mounted disks

    m_instance = this;
}

MSG_HEADER* MountManager::Handler(void* _this, MSG_HEADER* msg)
{
    return nullptr;
}
