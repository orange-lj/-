#pragma once
#include"PipeServer.h"



class ProcessServer
{
public:
	ProcessServer(PipeServer* pipeServer);
protected:
	static MSG_HEADER* Handler(void* _this, MSG_HEADER* msg);
	MSG_HEADER* CheckInitCompleteHandler();
};

