#pragma once
#include "PipeServer.h"

class PStoreServer
{
public:
	PStoreServer(PipeServer* pipeServer);
protected:

	static MSG_HEADER* Handler(void* _this, MSG_HEADER* msg);
	static DWORD connectToPStore(void* __this);
protected:

	void* m_pStore;
};

