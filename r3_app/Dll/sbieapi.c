#include "sbieapi.h"
#include"ntproto.h"
#include"../r0_drv/api_defs.h"

#include"../common/defines.h"

//变量
HANDLE SbieApi_DeviceHandle = INVALID_HANDLE_VALUE;

//SboxDll在CRT中不链接。相反，它继承了ntdll.dll中的CRT例程。
//然而，7600 DDK中的ntdll.lib并不能导出我们需要的所有内容。所以我们必须使用运行时动态链接。
int __CRTDECL Sbie_snwprintf(wchar_t* _Buffer, size_t Count, const wchar_t* const _Format, ...)
{
    int _Result;
    va_list _ArgList;

    extern int(*P_vsnwprintf)(wchar_t* _Buffer, size_t Count, const wchar_t* const, va_list Args);

    va_start(_ArgList, _Format);
    _Result = P_vsnwprintf(_Buffer, Count, _Format, _ArgList);
    va_end(_ArgList);

    if (_Result == -1 && _Buffer != NULL && Count != 0)
        _Buffer[Count - 1] = 0; // ensure the resulting string is always null terminated

    return _Result;
}

int __CRTDECL Sbie_snprintf(char* _Buffer, size_t Count, const char* const _Format, ...)
{
    int _Result;
    va_list _ArgList;

    extern int(*P_vsnprintf)(char* _Buffer, size_t Count, const char* const, va_list Args);

    va_start(_ArgList, _Format);
    _Result = P_vsnprintf(_Buffer, Count, _Format, _ArgList);
    va_end(_ArgList);

    if (_Result == -1 && _Buffer != NULL && Count != 0)
        _Buffer[Count - 1] = 0; // ensure the resulting string is always null terminated

    return _Result;
}

long SbieApi_Ioctl(ULONG64* parms)
{
    NTSTATUS status;
    OBJECT_ATTRIBUTES objattrs;
    UNICODE_STRING uni;
    IO_STATUS_BLOCK MyIoStatusBlock;

    if (parms == NULL) { // close request as used by kmdutil
        //if (SbieApi_DeviceHandle != INVALID_HANDLE_VALUE)
        //    NtClose(SbieApi_DeviceHandle);
        //SbieApi_DeviceHandle = INVALID_HANDLE_VALUE;
    }

    if (Dll_SbieTrace && parms[0] != API_MONITOR_PUT2) {
        //WCHAR dbg[1024];
        //extern const wchar_t* Trace_SbieDrvFunc2Str(ULONG func);
        //Sbie_snwprintf(dbg, 1024, L"SbieApi_Ioctl: %s %s", Dll_ImageName, Trace_SbieDrvFunc2Str((ULONG)parms[0]));
        //SbieApi_MonitorPutMsg(MONITOR_OTHER | MONITOR_TRACE, dbg);
    }
    if (SbieApi_DeviceHandle == INVALID_HANDLE_VALUE) 
    {
        RtlInitUnicodeString(&uni, API_DEVICE_NAME);
        InitializeObjectAttributes(
            &objattrs, &uni, OBJ_CASE_INSENSITIVE, NULL, NULL);

        status = NtOpenFile(
            &SbieApi_DeviceHandle, FILE_GENERIC_READ, &objattrs,
            &MyIoStatusBlock,
            FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
            0);

        if (status == STATUS_OBJECT_NAME_NOT_FOUND ||
            status == STATUS_NO_SUCH_DEVICE)
            status = STATUS_SERVER_DISABLED;
    }
    else 
    {
        status = STATUS_SUCCESS;
    }
    if (status != STATUS_SUCCESS) {

        SbieApi_DeviceHandle = INVALID_HANDLE_VALUE;
    }
    else 
    {
        //请注意，所有的请求都是同步的，这意味着
        //NtDeviceIoControlFile将等待，直到SbieDrv完成
        //在发送下一个请求之前处理一个请求
        extern P_NtDeviceIoControlFile __sys_NtDeviceIoControlFile;
        if (__sys_NtDeviceIoControlFile) 
        {
            //一旦NtDeviceIoControlFile被钩住，绕过它      
            status = __sys_NtDeviceIoControlFile(
                SbieApi_DeviceHandle, NULL, NULL, NULL, &MyIoStatusBlock,
                API_SBIEDRV_CTLCODE, parms, sizeof(ULONG64) * 8, NULL, 0);
        }
        else 
        {
            status = NtDeviceIoControlFile(
                SbieApi_DeviceHandle, NULL, NULL, NULL, &MyIoStatusBlock,
                API_SBIEDRV_CTLCODE, parms, sizeof(ULONG64) * 8, NULL, 0);
        }
    }
    return status;
}

SBIEAPI_EXPORT LONG SbieApi_GetHomePath(WCHAR* NtPath, ULONG NtPathMaxLen, WCHAR* DosPath, ULONG DosPathMaxLen)
{
    NTSTATUS status;
    __declspec(align(8)) UNICODE_STRING64 nt_path_uni;
    __declspec(align(8)) UNICODE_STRING64 dos_path_uni;
    __declspec(align(8)) ULONG64 parms[API_NUM_ARGS];
    API_GET_HOME_PATH_ARGS* args = (API_GET_HOME_PATH_ARGS*)parms;

    nt_path_uni.Length = 0;
    nt_path_uni.MaximumLength = (USHORT)(NtPathMaxLen * sizeof(WCHAR));
    nt_path_uni.Buffer = (ULONG64)(ULONG_PTR)NtPath;

    dos_path_uni.Length = 0;
    dos_path_uni.MaximumLength = (USHORT)(DosPathMaxLen * sizeof(WCHAR));
    dos_path_uni.Buffer = (ULONG64)(ULONG_PTR)DosPath;

    memzero(parms, sizeof(parms));
    args->func_code = API_GET_HOME_PATH;
    if (NtPath)
        args->nt_path.val64 = (ULONG64)(ULONG_PTR)&nt_path_uni;
    if (DosPath)
        args->dos_path.val64 = (ULONG64)(ULONG_PTR)&dos_path_uni;
    status = SbieApi_Ioctl(parms);
    if (!NT_SUCCESS(status)) {
        if (NtPath)
            NtPath[0] = L'\0';
        if (DosPath)
            DosPath[0] = L'\0';
    }

    return status;
}

SBIEAPI_EXPORT BOOLEAN SbieApi_QueryConfBool(const WCHAR* section_name, const WCHAR* setting_name, BOOLEAN def)
{
    WCHAR value[16];
    *value = L'\0';
    SbieApi_QueryConfAsIs(
        section_name, setting_name, 0, value, sizeof(value));
    if (*value == L'y' || *value == L'Y')
        return TRUE;
    if (*value == L'n' || *value == L'N')
        return FALSE;
    return def;
}


LONG SbieApi_QuerySymbolicLink(
    WCHAR* NameBuf,
    ULONG NameLen) 
{
    NTSTATUS status;
    __declspec(align(8)) ULONG64 parms[API_NUM_ARGS];
    API_QUERY_SYMBOLIC_LINK_ARGS* args =
        (API_QUERY_SYMBOLIC_LINK_ARGS*)parms;

    memzero(parms, sizeof(parms));
    args->func_code = API_QUERY_SYMBOLIC_LINK;
    args->name_buf.val64 = (ULONG64)(ULONG_PTR)NameBuf;
    args->name_len.val64 = (ULONG64)(ULONG_PTR)NameLen;
    status = SbieApi_Ioctl(parms);

    if (!NT_SUCCESS(status)) {
        if (NameBuf)
            *NameBuf = L'\0';
    }

    return status;
}

LONG SbieApi_CheckInternetAccess(HANDLE ProcessId, const WCHAR* DeviceName32, BOOLEAN IssueMessage)
{
    NTSTATUS status;
    __declspec(align(8)) ULONG64 parms[API_NUM_ARGS];
    API_CHECK_INTERNET_ACCESS_ARGS* args =
        (API_CHECK_INTERNET_ACCESS_ARGS*)parms;
    WCHAR MyDeviceName[34];
    ULONG len;
    if (DeviceName32) {
        len = wcslen(DeviceName32);
        if (len > 32)
            len = 32;
        memzero(MyDeviceName, sizeof(MyDeviceName));
        wmemcpy(MyDeviceName, DeviceName32, len);

        const WCHAR* ptr = DeviceName32;

        static const WCHAR* File_RawIp = L"rawip6";
        static const WCHAR* File_Http = L"http\\";
        static const WCHAR* File_Tcp = L"tcp6";
        static const WCHAR* File_Udp = L"udp6";
        static const WCHAR* File_Ip = L"ip6";
        static const WCHAR* File_Afd = L"afd";
        static const WCHAR* File_Nsi = L"nsi";
        //检查是否是一个网络设备
        BOOLEAN chk = FALSE;
        if (len == 12) {

            if (_wcsnicmp(ptr, File_Afd, 3) == 0) // Afd\Endpoint
                chk = TRUE;

        }
        else if (len == 6) {

            if (_wcsnicmp(ptr, File_RawIp, 6) == 0)
                chk = TRUE;

        }
        else if (len == 5) {

            if (_wcsnicmp(ptr, File_RawIp, 5) == 0)
                chk = TRUE;

        }
        else if (len == 4) {

            if (_wcsnicmp(ptr, File_Http, 4) == 0)
                chk = TRUE;
            if (_wcsnicmp(ptr, File_Tcp, 4) == 0)
                chk = TRUE;
            else if (_wcsnicmp(ptr, File_Udp, 4) == 0)
                chk = TRUE;

        }
        else if (len == 3) {

            if (_wcsnicmp(ptr, File_Tcp, 3) == 0)
                chk = TRUE;
            else if (_wcsnicmp(ptr, File_Udp, 3) == 0)
                chk = TRUE;
            else if (_wcsnicmp(ptr, File_Ip, 3) == 0)
                chk = TRUE;
            else if (_wcsnicmp(ptr, File_Afd, 3) == 0)
                chk = TRUE;
            else if (_wcsnicmp(ptr, File_Nsi, 3) == 0)
                chk = TRUE;

        }
        else if (len == 2) {

            if (_wcsnicmp(ptr, File_Ip, 2) == 0)
                chk = TRUE;
        }
        if (!chk) // quick bail out, don't bother the driver with irrelevant devices
            return STATUS_OBJECT_NAME_INVALID;
    }
}

LONG SbieApi_MonitorPut(ULONG Type, const WCHAR* Name)
{
    return SbieApi_MonitorPut2(Type, Name, TRUE);
}

LONG SbieApi_MonitorPut2(ULONG Type, const WCHAR* Name, BOOLEAN bCheckObjectExists)
{
    return SbieApi_MonitorPut2Ex(Type, wcslen(Name), Name, bCheckObjectExists, FALSE);;
}

LONG SbieApi_MonitorPut2Ex(ULONG Type, ULONG NameLen, const WCHAR* Name, BOOLEAN bCheckObjectExists, BOOLEAN bIsMessage)
{
    NTSTATUS status;
    __declspec(align(8)) ULONG64 parms[API_NUM_ARGS];
    API_MONITOR_PUT2_ARGS* args = (API_MONITOR_PUT2_ARGS*)parms;
    memset(parms, 0, sizeof(parms));
    args->func_code = API_MONITOR_PUT2;
    args->log_type.val = Type;
    args->log_len.val64 = NameLen * sizeof(WCHAR);
    args->log_ptr.val64 = (ULONG64)(ULONG_PTR)Name;
    args->check_object_exists.val64 = bCheckObjectExists;
    args->is_message.val64 = bIsMessage;
    status = SbieApi_Ioctl(parms);
    return status;
}

LONG SbieApi_QueryConf(
    const WCHAR* section_name,      // WCHAR [66]
    const WCHAR* setting_name,      // WCHAR [66]
    ULONG setting_index,
    WCHAR* out_buffer,
    ULONG buffer_len) 
{
    NTSTATUS status;
    __declspec(align(8)) UNICODE_STRING64 Output;
    __declspec(align(8)) ULONG64 parms[API_NUM_ARGS];
    WCHAR x_section[66];
    WCHAR x_setting[66];

    memzero(x_section, sizeof(x_section));
    memzero(x_setting, sizeof(x_setting));
    if (section_name)
        wcsncpy(x_section, section_name, 64);
    if (setting_name)
        wcsncpy(x_setting, setting_name, 64);

    Output.Length = 0;
    Output.MaximumLength = (USHORT)buffer_len;
    Output.Buffer = (ULONG64)(ULONG_PTR)out_buffer;

    memset(parms, 0, sizeof(parms));
    parms[0] = API_QUERY_CONF;
    parms[1] = (ULONG64)(ULONG_PTR)x_section;
    parms[2] = (ULONG64)(ULONG_PTR)x_setting;
    parms[3] = (ULONG64)(ULONG_PTR)&setting_index;
    parms[4] = (ULONG64)(ULONG_PTR)&Output;
    status = SbieApi_Ioctl(parms);

    if (!NT_SUCCESS(status)) {
        if (buffer_len > sizeof(WCHAR))
            out_buffer[0] = L'\0';
    }

    return status;
}


LONG SbieApi_GetVersionEx(
    WCHAR* version_string,  // WCHAR [16]
    ULONG* abi_version) 
{
    NTSTATUS status;
    __declspec(align(8)) ULONG64 parms[API_NUM_ARGS];
    API_GET_VERSION_ARGS* args = (API_GET_VERSION_ARGS*)parms;
    memzero(parms, sizeof(parms));
    args->func_code = API_GET_VERSION;
    args->string.val = version_string;
    args->abi_ver.val = abi_version;
    status = SbieApi_Ioctl(parms);
    if (!NT_SUCCESS(status)) {
        if (version_string) wcscpy(version_string, L"unknown");
        if (abi_version) *abi_version = 0;
    }

    return status;
}


LONG SbieApi_Call(ULONG api_code, LONG arg_num, ...) 
{
    va_list valist;
    NTSTATUS status;
    __declspec(align(8)) ULONG64 parms[API_NUM_ARGS];

    memzero(parms, sizeof(parms));
    parms[0] = api_code;

    if (arg_num >= (API_NUM_ARGS - 1))
        return STATUS_INVALID_PARAMETER;

    va_start(valist, arg_num);
    for (LONG i = 1; i <= arg_num; i++)
        parms[i] = (ULONG64)va_arg(valist, ULONG_PTR);
    va_end(valist);

    status = SbieApi_Ioctl(parms);

    return status;
}

ULONG SbieApi_GetMessage(ULONG* MessageNum, ULONG SessionId, ULONG* MessageId, ULONG* Pid, wchar_t* Buffer, ULONG Length)
{
    NTSTATUS status;
    __declspec(align(8)) UNICODE_STRING64 msgtext;
    __declspec(align(8)) ULONG64 parms[API_NUM_ARGS];
    API_GET_MESSAGE_ARGS* args = (API_GET_MESSAGE_ARGS*)parms;

    msgtext.MaximumLength = (USHORT)Length;
    msgtext.Buffer = (ULONG_PTR)Buffer;
    msgtext.Length = 0;

    memzero(parms, sizeof(parms));
    args->func_code = API_GET_MESSAGE;
    args->msg_num.val = MessageNum;
    args->session_id.val = SessionId;
    args->msgid.val = MessageId;
    args->msgtext.val = &msgtext;
    args->process_id.val = Pid;

    status = SbieApi_Ioctl(parms);

    return status;
}


LONG SbieApi_QueryProcess(
    HANDLE ProcessId,
    WCHAR* out_box_name_wchar34,
    WCHAR* out_image_name_wchar96,
    WCHAR* out_sid_wchar96,
    ULONG* out_session_id)
{
    return SbieApi_QueryProcessEx2(
        ProcessId, 96, out_box_name_wchar34, out_image_name_wchar96,
        out_sid_wchar96, out_session_id, NULL);
}


LONG SbieApi_QueryProcessEx2(
    HANDLE ProcessId,
    ULONG image_name_len_in_wchars,
    WCHAR* out_box_name_wchar34,
    WCHAR* out_image_name_wcharXXX,
    WCHAR* out_sid_wchar96,
    ULONG* out_session_id,
    ULONG64* out_create_time)
{
    NTSTATUS status;
    __declspec(align(8)) UNICODE_STRING64 BoxName;
    __declspec(align(8)) UNICODE_STRING64 ImageName;
    __declspec(align(8)) UNICODE_STRING64 SidString;
    __declspec(align(8)) ULONG64 parms[API_NUM_ARGS];
    API_QUERY_PROCESS_ARGS* args = (API_QUERY_PROCESS_ARGS*)parms;

    memzero(parms, sizeof(parms));
    args->func_code = API_QUERY_PROCESS;

    args->process_id.val64 = (ULONG64)(ULONG_PTR)ProcessId;

    if (out_box_name_wchar34) {
        BoxName.Length = 0;
        BoxName.MaximumLength = (USHORT)(sizeof(WCHAR) * BOXNAME_COUNT);
        BoxName.Buffer = (ULONG64)(ULONG_PTR)out_box_name_wchar34;
        args->box_name.val64 = (ULONG64)(ULONG_PTR)&BoxName;
    }

    if (out_image_name_wcharXXX) {
        ImageName.Length = 0;
        ImageName.MaximumLength =
            (USHORT)(sizeof(WCHAR) * image_name_len_in_wchars);
        ImageName.Buffer = (ULONG64)(ULONG_PTR)out_image_name_wcharXXX;
        args->image_name.val64 = (ULONG64)(ULONG_PTR)&ImageName;
    }

    if (out_sid_wchar96) {
        SidString.Length = 0;
        SidString.MaximumLength = (USHORT)(sizeof(WCHAR) * 96);
        SidString.Buffer = (ULONG64)(ULONG_PTR)out_sid_wchar96;
        args->sid_string.val64 = (ULONG64)(ULONG_PTR)&SidString;
    }

    if (out_session_id)
        args->session_id.val64 = (ULONG64)(ULONG_PTR)out_session_id;

    if (out_create_time)
        args->create_time.val64 = (ULONG64)(ULONG_PTR)out_create_time;

    status = SbieApi_Ioctl(parms);

    if (!NT_SUCCESS(status)) {

        ULONG_PTR x = (ULONG_PTR)out_session_id;
        if (x == 0 || x > 4) {

            //发生错误时重置参数，out_session_id是1到4范围内的特殊内部标志时除外
            if (out_box_name_wchar34)
                *out_box_name_wchar34 = L'\0';
            if (out_image_name_wcharXXX)
                *out_image_name_wcharXXX = L'\0';
            if (out_sid_wchar96)
                *out_sid_wchar96 = L'\0';
            if (out_session_id)
                *out_session_id = 0;
        }
    }

    return status;
}


LONG SbieApi_SessionLeader(HANDLE TokenHandle, HANDLE* ProcessId)
{
    NTSTATUS status;
    __declspec(align(8)) ULONG64 ResultValue;
    __declspec(align(8)) ULONG64 parms[API_NUM_ARGS];
    API_SESSION_LEADER_ARGS* args = (API_SESSION_LEADER_ARGS*)parms;

    memset(parms, 0, sizeof(parms));
    args->func_code = API_SESSION_LEADER;
    if (ProcessId) {
        args->token_handle.val64 = (ULONG64)(ULONG_PTR)TokenHandle;
        args->process_id.val64 = (ULONG64)(ULONG_PTR)&ResultValue;
    }
    else {
        args->token_handle.val64 = 0;
        args->process_id.val64 = 0;
    }
    status = SbieApi_Ioctl(parms);

    if (!NT_SUCCESS(status))
        ResultValue = 0;
    if (ProcessId)
        *ProcessId = (HANDLE)ResultValue;

    return status;
}