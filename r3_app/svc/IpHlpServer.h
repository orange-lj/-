#pragma once
#include "PipeServer.h"
#include "ProxyHandle.h"

class IpHlpServer
{
public:
	IpHlpServer(PipeServer* pipeServer);
protected:
	static MSG_HEADER* Handler(void* _this, MSG_HEADER* msg);
	static void CloseCallback(void* context, void* data);
protected:
	ProxyHandle* m_ProxyHandle;
	void* m_IcmpCreateFile;
	void* m_Icmp6CreateFile;
	void* m_IcmpCloseHandle;

	void* m_IcmpSendEcho2;
	void* m_IcmpSendEcho2Ex;
	void* m_Icmp6SendEcho2;
};

