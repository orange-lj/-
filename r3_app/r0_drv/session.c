#include "session.h"
#include"mem.h"
#include"api.h"
#include"api_defs.h"
#include"util.h"

struct _SESSION {

    // 对SESSION块的链接列表的更改由SESSION_ListLock上的独占锁同步

    LIST_ELEM list_elem;

    //
    // session id
    //

    ULONG session_id;

    //
    //会话领导进程id
    //

    HANDLE leader_pid;

    //
    //禁用强制进程
    //

    LONGLONG disable_force_time;

    //
    // resource monitor
    //

    //LOG_BUFFER* monitor_log;

    BOOLEAN monitor_stack_trace;

    BOOLEAN monitor_overflow;

};

typedef struct _SESSION             SESSION;

static NTSTATUS Session_Api_Leader(PROCESS* proc, ULONG64* parms);
static void Session_Unlock(KIRQL irql);
static SESSION* Session_Get(
    BOOLEAN create, ULONG SessionId, KIRQL* out_irql);

//变量
static LIST Session_List;
PERESOURCE Session_ListLock = NULL;

BOOLEAN Session_Init(void)
{
    List_Init(&Session_List);

    if (!Mem_GetLockResource(&Session_ListLock, TRUE))
        return FALSE;
    Api_SetFunction(API_SESSION_LEADER, Session_Api_Leader);
    //Api_SetFunction(API_DISABLE_FORCE_PROCESS, Session_Api_DisableForce);
    //Api_SetFunction(API_MONITOR_CONTROL, Session_Api_MonitorControl);
    ////Api_SetFunction(API_MONITOR_PUT,            Session_Api_MonitorPut);
    //Api_SetFunction(API_MONITOR_PUT2, Session_Api_MonitorPut2);
    ////Api_SetFunction(API_MONITOR_GET,            Session_Api_MonitorGet);
    //Api_SetFunction(API_MONITOR_GET_EX, Session_Api_MonitorGetEx);
    //Api_SetFunction(API_MONITOR_GET2, Session_Api_MonitorGet2);
    return TRUE;
}

NTSTATUS Session_Api_Leader(PROCESS* proc, ULONG64* parms)
{
    API_SESSION_LEADER_ARGS* args = (API_SESSION_LEADER_ARGS*)parms;
    NTSTATUS status = STATUS_SUCCESS;
    ULONG64 ProcessIdToReturn = 0;
    SESSION* session = NULL;
    KIRQL irql;

    ULONG64* user_pid = args->process_id.val;
    if (!user_pid) 
    {
        //集合领导者
        if (proc)
            status = STATUS_NOT_IMPLEMENTED;
        else if (!MyIsCallerSigned())
            status = STATUS_INVALID_SIGNATURE; // STATUS_ACCESS_DENIED
        else 
        {
            session = Session_Get(TRUE, -1, &irql);
            if (!session)
                status = STATUS_INSUFFICIENT_RESOURCES;
            else if (session->leader_pid != PsGetCurrentProcessId())
                status = STATUS_DEVICE_ALREADY_ATTACHED;  // STATUS_ALREADY_REGISTERED
        }
    }
    else 
    {
    
    }
    if (session) 
    {
        Session_Unlock(irql);
    }
    if (user_pid && NT_SUCCESS(status)) 
    {
    
    }
    return status;
}

void Session_Unlock(KIRQL irql)
{
    ExReleaseResourceLite(Session_ListLock);
    KeLowerIrql(irql);
}

SESSION* Session_Get(BOOLEAN create, ULONG SessionId, KIRQL* out_irql)
{
    NTSTATUS status;
    SESSION* session;

    if (SessionId == -1) 
    {
        status = MyGetSessionId(&SessionId);
        if (!NT_SUCCESS(status))
            return NULL;
    }
    // 查找现有SESSION块或创建新块
    KeRaiseIrql(APC_LEVEL, out_irql);
    ExAcquireResourceExclusiveLite(Session_ListLock, TRUE);

    session = List_Head(&Session_List);
    while (session) 
    {
        if (session->session_id == SessionId)
            break;
        session = List_Next(session);
    }
    if ((!session) && create) 
    {
        session = Mem_Alloc(Driver_Pool, sizeof(SESSION));
        if (session) {
            memzero(session, sizeof(SESSION));
            session->session_id = SessionId;
            session->leader_pid = PsGetCurrentProcessId();
            List_Insert_After(&Session_List, NULL, session);
        }
    }
    if (!session)
        Session_Unlock(*out_irql);

    return session;
}
