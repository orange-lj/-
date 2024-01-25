#include "PipeServer.h"
#include"misc.h"
#include"../common/defines.h"

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
