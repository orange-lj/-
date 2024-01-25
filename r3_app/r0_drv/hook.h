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


//HOOK_GetService������DllProcָ����NTDLL��USER32�û�ģʽ�����������ϵͳ����ţ�
//��ʹ�øñ�ŷ���ʵ�ʷ����������NTOSKRNL���̡���ȷ���NTOSKRNL�ڵ�ʵ������(NtXxx)��
//Ҳ�����ں�ģʽZw���ȳ�����(ZwXxx)��
BOOLEAN Hook_GetService(
	void* DllProc, LONG* SkipIndexes, ULONG ParamCount,
	void** NtService, void** ZwService);

//Hook_GetServiceIndex������DllProcָ����NTDLL��USER32�û�ģʽ�����������ϵͳ����š�
LONG Hook_GetServiceIndex(void* DllProc, LONG* SkipIndexes);

//������ָ���ķ���������ʶ��NTOS�ں˷���ĵ�ַ�������������ָʾ�Ĳ�����ȫ��ͬ�Ĳ���������32λ��64λģʽ�������̵�ʵ�ַ�ʽ��ͬ��
void* Hook_GetNtServiceInternal(ULONG ServiceIndex, ULONG ParamCount);
void* Hook_GetZwServiceInternal(ULONG ServiceIndex);

//����ָ����ַ�ĵ���ָ��
BOOLEAN Hook_Analyze(
    void* address,
    BOOLEAN probe_address,
    BOOLEAN is64,
    HOOK_INST* inst);