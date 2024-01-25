#include"dll.h"
#include"../svc/sbieiniwire.h"
#include"../common/my_version.h"

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
                    
                    }
                }
            }
        }
    }
}
