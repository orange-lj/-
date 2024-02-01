#include "PipeServer.h"
#include"misc.h"
#include"../common/defines.h"
#include"../Dll/sbiedll.h"

#define MAX_REQUEST_LENGTH      (2048 * 1024)
#define MSG_DATA_LEN            (MAX_PORTMSG_LENGTH - sizeof(PORT_MESSAGE))
typedef struct tagTARGET
{
    LIST_ELEM list_elem;
    ULONG serverId;
    void* context;
    PipeServer::Handler handler;
} TARGET;


typedef struct tagCLIENT_THREAD
{
    HANDLE idThread;
    BOOLEAN replying;
    volatile BOOLEAN in_use;
    UCHAR sequence;
    HANDLE hPort;
    MSG_HEADER* buf_hdr;
    UCHAR* buf_ptr;
} CLIENT_THREAD;

typedef struct tagCLIENT_PROCESS
{
    HANDLE idProcess;
    LARGE_INTEGER CreateTime;
    HASH_MAP thread_map;
} CLIENT_PROCESS;

typedef struct tagCLIENT_TLS_DATA
{
    HANDLE PortHandle;
    PORT_MESSAGE* PortMessage;
} CLIENT_TLS_DATA;

PipeServer* PipeServer::m_instance = NULL;

PipeServer* PipeServer::GetPipeServer()
{
    if (!m_instance) 
    {
        ULONG TlsIndex = TlsAlloc();
        if (TlsIndex == TLS_OUT_OF_INDEXES)
            return NULL;
        m_instance = new PipeServer();
        m_instance->m_TlsIndex = TlsIndex;
        if (!m_instance->Init()) 
        {
        
        }
    }
    return m_instance;
}

bool PipeServer::Start()
{
    NTSTATUS status;
    OBJECT_ATTRIBUTES objattrs;
    UNICODE_STRING PortName;
    ULONG i;
    ULONG idThread;
    //服务器端口应该具有NULL DACL，以便任何进程都可以连接
    ULONG sd_space[16];
    memzero(&sd_space, sizeof(sd_space));
    PSECURITY_DESCRIPTOR sd = (PSECURITY_DESCRIPTOR)&sd_space;
    InitializeSecurityDescriptor(sd, SECURITY_DESCRIPTOR_REVISION);
    SetSecurityDescriptorDacl(sd, TRUE, NULL, FALSE);
    //创建服务器端口
    RtlInitUnicodeString(&PortName, SbieDll_PortName());

    InitializeObjectAttributes(
        &objattrs, &PortName, OBJ_CASE_INSENSITIVE, NULL, sd);

    status = NtCreatePort(
        (HANDLE*)&m_hServerPort, &objattrs, 0, MAX_PORTMSG_LENGTH, NULL);

    if (!NT_SUCCESS(status)) 
    {
        //LogEvent(MSG_9234, 0x9252, status);
        return false;
    }
    //确保其他CPU上的线程将看到该端口
    InterlockedExchangePointer(&m_hServerPort, m_hServerPort);

    //
    //创建服务器线程
    //

    for (i = 0; i < NUMBER_OF_THREADS; ++i) {

        m_Threads[i] = CreateThread(
            NULL, 0, (LPTHREAD_START_ROUTINE)ThreadStub, this, 0, &idThread);
        if (!m_Threads[i]) {
            //LogEvent(MSG_9234, 0x9253, GetLastError());
            return false;
        }
    }

    return true;
}

void PipeServer::Register(ULONG serverId, void* context, Handler handler)
{
    TARGET* target = (TARGET*)Pool_Alloc(m_pool, sizeof(TARGET));
    if (target) 
    {
        target->serverId = serverId;
        target->context = context;
        target->handler = handler;
        List_Insert_After(&m_targets, NULL, target);
    }
}

MSG_HEADER* PipeServer::AllocShortMsg(ULONG status)
{
    MSG_HEADER* msg = AllocMsg(sizeof(MSG_HEADER));
    if (msg)
        msg->status = status;
    return msg;
}

MSG_HEADER* PipeServer::AllocMsg(ULONG length)
{
    UCHAR* buf = (UCHAR*)Pool_Alloc(m_pool, length + sizeof(ULONG) * 2);
    if (buf) {
        ((MSG_HEADER*)buf)->length = length;
        ((MSG_HEADER*)buf)->status = 0;
        *(ULONG*)(buf + length) = 0;
        *(ULONG*)(buf + length + sizeof(ULONG)) = tzuk;
    }
    return (MSG_HEADER*)buf;
}

void PipeServer::FreeMsg(MSG_HEADER* msg)
{
    UCHAR* buf = (UCHAR*)msg;
    if (*(ULONG*)(buf + msg->length) != 0 ||
        *(ULONG*)(buf + msg->length + sizeof(ULONG)) != tzuk) {
        //SbieApi_Log(2316, NULL);
        __debugbreak();
    }
    Pool_Free(msg, msg->length + sizeof(ULONG) * 2);
}

ULONG PipeServer::GetCallerProcessId()
{
    CLIENT_TLS_DATA* TlsData =
        (CLIENT_TLS_DATA*)TlsGetValue(m_instance->m_TlsIndex);
    return (ULONG)(ULONG_PTR)TlsData->PortMessage->ClientId.UniqueProcess;
}

ULONG PipeServer::GetCallerSessionId()
{
    ULONG SessionId;
    if (!ProcessIdToSessionId(GetCallerProcessId(), &SessionId))
        SessionId = 0;
    return SessionId;
}

PipeServer::PipeServer()
{
    InitializeCriticalSectionAndSpinCount(&m_lock, 1000);
    List_Init(&m_targets);
    map_init(&m_client_map, NULL);
    m_hServerPort = NULL;
    ULONG len_threads = (NUMBER_OF_THREADS) * sizeof(HANDLE);
    m_Threads = (HANDLE*)HeapAlloc(GetProcessHeap(), 0, len_threads);
    if (m_Threads)
        memzero(m_Threads, len_threads);
    else 
    {
        //LogEvent(MSG_9234, 0x9251, GetLastError());
    }
}

bool PipeServer::Init()
{
    m_instance->m_pool = Pool_Create();
    if (!m_instance->m_pool)
        return false;
    m_client_map.mem_pool = m_pool;
    map_resize(&m_client_map, 128);

    return false;
}

void PipeServer::ThreadStub(void* parm)
{
    ((PipeServer*)parm)->Thread();
}

void PipeServer::Thread(void)
{
    NTSTATUS status;
    UCHAR space[MAX_PORTMSG_LENGTH], spaceReply[MAX_PORTMSG_LENGTH];
    PORT_MESSAGE* msg = (PORT_MESSAGE*)space;
    HANDLE hReplyPort;
    PORT_MESSAGE* ReplyMsg;

    //最初我们没有回复发送。在每次发送回复后，我们也将恢复到无回复状态
    hReplyPort = m_hServerPort;
    ReplyMsg = NULL;

    while (1) 
    {
        //将ReplyMsg中的传出答复（如果有）发送到hReplyPort中的端口。然后等待传入消息。请注意，即使hReplyPort指示要发送消息的客户端端口，
        //服务器端NtReplyWaitReceivePort也会在发送消息后侦听关联的服务器端口。
        if (ReplyMsg) 
        {
            memcpy(spaceReply, ReplyMsg, ReplyMsg->u1.s1.TotalLength);
            ReplyMsg = (PORT_MESSAGE*)spaceReply;
        }
        status = NtReplyWaitReceivePort(hReplyPort, NULL, ReplyMsg, msg);
        if (!m_hServerPort) 
        {
            break;
        }
        if (ReplyMsg) 
        {
            hReplyPort = m_hServerPort;
            ReplyMsg = NULL;

            if (!NT_SUCCESS(status))
                continue;//忽略客户端端口上的错误
        }
        else if (!NT_SUCCESS(status)) 
        {
            if (status == STATUS_UNSUCCESSFUL) 
            {
                //可以被视为警告而不是错误
                continue;
            }
            break;//服务器端口出现错误时中止
        }
        if (msg->u2.s2.Type == LPC_CONNECTION_REQUEST) 
        {
            //PortConnect(msg);
        }
        else if (msg->u2.s2.Type == LPC_REQUEST) 
        {
            CLIENT_THREAD* client = (CLIENT_THREAD*)PortFindClient(msg);
            if (!client)
                continue;
            if (!client->replying)
                PortRequest(client->hPort, msg, client);
            msg->u2.ZeroInit = 0;
            if (client->replying)
                PortReply(msg, client);
            else {
                msg->u1.s1.DataLength = (USHORT)0;
                msg->u1.s1.TotalLength = sizeof(PORT_MESSAGE);
            }

            hReplyPort = client->hPort;
            ReplyMsg = msg;

            client->in_use = FALSE;
        }
        else if (msg->u2.s2.Type == LPC_PORT_CLOSED ||
            msg->u2.s2.Type == LPC_CLIENT_DIED)
        {
            //以后实现
        }
    }
}

void PipeServer::PortRequest(HANDLE PortHandle, PORT_MESSAGE* msg, void* voidClient)
{
    CLIENT_THREAD* client = (CLIENT_THREAD*)voidClient;
    ULONG buf_len;
    void* buf_ptr = NULL;

    if (!client->buf_hdr) {

        ULONG* msg_Data = (ULONG*)msg->Data;
        ULONG msgid = msg_Data[1];

        client->sequence = ((UCHAR*)msg_Data)[3];
        ((UCHAR*)msg_Data)[3] = 0;

        buf_len = msg_Data[0];

        if (msgid && buf_len &&
            buf_len < MAX_REQUEST_LENGTH &&
            buf_len >= sizeof(MSG_HEADER) &&
            buf_len >= msg->u1.s1.DataLength) {

            client->buf_hdr = AllocMsg(buf_len);
            client->buf_ptr = (UCHAR*)client->buf_hdr;
        }

        if (!client->buf_hdr) {
            client->sequence = 0;
            goto finish;
        }

        buf_len = 0;

    }
    else {

        buf_len = (ULONG)(client->buf_ptr - (UCHAR*)client->buf_hdr);

        if (buf_len + msg->u1.s1.DataLength > client->buf_hdr->length)
            goto finish;
    }

    memcpy(client->buf_ptr, msg->Data, msg->u1.s1.DataLength);
    client->buf_ptr += msg->u1.s1.DataLength;
    buf_len += msg->u1.s1.DataLength;

    if (buf_len < client->buf_hdr->length)
        return;

    buf_ptr = CallTarget(client->buf_hdr, PortHandle, msg);

finish:

    if (client->buf_hdr)
        FreeMsg(client->buf_hdr);

    client->buf_hdr = (MSG_HEADER*)buf_ptr;
    client->buf_ptr = (UCHAR*)buf_ptr;
    client->replying = TRUE;
}

void PipeServer::PortReply(PORT_MESSAGE* msg, void* voidClient)
{
    CLIENT_THREAD* client = (CLIENT_THREAD*)voidClient;
    ULONG buf_len;

    if (!client->buf_ptr) {
        msg->u1.s1.DataLength = (USHORT)0;
        msg->u1.s1.TotalLength = sizeof(PORT_MESSAGE);
        client->replying = FALSE;
        return;
    }

    buf_len = client->buf_hdr->length
        - (ULONG)(client->buf_ptr - (UCHAR*)client->buf_hdr);
    if (buf_len > MSG_DATA_LEN)
        buf_len = MSG_DATA_LEN;

    msg->u1.s1.DataLength = (USHORT)buf_len;
    msg->u1.s1.TotalLength = (USHORT)(sizeof(PORT_MESSAGE) + buf_len);
    memcpy(msg->Data, client->buf_ptr, buf_len);

    if (client->buf_ptr == (UCHAR*)client->buf_hdr)
        ((UCHAR*)msg->Data)[3] = client->sequence;

    client->buf_ptr += buf_len;

    buf_len = (ULONG)(client->buf_ptr - (UCHAR*)client->buf_hdr);
    if (buf_len >= client->buf_hdr->length) {
        FreeMsg(client->buf_hdr);
        client->buf_hdr = NULL;
        client->buf_ptr = NULL;
        client->replying = FALSE;
    }
}

void* PipeServer::PortFindClient(PORT_MESSAGE* msg)
{
    CLIENT_PROCESS* clientProcess;
    CLIENT_THREAD* clientThread;
    EnterCriticalSection(&m_lock);
    PortFindClientUnsafe(msg->ClientId, clientProcess, clientThread);
    if (clientThread)
        clientThread->in_use = TRUE;

    LeaveCriticalSection(&m_lock);

    return clientThread;
}

MSG_HEADER* PipeServer::CallTarget(MSG_HEADER* msg, HANDLE PortHandle, PORT_MESSAGE* PortMessage)
{
    //查找目标服务器。
    //不要让调用者指定NOTIFICATION消息id
    TARGET* target = NULL;

    ULONG msgid = msg->msgid;
    if ((msgid & 0xFF) != 0xFF) {

        ULONG serverId = msgid & 0xFFFFFF00;
        target = (TARGET*)List_Head(&m_targets);
        while (target) {
            if (target->serverId == serverId)
                break;
            target = (TARGET*)List_Next(target);
        }
    }

    if (!target)
        return AllocShortMsg(STATUS_INVALID_SYSTEM_SERVICE);
    //调用目标服务器
    CLIENT_TLS_DATA TlsData;
    TlsData.PortHandle = PortHandle;
    TlsData.PortMessage = PortMessage;
    TlsSetValue(m_TlsIndex, &TlsData);

    MSG_HEADER* msgOut = NULL;

    __try {

        msgOut = (*target->handler)(target->context, msg);

    }
    __except (EXCEPTION_EXECUTE_HANDLER) {

        ULONG exc = GetExceptionCode();
        msgOut = AllocShortMsg(exc);
    }

    RevertToSelf();
    TlsSetValue(m_TlsIndex, NULL);

    return msgOut;
}

void PipeServer::PortFindClientUnsafe(const CLIENT_ID& ClientId, tagCLIENT_PROCESS*& clientProcess, tagCLIENT_THREAD*& clientThread)
{
    //注意：这不是线程安全的，在调用此函数之前必须锁定m_lock
    clientProcess = (CLIENT_PROCESS*)map_get(&m_client_map, ClientId.UniqueProcess);
    clientThread = clientProcess ? (CLIENT_THREAD*)map_get(&clientProcess->thread_map, ClientId.UniqueThread) : NULL;
}
