#pragma once
#include "PipeServer.h"
#include "ProxyHandle.h"

class NamedPipeServer
{
public:

	NamedPipeServer(PipeServer* pipeServer);
protected:

    static MSG_HEADER* Handler(void* _this, MSG_HEADER* msg);

    static void CloseCallback(void* context, void* data);
protected:

    ProxyHandle* m_ProxyHandle;
    void* m_pNtAlpcConnectPort;
    void* m_pNtAlpcSendWaitReceivePort;
};

