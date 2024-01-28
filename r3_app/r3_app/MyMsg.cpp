#include "pch.h"
#include "MyMsg.h"
#include"../Dll/sbiedll.h"

const CString& CMyMsg::m_unknown = CString(L"???");

void CMyMsg::Construct(WCHAR* str)
{
    if (str) {
        CString::operator=(str);
        LocalFree(str);
    }
    else
        CString::operator=(m_unknown);
}

CMyMsg::CMyMsg(ULONG msgid)
{
	Construct(SbieDll_FormatMessage0(msgid));
}
