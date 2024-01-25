#pragma once
#include "PipeServer.h"

class NetApiServer
{
public:

	NetApiServer(PipeServer* pipeServer);
protected:

	static MSG_HEADER* Handler(void* _this, MSG_HEADER* msg);
};

