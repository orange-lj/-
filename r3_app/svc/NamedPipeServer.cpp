#include "NamedPipeServer.h"
#include"misc.h"

typedef struct _PROXY_PIPE 
{
	HANDLE hPipe;
	HANDLE hEvent;
} PROXY_PIPE;


NamedPipeServer::NamedPipeServer(PipeServer* pipeServer)
{
	m_pNtAlpcConnectPort =
		GetProcAddress(_Ntdll, "NtAlpcConnectPort");
	m_pNtAlpcSendWaitReceivePort =
		GetProcAddress(_Ntdll, "NtAlpcSendWaitReceivePort");
	m_ProxyHandle = new ProxyHandle(NULL, sizeof(PROXY_PIPE),
		CloseCallback, NULL);
	pipeServer->Register(MSGID_NAMED_PIPE, this, Handler);
}
