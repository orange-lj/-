#include "TerminalServer.h"

TerminalServer::TerminalServer(PipeServer* pipeServer)
{
    m_WinStaQueryInformation = NULL;
    m_WinStationIsSessionRemoteable = NULL;
    m_WinStationNameFromLogonId = NULL;
    m_WinStationGetConnectionProperty = NULL;
    m_WinStationFreePropertyValue = NULL;

    HMODULE _winsta = LoadLibrary(L"winsta.dll");
    if (_winsta) 
    {
        m_WinStaQueryInformation =
            GetProcAddress(_winsta, "WinStationQueryInformationW");
        m_WinStationIsSessionRemoteable =
            GetProcAddress(_winsta, "WinStationIsSessionRemoteable");
        m_WinStationNameFromLogonId =
            GetProcAddress(_winsta, "WinStationNameFromLogonIdW");
        m_WinStationGetConnectionProperty =
            GetProcAddress(_winsta, "WinStationGetConnectionProperty");
        m_WinStationFreePropertyValue =
            GetProcAddress(_winsta, "WinStationFreePropertyValue");
        m_WinStationDisconnect =
            GetProcAddress(_winsta, "WinStationDisconnect");
    }
    pipeServer->Register(MSGID_TERMINAL, this, Handler);
}

MSG_HEADER* TerminalServer::Handler(void* _this, MSG_HEADER* msg)
{
    return nullptr;
}
