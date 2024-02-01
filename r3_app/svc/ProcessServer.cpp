#include "ProcessServer.h"
#include"DriverAssist.h"
ProcessServer::ProcessServer(PipeServer* pipeServer)
{
	pipeServer->Register(MSGID_PROCESS, this, Handler);
}

MSG_HEADER* ProcessServer::Handler(void* _this, MSG_HEADER* msg)
{
	ProcessServer* pThis = (ProcessServer*)_this;

	if (msg->msgid == MSGID_PROCESS_CHECK_INIT_COMPLETE)
		return pThis->CheckInitCompleteHandler();
}

MSG_HEADER* ProcessServer::CheckInitCompleteHandler()
{
	ULONG status = STATUS_SUCCESS;
	if (!DriverAssist::IsDriverReady())
		status = STATUS_DEVICE_NOT_READY;
	return SHORT_REPLY(status);
}
