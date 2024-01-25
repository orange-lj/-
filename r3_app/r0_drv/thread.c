#include "thread.h"
#include"syscall.h"
#include"api.h"
#include"../common/ntproto.h"

void* Thread_AnonymousToken = NULL;
static BOOLEAN Thread_NotifyInstalled = FALSE;

static void Thread_Notify(HANDLE ProcessId, HANDLE ThreadId, BOOLEAN Create);
static void Thread_InitAnonymousToken(void);

BOOLEAN Thread_Init(void)
{
	NTSTATUS status;
	//设置线程通知例程
    status = PsSetCreateThreadNotifyRoutine(Thread_Notify);

    if (NT_SUCCESS(status)) {

        Thread_NotifyInstalled = TRUE;

    }
    else {

        // too many notify routines are already installed in the system
        //Log_Status(MSG_PROCESS_NOTIFY, 0x33, status);
        return FALSE;
    }
    //获取Thread_ImpersonateNonymousToken的模拟令牌
    Thread_InitAnonymousToken();
    //设置系统调用处理程序
    //if (!Syscall_Set1("OpenProcessToken", Thread_OpenProcessToken))
    //    return FALSE;
    //if (!Syscall_Set1("OpenProcessTokenEx", Thread_OpenProcessTokenEx))
    //    return FALSE;
    //
    //if (!Syscall_Set1("OpenThreadToken", Thread_OpenThreadToken))
    //    return FALSE;
    //if (!Syscall_Set1("OpenThreadTokenEx", Thread_OpenThreadTokenEx))
    //    return FALSE;
    //
    //if (!Syscall_Set1("SetInformationProcess", Thread_SetInformationProcess))
    //    return FALSE;
    //if (!Syscall_Set1("SetInformationThread", Thread_SetInformationThread))
    //    return FALSE;
    //
    //if (!Syscall_Set1(
    //    "ImpersonateAnonymousToken", Thread_ImpersonateAnonymousToken))
    //    return FALSE;

    //设置对象打开处理程序
    //if (!Syscall_Set2("OpenProcess", Thread_CheckProcessObject))
    //    return FALSE;
    //
    //if (!Syscall_Set2("OpenThread", Thread_CheckThreadObject))
    //    return FALSE;
    //
    //if (Driver_OsVersion >= DRIVER_WINDOWS_VISTA) {
    //
    //    if (!Syscall_Set2(
    //        "AlpcOpenSenderProcess", Thread_CheckProcessObject))
    //        return FALSE;
    //
    //    if (!Syscall_Set2(
    //        "AlpcOpenSenderThread", Thread_CheckThreadObject))
    //        return FALSE;
    //}
    //// set API handlers
    //
    //Api_SetFunction(API_OPEN_PROCESS, Thread_Api_OpenProcess);

    return TRUE;


}

void Thread_Notify(HANDLE ProcessId, HANDLE ThreadId, BOOLEAN Create)
{
    //以后实现
}

void Thread_InitAnonymousToken(void)
{
    SYSCALL_ENTRY* syscall_entry;
    P_NtImpersonateAnonymousToken pNtImpersonateAnonymousToken;
    void* TokenObject;
    BOOLEAN CopyOnOpen;
    BOOLEAN EffectiveOnly;
    SECURITY_IMPERSONATION_LEVEL ImpersonationLevel;
    NTSTATUS status;

    syscall_entry = Syscall_GetByName("ImpersonateAnonymousToken");
    if (!syscall_entry)
        return;
    TokenObject = PsReferenceImpersonationToken(PsGetCurrentThread(),
        &CopyOnOpen, &EffectiveOnly, &ImpersonationLevel);
    pNtImpersonateAnonymousToken =
        (P_NtImpersonateAnonymousToken)syscall_entry->ntos_func;

    status = pNtImpersonateAnonymousToken(NtCurrentThread());
    if (NT_SUCCESS(status)) {

        BOOLEAN CopyOnOpen2;
        BOOLEAN EffectiveOnly2;
        SECURITY_IMPERSONATION_LEVEL ImpersonationLevel2;

        Thread_AnonymousToken =
            PsReferenceImpersonationToken(PsGetCurrentThread(),
                &CopyOnOpen2, &EffectiveOnly2, &ImpersonationLevel2);
    }
    PsImpersonateClient(PsGetCurrentThread(), TokenObject,
        CopyOnOpen, EffectiveOnly, ImpersonationLevel);
}
