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

CHAR* log_buffer_get_next(LOG_BUFFER_SEQ_T seq_number, LOG_BUFFER* ptr_buffer)
{
	//向后遍历列表以查找下一个条目
	for (SIZE_T size_left = ptr_buffer->buffer_used; size_left > 0;)
	{
		CHAR* end_ptr = ptr_buffer->buffer_start_ptr + size_left - sizeof(LOG_BUFFER_SIZE_T);
		LOG_BUFFER_SIZE_T size = log_buffer_get_size(&end_ptr, ptr_buffer);
		SIZE_T total_size = size + sizeof(LOG_BUFFER_SIZE_T) * 2 + sizeof(LOG_BUFFER_SEQ_T);

		CHAR* read_ptr = end_ptr - total_size;

		CHAR* seq_ptr = read_ptr + sizeof(LOG_BUFFER_SIZE_T);
		LOG_BUFFER_SEQ_T cur_number = log_buffer_get_seq_num(&seq_ptr, ptr_buffer);

		if (cur_number == seq_number && size_left == ptr_buffer->buffer_used)
			return NULL; //列表中的最后一个条目是我们已经得到的最后一个，返回NULL

		if (cur_number == seq_number + 1)
			return read_ptr; //这个条目是我们已经得到的最后一个条目之后的一个，返回它

		size_left -= total_size;
	}

	if (ptr_buffer->buffer_used != 0)
		return ptr_buffer->buffer_start_ptr; //我们还没有找到下一个条目，我们有条目，所以返回第一个条目
	return NULL; //缓冲区显然是空的，返回NULL
}

LOG_BUFFER_SIZE_T log_buffer_get_size(CHAR** read_ptr, LOG_BUFFER* ptr_buffer)
{
	LOG_BUFFER_SIZE_T size = 0;
	log_buffer_get_bytes((char*)&size, sizeof(LOG_BUFFER_SIZE_T), read_ptr, ptr_buffer);
	return size;
}

BOOLEAN log_buffer_get_bytes(CHAR* data, SIZE_T size, CHAR** read_ptr, LOG_BUFFER* ptr_buffer)
{
	for (ULONG i = 0; i < size; i++)
		data[i] = *log_buffer_byte_at(read_ptr, ptr_buffer);
	return TRUE;
}

CHAR* log_buffer_byte_at(CHAR** data_ptr, LOG_BUFFER* ptr_buffer)
{
	if (*data_ptr >= ptr_buffer->buffer_data + ptr_buffer->buffer_size) // wrap around
		*data_ptr -= ptr_buffer->buffer_size;
	else if (*data_ptr < ptr_buffer->buffer_data) // wrap around
		*data_ptr += ptr_buffer->buffer_size;
	char* data = *data_ptr;
	*data_ptr += 1;
	return data;
}

LOG_BUFFER_SEQ_T log_buffer_get_seq_num(CHAR** read_ptr, LOG_BUFFER* ptr_buffer)
{
	LOG_BUFFER_SEQ_T seq_number = 0;
	log_buffer_get_bytes((char*)&seq_number, sizeof(LOG_BUFFER_SEQ_T), read_ptr, ptr_buffer);
	return seq_number;
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
