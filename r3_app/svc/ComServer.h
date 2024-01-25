#pragma once
#include "PipeServer.h"

class ComServer
{
public:

    ComServer(PipeServer* pipeServer);
protected:

    static MSG_HEADER* Handler(void* _this, MSG_HEADER* msg);
protected:

    CRITICAL_SECTION m_SlavesLock;
    LIST m_SlavesList;
    HANDLE m_SlaveReleasedEvent;
};

