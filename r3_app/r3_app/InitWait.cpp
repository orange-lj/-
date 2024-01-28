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
		
		}
		if (!init_complete)
			return;
	}
}

void CInitWait::OnTimer(UINT_PTR nIDEvent)
{
	GetVersions();
}

CInitWait::CInitWait(CWinApp* myApp)
{
	//输入时，将版本与驱动程序和服务进行比较
	m_app_ver.Format(L"%S", MY_VERSION_STRING);
	m_svc_ver = L"?";
	m_svc_abi = 0;
	m_drv_ver = L"?";
	m_drv_abi = 0;

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
