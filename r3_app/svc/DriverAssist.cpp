#include <windows.h>
#include <sddl.h>
#include <stdio.h>
#include<iostream>

#include "DriverAssist.h"
#include"../Dll/sbiedll.h"
#include"../r0_drv/api_defs.h"
#include"misc.h"
#include"../r0_drv/msgs.h"
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

bool DriverAssist::IsDriverReady()
{
    if (m_instance && m_instance->m_DriverReady)
        return true;
    else
        return false;
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
    if (rc == 0) 
    {
        ok = true;
        goto driver_started;
    }
    //否则，尝试启动它
    UNICODE_STRING uni;
    RtlInitUnicodeString(&uni,
        L"\\Registry\\Machine\\System\\CurrentControlSet"
        L"\\Services\\" SBIEDRV);
    rc = NtLoadDriver(&uni);
    if (rc == 0 || rc == STATUS_IMAGE_ALREADY_LOADED) 
    {
        ok = true;
        goto driver_started;
    }
    if (rc != STATUS_PRIVILEGE_NOT_HELD || rc == STATUS_ACCESS_DENIED) 
    {
        //LogEvent(MSG_9234, 0x9153, rc);
        goto driver_started;
    }
    //我们必须启用加载驱动程序的权限
    WCHAR priv_space[64];
    TOKEN_PRIVILEGES* privs = (TOKEN_PRIVILEGES*)priv_space;
    HANDLE hToken;
    BOOL b = LookupPrivilegeValue(
        L"", SE_LOAD_DRIVER_NAME, &privs->Privileges[0].Luid);
    if (b) 
    {
        privs->Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
        privs->PrivilegeCount = 1;
        b = OpenProcessToken(
            GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES, &hToken);
        if (b) 
        {
            b = AdjustTokenPrivileges(hToken, FALSE, privs, 0, NULL, NULL);
            CloseHandle(hToken);
        }
    }
    rc = NtLoadDriver(&uni);
    if (rc == 0 || rc == STATUS_IMAGE_ALREADY_LOADED)
        ok = true;
    else 
    {
        //LogEvent(MSG_9234, 0x9153, rc);
    }
    //驱动程序已经启动（或已经启动），在我们继续初始化之前，请检查版本号
driver_started:
    if (ok) 
    {
        ULONG drv_abi_ver = 0;

        for (ULONG retries = 0; retries < 20; ++retries) 
        {
            rc = SbieApi_GetVersionEx(NULL, &drv_abi_ver);
            if (rc == 0)
                break;
            Sleep(500);
        }
        if (drv_abi_ver != MY_ABI_VERSION) {
            //LogEvent(MSG_9234, 0x9154, 0);
            ok = false;
        }
    }
    //版本号匹配
    if (ok) 
    {
        const char BlockList0[] =
            "2687F3F7D9DBF317A05251E2ED3C3D0A" "\r\n"
            "6AF28A3722ADC9FDB0F4B298508DD9E6" "\r\n"
            "54F2DF068889C1789AE9D7E948618EB6" "\r\n"
            "665FBAD025408A5B9355230EBD3381DC" "\r\n"

            "63F49D96BDBA28F8428B4A5008D1A587" "\r\n"

            "622D831B13B70B7CFFEC12E3778BF740" "\r\n"
            "2FDEB3584ED4DA007C2A1D49CFFF1062" "\r\n"
            "413616148FA9D3793B0E9BA4A396D3EE" "\r\n"
            "CC4DCCD36A13097B4478ADEB0FEF00CD" "\r\n"
            "32DE2C3B8E8859B6ECB6FF98BDF8DB15";

        const unsigned char BlockListSig0[64] = {
            0x22, 0x45, 0x90, 0x69, 0x5f, 0xb1, 0x3c, 0x7f,
            0x8b, 0xbf, 0x9e, 0xd0, 0x75, 0x01, 0xaf, 0x2a,
            0x92, 0x70, 0x5c, 0x1a, 0xe8, 0xc1, 0x8e, 0x5f,
            0xcf, 0x36, 0x0f, 0xb3, 0xbd, 0x8d, 0x60, 0x02,
            0x37, 0x1a, 0xa5, 0x3a, 0xea, 0x14, 0x32, 0xd1,
            0x2e, 0xce, 0x86, 0x02, 0xe3, 0x21, 0x33, 0x0f,
            0x7e, 0xa2, 0x93, 0xc9, 0xd8, 0x24, 0xfd, 0xed,
            0x01, 0x62, 0x77, 0x59, 0x36, 0x16, 0xc7, 0x49
        };
        std::string BlockList;
        BlockList.resize(0x1000, 0);
        ULONG BlockListLen = 0;
        SbieApi_Call(API_GET_SECURE_PARAM, 5, L"CertBlockList", (ULONG_PTR)BlockList.c_str(), BlockList.size(), (ULONG_PTR)&BlockListLen, 1);
        if (BlockListLen < sizeof(BlockList0) - 1)
        {
            //SbieApi_Call(API_SET_SECURE_PARAM, 3, L"CertBlockList", BlockList0, sizeof(BlockList0) - 1);
            //SbieApi_Call(API_SET_SECURE_PARAM, 3, L"CertBlockListSig", BlockListSig0, sizeof(BlockListSig0));
            //BlockList = BlockList0;
        }
    }
    //继续驱动程序/服务初始化
    if (ok) 
    {
        rc = SbieApi_Call(API_SET_SERVICE_PORT, 1, (ULONG_PTR)m_instance->m_PortHandle);
        if (rc != 0) {
            //LogEvent(MSG_9234, 0x9361, rc);
            ok = false;
        }
    }
    if (ok) 
    {
        SbieDll_InjectLow_InitSyscalls(TRUE);
        if (rc != 0) {
            //LogEvent(MSG_9234, 0x9362, rc);
            ok = false;
        }
    }
    if (ok) 
    {

        if (osvi.dwMajorVersion >= 6) {

            InitClipboard();
        }

        rc = SbieApi_Call(API_INIT_GUI, 0);

        if (rc != 0) {
            //LogEvent(MSG_9234, 0x9156, rc);
            ok = false;
        }
    }
    if (ok) 
    {
        //触发LogMessage的手动调用，以收集驱动程序启动时记录的所有消息
        m_instance->LogMessage();
    }
    return STATUS_SUCCESS;
}

void DriverAssist::InitClipboard()
{
    //在WindowsVista及更高版本上，我们需要弄清楚内部剪贴板项目的结构。我们把一些数据放在剪贴板上，
    //让文件core/drv/gi.c中的Gui_InitClipboard计算出内部结构
    HANDLE hGlobal1 = GlobalAlloc(GMEM_MOVEABLE, 8 * sizeof(WCHAR));
    HANDLE hGlobal2 = GlobalAlloc(GMEM_MOVEABLE, 8 * sizeof(WCHAR));

    if (hGlobal1 && hGlobal2) {

        WCHAR* pGlobal = (WCHAR*)GlobalLock(hGlobal1);
        *pGlobal = L'\0';
        GlobalUnlock(hGlobal1);
        pGlobal = (WCHAR*)GlobalLock(hGlobal2);
        *pGlobal = L'\0';
        GlobalUnlock(hGlobal2);

        for (int retry = 0; retry < 8 * (1000 / 250); ++retry) {

            if (OpenClipboard(NULL)) {

                EmptyClipboard();
                SetClipboardData(0x111111, hGlobal1);
                SetClipboardData(0x222222, hGlobal1);
                SetClipboardData(0x333333, hGlobal2);
                SetClipboardData(0x444444, hGlobal2);

                SbieApi_Call(API_GUI_CLIPBOARD, 1, (ULONG_PTR)-1);

                EmptyClipboard();
                CloseClipboard();

                break;

            }
            else
                Sleep(250);
        }
    }

    if (hGlobal1)
        GlobalFree(hGlobal1);

    if (hGlobal2)
        GlobalFree(hGlobal2);
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

DWORD DriverAssist::MsgWorkerThreadStub(void* MyMsg)
{
    if (!MyMsg) 
    {
        return -1;
    }

    MSG_DATA* MsgData = (MSG_DATA*)MyMsg;
    ((DriverAssist*)(MsgData->ClassContext))->MsgWorkerThread(&MsgData->msg[0]);
    //Memory allocated in parent thread
    VirtualFree(MyMsg, 0, MEM_RELEASE);

    return NO_ERROR;
}

void DriverAssist::MsgWorkerThread(void* MyMsg)
{
    PORT_MESSAGE* msg = (PORT_MESSAGE*)MyMsg;
    //调用方检查的空指针
    if (msg->u2.s2.Type != LPC_DATAGRAM) 
    {
        return;
    }
    ULONG data_len = msg->u1.s1.DataLength;
    if (data_len < sizeof(ULONG)) {
        return;
    }
    data_len -= sizeof(ULONG);

    ULONG* data_ptr = (ULONG*)((UCHAR*)msg + sizeof(PORT_MESSAGE));
    ULONG msgid = *data_ptr;
    ++data_ptr;
    if (msgid == SVC_LOOKUP_SID) {

        //LookupSid(data_ptr);

    }
    else if (msgid == SVC_INJECT_PROCESS) {

        //InjectLow(data_ptr);

    }
    else if (msgid == SVC_CANCEL_PROCESS) {

        //CancelProcess(data_ptr);

    }
    else if (msgid == SVC_MOUNTED_HIVE) {

        //HiveMounted(data_ptr);

    }
    else if (msgid == SVC_UNMOUNT_HIVE) {

        //UnmountHive(data_ptr);

    }
    else if (msgid == SVC_LOG_MESSAGE) {

        LogMessage();

    }
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
        if (!m_PortHandle) 
        {
            //服务正在关闭
        }
        MsgData->ClassContext = this;
        hThread = CreateThread(NULL, 0, MsgWorkerThreadStub, (void*)MsgData, 0, &threadId);
        if (hThread)
            CloseHandle(hThread);
        else
            VirtualFree(MsgData, 0, MEM_RELEASE);
    }
}

void DriverAssist::LogMessage()
{
    EnterCriticalSection(&m_LogMessage_CritSec);

    ULONG m_workItemLen = 4096;
    void* m_workItemBuf = NULL;
    while (1) {

        m_workItemBuf = HeapAlloc(GetProcessHeap(), 0, m_workItemLen);
        if (!m_workItemBuf)
            break;

        ULONG len = m_workItemLen;
        ULONG message_number = m_last_message_number;
        ULONG code = -1;
        ULONG pid = 0;
        ULONG status = SbieApi_GetMessage(&message_number, -1, &code, &pid, (wchar_t*)m_workItemBuf, len);

        if (status == STATUS_BUFFER_TOO_SMALL) {
            //HeapFree(GetProcessHeap(), 0, m_workItemBuf);
            //m_workItemBuf = NULL;
            //m_workItemLen += 4096;
            //continue;
        }

        if (status != 0)
            break; // error or no more entries
        m_last_message_number = message_number;

        //
        //跳过恶意消息
        //

        if (code == MSG_2199) //自动恢复通知
            continue;
        if (code == MSG_2198) //文件迁移进度通知
            continue;
        if (code == MSG_1399) //进程启动通知
            continue;

        //
        //添加到日志
        //

        LogMessage_Single(code, (wchar_t*)m_workItemBuf, pid);
    }

    if (m_workItemBuf)
        HeapFree(GetProcessHeap(), 0, m_workItemBuf);

    LeaveCriticalSection(&m_LogMessage_CritSec);
}

void DriverAssist::LogMessage_Single(ULONG code, wchar_t* data, ULONG pid)
{
    //检查是否启用了日志记录
    union {
        KEY_VALUE_PARTIAL_INFORMATION info;
        WCHAR space[MAX_PATH + 8];
    } u;
    if (!SbieDll_GetServiceRegistryValue(L"LogFile", &u.info, sizeof(u)))
        return;
}
