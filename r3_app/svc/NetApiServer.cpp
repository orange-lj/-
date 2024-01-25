#include "NetApiServer.h"

NetApiServer::NetApiServer(PipeServer* pipeServer)
{
	pipeServer->Register(MSGID_NETAPI, this, Handler);
}
