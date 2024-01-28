#include"dll.h"
//#include"sbiedll.h"
#include"../svc/sbieiniwire.h"
#include"../common/my_version.h"

NTSTATUS SbieDll_ConnectPort() 
{
    THREAD_DATA* data = Dll_GetTlsData(NULL);
    if (!data->PortHandle) 
    {
        NTSTATUS status;
        SECURITY_QUALITY_OF_SERVICE QoS;
        UNICODE_STRING PortName;
        QoS.Length = sizeof(SECURITY_QUALITY_OF_SERVICE);
        QoS.ImpersonationLevel = SecurityImpersonation;
        QoS.ContextTrackingMode = SECURITY_DYNAMIC_TRACKING;
        QoS.EffectiveOnly = TRUE;
        RtlInitUnicodeString(&PortName, SbieDll_PortName());
        status = NtConnectPort(
            &data->PortHandle, &PortName, &QoS,
            NULL, NULL, &data->MaxDataLen, NULL, NULL);
        if (!NT_SUCCESS(status))
            return status;
    }
}


MSG_HEADER* SbieDll_CallServer(MSG_HEADER* req) 
{
    static volatile ULONG last_sequence = 0;
    UCHAR curr_sequence;
    THREAD_DATA* data = Dll_GetTlsData(NULL);
    UCHAR spaceReq[MAX_PORTMSG_LENGTH], spaceRpl[MAX_PORTMSG_LENGTH];
    NTSTATUS status;
    PORT_MESSAGE* msg;
    UCHAR* buf, * msg_data;
    ULONG buf_len, send_len;
    MSG_HEADER* rpl;

    if (Dll_SbieTrace) 
    {
    
    }
    //连接到服务API端口。请注意，对于一些特定的请求代码，我们不会发出错误消息
    if (!data->PortHandle) 
    {
        BOOLEAN Silent = (req->msgid == MSGID_SBIE_INI_GET_VERSION ||
            req->msgid == MSGID_SBIE_INI_GET_USER ||
            req->msgid == MSGID_PROCESS_CHECK_INIT_COMPLETE);
        status = SbieDll_ConnectPort();
        if (!NT_SUCCESS(status)) {
            if (!Dll_AppContainerToken && !Silent) //todo:修复让服务可用于appcontainer进程的问题
            {
                //SbieApi_Log(2203, L"connect %08X (msg_id 0x%04X)", status, req->msgid);
            } 
            return NULL;
        }
    }
}

const WCHAR* SbieDll_PortName(void) 
{
    static const WCHAR* _name = L"\\RPC Control\\" SBIESVC L"Port";
    return _name;
}