#pragma once
//#include"pool.h"
#include"main.h"

typedef struct _IPC_DYNAMIC_PORT {
    LIST_ELEM list_elem;

    WCHAR       wstrPortId[DYNAMIC_PORT_ID_CHARS];
    WCHAR       wstrPortName[DYNAMIC_PORT_NAME_CHARS];
} IPC_DYNAMIC_PORT;


typedef struct _IPC_DYNAMIC_PORTS {
    PERESOURCE  pPortLock;

    LIST        Ports;

    IPC_DYNAMIC_PORT* pSpoolerPort;
} IPC_DYNAMIC_PORTS;

extern IPC_DYNAMIC_PORTS Ipc_Dynamic_Ports;

BOOLEAN Ipc_Init(void);