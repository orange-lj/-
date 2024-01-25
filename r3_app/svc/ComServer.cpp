#include "ComServer.h"

ComServer::ComServer(PipeServer* pipeServer)
{
    m_SlaveReleasedEvent = CreateEvent(NULL, FALSE, FALSE, NULL);

    InitializeCriticalSection(&m_SlavesLock);
    List_Init(&m_SlavesList);

    pipeServer->Register(MSGID_COM, this, Handler);
}
