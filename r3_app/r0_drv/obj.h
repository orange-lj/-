#pragma once
//#include"pool.h"
#include"main.h"

typedef void* (*P_ObGetObjectType)(void* Object);
typedef ULONG(*P_ObQueryNameInfo)(void* Object);

NTSTATUS Obj_GetName(
    POOL* pool, void* Object,
    OBJECT_NAME_INFORMATION** Name, ULONG* NameLength);

BOOLEAN Obj_Init(void);

POBJECT_TYPE Obj_GetTypeObjectType(void);

BOOLEAN Obj_Load_Filter(void);