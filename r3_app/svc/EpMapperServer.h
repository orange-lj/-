#pragma once
#include "PipeServer.h"

class EpMapperServer
{
public:

    EpMapperServer(PipeServer* pipeServer);

protected:

    static MSG_HEADER* Handler(void* _this, MSG_HEADER* msg);
};

