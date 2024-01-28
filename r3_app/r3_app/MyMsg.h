#pragma once
#include"../r0_drv/msgs.h"

class CMyMsg : public CString
{
	static const CString& m_unknown;
	void Construct(WCHAR* str);
public:

	CMyMsg(ULONG msgid);
};

