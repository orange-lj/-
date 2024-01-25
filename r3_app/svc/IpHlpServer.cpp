#include "IpHlpServer.h"

typedef struct _PROXY_ICMP_HANDLE {

	HANDLE handle;
	int ipver;

} PROXY_ICMP_HANDLE;

IpHlpServer::IpHlpServer(PipeServer* pipeServer)
{
	//初始化代理句柄管理器
	m_ProxyHandle = new ProxyHandle(NULL, sizeof(PROXY_ICMP_HANDLE),
		CloseCallback, this);
	//获取API入口点
    m_IcmpCreateFile = NULL;
    m_Icmp6CreateFile = NULL;
    m_IcmpCloseHandle = NULL;

    HMODULE _iphlpapi = LoadLibrary(L"iphlpapi.dll");
    if (_iphlpapi) {

        m_IcmpCreateFile = GetProcAddress(_iphlpapi, "IcmpCreateFile");
        m_Icmp6CreateFile = GetProcAddress(_iphlpapi, "Icmp6CreateFile");
        m_IcmpCloseHandle = GetProcAddress(_iphlpapi, "IcmpCloseHandle");

        m_IcmpSendEcho2 = GetProcAddress(_iphlpapi, "IcmpSendEcho2");
        m_IcmpSendEcho2Ex = GetProcAddress(_iphlpapi, "IcmpSendEcho2Ex");
        m_Icmp6SendEcho2 = GetProcAddress(_iphlpapi, "Icmp6SendEcho2");
    }
    //安装管道服务器目标
    pipeServer->Register(MSGID_IPHLP, this, Handler);
}

void IpHlpServer::CloseCallback(void* context, void* data)
{

}
