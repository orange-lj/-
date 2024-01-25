#pragma once
#include"r3_appDlg.h"
class CInitWait :public Cr3appDlg
{
	DECLARE_MESSAGE_MAP()
	CString m_app_ver;
	CString m_svc_ver;
	ULONG   m_svc_abi;
	CString m_drv_ver;
	ULONG   m_drv_abi;

	void GetVersions();
public:
	CInitWait(Cr3appDlg* myApp);
	~CInitWait();
};

