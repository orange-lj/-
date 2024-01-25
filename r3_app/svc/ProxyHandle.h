#pragma once

#include"../common/list.h"
typedef void (*P_ProxyHandle_CloseCallback)(void* context, void* proxy_data);
class ProxyHandle
{
public:

    ProxyHandle(HANDLE heap, ULONG size_of_data,
        P_ProxyHandle_CloseCallback close_callback,
        void* context_for_callback);
protected:

    CRITICAL_SECTION m_lock;
    LIST m_list;
    P_ProxyHandle_CloseCallback m_close_callback;
    void* m_context_for_callback;

    HANDLE m_heap;

    ULONG m_size_of_data;

    volatile LONG m_unique_id;
};

