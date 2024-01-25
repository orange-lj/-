#pragma once

#include"mem.h"
#include"main.h"


void* Mem_AllocEx(POOL* pool, ULONG size, BOOLEAN InitMsg)
{
	void* ptr = Pool_Alloc(pool, size);
	return ptr;
}

WCHAR* Mem_AllocStringEx(POOL* pool, const WCHAR* model_string, BOOLEAN InitMsg)
{
    WCHAR* str;
    ULONG num_bytes = (wcslen(model_string) + 1) * sizeof(WCHAR);
    str = Mem_AllocEx(pool, num_bytes, InitMsg);
    if (str)
        memcpy(str, model_string, num_bytes);
    return str;
}

BOOLEAN Mem_GetLockResource(PERESOURCE* ppResource, BOOLEAN InitMsg)
{
    *ppResource = ExAllocatePoolWithTag(NonPagedPool, sizeof(ERESOURCE), tzuk);
    if (*ppResource) 
    {
        ExInitializeResourceLite(*ppResource);
        return TRUE;
    }
    else 
    {
        return FALSE;
    }
}
