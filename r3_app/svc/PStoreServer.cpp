#include "PStoreServer.h"

PStoreServer::PStoreServer(PipeServer* pipeServer)
{
	m_pStore = NULL;

	pipeServer->Register(MSGID_PSTORE, this, Handler);
	QueueUserWorkItem(connectToPStore, this, WT_EXECUTELONGFUNCTION);
}

MSG_HEADER* PStoreServer::Handler(void* _this, MSG_HEADER* msg)
{
	return nullptr;
}

DWORD PStoreServer::connectToPStore(void* __this)
{
	PStoreServer* _this = (PStoreServer*)__this;

	Sleep(1000);
	return 0;
}
