#pragma once
//#include"pool.h"
#include"main.h"


struct _THREAD {
    HANDLE tid;

    void* token_object;
    BOOLEAN token_CopyOnOpen;
    BOOLEAN token_EffectiveOnly;
    SECURITY_IMPERSONATION_LEVEL token_ImpersonationLevel;
    BOOLEAN create_process_in_progress;
};

BOOLEAN Thread_Init(void);