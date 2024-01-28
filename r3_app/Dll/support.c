#include"dll.h"
#include"../svc/sbieiniwire.h"
#include"../common/my_version.h"

const WCHAR* Support_SbieSvcKeyPath =
L"\\registry\\machine\\system\\currentcontrolset\\services\\" SBIESVC;

BOOLEAN SbieDll_StartSbieSvc(BOOLEAN retry) 
{
    typedef void* (*P_OpenSCManager)(void* p1, void* p2, ULONG acc);
    typedef void* (*P_OpenService)(void* hSCM, void* name, ULONG acc);
    typedef BOOL(*P_StartService)(void* hSvc, ULONG p2, void* p3);
    typedef void* (*P_CloseServiceHandle)(void* h);
    static P_OpenSCManager pOpenSCManagerW = NULL;
    static P_OpenService pOpenServiceW = NULL;
    static P_StartService pStartServiceW = NULL;
    static P_CloseServiceHandle pCloseServiceHandle = NULL;
    ULONG retries;
    if (!pOpenSCManagerW) 
    {
        HMODULE mod = LoadLibrary(DllName_advapi32);
        if (mod) 
        {
            pOpenSCManagerW = (P_OpenSCManager)
                GetProcAddress(mod, "OpenSCManagerW");
            pOpenServiceW = (P_OpenService)
                GetProcAddress(mod, "OpenServiceW");
            pStartServiceW = (P_StartService)
                GetProcAddress(mod, "StartServiceW");
            pCloseServiceHandle = (P_CloseServiceHandle)
                GetProcAddress(mod, "CloseServiceHandle");
        }
    }
    for (retries = 0; retries < 3; ++retries) 
    {
        SBIE_INI_GET_VERSION_REQ req;
        SBIE_INI_GET_VERSION_RPL* rpl;
        req.h.length = sizeof(SBIE_INI_GET_VERSION_REQ);
        req.h.msgid = MSGID_SBIE_INI_GET_VERSION;
        rpl = (SBIE_INI_GET_VERSION_RPL*)SbieDll_CallServer(&req.h);
        if (rpl) 
        {
            Dll_Free(rpl);
            return TRUE;
        }
        if (retries == 0) 
        {
            void* hScm = NULL;
            if (pOpenSCManagerW)
            {

                hScm = pOpenSCManagerW(NULL, NULL, GENERIC_READ);
                if (!hScm) 
                {
                    //SbieDll_SetStartError(0x11);
                }
            }
            if (hScm) 
            {
                void* hSvc = NULL;
                if (pOpenServiceW) 
                {
                    hSvc = pOpenServiceW(hScm, SBIESVC, SERVICE_START);
                    if (!hSvc) 
                    {
                    
                    }
                }
                if (hSvc) 
                {
                    BOOL ok = FALSE;
                    if (pStartServiceW) 
                    {
                        ok = pStartServiceW(hSvc, 0, NULL);
                        if (!ok) 
                        {
                            int a = GetLastError();
                        }
                    }
                    pCloseServiceHandle(hSvc);
                }
                pCloseServiceHandle(hScm);
            }
        }
        else if (!retry) 
        {
            return FALSE;
        }
        Sleep(200);
    }
    return FALSE;
}


BOOLEAN SbieDll_GetServiceRegistryValue(
    const WCHAR* name, void* kvpi, ULONG sizeof_kvpi)
{
    NTSTATUS status;
    OBJECT_ATTRIBUTES objattrs;
    UNICODE_STRING objname;
    HANDLE handle;
    ULONG len;

    InitializeObjectAttributes(
        &objattrs, &objname, OBJ_CASE_INSENSITIVE, NULL, NULL);

    RtlInitUnicodeString(&objname, Support_SbieSvcKeyPath);

    status = NtOpenKey(&handle, KEY_READ, &objattrs);
    if (!NT_SUCCESS(status))
        return FALSE;

    RtlInitUnicodeString(&objname, name);

    status = NtQueryValueKey(
        handle, &objname, KeyValuePartialInformation,
        kvpi, sizeof_kvpi, &len);

    NtClose(handle);

    if (!NT_SUCCESS(status))
        return FALSE;

    return TRUE;
}


WCHAR* SbieDll_FormatMessage0(ULONG code)
{
    return SbieDll_FormatMessage(code, NULL);
}

WCHAR* SbieDll_FormatMessage(ULONG code, const WCHAR** ins) 
{
    static HMODULE SbieMsgDll = NULL;
    const ULONG FormatFlags = FORMAT_MESSAGE_FROM_HMODULE |
        FORMAT_MESSAGE_ARGUMENT_ARRAY |
        FORMAT_MESSAGE_ALLOCATE_BUFFER;
    WCHAR* out;
    ULONG rc;
    ULONG err = GetLastError();
    //获取 SbieMsg.dll 的句柄
    if (!SbieMsgDll) 
    {
        STARTUPINFOW si;
        if (SbieDll_RunFromHome(SBIEMSG_DLL, NULL, &si, NULL)) 
        {
            WCHAR* path2 = (WCHAR*)si.lpReserved;
            SbieMsgDll =
                LoadLibraryEx(path2, NULL, LOAD_LIBRARY_AS_DATAFILE);
            HeapFree(GetProcessHeap(), HEAP_GENERATE_EXCEPTIONS, path2);
        }
        if (!SbieMsgDll) 
        {
        
        }
    }
    //尝试翻译字符串
    rc = 0;
    out = NULL;

    if (SbieMsgDll) 
    {
        rc = FormatMessage(FormatFlags, SbieMsgDll, code,
            SbieDll_GetLanguage(NULL),
            (LPWSTR)&out, 4, (va_list*)ins);
        if (rc != 0) 
        {
            ULONG xrc = SbieDll_FormatMessage_2(&out, ins);
            if (xrc) 
            {
                rc = xrc;
            }
        }
    }
    if (rc == 0) 
    {
    
    }
    if (out) 
    {
        if (out[rc - 1] == L'\r' || out[rc - 1] == L'\n') {
            out[rc - 1] = L'\0';
            --rc;
        }
        if (out[rc - 1] == L'\r' || out[rc - 1] == L'\n') {
            out[rc - 1] = L'\0';
            --rc;
        }
    }
    SetLastError(err);
    return out;
}


ULONG SbieDll_GetLanguage(BOOLEAN* rtl) 
{
    static ULONG lang = 0;
    union {
        KEY_VALUE_PARTIAL_INFORMATION info;
        WCHAR space[32];
    } u;
    if (!lang) 
    {
    
    }
    if (rtl) 
    {
    
    }
    return lang;
}




ULONG SbieDll_FormatMessage_2(WCHAR** text_ptr, const WCHAR** ins) 
{
    //对于从右到左的语言文本文件（希伯来语和阿拉伯语），百分号可能会使在记事本中编辑文本文件变得困难。 作为解决方法，除了 %N 作为 FormatMessage 的参数外，还支持 .N.。 此解决方法仅供希伯来语和阿拉伯语文本文件使用。
    const ULONG FormatFlags = FORMAT_MESSAGE_FROM_STRING |
        FORMAT_MESSAGE_ARGUMENT_ARRAY |
        FORMAT_MESSAGE_ALLOCATE_BUFFER;
    const WCHAR* _x2 = L".2.";
    const WCHAR* _x3 = L".3.";
    const WCHAR* _x4 = L".4.";
    WCHAR* oldtxt, * newtxt, * ptr;
    ULONG rc;

    oldtxt = *text_ptr;
    ptr = wcsstr(oldtxt, _x2);
    if (!ptr)
        return 0;
    //如果 .2.在插入而不是实际消息中，然后退出。 这并不理想，但也不常见。（已在版本 4.05 Build 012 中修复）
    if (ins[1] && wcsstr(ins[1], _x2))
        return 0;
    if (ins[2] && wcsstr(ins[2], _x2))
        return 0;

    newtxt = LocalAlloc(LMEM_FIXED, (wcslen(oldtxt) + 1) * sizeof(WCHAR));
    if (!newtxt)
        return 0;
    wcscpy(newtxt, oldtxt);
    ptr = newtxt + (ptr - oldtxt);

    while (ptr) {
        ptr[0] = L'%';
        wmemmove(ptr + 2, ptr + 3, wcslen(ptr + 3) + 1);
        ptr = wcsstr(newtxt, _x2);
        if (!ptr)
            ptr = wcsstr(newtxt, _x3);
        if (!ptr)
            ptr = wcsstr(newtxt, _x4);
    }

    rc = FormatMessage(FormatFlags, newtxt, 0, 0,
        (LPWSTR)&ptr, 4, (va_list*)ins);
    if (rc != 0) {
        *text_ptr = ptr;
        LocalFree(oldtxt);
    }

    LocalFree(newtxt);
    return rc;
}