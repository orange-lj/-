#pragma once
#include "PipeServer.h"


class TerminalServer
{
public:

	TerminalServer(PipeServer* pipeServer);
protected:

	static MSG_HEADER* Handler(void* _this, MSG_HEADER* msg);

protected:

    void* m_WinStaQueryInformation;
    void* m_WinStationIsSessionRemoteable;
    void* m_WinStationNameFromLogonId;
    void* m_WinStationGetConnectionProperty;
    void* m_WinStationFreePropertyValue;
    void* m_WinStationDisconnect;
};

