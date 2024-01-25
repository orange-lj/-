#include "pch.h"
#include "InitWait.h"
#include"../common/my_version.h"
#include"../svc/sbieiniwire.h"
#include"../Dll/sbiedll.h"

BEGIN_MESSAGE_MAP(CInitWait, Cr3appDlg)

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

CInitWait::CInitWait(Cr3appDlg* myApp)
{
	//����ʱ�����汾����������ͷ�����бȽ�
	m_app_ver.Format(L"%S", MY_VERSION_STRING);
	m_svc_ver = L"?";
	m_svc_abi = 0;
	m_drv_ver = L"?";
	m_drv_abi = 0;

	GetVersions();
	if (m_svc_abi == MY_ABI_VERSION && m_drv_abi == MY_ABI_VERSION)
		return;
	//������ǵ����������ζ�ŷ������������û��׼���ã����Դ������ں�����ͼ���Եȴ�
	/*CreateEx(0, (LPCTSTR)Cr3appDlg::m_atom, Cr3appDlg::m_appTitle,
		WS_OVERLAPPEDWINDOW | WS_CAPTION | WS_SYSMENU,
		0, 0, 0, 0, NULL, NULL, NULL);*/

	// start SbieSvc
	SbieDll_StartSbieSvc(FALSE);
}

CInitWait::~CInitWait()
{
}
