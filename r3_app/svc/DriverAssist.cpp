#include <windows.h>
#include <sddl.h>
#include <stdio.h>


#include "DriverAssist.h"
#include"../Dll/sbiedll.h"
#include"../r0_drv/api_defs.h"
#include"misc.h"
#pragma comment(lib,"Dll.lib")
#pragma comment(lib,"Advapi32.lib")



typedef struct _MSG_DATA
{
    void* ClassContext;
    UCHAR msg[MAX_PORTMSG_LENGTH];
} MSG_DATA;

DriverAssist* DriverAssist::m_instance = NULL;

bool DriverAssist::Initialize()
{
    m_instance = new DriverAssist();
    ULONG tid;
    HANDLE hThread;
    if (!m_instance) 
    {
        return false;
    }
    if (!m_instance->InjectLow_Init()) 
    {
        return false;
    }
    if (!m_instance->InitializePortAndThreads()) {
        return false;
    }
    hThread = CreateThread(NULL, 0,
        (LPTHREAD_START_ROUTINE)StartDriverAsync, m_instance, 0, &tid);
    CloseHandle(hThread);

    return true;
}

DriverAssist::DriverAssist()
{
    m_PortHandle = NULL;
    m_Threads = NULL;
    m_DriverReady = false;

    m_last_message_number = 0;
    InitializeCriticalSection(&m_LogMessage_CritSec);
    InitializeCriticalSection(&m_critSecHostInjectedSvcs);
}

bool DriverAssist::InitializePortAndThreads()
{
    NTSTATUS status;
    UNICODE_STRING objname;
    OBJECT_ATTRIBUTES objattrs;
    WCHAR PortName[64];
    PSECURITY_DESCRIPTOR sd;
    ULONG i, n;

    //使用有限的DACL创建安全描述符
    //所有者：系统，组：系统，dacl（allow；generic_all；system）
    if (!ConvertStringSecurityDescriptorToSecurityDescriptor(
        L"O:SYG:SYD:(A;;GA;;;SY)", SDDL_REVISION_1, &sd, NULL)) {
        //LogEvent(MSG_9234, 0x9244, GetLastError());
        return false;
    }
    //创建LPC端口，驱动程序将使用该端口向我们发送消息-该端口必须有名称，
    //否则SbieDrv中的LpcRequestPort将失败
    wsprintf(PortName, L"%s-internal-%d",
        SbieDll_PortName(), GetTickCount());
    RtlInitUnicodeString(&objname, PortName);
    InitializeObjectAttributes(
        &objattrs, &objname, OBJ_CASE_INSENSITIVE, NULL, sd);
    status = NtCreatePort(
        (HANDLE*)&m_PortHandle, &objattrs, 0, MAX_PORTMSG_LENGTH, NULL);
    if (!NT_SUCCESS(status)) {
        //LogEvent(MSG_9234, 0x9254, status);
        return false;
    }
    LocalFree(sd);
    //确保其他CPU上的线程将看到该端口
    InterlockedExchangePointer(&m_PortHandle, m_PortHandle);
    //创建工作线程
    n = (NUMBER_OF_THREADS) * sizeof(HANDLE);
    m_Threads = (HANDLE*)HeapAlloc(GetProcessHeap(), 0, n);
    if (!m_Threads) {
        //LogEvent(MSG_9234, 0x9251, GetLastError());
        return false;
    }
    memzero(m_Threads, n);
    for (i = 0; i < NUMBER_OF_THREADS; ++i) {

        m_Threads[i] = CreateThread(
            NULL, 0, (LPTHREAD_START_ROUTINE)ThreadStub, this, 0, &n);
        if (!m_Threads[i]) {
            //LogEvent(MSG_9234, 0x9253, GetLastError());
            return false;
        }
    }

    return true;
}

ULONG DriverAssist::StartDriverAsync(void* arg)
{
    //获取windows版本
    OSVERSIONINFO osvi;
    memzero(&osvi, sizeof(osvi));
    osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
    GetVersionEx(&osvi);
    //启动驱动程序，但前提是它尚未激活
    bool ok = false;
    LONG rc = SbieApi_GetVersionEx(NULL, NULL);
}

bool DriverAssist::InjectLow_Init()
{
    ULONG errlvl = SbieDll_InjectLow_InitHelper();
    if (errlvl != 0) 
    {
        //LogEvent(MSG_9234, 0x9241, errlvl);
        return false;
    }

    return true;
}


void DriverAssist::ThreadStub(void* parm)
{
    ((DriverAssist*)parm)->Thread();
}

void DriverAssist::Thread()
{
    NTSTATUS status;
    HANDLE hThread;
    DWORD threadId;
    MSG_DATA* MsgData;

    SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_HIGHEST);

    while (1) 
    {
        MsgData = (MSG_DATA*)VirtualAlloc(0, sizeof(MSG_DATA), MEM_COMMIT, PAGE_READWRITE);
        if (!MsgData) 
        {
            break;  // out of memory
        }
        status = NtReplyWaitReceivePort(m_PortHandle, NULL, NULL, (PORT_MESSAGE*)MsgData->msg);
    }
}
