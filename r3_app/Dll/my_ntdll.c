#include <ntstatus.h>
#define WIN32_NO_STATUS
typedef long NTSTATUS;

#include <windows.h>
#include "../common/win32_ntddk.h"

//��Ҫ��sboxdll��SbieDll.dll����Ŀ�а����κ��ⲿCRT������
//���DLL����ǰע�뵽�������������У������������ܻ��ƻ�Sandboxie��ͨ��������ֱ�����ӵ�ntdll.dll�е�CRT������
//����ntdll.lib�����°汾���ṩ���CRT���������Ǳ��봴���Լ���lib���йغ�������ֱ�μ�NtCRT_x64.def��NtCRT_x86.def����������Ӧ��def�ļ���ʹ���Զ��幹�����衰lib/def:%��FullPath��/out:$��SolutionDir��Bin\$��PlatformName��\$��Configuration��\NtCRT.lib/machine:x64��������x86������ʹ�ÿ���/machine:x86��
//����x86���������ǻ���Ҫ_except_handler3�������Ѿ���except_handler3.asm�����´�������
//���ߣ����ǿ��Դ�InitMyNtDll��̬����������Ҫ�ĺ���
int(*P_vsnwprintf)(wchar_t* _Buffer, size_t Count, const wchar_t* const, va_list Args) = NULL;
int(*P_vsnprintf)(char* _Buffer, size_t Count, const char* const, va_list Args) = NULL;

void InitMyNtDll(HMODULE Ntdll)
{
	*(FARPROC*)&P_vsnwprintf = GetProcAddress(Ntdll, "_vsnwprintf");
	*(FARPROC*)&P_vsnprintf = GetProcAddress(Ntdll, "_vsnprintf");
}
