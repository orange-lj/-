#pragma once
#include"main.h"

NTSTATUS GetRegString(ULONG RelativeTo, const WCHAR* Path, const WCHAR* ValueName, UNICODE_STRING* pData);

//�����ǰ���������ǵ����ַ�����̣��򷵻�TRUE
BOOLEAN MyIsCallerMyServiceProcess(void);


//�����ǰ������LocalSystem�ʻ����У��򷵻�TRUE
BOOLEAN MyIsCurrentProcessRunningAsLocalSystem(void);

//�����ǰ���̾�����Ч���Զ���ǩ�����򷵻�TRUE
BOOLEAN MyIsCallerSigned(void);