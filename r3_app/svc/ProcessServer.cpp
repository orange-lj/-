#include "ProcessServer.h"

ProcessServer::ProcessServer(PipeServer* pipeServer)
{
	pipeServer->Register(MSGID_PROCESS, this, Handler);
}

MSG_HEADER* ProcessServer::Handler(void* _this, MSG_HEADER* msg)
{
	return nullptr;
}
