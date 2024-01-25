#include "session.h"
#include"mem.h"
#include"api.h"

//±‰¡ø
static LIST Session_List;
PERESOURCE Session_ListLock = NULL;

BOOLEAN Session_Init(void)
{
    List_Init(&Session_List);

    if (!Mem_GetLockResource(&Session_ListLock, TRUE))
        return FALSE;
    //Api_SetFunction(API_SESSION_LEADER, Session_Api_Leader);
    //Api_SetFunction(API_DISABLE_FORCE_PROCESS, Session_Api_DisableForce);
    //Api_SetFunction(API_MONITOR_CONTROL, Session_Api_MonitorControl);
    ////Api_SetFunction(API_MONITOR_PUT,            Session_Api_MonitorPut);
    //Api_SetFunction(API_MONITOR_PUT2, Session_Api_MonitorPut2);
    ////Api_SetFunction(API_MONITOR_GET,            Session_Api_MonitorGet);
    //Api_SetFunction(API_MONITOR_GET_EX, Session_Api_MonitorGetEx);
    //Api_SetFunction(API_MONITOR_GET2, Session_Api_MonitorGet2);
    return TRUE;
}
