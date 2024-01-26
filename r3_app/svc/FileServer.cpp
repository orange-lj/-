#include "FileServer.h"
#include "../common/win32_ntddk.h"
#define MyAlloc_WCHAR(n) (WCHAR *)MyAlloc((n) * sizeof(WCHAR))

FileServer::FileServer(PipeServer* pipeServer)
{
    m_heap = HeapCreate(0, 0, 0);
    if (!m_heap)
        m_heap = GetProcessHeap();

    m_windows = NULL;
    m_winsxs = NULL;
    m_PublicSd = NULL;

    WCHAR priv_space[64];
    TOKEN_PRIVILEGES* privs = (TOKEN_PRIVILEGES*)priv_space;

    BOOL b = LookupPrivilegeValue(
        L"", SE_RESTORE_NAME, &privs->Privileges[0].Luid);
    if (b) {

        privs->Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
        privs->PrivilegeCount = 1;

        HANDLE hToken;
        b = OpenProcessToken(
            GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES, &hToken);
        if (b) {

            b = AdjustTokenPrivileges(hToken, FALSE, privs, 0, NULL, NULL);
            CloseHandle(hToken);
        }
    }

    if (!b) 
    {
        //LogEvent(MSG_9234, 0x9208, GetLastError());
    }

    //
    //将x:\Windows\WinSxS转换为沙盒格式\drive\x\Windows\WinSxS\。
    //

    m_windows = MyAlloc_WCHAR(MAX_PATH + 8);

    if (!m_windows) {
        //LogEvent(MSG_9234, 0x9209, STATUS_INSUFFICIENT_RESOURCES);
        return;
    }

    GetSystemWindowsDirectory(m_windows, MAX_PATH);
    if (!m_windows[0])
        wcscpy(m_windows, L"C:\\WINDOWS");
    else if (m_windows[wcslen(m_windows) - 1] == L'\\')
        m_windows[wcslen(m_windows) - 1] = L'\0';

    m_winsxs = MyAlloc_WCHAR(wcslen(m_windows) + 64);
    if (!m_winsxs) {
        //LogEvent(MSG_9234, 0x9210, STATUS_INSUFFICIENT_RESOURCES);
        return;
    }
    wsprintf(m_winsxs,
        L"\\drive\\%c%s\\winsxs\\", m_windows[0], &m_windows[2]);

    //
    //准备用于在WinSxS中创建文件的安全描述符
    //

    m_PublicSd = MyAlloc(64);
    if (!m_PublicSd) {
        //LogEvent(MSG_9234, 0x9211, STATUS_INSUFFICIENT_RESOURCES);
        return;
    }
    RtlCreateSecurityDescriptor(m_PublicSd, SECURITY_DESCRIPTOR_REVISION);
    RtlSetDaclSecurityDescriptor(m_PublicSd, TRUE, NULL, FALSE);

    //
    //作为管道服务器的目标进行订阅
    //

    pipeServer->Register(MSGID_FILE, this, Handler);
}

MSG_HEADER* FileServer::Handler(void* _this, MSG_HEADER* msg)
{
    return nullptr;
}

void* FileServer::MyAlloc(ULONG len)
{
    ULONG len2 = len + sizeof(ULONG_PTR) * 2;
    char* buf = (char*)HeapAlloc(m_heap, HEAP_ZERO_MEMORY, len2);
    if (buf) {
        *(ULONG_PTR*)buf = len;
        buf += sizeof(ULONG_PTR);
        *(ULONG_PTR*)(buf + len) = tzuk;
    }
    return buf;
}
