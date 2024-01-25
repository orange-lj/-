#include "key.h"
#include"mem.h"

static LIST Key_Mounts;
static PERESOURCE Key_MountsLock = NULL;
static LARGE_INTEGER Key_Cookie;
static BOOLEAN Key_CallbackInstalled = FALSE;

static BOOLEAN Key_Init_Filter(void);
static NTSTATUS Key_Callback(void* Context, void* Arg1, void* Arg2);


BOOLEAN Key_Init(void)
{
	//��Windows XP/2003�ϰ�װ�������̹ҹ�
	//��Vista�����߰汾��ע��Ϊע��������
	typedef BOOLEAN(*P_Key_Init_2)(void);
	P_Key_Init_2 p_Key_Init_2 = Key_Init_Filter;
	if (!p_Key_Init_2())
		return FALSE;
	//init keyװ������
	List_Init(&Key_Mounts);
	if (!Mem_GetLockResource(&Key_MountsLock, TRUE))
		return FALSE;
	// set API functions

	//Api_SetFunction(API_GET_UNMOUNT_HIVE, Key_Api_GetUnmountHive);
	//Api_SetFunction(API_OPEN_KEY, Key_Api_Open);
	//Api_SetFunction(API_SET_LOW_LABEL_KEY, Key_Api_SetLowLabel);

	return TRUE;
}

BOOLEAN Key_Init_Filter(void)
{
	NTSTATUS status;
	UNICODE_STRING uni;
	P_CmRegisterCallbackEx pCmRegisterCallbackEx;
	//Windows Vista�����߰汾�ϵ�ע���֪ͨ����DesiredAccess�ֶΣ�
	//������ǿ���ʹ�ô˹ٷ���������������Windows XP���������������̹ҹ�
	RtlInitUnicodeString(&uni, L"CmRegisterCallbackEx");
	pCmRegisterCallbackEx = (P_CmRegisterCallbackEx)
		MmGetSystemRoutineAddress(&uni);
	if (!pCmRegisterCallbackEx)
		status = STATUS_PROCEDURE_NOT_FOUND;
	else {

		status = pCmRegisterCallbackEx(
			Key_Callback, &Driver_Altitude, Driver_Object, NULL,
			&Key_Cookie, NULL);
	}

	if (!NT_SUCCESS(status)) {
		//Log_Status_Ex(MSG_OBJ_HOOK_ANY_PROC, 0x81, status, L"Registry");
		return FALSE;
	}

	Key_CallbackInstalled = TRUE;

	return TRUE;
}

NTSTATUS Key_Callback(void* Context, void* Arg1, void* Arg2)
{
	return STATUS_SUCCESS;
}
