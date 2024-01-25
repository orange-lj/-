#include"dll.h"
#include"pool.h"

POOL* Dll_Pool = NULL;
POOL* Dll_PoolTemp = NULL;
POOL* Dll_PoolCode = NULL;
static ULONG Dll_TlsIndex = TLS_OUT_OF_INDEXES;

BOOLEAN Dll_InitMem(void);
static void* Dll_AllocFromPool(POOL* pool, ULONG size);

THREAD_DATA* Dll_GetTlsData(ULONG* pLastError) 
{
	THREAD_DATA* data;
	if (Dll_TlsIndex == TLS_OUT_OF_INDEXES)
		data = NULL;
	else 
	{
		ULONG LastError = GetLastError();
		data = TlsGetValue(Dll_TlsIndex);
		if (!data) 
		{
			data = Dll_Alloc(sizeof(THREAD_DATA));
			memzero(data, sizeof(THREAD_DATA));

			TlsSetValue(Dll_TlsIndex, data);
		}
		SetLastError(LastError);
		if (pLastError)
			*pLastError = LastError;
	}
	return data;
}

BOOLEAN Dll_InitMem(void)
{
	if (!Dll_Pool) {
		Dll_Pool = Pool_Create();
		if (!Dll_Pool)
			return FALSE;
	}
	if (!Dll_PoolTemp) {
		Dll_PoolTemp = Pool_Create();
		if (!Dll_PoolTemp)
			return FALSE;
	}

	if (!Dll_PoolCode) {
		Dll_PoolCode = Pool_CreateTagged(tzuk | 0xFF);
		if (!Dll_PoolCode)
			return FALSE;
	}
	if (Dll_TlsIndex == TLS_OUT_OF_INDEXES) {
		Dll_TlsIndex = TlsAlloc();
		if (Dll_TlsIndex == TLS_OUT_OF_INDEXES)
			return FALSE;
	}
	return TRUE;
}

void* Dll_AllocFromPool(POOL* pool, ULONG size)
{
	UCHAR* ptr;
	size += sizeof(ULONG_PTR);
	ptr = Pool_Alloc(pool, size);
	if (!ptr) 
	{
	
	}
	*(ULONG_PTR*)ptr = size;
	ptr += sizeof(ULONG_PTR);
	return ptr;
}


void* Dll_Alloc(ULONG size)
{
	return Dll_AllocFromPool(Dll_Pool, size);
}


void Dll_PushTlsNameBuffer(THREAD_DATA* data) 
{
	++data->name_buffer_depth;
	//��ʼ��
	data->name_buffer_count[data->name_buffer_depth] = 0;
	if (data->name_buffer_depth > NAME_BUFFER_DEPTH - 4) 
	{
	
	}
	if (data->name_buffer_depth >= NAME_BUFFER_DEPTH) 
	{
	
	}
}

void Dll_Free(void* ptr) 
{
	UCHAR* ptr2 = ((UCHAR*)ptr) - sizeof(ULONG_PTR);
	ULONG size = (ULONG)(*(ULONG_PTR*)ptr2);
	Pool_Free(ptr2, size);
}

WCHAR* Dll_GetTlsNameBuffer(THREAD_DATA* data, ULONG which, ULONG size) 
{
	WCHAR* old_name_buffer;
	ULONG old_name_buffer_len;
	WCHAR** name_buffer;
	ULONG* name_buffer_len;
	//�������������и���ĵط�������Ҫ���ƻ������������Ǽ��ʹ�á��ĸ����������Ŭ�����ظ�ʹ����Ȼ��Ҫ���ض�����������������ֻ����һ����������
	//������������Ļ���������ʱ����ȡ��һ���µĻ�����
	if (which >= MISC_NAME_BUFFER) 
	{
	
	}
	if (which >= NAME_BUFFER_COUNT - 2)
		//SbieApi_Log(2310, L"%d", which);
	if (which >= NAME_BUFFER_COUNT) {
		//ExitProcess(-1);
	}
	name_buffer = &data->name_buffer[which][data->name_buffer_depth];
	name_buffer_len = &data->name_buffer_len[which][data->name_buffer_depth];
	//������Ĵ�С�� + һЩ�ֽڵĶ�����䣩��������ΪPAGE_size�ı�����
	size = (size + 64 + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1);
	if (size > *name_buffer_len)
	{
		//����Ĵ�С���ڵ�ǰ���ƻ��������������Ƿ�����һ������Ļ����������������Ƶ�������
		old_name_buffer = *name_buffer;
		old_name_buffer_len = *name_buffer_len;

		*name_buffer_len = size;
		*name_buffer = Dll_Alloc(*name_buffer_len);

		if (old_name_buffer) {
			memcpy(*name_buffer, old_name_buffer, old_name_buffer_len);
			Dll_Free(old_name_buffer);
		}
	}
	return *name_buffer;
}

void* Dll_AllocTemp(ULONG size)
{
	return Dll_AllocFromPool(Dll_PoolTemp, size);
}

void Dll_PopTlsNameBuffer(THREAD_DATA* data) 
{
	--data->name_buffer_depth;
	if (data->name_buffer_depth < 0) 
	{
		//SbieApi_Log(2310, L"%d", data->name_buffer_depth);
	}
}