#include "log.h"
#include"msgs.h"

static void Log_Event_Msg(
	NTSTATUS error_code,
	const WCHAR* string1,
	const WCHAR* string2);

LOG_BUFFER* log_buffer_init(SIZE_T buffer_size)
{
	LOG_BUFFER* ptr_buffer = (LOG_BUFFER*)ExAllocatePoolWithTag(PagedPool, sizeof(LOG_BUFFER) + buffer_size, tzuk);
	if (ptr_buffer != NULL)
	{
		ptr_buffer->seq_counter = 0;
		ptr_buffer->buffer_used = 0;
		ptr_buffer->buffer_size = buffer_size;
		ptr_buffer->buffer_start_ptr = ptr_buffer->buffer_data;
	}
	return ptr_buffer;
}

void Log_Msg(NTSTATUS error_code, const WCHAR* string1, const WCHAR* string2)
{
	Log_Msg_Session(error_code, string1, string2, -1);
}

void Log_Msg_Session(NTSTATUS error_code, const WCHAR* string1, const WCHAR* string2, ULONG session_id)
{
	Log_Msg_Process(error_code, string1, string2, session_id, (HANDLE)4);
}

void Log_Msg_Process(NTSTATUS error_code, const WCHAR* string1, const WCHAR* string2, ULONG session_id, HANDLE process_id)
{
	ULONG facility = (error_code >> 16) & 0x0F;
	if (facility & MSG_FACILITY_EVENT)
		Log_Event_Msg(error_code, string1, string2);
	if (facility & MSG_FACILITY_POPUP) 
	{
		//Log_Popup_Msg(error_code, string1, string2, session_id, process_id);
	}
}

void Log_Popup_Msg(NTSTATUS error_code, const WCHAR* string1, const WCHAR* string2, ULONG session_id, HANDLE pid)
{
	ULONG string1_len, string2_len;

	if (string1)
		string1_len = wcslen(string1);
	else
		string1_len = 0;

	if (string2)
		string2_len = wcslen(string2);
	else
		string2_len = 0;

	Log_Popup_MsgEx(error_code, string1, string1_len, string2, string2_len, session_id, pid);
}

void Log_Popup_MsgEx(NTSTATUS error_code, const WCHAR* string1, ULONG string1_len, const WCHAR* string2, ULONG string2_len, ULONG session_id, HANDLE pid)
{
	//将消息记录到目标会话
	if (session_id == -1) {
		//NTSTATUS status = MyGetSessionId(&session_id);
		//if (!NT_SUCCESS(status))
		//	session_id = 0;
	}

}

void Log_Event_Msg(NTSTATUS error_code, const WCHAR* string1, const WCHAR* string2)
{
	int entry_size;
	int string1_len = 0;
	int string2_len = 0;

	if (string1)
		string1_len = (wcslen(string1) + 1) * sizeof(WCHAR);
	if (string2)
		string2_len = (wcslen(string2) + 1) * sizeof(WCHAR);
	entry_size = sizeof(IO_ERROR_LOG_PACKET) + string1_len + string2_len;
	if (entry_size <= ERROR_LOG_MAXIMUM_SIZE) 
	{
		IO_ERROR_LOG_PACKET* entry =
			(IO_ERROR_LOG_PACKET*)IoAllocateErrorLogEntry(
				Driver_Object, (UCHAR)entry_size);
		if (entry) {
			UCHAR* strings;

			entry->RetryCount = 0;
			entry->DumpDataSize = 0;

			entry->NumberOfStrings = 0;
			strings = ((UCHAR*)entry) + sizeof(IO_ERROR_LOG_PACKET);
			if (string1_len > 0) {
				++entry->NumberOfStrings;
				memcpy(strings, string1, string1_len);
				strings += string1_len;
			}
			if (string2_len > 0) {
				++entry->NumberOfStrings;
				memcpy(strings, string2, string2_len);
				strings += string2_len;
			}
			entry->StringOffset = sizeof(IO_ERROR_LOG_PACKET);

			entry->ErrorCode = error_code;
			entry->FinalStatus = STATUS_SUCCESS;

			IoWriteErrorLogEntry(entry);
		}
	}
}
