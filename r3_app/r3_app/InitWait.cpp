#include "pch.h"
#include "InitWait.h"
#include"../common/my_version.h"
#include"../svc/sbieiniwire.h"
#include"../Dll/sbiedll.h"
#include"r3_app.h"

#define ID_TIMER    1003

BEGIN_MESSAGE_MAP(CInitWait, CWnd)
	ON_WM_TIMER()
END_MESSAGE_MAP()

void CInitWait::GetVersions()
{
	BOOL fail = FALSE;
	WCHAR drv_ver[16];
	if (1) 
	{
		bool init_complete = false;
		MSG_HEADER req;
		req.length = sizeof(MSG_HEADER);
		req.msgid = MSGID_PROCESS_CHECK_INIT_COMPLETE;
		MSG_HEADER* rpl = SbieDll_CallServer(&req);
		if (rpl) 
		{
			if (rpl->status == 0) 
			{
				init_complete = true;
			}
			SbieDll_FreeMem(rpl);
		}
		if (!init_complete)
			return;
	}
	if (m_svc_ver.GetAt(0) == L'?') 
	{
		SBIE_INI_GET_VERSION_REQ req;
		req.h.length = sizeof(SBIE_INI_GET_VERSION_REQ);
		req.h.msgid = MSGID_SBIE_INI_GET_VERSION;
		SBIE_INI_GET_VERSION_RPL* rpl =
			(SBIE_INI_GET_VERSION_RPL*)SbieDll_CallServer(&req.h);
		if (rpl) 
		{
			if (rpl->h.status == 0 && rpl->version[0]) 
			{
				m_svc_ver = rpl->version;
				m_svc_abi = rpl->abi_ver;
				if (m_svc_abi != MY_ABI_VERSION)
					fail = TRUE;
			}
			SbieDll_FreeMem(rpl);
		}
	}
	if (m_drv_ver.GetAt(0) == L'?') {

		SbieApi_GetVersionEx(drv_ver, &m_drv_abi);
		if (drv_ver[0] && _wcsicmp(drv_ver, L"unknown") != 0) {
			m_drv_ver = drv_ver;
			if (m_drv_abi != MY_ABI_VERSION)
				fail = TRUE;
		}
	}

	if (fail) {
		//CMyMsg msg(MSG_3304, m_app_ver, m_svc_ver, m_drv_ver);
		//CMyApp::MsgBox(NULL, msg, MB_OK);
		exit(0);
	}
}

void CInitWait::OnTimer(UINT_PTR nIDEvent)
{
	GetVersions();
	if (m_svc_abi == MY_ABI_VERSION && m_drv_abi == MY_ABI_VERSION) 
	{
	
	}
	else if (m_try_elevate) 
	{
		//在Windows Vista上，提升以启动服务
		const WCHAR* StartError = SbieDll_GetStartError();
		if (StartError && wcsstr(StartError, L"[22 / 5]")) 
		{
		
		}
	}
}

CInitWait::CInitWait(CWinApp* myApp)
{
	//输入时，将版本与驱动程序和服务进行比较
	m_app_ver.Format(L"%S", MY_VERSION_STRING);
	m_svc_ver = L"?";
	m_svc_abi = 0;
	m_drv_ver = L"?";
	m_drv_abi = 0;
	m_try_elevate = Cr3appApp::m_WindowsVista;
	GetVersions();
	if (m_svc_abi == MY_ABI_VERSION && m_drv_abi == MY_ABI_VERSION)
		return;
	//如果我们到了这里，这意味着服务或驱动程序还没有准备好，所以创建窗口和托盘图标以等待
	CreateEx(0, (LPCTSTR)Cr3appApp::m_atom, Cr3appApp::m_appTitle,
		WS_OVERLAPPEDWINDOW | WS_CAPTION | WS_SYSMENU,
		0, 0, 0, 0, NULL, NULL, NULL);
	myApp->m_pMainWnd = this;
	SetTimer(ID_TIMER, 1000, NULL);

	// start SbieSvc
	SbieDll_StartSbieSvc(FALSE);

	//临时消息循环，直到我们初始化
	while (1) 
	{
		MSG msg;
		BOOL b = ::GetMessage(&msg, NULL, 0, 0);
		if (!b) 
		{
			break;
		}
		::TranslateMessage(&msg);
		::DispatchMessage(&msg);
	}
	if (m_svc_abi != MY_ABI_VERSION || m_drv_abi != MY_ABI_VERSION)
		exit(0);
	KillTimer(ID_TIMER);
}

CInitWait::~CInitWait()
{
}
