#include "EpMapperServer.h"

EpMapperServer::EpMapperServer(PipeServer* pipeServer)
{
	pipeServer->Register(MSGID_EPMAPPER, this, Handler);
}

MSG_HEADER* EpMapperServer::Handler(void* _this, MSG_HEADER* msg)
{
	return nullptr;
}
