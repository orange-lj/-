#pragma once
#include"r3_appDlg.h"

bool temp = true;
class CInitWait :public CWnd
{
	DECLARE_MESSAGE_MAP()
	CString m_app_ver;
	CString m_svc_ver;
	ULONG   m_svc_abi;
	CString m_drv_ver;
	ULONG   m_drv_abi;
	BOOL m_try_elevate;
	void GetVersions();
	afx_msg void OnTimer(UINT_PTR nIDEvent);
public:
	CInitWait(CWinApp* myApp);
	~CInitWait();
};

