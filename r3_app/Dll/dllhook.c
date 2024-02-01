#include"dll.h"

static const WCHAR* _fmt1 = L"%s (%d)";

LIST Dll_ModuleHooks;
CRITICAL_SECTION  Dll_ModuleHooks_CritSec;

BOOLEAN SbieDll_FuncSkipHook(const char* func);

void* SbieDll_Hook_x86(
    const char* SourceFuncName, void* SourceFunc, void* DetourFunc, HMODULE module) 
{
    UCHAR* tramp, * func = NULL;
    void* RegionBase;
    SIZE_T RegionSize;
    ULONG prot, dummy_prot;
    ULONG_PTR diff;
    ULONG_PTR target;
    long long delta;
    BOOLEAN CallInstruction64 = FALSE;

    //��֤����
    if (!SourceFunc) {
        //SbieApi_Log(2303, _fmt1, SourceFuncName, 1);
        return NULL;
    }

    //���Դ�����������תEB xx��ʼ������ζ���������Ѿ���ס������
    //������ǳ��Թ�ס�ù��ӣ�����������Cisco Security Agent���棩
    if (*(UCHAR*)SourceFunc == 0xEB) 
    { // jmp xx;
        signed char offset = *((signed char*)SourceFunc + 1);
        SourceFunc = (UCHAR*)SourceFunc + offset + 2;
    }

    //���Դ�����Խӽ���ԾE9 xx xx xx xx��ʼ���������Ծ������Ŀ�ĵء�
    //���Ŀ����DetourFunc��ͬ������ֹ�����򣨶���32λ���룩ֻ�滻��תĿ��
    /*while (*(UCHAR*)SourceFunc == 0xE9) 
    {
    
    }*/
}



void* SbieDll_Hook(
    const char* SourceFuncName, void* SourceFunc, void* DetourFunc, HMODULE module) 
{
    if (SbieDll_FuncSkipHook(SourceFuncName))
        return SourceFunc;
    //Chromeɳ��֧��
    //SourceFunc = Hook_CheckChromeHook(SourceFunc);
    return SbieDll_Hook_x86(SourceFuncName, SourceFunc, DetourFunc, module);
}

BOOLEAN SbieDll_FuncSkipHook(const char* func)
{
    static const WCHAR* setting = L"FuncSkipHook";

    static BOOLEAN Disable = FALSE;
    if (Disable) return FALSE;

    WCHAR buf[66];
    ULONG index = 0;
    while (1) {
        NTSTATUS status = SbieApi_QueryConfAsIs(NULL, setting, index, buf, 64 * sizeof(WCHAR));
        if (NT_SUCCESS(status)) {
            WCHAR* ptr = buf;
            for (const char* tmp = func; *ptr && *tmp && *ptr == *tmp; ptr++, tmp++);
            if (*ptr == L'\0') //if (_wcsicmp(buf, func) == 0)
                return TRUE;
        }
        else if (status != STATUS_BUFFER_TOO_SMALL)
            break;
        ++index;
    }

    // if there are no configured functions to skip, disable the check
    if (index == 0) Disable = TRUE;

    return FALSE;
}


void SbieDll_HookInit() 
{
    InitializeCriticalSection(&Dll_ModuleHooks_CritSec);
    List_Init(&Dll_ModuleHooks);
}