#include"dll.h"

BOOLEAN SbieDll_FuncSkipHook(const char* func);

void* SbieDll_Hook(
    const char* SourceFuncName, void* SourceFunc, void* DetourFunc, HMODULE module) 
{
    if (SbieDll_FuncSkipHook(SourceFuncName))
        return SourceFunc;
    //Chrome…≥œ‰÷ß≥÷
    //SourceFunc = Hook_CheckChromeHook(SourceFunc);
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