#include"dll.h"
#include"ntproto.h"
#define MOUNTMGRCONTROLTYPE  ((ULONG) 'm')
#define IOCTL_MOUNTMGR_QUERY_POINTS \
    CTL_CODE(MOUNTMGRCONTROLTYPE, 2, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_MOUNTMGR_QUERY_DOS_VOLUME_PATHS \
    CTL_CODE(MOUNTMGRCONTROLTYPE, 13, METHOD_BUFFERED, FILE_ANY_ACCESS)

//�ṹ������
struct _FILE_DRIVE {

    WCHAR letter;
    WCHAR sn[10];
    BOOLEAN subst;
    ULONG len;          // in characters, excluding NULL
    WCHAR path[0];

};
struct _FILE_DRIVE;
typedef struct _FILE_DRIVE FILE_DRIVE;

struct _FILE_GUID {

    LIST_ELEM list_elem;
    WCHAR guid[38 + 1];
    ULONG len;          // in characters, excluding NULL
    WCHAR path[0];
};
struct _FILE_GUID;
typedef struct _FILE_GUID FILE_GUID;

struct _FILE_LINK {

    LIST_ELEM list_elem;
    ULONG ticks;
    BOOLEAN same;
    BOOLEAN stop;
    ULONG src_len;      // in characters, excluding NULL
    ULONG dst_len;      // in characters, excluding NULL
    WCHAR* dst;
    WCHAR src[1];
};
struct _FILE_LINK;
typedef struct _FILE_LINK FILE_LINK;

typedef struct _MOUNTMGR_MOUNT_POINT {
    ULONG   SymbolicLinkNameOffset;
    USHORT  SymbolicLinkNameLength;
    ULONG   UniqueIdOffset;
    USHORT  UniqueIdLength;
    ULONG   DeviceNameOffset;
    USHORT  DeviceNameLength;
} MOUNTMGR_MOUNT_POINT, * PMOUNTMGR_MOUNT_POINT;

typedef struct _MOUNTMGR_MOUNT_POINTS {
    ULONG                   Size;
    ULONG                   NumberOfMountPoints;
    MOUNTMGR_MOUNT_POINT    MountPoints[1];
} MOUNTMGR_MOUNT_POINTS, * PMOUNTMGR_MOUNT_POINTS;

typedef struct _MOUNTDEV_NAME {
    USHORT  NameLength;
    WCHAR   Name[1];
} MOUNTDEV_NAME, * PMOUNTDEV_NAME;


typedef struct _MOUNTMGR_VOLUME_PATHS {
    ULONG   MultiSzLength;
    WCHAR   MultiSz[1];
} MOUNTMGR_VOLUME_PATHS, * PMOUNTMGR_VOLUME_PATHS;

//����
const WCHAR* File_BQQB = L"\\??\\";
static const WCHAR* File_NamedPipe = L"\\device\\namedpipe\\";
static const WCHAR* File_MailSlot = L"\\device\\mailslot\\";
//Lan man�ض������豸��Windows 2000��Windows XP����
static const WCHAR* File_Redirector = L"\\device\\lanmanredirector\\";
static const ULONG File_RedirectorLen = 25;
// Windows Vista name for the LanmanRedirector device
static const WCHAR* File_MupRedir = L"\\device\\mup\\;lanmanredirector\\";
static const ULONG File_MupRedirLen = 30;
const WCHAR* File_Mup = L"\\device\\mup\\";
static const ULONG File_MupLen = 12;

static FILE_DRIVE** File_Drives = NULL;
static CRITICAL_SECTION* File_DrivesAndLinks_CritSec = NULL;
static LIST* File_PermLinks = NULL;
static LIST* File_TempLinks = NULL;
static LIST* File_GuidLinks = NULL;
static BOOLEAN File_DriveAddSN = FALSE;
P_NtDeviceIoControlFile      __sys_NtDeviceIoControlFile = NULL;
//����
static BOOLEAN File_InitDrives(ULONG DriveMask);
static void File_InitLinks(THREAD_DATA* TlsData);
static ULONG File_IsNamedPipe(const WCHAR* path, const WCHAR** server);
static WCHAR* File_GetName_TranslateSymlinks(
    THREAD_DATA* TlsData, const WCHAR* objname_buf, ULONG objname_len,
    BOOLEAN* translated);

const WCHAR* SbieDll_GetDrivePath(ULONG DriveIndex)
{
    if ((!File_Drives) || (DriveIndex == -1)) {
        File_InitDrives(0xFFFFFFFF);
        if (DriveIndex == -1)
            return NULL;
    }

    if (File_Drives && (DriveIndex < 26) && File_Drives[DriveIndex])
        return File_Drives[DriveIndex]->path;

    return NULL;
}

BOOLEAN File_InitDrives(ULONG DriveMask)
{
    THREAD_DATA* TlsData = Dll_GetTlsData(NULL);
    NTSTATUS status;
    OBJECT_ATTRIBUTES objattrs;
    UNICODE_STRING objname;
    HANDLE handle;
    FILE_DRIVE* file_drive;
    ULONG file_drive_len;
    ULONG drive;
    ULONG path_len;
    //ULONG drive_count;
    WCHAR* path;
    WCHAR error_str[16];
    BOOLEAN CallInitLinks;
    //���������������ӽṹ
    if (!File_DrivesAndLinks_CritSec) 
    {
        File_DrivesAndLinks_CritSec = Dll_Alloc(sizeof(CRITICAL_SECTION));
        InitializeCriticalSectionAndSpinCount(
            File_DrivesAndLinks_CritSec, 1000);
    }
    EnterCriticalSection(File_DrivesAndLinks_CritSec);
    //��ʼ����װ�������ӵ����·������б�
    if (!File_PermLinks) 
    {
        File_PermLinks = Dll_Alloc(sizeof(LIST));
        List_Init(File_PermLinks);
        File_TempLinks = Dll_Alloc(sizeof(LIST));
        List_Init(File_TempLinks);

        File_GuidLinks = Dll_Alloc(sizeof(LIST));
        List_Init(File_GuidLinks);

        CallInitLinks = TRUE;
    }
    else
        CallInitLinks = FALSE;
    //����һ������������26�����ܵ�
    //�������ŵ����ǵ�����NT·������ӳ��
    Dll_PushTlsNameBuffer(TlsData);
    if (!File_Drives) 
    {
        File_Drives = Dll_Alloc(26 * sizeof(FILE_DRIVE*));
        memzero(File_Drives, 26 * sizeof(FILE_DRIVE*));
    }
    InitializeObjectAttributes(
        &objattrs, &objname, OBJ_CASE_INSENSITIVE, NULL, NULL);
    for (drive = 0; drive < 26; ++drive) 
    {
        BOOLEAN HaveSymlinkTarget = FALSE;

        FILE_DRIVE* old_drive = NULL;
        //�����1��������������ָ���ģ�
        //���ߣ�2���������ǰ��һ��������������������ʽΪ\����\X�� \������������ָ����������X
        if (!(DriveMask & (1 << drive))) 
        {
        
        }
        if (File_Drives[drive]) 
        {
        
        }
        //��DosDevices�������ӡ�\����\x:��ת��Ϊ���豸����
        //��ע�⣬��ʱDosDevices�����ƻ�ת��Ϊ�������ӱ���
        path_len = 16;
        path = Dll_Alloc(path_len);
        Sbie_snwprintf(path, 8, L"\\??\\%c:", L'A' + drive);
        RtlInitUnicodeString(&objname, path);

        status = NtOpenSymbolicLinkObject(
            &handle, SYMBOLIC_LINK_QUERY, &objattrs);
        if (!NT_SUCCESS(status)) 
        {
            //���������һ����Ч�ķ������ӣ�������û��
            //����Ȩ����������������ӣ���ô����Ҫ��
            //��������Ϊ���ǲ�ѯ�������
            WCHAR* path2 = Dll_AllocTemp(1024 * sizeof(WCHAR));
            wcscpy(path2, path);

            NTSTATUS status2 = SbieApi_QuerySymbolicLink(path2, 1024 * sizeof(WCHAR));
            if (NT_SUCCESS(status2)) 
            {
            
            }
            else 
            {
                Dll_Free(path2);
            }
        }
        //�����ʾ�����������ڡ�������Ч���δ��Ч�������������ʾΪClosedFilePath������Ҳ���Ծܾ����ʡ�
        //���κ�����£������Ժ���
        if (!NT_SUCCESS(status)) 
        {
            Dll_Free(path);

            if (status != STATUS_OBJECT_NAME_NOT_FOUND &&
                status != STATUS_OBJECT_PATH_NOT_FOUND &&
                status != STATUS_OBJECT_TYPE_MISMATCH &&
                status != STATUS_ACCESS_DENIED) 
            {
                //Sbie_snwprintf(error_str, 16, L"%c [%08X]", L'A' + drive, status);
                //SbieApi_Log(2307, error_str);
            }
            if (old_drive) 
            {
                //File_AdjustDrives(drive, old_drive->subst, old_drive->path);
                //Dll_Free(old_drive);
            }
            continue;
        }
        //��ȡ�������ӵ�Ŀ��
        if (handle) 
        {
            memzero(&objname, sizeof(UNICODE_STRING));
            status = NtQuerySymbolicLinkObject(handle, &objname, &path_len);
            if (status != STATUS_BUFFER_TOO_SMALL) 
            {
                if (NT_SUCCESS(status))
                    status = STATUS_UNSUCCESSFUL;
            }
            else 
            {
                Dll_Free(path);
                path_len += 32;
                path = Dll_Alloc(path_len);
                objname.MaximumLength = (USHORT)(path_len - 8);
                objname.Length = objname.MaximumLength - sizeof(WCHAR);
                objname.Buffer = path;
                status = NtQuerySymbolicLinkObject(handle, &objname, NULL);
                if (NT_SUCCESS(status)) 
                {
                    HaveSymlinkTarget = TRUE;
                }
            }
            NtClose(handle);
        }
        //����µ���������Ŀ
        if (HaveSymlinkTarget) 
        {
            BOOLEAN subst = FALSE;
            BOOLEAN translated = FALSE;
            WCHAR* save_path = path;
            path[objname.Length / sizeof(WCHAR)] = L'\0';
            if (wmemcmp(path, File_BQQB, 4) == 0 && path[4] && path[5] == L':') 
            {
            
            }
            path = File_GetName_TranslateSymlinks(
                TlsData, path, objname.Length, &translated);
            if (path) 
            {
                //�����ص�·��ָ��NameBuffer����˲���Ҫ��ʽ�ͷ�����
                path_len = wcslen(path);
                file_drive_len = sizeof(FILE_DRIVE)
                    + (path_len + 1) * sizeof(WCHAR);
                file_drive = Dll_Alloc(file_drive_len);
                file_drive->letter = (WCHAR)(drive + L'A');
                file_drive->subst = subst;
                file_drive->len = path_len;
                wcscpy(file_drive->path, path);
                *file_drive->sn = 0;
                if (File_DriveAddSN)
                {
                }
                File_Drives[drive] = file_drive;
                SbieApi_MonitorPut(MONITOR_DRIVE, path);
            }
            Dll_Free(save_path);
        }
        //��������������Ŀ
        if (old_drive)
            //Dll_Free(old_drive);

        if (!NT_SUCCESS(status)) 
        {

            //Sbie_snwprintf(error_str, 16, L"%c [%08X]", L'A' + drive, status);
            //SbieApi_Log(2307, error_str);
        }
    }
    //��ʼ��װ�ص�Ŀ¼�������������ľ���б�
    if (CallInitLinks) 
    {
        File_InitLinks(TlsData);
    }
    Dll_PopTlsNameBuffer(TlsData);

    LeaveCriticalSection(File_DrivesAndLinks_CritSec);

    return TRUE;
}

ULONG File_IsNamedPipe(const WCHAR* path, const WCHAR** server)
{
    ULONG len;
    //�������\Device\NamedPipe����\Device\MailSlot��
    //���߸���mup��ʽ����\Device\ mup\NamedPpipe��\Device\LanManRedirector\MailSlot
    len = wcslen(path);
    if (len >= 18) 
    {
        if (_wcsnicmp(path, File_NamedPipe, 18) == 0)
            //return TYPE_NAMED_PIPE;
        if (_wcsnicmp(path, File_MailSlot, 17) == 0)
            //return TYPE_MAIL_SLOT;

        if (_wcsnicmp(path, File_Redirector, File_RedirectorLen) == 0) {
            //WCHAR* ptr = wcschr(path + File_RedirectorLen, L'\\');
            //if (File_IsPipeSuffix(ptr)) {
            //    if (server)
            //        *server = path + File_RedirectorLen;
            //    return TYPE_NAMED_PIPE;
            //}
        }

        if (_wcsnicmp(path, File_MupRedir, File_MupRedirLen) == 0) {
            //WCHAR* ptr = wcschr(path + File_MupRedirLen, L'\\');
            //if (File_IsPipeSuffix(ptr)) {
            //    if (server)
            //        *server = path + File_MupRedirLen;
            //    return TYPE_NAMED_PIPE;
            //}
        }

        if (_wcsnicmp(path, File_Mup, File_MupLen) == 0) {
            //WCHAR* ptr = wcschr(path + File_MupLen, L'\\');
            //if (File_IsPipeSuffix(ptr)) {
            //    if (server)
            //        *server = path + File_MupLen;
            //    return TYPE_NAMED_PIPE;
            //}
        }
    }
    // ������Ƿ�����ClosedFilePathƥ���Internet�豸
    if (len >= 10 && _wcsnicmp(path, File_Mup, 8) == 0) 
    {
        BOOLEAN prompt = SbieApi_QueryConfBool(NULL, L"PromptForInternetAccess", FALSE);
    }
    return 0;
}

WCHAR* File_GetName_TranslateSymlinks(THREAD_DATA* TlsData, const WCHAR* objname_buf, ULONG objname_len, BOOLEAN* translated)
{
    NTSTATUS status;
    HANDLE handle;
    OBJECT_ATTRIBUTES objattrs;
    UNICODE_STRING objname;
    WCHAR* name;
    ULONG path_len;
    const WCHAR* suffix;
    ULONG suffix_len;
    //����Է�ֹFile_GetBoxedPipeName��Ӷ����\device\pipenameǰ׺����лChrome����
    if (objname_len > 26 && !_wcsnicmp(objname_buf, L"\\??\\pipe", 8)) {
        //if (!_wcsnicmp(objname_buf + 8, File_NamedPipe, 17)) {
        //    objname_buf += 8;
        //}
    }
    if (objname_len >= 18 && File_IsNamedPipe(objname_buf, NULL)) {
        //handle = NULL;
        //goto not_link;
    }
    InitializeObjectAttributes(
        &objattrs, &objname, OBJ_CASE_INSENSITIVE, NULL, NULL);

    objname.Length = (USHORT)objname_len;
    objname.Buffer = (WCHAR*)objname_buf;
    //���Ŵ������ҵ�����ķ������ӡ�
    //���磬�����������Ϊ\����\PIPE\MyPipe�����ǽ������ӡ�\����\PPIPE�������ܡ�\��������Ҳ��һ������
    while (1) 
    {
        objname.MaximumLength = objname.Length;
        status = NtOpenSymbolicLinkObject(
            &handle, SYMBOLIC_LINK_QUERY, &objattrs);
        if (NT_SUCCESS(status))
            break;
        if (status == STATUS_ACCESS_DENIED &&
            objname.Length <= 1020 * sizeof(WCHAR)) 
        {

        }
        //·������һ���������ӣ��������ٴμ��·��֮ǰ�����е����һ��·�����
        handle = NULL;

        if (objname.Length <= sizeof(WCHAR))
            break;

        do {
            objname.Length -= sizeof(WCHAR);
        } while (objname.Length &&
            objname_buf[objname.Length / sizeof(WCHAR)] != L'\\');

        if (objname.Length <= sizeof(WCHAR))
            break;
    }
    //��������Ҳ����������ӣ���ô���Ǿ�����
not_link:

    if (!handle) {

        name = Dll_GetTlsNameBuffer(
            TlsData, TRUE_NAME_BUFFER,
            objname_len + sizeof(WCHAR));

        memcpy(name, objname_buf, objname_len);
        *(name + objname_len / sizeof(WCHAR)) = L'\0';

        return name;
    }
}

void File_InitLinks(THREAD_DATA* TlsData) 
{
    NTSTATUS status;
    OBJECT_ATTRIBUTES objattrs;
    UNICODE_STRING objname;
    IO_STATUS_BLOCK IoStatusBlock;
    HANDLE hMountMgr;
    MOUNTMGR_MOUNT_POINT Input1;
    MOUNTMGR_MOUNT_POINTS* Output1;
    MOUNTDEV_NAME* Input2;
    MOUNTMGR_VOLUME_PATHS* Output2;
    ULONG index1;
    WCHAR save_char;
    FILE_GUID* guid;
    ULONG alloc_len;

    //����ɵ�guid��
    EnterCriticalSection(File_DrivesAndLinks_CritSec);
    guid = List_Head(File_GuidLinks);
    while (guid) {
        FILE_LINK* next_guid = List_Next(guid);
        List_Remove(File_GuidLinks, guid);
        Dll_Free(guid);
        guid = next_guid;
    }
    LeaveCriticalSection(File_DrivesAndLinks_CritSec);
    //��װ�ص�������豸
    RtlInitUnicodeString(&objname, L"\\Device\\MountPointManager");
    InitializeObjectAttributes(
        &objattrs, &objname, OBJ_CASE_INSENSITIVE, NULL, NULL);

    status = NtCreateFile(
        &hMountMgr, FILE_GENERIC_EXECUTE, &objattrs, &IoStatusBlock,
        NULL, FILE_ATTRIBUTE_NORMAL, FILE_SHARE_VALID_FLAGS,
        FILE_OPEN, FILE_SYNCHRONOUS_IO_NONALERT | FILE_NON_DIRECTORY_FILE,
        NULL, 0);
    if (!NT_SUCCESS(status))
        return;
    //��ѯ����װ�ص�
    memset(&Input1, 0, sizeof(MOUNTMGR_MOUNT_POINT));
    Output1 = Dll_Alloc(8192);
    status = NtDeviceIoControlFile(
        hMountMgr, NULL, NULL, NULL, &IoStatusBlock,
        IOCTL_MOUNTMGR_QUERY_POINTS,
        &Input1, sizeof(MOUNTMGR_MOUNT_POINT),
        Output1, 8192);

    if (!NT_SUCCESS(status)) {
        NtClose(hMountMgr);
        return;
    }

    Input2 = Dll_Alloc(128);
    Output2 = Dll_Alloc(8192);
    //����ÿ�����ÿ����װ�ص�
    for (index1 = 0; index1 < Output1->NumberOfMountPoints; ++index1) 
    {
        MOUNTMGR_MOUNT_POINT* MountPoint = &Output1->MountPoints[index1];
        WCHAR* DeviceName =
            (WCHAR*)((UCHAR*)Output1 + MountPoint->DeviceNameOffset);
        ULONG DeviceNameLen =
            MountPoint->DeviceNameLength / sizeof(WCHAR);
        WCHAR* VolumeName =
            (WCHAR*)((UCHAR*)Output1 + MountPoint->SymbolicLinkNameOffset);
        ULONG VolumeNameLen =
            MountPoint->SymbolicLinkNameLength / sizeof(WCHAR);
        WCHAR text[256];
        Sbie_snwprintf(text, 256, L"Found mountpoint: %.*s <-> %.*s", VolumeNameLen, VolumeName, DeviceNameLen, DeviceName);
        SbieApi_MonitorPut2(MONITOR_DRIVE | MONITOR_TRACE, text, FALSE);
        if (VolumeNameLen != 48 && VolumeNameLen != 49)
            continue;
        if (_wcsnicmp(VolumeName, L"\\??\\Volume{", 11) != 0)
            continue;
        //�洢guid��nt�豸����
        alloc_len = sizeof(FILE_GUID)
            + (VolumeNameLen + 1) * sizeof(WCHAR);
        guid = Dll_Alloc(alloc_len);
        wmemcpy(guid->guid, &VolumeName[10], 38);
        guid->guid[38] = 0;
        guid->len = DeviceNameLen;
        wmemcpy(guid->path, DeviceName, DeviceNameLen);
        guid->path[DeviceNameLen] = 0;
        EnterCriticalSection(File_DrivesAndLinks_CritSec);
        List_Insert_Before(File_GuidLinks, NULL, guid);
        LeaveCriticalSection(File_DrivesAndLinks_CritSec);
        //��ȡ�����ڵ�����DOS·��
        Input2->NameLength = 48 * sizeof(WCHAR);
        wmemcpy(Input2->Name, VolumeName, 48);
        Input2->Name[48] = L'\0';

        status = NtDeviceIoControlFile(
            hMountMgr, NULL, NULL, NULL, &IoStatusBlock,
            IOCTL_MOUNTMGR_QUERY_DOS_VOLUME_PATHS,
            Input2, 98,
            Output2, 8192);

        if (!NT_SUCCESS(status))
            continue;
        //process DOS·��
        save_char = DeviceName[DeviceNameLen];
        DeviceName[DeviceNameLen] = L'\0';
        if (Output2->MultiSzLength && *Output2->MultiSz) 
        {

            WCHAR* DosPath = Output2->MultiSz;
            ULONG DosPathLen = wcslen(DosPath);
            if (DosPathLen <= 3)
            {
                //�����Ҳ��Ϊ�������Ű�װ�������
                //1.���Ե�һ��װ���·�����������ţ�
                //2.���κ�����װ��·���е��ط�������ӵ��豸����
                DosPath += DosPathLen + 1;
                while (*DosPath) 
                {
                    //File_AddLink(TRUE, DosPath, DeviceName);
                    //DosPath += wcslen(DosPath) + 1;
                }
            }
            else 
            {
            
            }
        }
        DeviceName[DeviceNameLen] = save_char;
    }
    Dll_Free(Output2);
    Dll_Free(Input2);
    Dll_Free(Output1);
    NtClose(hMountMgr);
    //ɳ��·��������Ŀ¼װ�ص�C:\mount\sandbox��ָ��������������£�Dll_BoxFilePath��������ΪĿ���豸\device\HarddiskVolume2��
    //������Ҳ��Ҫ����File_GetName��ʹ�õ�װ�ص�λ��\device\ HarddiskVolume1\mount\SSANDBOX
    if (Dll_BoxName) 
    {
    
    }
}