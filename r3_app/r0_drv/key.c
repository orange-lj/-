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
	//在Windows XP/2003上安装解析过程挂钩
	//在Vista及更高版本上注册为注册表过滤器
	typedef BOOLEAN(*P_Key_Init_2)(void);
	P_Key_Init_2 p_Key_Init_2 = Key_Init_Filter;
	if (!p_Key_Init_2())
		return FALSE;
	//init key装入数据
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
	//Windows Vista及更高版本上的注册表通知包括DesiredAccess字段，
	//因此我们可以使用此官方方法，而不是像Windows XP那样依赖解析过程挂钩
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
