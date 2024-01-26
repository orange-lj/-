#pragma once
#include "PipeServer.h"

class MountManager
{
public:
	MountManager(PipeServer* pipeServer);
protected:

	static MSG_HEADER* Handler(void* _this, MSG_HEADER* msg);
protected:
	CRITICAL_SECTION m_CritSec;
	static MountManager* m_instance;
};

