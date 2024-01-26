#include "QueueServer.h"

QueueServer::QueueServer(PipeServer* pipeServer)
{
    m_heap = HeapCreate(0, 0, 0);
    if (!m_heap)
        m_heap = GetProcessHeap();

    InitializeCriticalSectionAndSpinCount(&m_lock, 1000);
    List_Init(&m_queues);

    m_RequestId = 0x00000001;

    pipeServer->Register(MSGID_QUEUE, this, Handler);
}

MSG_HEADER* QueueServer::Handler(void* _this, MSG_HEADER* msg)
{
    return nullptr;
}
