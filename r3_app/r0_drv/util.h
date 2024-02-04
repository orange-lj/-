#pragma once
#include"main.h"

NTSTATUS GetRegString(ULONG RelativeTo, const WCHAR* Path, const WCHAR* ValueName, UNICODE_STRING* pData);

//如果当前进程是我们的助手服务进程，则返回TRUE
BOOLEAN MyIsCallerMyServiceProcess(void);


//如果当前进程以LocalSystem帐户运行，则返回TRUE
BOOLEAN MyIsCurrentProcessRunningAsLocalSystem(void);

//如果当前进程具有有效的自定义签名，则返回TRUE
BOOLEAN MyIsCallerSigned(void);

//返回当前进程的终端服务会话id
NTSTATUS MyGetSessionId(ULONG* SessionId);

//NTSTATUS MyValidateCertificate(void);