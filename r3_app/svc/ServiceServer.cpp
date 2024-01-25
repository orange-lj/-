#include "ServiceServer.h"

ServiceServer::ServiceServer(PipeServer* pipeServer)
{
	pipeServer->Register(MSGID_SERVICE, this, Handler);
}

MSG_HEADER* ServiceServer::Handler(void* _this, MSG_HEADER* msg)
{
	return nullptr;
}
