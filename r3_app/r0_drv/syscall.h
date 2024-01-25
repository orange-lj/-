#pragma once
//#include"pool.h"
#include"main.h"


struct _SYSCALL_ENTRY;
typedef struct _SYSCALL_ENTRY SYSCALL_ENTRY;
typedef NTSTATUS(*P_Syscall_Handler1)(
	PROCESS* proc, SYSCALL_ENTRY* syscall_entry, ULONG_PTR* user_args);


typedef NTSTATUS(*P_Syscall_Handler2)(
	PROCESS* proc, void* Object, UNICODE_STRING* Name,
	ULONG Operation, ACCESS_MASK GrantedAccess);

typedef BOOLEAN(*P_Syscall_Handler3_Support_Procmon_Stack)(
	PROCESS* proc, SYSCALL_ENTRY* syscall_entry, ULONG_PTR* user_args);

void* Syscall_GetServiceTable(void);
BOOLEAN Syscall_Init(void);
SYSCALL_ENTRY* Syscall_GetByName(const UCHAR* name);
BOOLEAN Syscall_Set1(const UCHAR* name, P_Syscall_Handler1 handler_func);
BOOLEAN Syscall_Set2(const UCHAR* name, P_Syscall_Handler2 handler_func);
BOOLEAN Syscall_Set3(const UCHAR* name, P_Syscall_Handler3_Support_Procmon_Stack handler_func);

BOOLEAN Syscall_LoadHookMap(const WCHAR* setting_name, LIST* list);
int Syscall_HookMapMatch(const UCHAR* name, ULONG name_len, LIST* list);
VOID Syscall_FreeHookMap(LIST* list);

struct _SYSCALL_ENTRY {
	USHORT syscall_index;
	USHORT param_count;
	ULONG ntdll_offset;
	void* ntos_func;
	P_Syscall_Handler1 handler1_func;
	P_Syscall_Handler2 handler2_func;
	P_Syscall_Handler3_Support_Procmon_Stack handler3_func_support_procmon;
	UCHAR approved;
	USHORT name_len;
	UCHAR name[1];
};