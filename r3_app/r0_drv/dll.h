#pragma once
//#include"pool.h"
#include"main.h"
extern const WCHAR* Dll_NTDLL;
struct _DLL_ENTRY;
typedef struct _DLL_ENTRY DLL_ENTRY;

BOOLEAN Dll_Init(void);

DLL_ENTRY* Dll_Load(const WCHAR* DllBaseName);
void Dll_Unload(void);

void* Dll_RvaToAddr(DLL_ENTRY* dll, ULONG rva);

ULONG Dll_GetNextProc(
    DLL_ENTRY* dll, const UCHAR* SearchName,
    UCHAR** FoundName, ULONG* FoundIndex);

void* Dll_GetProc(
    const WCHAR* DllName, const UCHAR* ProcName, BOOLEAN returnOffset);