#include <ntstatus.h>
#define WIN32_NO_STATUS
typedef long NTSTATUS;

#include <windows.h>
#include "../common/win32_ntddk.h"

//不要在sboxdll（SbieDll.dll）项目中包含任何外部CRT！！！
//这个DLL被提前注入到进程启动序列中，添加依赖项可能会破坏Sandboxie。通常，我们直接链接到ntdll.dll中的CRT构建。
//由于ntdll.lib的最新版本不提供许多CRT函数，我们必须创建自己的lib。有关函数，请分别参见NtCRT_x64.def和NtCRT_x86.def，我们在相应的def文件上使用自定义构建步骤“lib/def:%（FullPath）/out:$（SolutionDir）Bin\$（PlatformName）\$（Configuration）\NtCRT.lib/machine:x64”，对于x86，我们使用开关/machine:x86。
//对于x86构建，我们还需要_except_handler3，我们已经在except_handler3.asm中重新创建了它
//或者，我们可以从InitMyNtDll动态链接所有需要的函数
int(*P_vsnwprintf)(wchar_t* _Buffer, size_t Count, const wchar_t* const, va_list Args) = NULL;
int(*P_vsnprintf)(char* _Buffer, size_t Count, const char* const, va_list Args) = NULL;

void InitMyNtDll(HMODULE Ntdll)
{
	*(FARPROC*)&P_vsnwprintf = GetProcAddress(Ntdll, "_vsnwprintf");
	*(FARPROC*)&P_vsnprintf = GetProcAddress(Ntdll, "_vsnprintf");
}
