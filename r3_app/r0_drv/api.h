#pragma once
//#include"pool.h"
#include"main.h"

//��64λWindows�����е�32λӦ�ó���������⡣��32λӦ�ó����У�ͨ��API_ARGS�ṹ֮һ���ݵĽ��̾����handle������32λ��VOID*��64λ��������Ὣ����NtCurrentProcess�������бȽϣ�������64λ-1����������Զ����ƥ�䡣���������������⡣
#define IS_ARG_CURRENT_PROCESS(h) ((ULONG)h == 0xffffffff)      // -1


typedef NTSTATUS(*P_Api_Function)(PROCESS*, ULONG64* parms);

void Api_SetFunction(ULONG func_code, P_Api_Function func_ptr);

BOOLEAN Api_Init(void);

//��'len'�ֽڴ�'str'�����ں�ģʽ���������Ƶ�'uni'ָ�����û�ģʽ��������������uni->Length��
//��������STATUS_BUFFER_TOO_SMALL��STATUS_ACCESS_VIOLATION
void Api_CopyStringToUser(
    UNICODE_STRING64* uni, WCHAR* str, size_t len);

extern volatile HANDLE Api_ServiceProcessId;

//���û�����boxname����
BOOLEAN Api_CopyBoxNameFromUser(
    WCHAR* boxname34, const WCHAR* user_boxname);