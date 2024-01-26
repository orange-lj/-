#pragma once
//#include"pool.h"
#include"main.h"


BOOLEAN Process_Init(void);

//�Ͳ�ϵͳ�ӿڣ�process_low.c��
BOOLEAN Process_Low_Init(void);

//���ؽ��̾�������ID��ָ���Ľ��̵�SID�ַ����ͻỰID��
//��Ҫ�ͷŷ��ص�SidString����ʹ��RtlFreeUnicodeString
NTSTATUS Process_GetSidStringAndSessionId(
    HANDLE ProcessHandle, HANDLE ProcessId,
    UNICODE_STRING* SidString, ULONG* SessionId);

//���ش�ָ���ط����idProcess��ProcessName.exe������ʱ��* out_bufָ��UNICODE_STRING�ṹ���ýṹָ������ڸýṹ�������NULL��ֹ�Ľ������ơ�
//*out_len����*out_buf�ĳ��ȡ�
//*out_ptr����out_buf->Buffer��
//ʹ��Mem_Free��*out_buf��*out_len���ͷŻ�������
//����ʱ��out_buf��out_len��out_ptr����NULL
void Process_GetProcessName(
    POOL* pool, ULONG_PTR idProcess,
    void** out_buf, ULONG* out_len, WCHAR** out_ptr);