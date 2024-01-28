#include<stdio.h>
#include<stdlib.h>
#include<windows.h>
#include"../common/my_version.h"
#include"DriverAssist.h"
#include"PipeServer.h"
#include"ProcessServer.h"
#include"SbieIniServer.h"
#include"ServiceServer.h"
#include"PStoreServer.h"
#include"TerminalServer.h"
#include"NamedPipeServer.h"
#include"FileServer.h"
#include"ComServer.h"
#include"IpHlpServer.h"
#include"NetApiServer.h"
#include"QueueServer.h"
#include"MountManager.h"
#include"EpMapperServer.h"
#include"../Dll/sbiedll.h"

//����
static WCHAR* ServiceName = SBIESVC;
static SERVICE_STATUS        ServiceStatus;
static SERVICE_STATUS_HANDLE ServiceStatusHandle = NULL;
static ComServer* pComServer = NULL;
static HANDLE                EventLog = NULL;
extern "C" {
    const  ULONG                 tzuk = 'xobs';
}
HMODULE               _Ntdll = NULL;
HMODULE               _Kernel32 = NULL;
SYSTEM_INFO           _SystemInfo;


DWORD InitializePipe(void);
DWORD WINAPI ServiceHandlerEx(
    DWORD dwControl, DWORD dwEventType, LPVOID lpEventData, LPVOID lpContext);

DWORD InitializeEventLog(void)
{
    EventLog = OpenEventLog(NULL, ServiceName);
    return 0;
}

void WINAPI ServiceMain(DWORD argc, WCHAR* argv[]) 
{
    ServiceStatusHandle = RegisterServiceCtrlHandlerEx(ServiceName, ServiceHandlerEx, NULL);
    if (!ServiceStatusHandle)
        return;
    ServiceStatus.dwServiceType = SERVICE_WIN32;
    ServiceStatus.dwCurrentState = SERVICE_START_PENDING;
    ServiceStatus.dwControlsAccepted = SERVICE_ACCEPT_STOP | SERVICE_ACCEPT_SHUTDOWN;
    ServiceStatus.dwWin32ExitCode = 0;
    ServiceStatus.dwServiceSpecificExitCode = 0;
    ServiceStatus.dwCheckPoint = 1;
    ServiceStatus.dwWaitHint = 6000;

    ULONG status = 0;
    if (!SetServiceStatus(ServiceStatusHandle, &ServiceStatus))
        status = GetLastError();
    if (status == 0) 
    {
        status = InitializeEventLog();
    }
    if (status == 0) 
    {
        bool ok = DriverAssist::Initialize();
        if (!ok) 
        {
            status = 0x1234;
        }
    }
    if (status == 0) 
    {
        status = InitializePipe();
        SbieDll_DisableCHPE();
    }
}

int WinMain(HINSTANCE hInstance,HINSTANCE hPrevInstance,LPSTR lpCmdLine,int nCmdShow) 
{
    Sleep(7000);
	SERVICE_TABLE_ENTRY myServiceTable[] =
	{
		{ServiceName,ServiceMain},
		{NULL,NULL}
	};
    _Ntdll = GetModuleHandle(L"ntdll.dll");
    _Kernel32 = GetModuleHandle(L"kernel32.dll");
    GetSystemInfo(&_SystemInfo);

    WCHAR* cmdline = GetCommandLine();
    if (cmdline) {

        WCHAR* cmdline2 = wcsstr(cmdline, SANDBOXIE L"_ComProxy");
        if (cmdline2) {
            //ComServer::RunSlave(cmdline2);
            return NO_ERROR;
        }

        WCHAR* cmdline3 = wcsstr(cmdline, SANDBOXIE L"_UacProxy");
        if (cmdline3) {
            //ServiceServer::RunUacSlave(cmdline3);
            return NO_ERROR;
        }

        WCHAR* cmdline4 = wcsstr(cmdline, SANDBOXIE L"_NetProxy");
        if (cmdline4) {
            //NetApiServer::RunSlave(cmdline4);
            return NO_ERROR;
        }

        WCHAR* cmdline5 = wcsstr(cmdline, SANDBOXIE L"_GuiProxy");
        if (cmdline5) {
            //GuiServer::RunSlave(cmdline5);
            return NO_ERROR;
        }
    }
    if (!StartServiceCtrlDispatcher(myServiceTable))
        return GetLastError();
    return NO_ERROR;
}

DWORD WINAPI ServiceHandlerEx(
    DWORD dwControl, DWORD dwEventType, LPVOID lpEventData, LPVOID lpContext) 
{
    return 0;
}

DWORD InitializePipe(void) 
{
    //���йܵ��������������ӷ�����
    PipeServer* pipeServer = PipeServer::GetPipeServer();
    if (!pipeServer)
        return (0x00300000 + GetLastError());
    new ProcessServer(pipeServer);
    new ProcessServer(pipeServer);
    new SbieIniServer(pipeServer);
    new ServiceServer(pipeServer);
    new PStoreServer(pipeServer);
    new TerminalServer(pipeServer);
    new NamedPipeServer(pipeServer);
    new FileServer(pipeServer);
    pComServer = new ComServer(pipeServer);
    new IpHlpServer(pipeServer);
    new NetApiServer(pipeServer);
    new QueueServer(pipeServer);
    new MountManager(pipeServer);
    new EpMapperServer(pipeServer);

    if (!pipeServer->Start())
        return (0x00200000 + GetLastError());
    return 0;
}