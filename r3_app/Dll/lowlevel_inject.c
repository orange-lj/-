#include"sbiedll.h"
#include"dll.h"
#include"../low/lowdata.h"
#include"../common/my_version.h"
#include"../r0_drv/api_defs.h"
#include"resource.h"
//变量
void* m_sbielow_ptr = NULL;
ULONG m_sbielow_len = 0;
//增加两个偏移量变量来替换“头”和“尾”的依赖关系
ULONG m_sbielow_start_offset = 0;
ULONG m_sbielow_data_offset = 0;
ULONG* m_syscall_data = NULL;
void* m_sbielow32_ptr = NULL;
ULONG m_sbielow32_len = 0;
ULONG m_sbielow32_detour_offset = 0;

ULONG_PTR m_LdrInitializeThunk = 0;

typedef struct _MY_TARGETS {
    unsigned long long entry;
    unsigned long long data;
    unsigned long long detour;
} MY_TARGETS;

ULONG SbieDll_InjectLow_LoadLow(BOOLEAN arch_64bit, void** sbielow_ptr, ULONG* sbielow_len, ULONG* start_offset, ULONG* data_offset, ULONG* detour_offset) 
{
	//锁定SbieLow资源（嵌入在SbieSvc可执行文件中，//请参阅lowlevel.rc）并找到可执行代码的偏移量和长度
    IMAGE_DOS_HEADER* dos_hdr = 0;
    IMAGE_NT_HEADERS* nt_hdrs = 0;
    IMAGE_SECTION_HEADER* section = 0;
    IMAGE_DATA_DIRECTORY* data_dirs = 0;
    ULONG_PTR imageBase = 0;
    MY_TARGETS* targets = 0;

    ULONG errlvl = 0x11;

    HRSRC hrsrc = FindResource(Dll_Instance, MAKEINTRESOURCE(IDR_TESTRES1), L"TESTRES");
    if (!hrsrc) 
    {
        int a = GetLastError();
        return errlvl;
    }
        

    ULONG binsize = SizeofResource(Dll_Instance, hrsrc);
    if (!binsize)
        return errlvl;

    HGLOBAL hglob = LoadResource(Dll_Instance, hrsrc);
    if (!hglob)
        return errlvl;

    UCHAR* bindata = (UCHAR*)LockResource(hglob);
    if (!bindata)
        return errlvl;

    errlvl = 0x22;
    dos_hdr = (IMAGE_DOS_HEADER*)bindata;

    if (dos_hdr->e_magic == 'MZ' || dos_hdr->e_magic == 'ZM') 
    {
        nt_hdrs = (IMAGE_NT_HEADERS*)((UCHAR*)dos_hdr + dos_hdr->e_lfanew);

        if (nt_hdrs->Signature != IMAGE_NT_SIGNATURE)   // 'PE\0\0'
            return errlvl;
        if (nt_hdrs->OptionalHeader.Magic != (arch_64bit ? IMAGE_NT_OPTIONAL_HDR64_MAGIC : IMAGE_NT_OPTIONAL_HDR32_MAGIC))
            return errlvl;

        if (!arch_64bit) 
        {
            IMAGE_NT_HEADERS32* nt_hdrs_32 = (IMAGE_NT_HEADERS32*)nt_hdrs;
            IMAGE_OPTIONAL_HEADER32* opt_hdr_32 = &nt_hdrs_32->OptionalHeader;
            data_dirs = &opt_hdr_32->DataDirectory[0];
            imageBase = opt_hdr_32->ImageBase;
        }
        else 
        {
            IMAGE_NT_HEADERS64* nt_hdrs_64 = (IMAGE_NT_HEADERS64*)nt_hdrs;
            IMAGE_OPTIONAL_HEADER64* opt_hdr_64 = &nt_hdrs_64->OptionalHeader;
            data_dirs = &opt_hdr_64->DataDirectory[0];
            imageBase = (ULONG_PTR)opt_hdr_64->ImageBase;
        }
    }
    ULONG zzzzz = 1;
    if (imageBase != 0) // x64 or x86
        return errlvl;
    section = IMAGE_FIRST_SECTION(nt_hdrs);
    if (nt_hdrs->FileHeader.NumberOfSections < 2) return errlvl;
    if (strncmp((char*)section[0].Name, SBIELOW_INJECTION_SECTION, strlen(SBIELOW_INJECTION_SECTION)) ||
        strncmp((char*)section[zzzzz].Name, SBIELOW_SYMBOL_SECTION, strlen(SBIELOW_SYMBOL_SECTION))) {
        return errlvl;
    }
    targets = (MY_TARGETS*)&bindata[section[zzzzz].PointerToRawData];
    if (start_offset) *start_offset = (ULONG)(targets->entry - imageBase - section[0].VirtualAddress);
    if (data_offset) *data_offset = (ULONG)(targets->data - imageBase - section[0].VirtualAddress);
    if (detour_offset) *detour_offset = (ULONG)(targets->detour - imageBase - section[0].VirtualAddress);

    *sbielow_ptr = bindata + section[0].PointerToRawData; //Old version: head;
    *sbielow_len = section[0].SizeOfRawData; //Old version: (ULONG)(ULONG_PTR)(tail - head);

    return 0;
}


ULONG SbieDll_InjectLow_InitHelper() 
{
	ULONG errlvl = SbieDll_InjectLow_LoadLow(TRUE, &m_sbielow_ptr, &m_sbielow_len, &m_sbielow_start_offset, &m_sbielow_data_offset, NULL);
    /*if (!errlvl)
        errlvl = SbieDll_InjectLow_LoadLow(FALSE, &m_sbielow32_ptr, &m_sbielow32_len, NULL, NULL, &m_sbielow32_detour_offset);*/
    if (errlvl)
        return errlvl;
    //记录有关ntdll和虚拟内存系统的信息
    errlvl = 0x33;
    m_LdrInitializeThunk = (ULONG_PTR)GetProcAddress(Dll_Ntdll, "LdrInitializeThunk");
    if (!m_LdrInitializeThunk)
        return errlvl;
    if (Dll_Windows >= 10) 
    {
        unsigned char* code;
        code = (unsigned char*)m_LdrInitializeThunk;
        if (*(ULONG*)&code[0] == 0x24048b48 && code[0xa] == 0x48) {
            m_LdrInitializeThunk += 0xa;
        }
    }
    return 0;
}


ULONG SbieDll_InjectLow_InitSyscalls(BOOLEAN drv_init) 
{
    const WCHAR* _SbieDll = L"\\" SBIEDLL L".dll";
    WCHAR sbie_home[512];
    ULONG status;
    ULONG len;
    SBIELOW_EXTRA_DATA* extra;
    WCHAR* ptr;
    ULONG* syscall_data;
    //获取SbieDll位置
    if (drv_init)
    {
        status = SbieApi_GetHomePath(NULL, 0, sbie_home, 512);
        if (status != 0)
            return status;
    }
    else
    {
        GetModuleFileName(Dll_Instance, sbie_home, 512);
        ptr = wcsrchr(sbie_home, L'\\');
        if (ptr)
            *ptr = L'\0';
    }
#define ULONG_DIFF(b,a) ((ULONG)((ULONG_PTR)(b) - (ULONG_PTR)(a)))
    //从驱动程序获取syscall的列表
    if (!m_syscall_data) {
        syscall_data = (ULONG*)HeapAlloc(GetProcessHeap(), 0, 8192);
        if (!syscall_data)
            return STATUS_INSUFFICIENT_RESOURCES;
        *syscall_data = 0;
    }
    else 
    {
        syscall_data = m_syscall_data;
    }
    if (drv_init)
    {
        //
        //从驱动程序获取完整的系统调用列表
        //

        status = SbieApi_Call(API_QUERY_SYSCALLS, 1, (ULONG_PTR)syscall_data);
        if (status != 0)
            return status;

        len = *syscall_data;
        if ((!len) || (len & 3) || (len > 4096))
            return STATUS_INVALID_IMAGE_FORMAT;
    }
    else
    {
        //
        //创建最小化的无人驾驶数据
        //

        *syscall_data = sizeof(ULONG) + sizeof(ULONG); // + 248; // total_size 4, extra_data_offset 4, work area sizeof(INJECT_DATA)

        const char* NtdllExports[] = NATIVE_FUNCTION_NAMES;
        for (ULONG i = 0; i < NATIVE_FUNCTION_COUNT; ++i) {
            void* func_ptr = GetProcAddress(Dll_Ntdll, NtdllExports[i]);
            memcpy((void*)((ULONG_PTR)syscall_data + (sizeof(ULONG) + sizeof(ULONG)) + (NATIVE_FUNCTION_SIZE * i)), func_ptr, NATIVE_FUNCTION_SIZE);
        }

        len = *syscall_data;
    }
    //
    // syscall_data中的第二个ULONG指向我们在驱动程序返回的数据之上附加的额外数据
    //

    extra = (SBIELOW_EXTRA_DATA*)((ULONG_PTR)syscall_data + len);

    syscall_data[1] = len;

    //为LdrLoadDll编写ASCII字符串（请参阅core / low / inject.c）    

    ptr = (WCHAR*)((ULONG_PTR)extra + sizeof(SBIELOW_EXTRA_DATA));

    strcpy((char*)ptr, "LdrLoadDll");

    extra->LdrLoadDll_offset = ULONG_DIFF(ptr, extra);
    ptr += 16 / sizeof(WCHAR);

    //
    //为LdrGetProcedureAddress写入ASCII字符串
    //

    strcpy((char*)ptr, "LdrGetProcedureAddress");

    extra->LdrGetProcAddr_offset = ULONG_DIFF(ptr, extra);
    ptr += 28 / sizeof(WCHAR);

    //
    //为NtProtectVirtualMemory写入ASCII字符串
    //

    strcpy((char*)ptr, "NtProtectVirtualMemory");

    extra->NtProtectVirtualMemory_offset = ULONG_DIFF(ptr, extra);
    ptr += 28 / sizeof(WCHAR);

    //
    //为NtRaiseHardError写入ASCII字符串
    //

    strcpy((char*)ptr, "NtRaiseHardError");

    extra->NtRaiseHardError_offset = ULONG_DIFF(ptr, extra);
    ptr += 20 / sizeof(WCHAR);

    //
    //为NtDeviceIoControlFile写入ASCII字符串
    //

    strcpy((char*)ptr, "NtDeviceIoControlFile");

    extra->NtDeviceIoControlFile_offset = ULONG_DIFF(ptr, extra);
    ptr += 28 / sizeof(WCHAR);

    //
    //为RtlFindActivationContextSectionString写入ASCII字符串
    //

    strcpy((char*)ptr, "RtlFindActivationContextSectionString");

    extra->RtlFindActCtx_offset = ULONG_DIFF(ptr, extra);
    ptr += 44 / sizeof(WCHAR);

    //ntdll在没有路径的情况下加载kernel32，我们将在RtlFindActivationContextSectionString的钩子中执行同样的操作，请参阅entry.asm
    wcscpy(ptr, L"kernel32.dll");

    len = wcslen(ptr);
    extra->KernelDll_offset = ULONG_DIFF(ptr, extra);
    extra->KernelDll_length = len * sizeof(WCHAR);
    ptr += len + 1;

    //将本机和wow64 SbieDll的路径附加到系统调用缓冲区
    wcscpy(ptr, sbie_home);
    wcscat(ptr, _SbieDll);

    len = (ULONG)wcslen(ptr);
    extra->NativeSbieDll_offset = ULONG_DIFF(ptr, extra);
    extra->NativeSbieDll_length = len * sizeof(WCHAR);
    ptr += len + 1;

    wcscpy(ptr, sbie_home);
    wcscat(ptr, L"\\32");
    wcscat(ptr, _SbieDll);

    len = (ULONG)wcslen(ptr);
    extra->Wow64SbieDll_offset = ULONG_DIFF(ptr, extra);
    extra->Wow64SbieDll_length = len * sizeof(WCHAR);
    ptr += len + 1;

    //注意：工作区域现在被移到了额外区域之后，因此不再覆盖syscall_data
    extra->InjectData_offset = ULONG_DIFF(ptr, extra);

    //调整系统调用缓冲区的大小以包括路径字符串
    *syscall_data = ULONG_DIFF(ptr, syscall_data) + sizeof(INJECT_DATA);

    m_syscall_data = syscall_data;
    return STATUS_SUCCESS;
}