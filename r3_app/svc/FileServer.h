#pragma once
#include "PipeServer.h"

class FileServer
{
public:

    FileServer(PipeServer* pipeServer);
protected:

    static MSG_HEADER* Handler(void* _this, MSG_HEADER* msg);
    void* MyAlloc(ULONG len);
protected:

    WCHAR* m_windows;
    WCHAR* m_winsxs;
    PSECURITY_DESCRIPTOR m_PublicSd;
    HANDLE m_heap;
};

