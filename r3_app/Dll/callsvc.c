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
        //����ߴ��ƫ��
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
    //���ӵ�����API�˿ڡ���ע�⣬����һЩ�ض���������룬���ǲ��ᷢ��������Ϣ
    if (!data->PortHandle) 
    {
        BOOLEAN Silent = (req->msgid == MSGID_SBIE_INI_GET_VERSION ||
            req->msgid == MSGID_SBIE_INI_GET_USER ||
            req->msgid == MSGID_PROCESS_CHECK_INIT_COMPLETE);
        status = SbieDll_ConnectPort();
        if (!NT_SUCCESS(status)) {
            if (!Dll_AppContainerToken && !Silent) //todo:�޸��÷��������appcontainer���̵�����
            {
                //SbieApi_Log(2203, L"connect %08X (msg_id 0x%04X)", status, req->msgid);
            } 
            return NULL;
        }
    }
    //�ڶ˿��ϴ���������Ϣ��LPC�˿���Ϊ����Ϣ��Ƶģ��������Ǳ��뽫��Ϣ�ֳɿ顣
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
            //������Ϣ�������0x00FFFFFF�ֽڣ���core/svc/PipeServer.h�������壩��
            //������ǿ���ʹ��MSG_HEADER.length�ĸ��ֽڣ���һ�����е�ƫ����3�����洢��֤���к�
            msg_data[3] = curr_sequence;
        }
        buf += send_len;
        buf_len -= send_len;
        //��LPC�˿��Ϸ��Ϳ鲢�ȴ�ȷ�ϡ�����������˲��ռ�����Ŀ�ʱ����ʹ���㳤�ȵĿ���лظ������������һ������ʱ��
        //����Ӧ�õõ�һ������ظ������а������Է������Ӧ��Ϣ�ĵ�һ������
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
    //�����Ӧ��Ϣ��ǰ���飬��Ӧ�þ���ƥ������кź���Ч����
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

    //�ռ���Ӧ��Ϣ�Ŀ顣���Ǳ����ڶ˿��ϲ��Ϸ��Ͷ̵�αLPC�飬�Ա������һ����Ӧ��
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