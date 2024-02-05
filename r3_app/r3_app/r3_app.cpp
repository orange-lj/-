
// r3_app.cpp: 定义应用程序的类行为。
//

#include "pch.h"
#include "framework.h"
#include "r3_app.h"
#include "r3_appDlg.h"
#include"Boxes.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// Cr3appApp

BEGIN_MESSAGE_MAP(Cr3appApp, CWinApp)
	ON_COMMAND(ID_HELP, &CWinApp::OnHelp)
END_MESSAGE_MAP()


// Cr3appApp 构造

Cr3appApp::Cr3appApp()
{
	// 支持重新启动管理器
	m_dwRestartManagerSupportFlags = AFX_RESTART_MANAGER_SUPPORT_RESTART;

	// TODO: 在此处添加构造代码，
	// 将所有重要的初始化放置在 InitInstance 中
}


// 唯一的 Cr3appApp 对象

Cr3appApp theApp;
bool Cr3appApp::m_Windows2000 = false;
bool Cr3appApp::m_WindowsVista = false;
ULONG Cr3appApp::m_session_id = 0;
ATOM Cr3appApp::m_atom = NULL;
CString Cr3appApp::m_appTitle;
// Cr3appApp 初始化

BOOL Cr3appApp::InitInstance()
{
	// 如果一个运行在 Windows XP 上的应用程序清单指定要
	// 使用 ComCtl32.dll 版本 6 或更高版本来启用可视化方式，
	//则需要 InitCommonControlsEx()。  否则，将无法创建窗口。
	INITCOMMONCONTROLSEX InitCtrls;
	InitCtrls.dwSize = sizeof(InitCtrls);
	// 将它设置为包括所有要在应用程序中使用的
	// 公共控件类。
	InitCtrls.dwICC = ICC_WIN95_CLASSES;
	InitCommonControlsEx(&InitCtrls);

	CWinApp::InitInstance();


	AfxEnableControlContainer();

	// 创建 shell 管理器，以防对话框包含
	// 任何 shell 树视图控件或 shell 列表视图控件。
	CShellManager *pShellManager = new CShellManager;

	// 激活“Windows Native”视觉管理器，以便在 MFC 控件中启用主题
	CMFCVisualManager::SetDefaultManager(RUNTIME_CLASS(CMFCVisualManagerWindows));

	// 标准初始化
	// 如果未使用这些功能并希望减小
	// 最终可执行文件的大小，则应移除下列
	// 不需要的特定初始化例程
	// 更改用于存储设置的注册表项
	// TODO: 应适当修改该字符串，
	// 例如修改为公司或组织名
	SetRegistryKey(_T("应用程序向导生成的本地应用程序"));
	static const WCHAR* WindowClassName = SANDBOXIE_CONTROL L"WndClass";
	BOOL ForceVisible = FALSE;
	BOOL ForceSync = FALSE;
	BOOL PostSetup = FALSE;
	WCHAR* CommandLine = GetCommandLine();
	if (CommandLine)
	{
		if (wcsstr(CommandLine, L"/open"))
			ForceVisible = TRUE;
		if (wcsstr(CommandLine, L"/sync"))
			ForceSync = TRUE;
		if (wcsstr(CommandLine, L"/postsetup"))
			PostSetup = TRUE;
		if (wcsstr(CommandLine, L"/uninstall"))
		{
			//CShellDialog::Sync(TRUE);
			return TRUE;
		}
	}

	//如果应用程序互斥已存在，则提前中止
	WCHAR* InstanceMutexName = SANDBOXIE L"_SingleInstanceMutex_Control";
	HANDLE hInstanceMutex =
		OpenMutex(MUTEX_ALL_ACCESS, FALSE, InstanceMutexName);
	if (hInstanceMutex)
	{
		//if (ForceVisible) 
		//{
		//	HWND hwnd = FindWindow(WindowClassName, NULL);
		//	if (hwnd) {
		//		ShowWindow(hwnd, SW_SHOWNORMAL);
		//		SetForegroundWindow(hwnd);
		//	}
		//}
		//return FALSE;
	}
	hInstanceMutex = CreateMutex(NULL, FALSE, InstanceMutexName);
	if (!hInstanceMutex)
		return FALSE;


	WCHAR* home_path = (WCHAR*)HeapAlloc(
		GetProcessHeap(), HEAP_GENERATE_EXCEPTIONS, 1024 * sizeof(WCHAR));
	SbieApi_GetHomePath(NULL, 0, home_path, 1020);
	SetCurrentDirectory(home_path);
	HeapFree(GetProcessHeap(), HEAP_GENERATE_EXCEPTIONS, home_path);


	//检查Windows XP API是否可用
	OSVERSIONINFO osvi;
	memzero(&osvi, sizeof(OSVERSIONINFO));
	osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
	GetVersionEx((OSVERSIONINFO*)&osvi);
	if (osvi.dwMajorVersion == 5 && osvi.dwMinorVersion == 0)
		m_Windows2000 = true;
	if (osvi.dwMajorVersion >= 6)
		m_WindowsVista = true;

	//初始化misc
	CoInitialize(NULL);

	m_appTitle = "huyue";

	if (!ProcessIdToSessionId(GetCurrentProcessId(), &m_session_id))
		m_session_id = 0;
	SbieDll_GetDrivePath(-1);

	//注册主窗口类
	WNDCLASSEX wc;
	wc.cbSize = sizeof(WNDCLASSEX);
	wc.style = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS | CS_GLOBALCLASS;
	wc.lpfnWndProc = ::DefWindowProc;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hInstance = AfxGetInstanceHandle();
	wc.hIcon = NULL;
	wc.hCursor = ::LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground = (HBRUSH)(COLOR_3DFACE + 1);
	wc.lpszMenuName = NULL;
	wc.lpszClassName = WindowClassName;
	wc.hIconSm = NULL;

	m_atom = RegisterClassEx(&wc);
	if (!m_atom) 
	{
		int a = GetLastError();
		return FALSE;
	}
	//等待Sandboxie完成初始化
	CInitWait initwait(this);

	//接任Sandboxie会话负责人
	SbieApi_SessionLeader(0, NULL);


	//刷新进程
	Boxes::GetInstance().RefreshProcesses();



	//Cr3appDlg dlg;
	//m_pMainWnd = &dlg;
	//INT_PTR nResponse = dlg.DoModal();
	//if (nResponse == IDOK)
	//{
	//	// TODO: 在此放置处理何时用
	//	//  “确定”来关闭对话框的代码
	//}
	//else if (nResponse == IDCANCEL)
	//{
	//	// TODO: 在此放置处理何时用
	//	//  “取消”来关闭对话框的代码
	//}
	//else if (nResponse == -1)
	//{
	//	TRACE(traceAppMsg, 0, "警告: 对话框创建失败，应用程序将意外终止。\n");
	//	TRACE(traceAppMsg, 0, "警告: 如果您在对话框上使用 MFC 控件，则无法 #define _AFX_NO_MFC_CONTROLS_IN_DIALOGS。\n");
	//}

	// 删除上面创建的 shell 管理器。
	if (pShellManager != nullptr)
	{
		delete pShellManager;
	}

#if !defined(_AFXDLL) && !defined(_AFX_NO_MFC_CONTROLS_IN_DIALOGS)
	ControlBarCleanUp();
#endif

	// 由于对话框已关闭，所以将返回 FALSE 以便退出应用程序，
	//  而不是启动应用程序的消息泵。
	return FALSE;
}

