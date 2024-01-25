#pragma once
//#include"pool.h"
#include"main.h"
#include"msgs.h"

#define MSG_DRIVER_ENTRY_OK                                 MSG_1101
#define LOG_BUFFER_SEQ_T ULONG
#define Log_Msg1(error_code,str1) \
    Log_Msg(error_code,str1,NULL)

typedef struct _LOG_BUFFER
{
	LOG_BUFFER_SEQ_T seq_counter;
	SIZE_T buffer_size;
	SIZE_T buffer_used;
	CHAR* buffer_start_ptr;
	CHAR buffer_data[0]; // [[SIZE 4][DATA n][SEQ 4][SITE 4]][...] // Note 2nd size tags allows to traverse the ring in both directions
} LOG_BUFFER;

LOG_BUFFER* log_buffer_init(SIZE_T buffer_size);
void Log_Msg(
	NTSTATUS error_code,
	const WCHAR* string1,
	const WCHAR* string2);
void Log_Msg_Session(
	NTSTATUS error_code,
	const WCHAR* string1,
	const WCHAR* string2,
	ULONG session_id);
void Log_Msg_Process(
	NTSTATUS error_code,
	const WCHAR* string1,
	const WCHAR* string2,
	ULONG session_id,
	HANDLE process_id);
void Log_Popup_Msg(
	NTSTATUS error_code,
	const WCHAR* string1,
	const WCHAR* string2,
	ULONG session_id,
	HANDLE pid);
void Log_Popup_MsgEx(
	NTSTATUS error_code,
	const WCHAR* string1, ULONG string1_len,
	const WCHAR* string2, ULONG string2_len,
	ULONG session_id,
	HANDLE pid);
