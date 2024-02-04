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
        NtRegisterThreadTerminatePort(data->PortHandle);
        //计算尺寸和偏移
        data->SizeofPortMsg = sizeof(PORT_MESSAGE);
        data->MaxDataLen -= data->SizeofPortMsg;
    }
    return STATUS_SUCCESS;
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
    //在端口上传输请求消息。LPC端口是为短消息设计的，所以我们必须将消息分成块。
    curr_sequence = (UCHAR)InterlockedIncrement(&last_sequence);

    buf = (UCHAR*)req;
    buf_len = req->length;

    while (buf_len) 
    {
        msg = (PORT_MESSAGE*)spaceReq;

        memzero(msg, data->SizeofPortMsg);
        msg_data = (UCHAR*)msg + data->SizeofPortMsg;

        if (buf_len > data->MaxDataLen)
            send_len = data->MaxDataLen;
        else
            send_len = buf_len;

        msg->u1.s1.DataLength = (USHORT)send_len;
        msg->u1.s1.TotalLength = (USHORT)(data->SizeofPortMsg + send_len);

        memcpy(msg_data, buf, send_len);
        if (buf == (UCHAR*)req) 
        {
            //服务消息必须短于0x00FFFFFF字节（如core/svc/PipeServer.h中所定义），
            //因此我们可以使用MSG_HEADER.length的高字节（第一个块中的偏移量3）来存储验证序列号
            msg_data[3] = curr_sequence;
        }
        buf += send_len;
        buf_len -= send_len;
        //在LPC端口上发送块并等待确认。当服务在其端部收集传入的块时，它使用零长度的块进行回复。当发送最后一个区块时，
        //我们应该得到一个非零回复，其中包含来自服务的响应消息的第一个区块
        status = NtRequestWaitReplyPort(data->PortHandle,
            (PORT_MESSAGE*)spaceReq, (PORT_MESSAGE*)spaceRpl);
        if (!NT_SUCCESS(status))
            break;
        msg = (PORT_MESSAGE*)spaceRpl;
        if (buf_len && msg->u1.s1.DataLength) {
            //SbieApi_Log(2203, L"early reply");
            return NULL;
        }
    }
    if (!NT_SUCCESS(status)) {

        //NtClose(data->PortHandle);
        //data->PortHandle = NULL;
        //
        //SbieApi_Log(2203, L"request %08X", status);
        //return NULL;
    }
    //检查响应消息的前几块，它应该具有匹配的序列号和有效长度
    msg = (PORT_MESSAGE*)spaceRpl;

    msg_data = ((UCHAR*)msg + data->SizeofPortMsg);
    if (msg->u1.s1.DataLength >= sizeof(MSG_HEADER)) {

        if (msg_data[3] != curr_sequence) {
            //SbieApi_Log(2203, L"mismatched reply");
            return NULL;
        }

        msg_data[3] = 0;
        buf_len = ((MSG_HEADER*)msg_data)->length;

    }
    else
        buf_len = 0;

    if (buf_len == 0) {
        //SbieApi_Log(2203, L"null reply (msg %08X len %d)",req->msgid, req->length);
        return NULL;
    }

    //收集响应消息的块。我们必须在端口上不断发送短的伪LPC块，以便接收下一个响应块
    rpl = Dll_AllocTemp(buf_len + 8);
    buf = (UCHAR*)rpl;
    while (1) 
    {
        ULONG buf_len_plus_msg = (ULONG)(ULONG_PTR)(buf - (UCHAR*)rpl)
            + msg->u1.s1.DataLength;
        if (buf_len_plus_msg > buf_len)
            status = STATUS_PORT_MESSAGE_TOO_LONG;
        else {

            msg_data = ((UCHAR*)msg + data->SizeofPortMsg);
            memcpy(buf, msg_data, msg->u1.s1.DataLength);

            buf += msg->u1.s1.DataLength;
            if ((ULONG_PTR)(buf - (UCHAR*)rpl) >= buf_len)
                break;

            msg = (PORT_MESSAGE*)spaceReq;

            memzero(msg, data->SizeofPortMsg);
            msg->u1.s1.TotalLength = (USHORT)data->SizeofPortMsg;

            status = NtRequestWaitReplyPort(data->PortHandle,
                (PORT_MESSAGE*)spaceReq, (PORT_MESSAGE*)spaceRpl);

            msg = (PORT_MESSAGE*)spaceRpl;
        }

        if (!NT_SUCCESS(status)) {

            Dll_Free(rpl);

            NtClose(data->PortHandle);
            data->PortHandle = NULL;

            //SbieApi_Log(2203, L"reply %08X", status);
            return NULL;
        }
    }
    memzero(buf, 8);
    return rpl;
}

const WCHAR* SbieDll_PortName(void) 
{
    static const WCHAR* _name = L"\\RPC Control\\" SBIESVC L"Port";
    return _name;
}


void SbieDll_FreeMem(void* data)
{
    if (data)
        Dll_Free(data);
}