#include "PipeServer.h"
#include"misc.h"
#include"../common/defines.h"
#include"../Dll/sbiedll.h"
typedef struct tagTARGET
{
    LIST_ELEM list_elem;
    ULONG serverId;
    void* context;
    PipeServer::Handler handler;
} TARGET;

PipeServer* PipeServer::m_instance = NULL;

PipeServer* PipeServer::GetPipeServer()
{
    if (!m_instance) 
    {
        ULONG TlsIndex = TlsAlloc();
        if (TlsIndex == TLS_OUT_OF_INDEXES)
            return NULL;
        m_instance = new PipeServer();
        m_instance->m_TlsIndex = TlsIndex;
        if (!m_instance->Init()) 
        {
        
        }
    }
    return m_instance;
}

bool PipeServer::Start()
{
    NTSTATUS status;
    OBJECT_ATTRIBUTES objattrs;
    UNICODE_STRING PortName;
    ULONG i;
    ULONG idThread;
    //服务器端口应该具有NULL DACL，以便任何进程都可以连接
    ULONG sd_space[16];
    memzero(&sd_space, sizeof(sd_space));
    PSECURITY_DESCRIPTOR sd = (PSECURITY_DESCRIPTOR)&sd_space;
    InitializeSecurityDescriptor(sd, SECURITY_DESCRIPTOR_REVISION);
    SetSecurityDescriptorDacl(sd, TRUE, NULL, FALSE);
    //创建服务器端口
    RtlInitUnicodeString(&PortName, SbieDll_PortName());

    InitializeObjectAttributes(
        &objattrs, &PortName, OBJ_CASE_INSENSITIVE, NULL, sd);

    status = NtCreatePort(
        (HANDLE*)&m_hServerPort, &objattrs, 0, MAX_PORTMSG_LENGTH, NULL);

    if (!NT_SUCCESS(status)) 
    {
        //LogEvent(MSG_9234, 0x9252, status);
        return false;
    }
    //确保其他CPU上的线程将看到该端口
    InterlockedExchangePointer(&m_hServerPort, m_hServerPort);

    //
    //创建服务器线程
    //

    for (i = 0; i < NUMBER_OF_THREADS; ++i) {

        m_Threads[i] = CreateThread(
            NULL, 0, (LPTHREAD_START_ROUTINE)ThreadStub, this, 0, &idThread);
        if (!m_Threads[i]) {
            //LogEvent(MSG_9234, 0x9253, GetLastError());
            return false;
        }
    }

    return true;
}

void PipeServer::Register(ULONG serverId, void* context, Handler handler)
{
    TARGET* target = (TARGET*)Pool_Alloc(m_pool, sizeof(TARGET));
    if (target) 
    {
        target->serverId = serverId;
        target->context = context;
        target->handler = handler;
        List_Insert_After(&m_targets, NULL, target);
    }
}

PipeServer::PipeServer()
{
    InitializeCriticalSectionAndSpinCount(&m_lock, 1000);
    List_Init(&m_targets);
    map_init(&m_client_map, NULL);
    m_hServerPort = NULL;
    ULONG len_threads = (NUMBER_OF_THREADS) * sizeof(HANDLE);
    m_Threads = (HANDLE*)HeapAlloc(GetProcessHeap(), 0, len_threads);
    if (m_Threads)
        memzero(m_Threads, len_threads);
    else 
    {
        //LogEvent(MSG_9234, 0x9251, GetLastError());
    }
}

bool PipeServer::Init()
{
    m_instance->m_pool = Pool_Create();
    if (!m_instance->m_pool)
        return false;
    m_client_map.mem_pool = m_pool;
    map_resize(&m_client_map, 128);

    return false;
}

void PipeServer::ThreadStub(void* parm)
{
    ((PipeServer*)parm)->Thread();
}

void PipeServer::Thread(void)
{
    NTSTATUS status;
    UCHAR space[MAX_PORTMSG_LENGTH], spaceReply[MAX_PORTMSG_LENGTH];
    PORT_MESSAGE* msg = (PORT_MESSAGE*)space;
    HANDLE hReplyPort;
    PORT_MESSAGE* ReplyMsg;

    //最初我们没有回复发送。在每次发送回复后，我们也将恢复到无回复状态
    hReplyPort = m_hServerPort;
    ReplyMsg = NULL;

}
