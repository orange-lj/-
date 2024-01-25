
// r3_appDlg.cpp: 实现文件
//

#include "pch.h"
#include "framework.h"
#include "r3_app.h"
#include "r3_appDlg.h"
#include "afxdialogex.h"
#include"../Dll/sbieapi.h"
#include"../common/my_version.h"
#include"../common/defines.h"
#include"InitWait.h"
#pragma comment(lib,"Dll.lib")

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

bool Cr3appDlg::m_Windows2000 = false;
bool Cr3appDlg::m_WindowsVista = false;
ULONG Cr3appDlg::m_session_id = 0;

// 用于应用程序“关于”菜单项的 CAboutDlg 对话框

class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();

// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_ABOUTBOX };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

// 实现
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialogEx(IDD_ABOUTBOX)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()


// Cr3appDlg 对话框



Cr3appDlg::Cr3appDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_R3_APP_DIALOG, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void Cr3appDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(Cr3appDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
END_MESSAGE_MAP()


// Cr3appDlg 消息处理程序

BOOL Cr3appDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// 将“关于...”菜单项添加到系统菜单中。

	// IDM_ABOUTBOX 必须在系统命令范围内。
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != nullptr)
	{
		BOOL bNameValid;
		CString strAboutMenu;
		bNameValid = strAboutMenu.LoadString(IDS_ABOUTBOX);
		ASSERT(bNameValid);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// 设置此对话框的图标。  当应用程序主窗口不是对话框时，框架将自动
	//  执行此操作
	SetIcon(m_hIcon, TRUE);			// 设置大图标
	SetIcon(m_hIcon, FALSE);		// 设置小图标

	// TODO: 在此添加额外的初始化代码
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
	/*BOOLEAN LayoutRTL;
	SbieDll_GetLanguage(&LayoutRTL);
	if (LayoutRTL)
		m_LayoutRTL = true;

	m_appTitle = CMyMsg(MSG_3301);*/
	if (!ProcessIdToSessionId(GetCurrentProcessId(), &m_session_id))
		m_session_id = 0;
	SbieDll_GetDrivePath(-1);
	//初始化图形元素----------------------看情况补充
	//等待Sandboxie完成初始化
	CInitWait initwait(this);

	return TRUE;  // 除非将焦点设置到控件，否则返回 TRUE
}

void Cr3appDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialogEx::OnSysCommand(nID, lParam);
	}
}

// 如果向对话框添加最小化按钮，则需要下面的代码
//  来绘制该图标。  对于使用文档/视图模型的 MFC 应用程序，
//  这将由框架自动完成。

void Cr3appDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // 用于绘制的设备上下文

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// 使图标在工作区矩形中居中
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// 绘制图标
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

//当用户拖动最小化窗口时系统调用此函数取得光标
//显示。
HCURSOR Cr3appDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

