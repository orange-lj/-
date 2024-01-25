#pragma once
//#include"pool.h"
#include"../common/pool.h"
#include<ntifs.h>
//typedef unsigned char UCHAR;
//typedef UCHAR BOOLEAN;
//typedef ERESOURCE* PERESOURCE;
void* Mem_AllocEx(POOL* pool, ULONG size, BOOLEAN InitMsg);
#define Mem_Alloc(pool,size) Mem_AllocEx((pool),(size),FALSE)

WCHAR* Mem_AllocStringEx(
    POOL* pool, const WCHAR* model_string, BOOLEAN InitMsg);
#define Mem_AllocString(pool,model) Mem_AllocStringEx((pool),(model),FALSE)

BOOLEAN Mem_GetLockResource(PERESOURCE* ppResource, BOOLEAN InitMsg);

#define Mem_Free Pool_Free