#include"sbiedll.h"
#include"dll.h"
#include"../low/lowdata.h"

//变量
void* m_sbielow_ptr = NULL;
ULONG m_sbielow_len = 0;
//增加两个偏移量变量来替换“头”和“尾”的依赖关系
ULONG m_sbielow_start_offset = 0;
ULONG m_sbielow_data_offset = 0;

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

    HRSRC hrsrc = FindResource(Dll_Instance, arch_64bit ? L"LOWLEVEL64" : L"LOWLEVEL32", RT_RCDATA);
    if (!hrsrc)
        return errlvl;

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
    if (!errlvl)
        errlvl = SbieDll_InjectLow_LoadLow(FALSE, &m_sbielow32_ptr, &m_sbielow32_len, NULL, NULL, &m_sbielow32_detour_offset);
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