// dllmain.cpp : 定义 DLL 应用程序的入口点。
#include"dll.h"

const ULONG tzuk = 'xobs';
HINSTANCE Dll_Instance = NULL;
ULONG Dll_OsBuild = 0;
ULONG Dll_Windows = 0;

BOOLEAN Dll_SbieTrace = FALSE;

ULONG Dll_SessionId = 0;
HMODULE Dll_DigitalGuardian = NULL;
const WCHAR* Dll_BoxName = NULL;
HMODULE Dll_Ntdll = NULL;
HMODULE Dll_Kernel32 = NULL;
HMODULE Dll_KernelBase = NULL;
const WCHAR* Dll_HomeDosPath = NULL;
BOOLEAN Dll_AppContainerToken = FALSE;

const WCHAR* DllName_advapi32 = L"advapi32.dll";
const WCHAR* DllName_kernel32 = L"kernel32.dll";
const WCHAR* DllName_kernelbase = L"kernelbase.dll";


static void Dll_InitGeneric(HINSTANCE hInstance);


BOOL APIENTRY DllMain( HINSTANCE hInstance,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
                     )
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
        Dll_DigitalGuardian = GetModuleHandleA("DgApi64.dll");
        Dll_OsBuild = GET_PEB_IMAGE_BUILD;
        if (GetProcAddress(GetModuleHandleA("ntdll.dll"), "LdrFastFailInLoaderCallout"))
            Dll_Windows = 10;
        else 
            Dll_Windows = 8;
        ProcessIdToSessionId(GetCurrentProcessId(), &Dll_SessionId);
        Dll_InitGeneric(hInstance);
        SbieDll_HookInit();
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}

void Dll_InitGeneric(HINSTANCE hInstance)
{
    //Dll_InitGeneric以通用的方式初始化SbieDll，
    //适用于可能在沙箱中也可能不在沙盒中的程序
    Dll_Instance = hInstance;

    Dll_Ntdll = GetModuleHandle(L"ntdll.dll");
    Dll_Kernel32 = GetModuleHandle(DllName_kernel32);
    Dll_KernelBase = GetModuleHandle(DllName_kernelbase);

    extern void InitMyNtDll(HMODULE Ntdll);
    InitMyNtDll(Dll_Ntdll);

    extern FARPROC __sys_GetModuleInformation;
    __sys_GetModuleInformation = GetProcAddress(LoadLibraryW(L"psapi.dll"), "GetModuleInformation");

    if (!Dll_InitMem()) 
    {
        //SbieApi_Log(2305, NULL);
        //ExitProcess(-1);
    }
}
