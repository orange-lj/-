#include "NetApiServer.h"

NetApiServer::NetApiServer(PipeServer* pipeServer)
{
	pipeServer->Register(MSGID_NETAPI, this, Handler);
}

MSG_HEADER* NetApiServer::Handler(void* _this, MSG_HEADER* msg)
{
	return nullptr;
}
