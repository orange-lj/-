#pragma once
#include "PipeServer.h"
class QueueServer
{
public:

	QueueServer(PipeServer* pipeServer);
protected:

	static MSG_HEADER* Handler(void* _this, MSG_HEADER* msg);
protected:

	HANDLE m_heap;
	CRITICAL_SECTION m_lock;
	LIST m_queues;

	volatile LONG m_RequestId;

};

