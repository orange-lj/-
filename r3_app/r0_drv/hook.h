#pragma once
//#include"pool.h"
#include"main.h"

enum HOOK_KIND {

    INST_UNKNOWN = 0,
    INST_MOVE,
    INST_CTLXFER,           // jmp/jcc/call with 32-bit disp
    INST_CTLXFER_REG,       // jmp/call reg or [reg]
    INST_CALL_MEM,          // call [mem]
    INST_JUMP_MEM,          // jmp  [mem]
    INST_SYSCALL,
    INST_RET
};

typedef struct _HOOK_INST 
{
    ULONG len;
    UCHAR kind;
    UCHAR op1, op2;
    ULONG64 parm;
    LONG* rel32;            // --> 32-bit relocation for control-xfer
    UCHAR* modrm;
    ULONG flags;
} HOOK_INST;


//HOOK_GetService分析由DllProc指定的NTDLL或USER32用户模式存根，并计算系统服务号，
//并使用该编号返回实际服务于请求的NTOSKRNL过程。这既返回NTOSKRNL内的实际例程(NtXxx)，
//也返回内核模式Zw调度程序存根(ZwXxx)。
BOOLEAN Hook_GetService(
	void* DllProc, LONG* SkipIndexes, ULONG ParamCount,
	void** NtService, void** ZwService);

//Hook_GetServiceIndex分析由DllProc指定的NTDLL或USER32用户模式存根，并计算系统服务号。
LONG Hook_GetServiceIndex(void* DllProc, LONG* SkipIndexes);

//返回由指定的服务索引标识的NTOS内核服务的地址。它必须采用与指示的参数完全相同的参数。对于32位和64位模式，此例程的实现方式不同。
void* Hook_GetNtServiceInternal(ULONG ServiceIndex, ULONG ParamCount);
void* Hook_GetZwServiceInternal(ULONG ServiceIndex);

//分析指定地址的单条指令
BOOLEAN Hook_Analyze(
    void* address,
    BOOLEAN probe_address,
    BOOLEAN is64,
    HOOK_INST* inst);