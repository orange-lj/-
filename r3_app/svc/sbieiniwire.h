#pragma once
#include "msgids.h"


struct tagSBIE_INI_GET_VERSION_REQ
{
    MSG_HEADER h;
};

struct tagSBIE_INI_GET_VERSION_RPL
{
    MSG_HEADER h;       // status is STATUS_SUCCESS or STATUS_UNSUCCESSFUL
    ULONG abi_ver;
    WCHAR version[1];
};


typedef struct tagSBIE_INI_GET_VERSION_REQ SBIE_INI_GET_VERSION_REQ;
typedef struct tagSBIE_INI_GET_VERSION_RPL SBIE_INI_GET_VERSION_RPL;


struct tagSBIE_INI_GET_USER_REQ
{
    MSG_HEADER h;
};

struct tagSBIE_INI_GET_USER_RPL
{
    MSG_HEADER h;       // status is STATUS_SUCCESS or STATUS_UNSUCCESSFUL
    BOOLEAN admin;
    WCHAR section[BOXNAME_COUNT];
    ULONG name_len;
    WCHAR name[1];
};

typedef struct tagSBIE_INI_GET_USER_REQ SBIE_INI_GET_USER_REQ;
typedef struct tagSBIE_INI_GET_USER_RPL SBIE_INI_GET_USER_RPL;