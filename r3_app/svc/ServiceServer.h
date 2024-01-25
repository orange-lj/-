#pragma once
#include "PipeServer.h"

class ServiceServer
{
public:
	ServiceServer(PipeServer* pipeServer);
protected:

	static MSG_HEADER* Handler(void* _this, MSG_HEADER* msg);
};

