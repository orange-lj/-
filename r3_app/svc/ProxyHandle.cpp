#include<windows.h>
#include "ProxyHandle.h"


ProxyHandle::ProxyHandle(HANDLE heap, ULONG size_of_data, P_ProxyHandle_CloseCallback close_callback, void* context_for_callback)
{
    InitializeCriticalSectionAndSpinCount(&m_lock, 1000);
    List_Init(&m_list);

    m_close_callback = close_callback;
    m_context_for_callback = context_for_callback;

    if (!heap)
        heap = GetProcessHeap();
    m_heap = heap;

    m_size_of_data = size_of_data;

    m_unique_id = 'sbox';
}
