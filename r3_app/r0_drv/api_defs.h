#pragma once
#include"../common/my_version.h"
#define API_DEVICE_NAME         L"\\Device\\" SANDBOXIE L"DriverApi"

#define API_SBIEDRV_CTLCODE \
    CTL_CODE(FILE_DEVICE_UNKNOWN, 0x801, METHOD_NEITHER, FILE_ANY_ACCESS)


#define API_ARGS_BEGIN(x)	typedef struct _##x {ULONG64 func_code;
#define API_ARGS_FIELD(t,m)	union{ULONG64 val64; t val; } m;
#define API_ARGS_CLOSE(x)	} x;

//保持API_NUM_ARGS与core/low/entry中的系统服务代码同步。
#define API_NUM_ARGS                8

enum {
	API_FIRST = 0x12340000L,
    API_GET_VERSION,
    API_GET_WORK_DEPRECATED,    			    // deprecated
    API_LOG_MESSAGE,
    API_GET_LICENSE_PRE_V3_48_DEPRECATED,       // deprecated
    API_SET_LICENSE_PRE_V3_48_DEPRECATED,       // deprecated
    API_START_PROCESS_PRE_V3_44_DEPRECATED,     // deprecated
    API_QUERY_PROCESS,
    API_QUERY_BOX_PATH,
    API_QUERY_PROCESS_PATH,
    API_QUERY_PATH_LIST,
    API_ENUM_PROCESSES,
    API_DISABLE_FORCE_PROCESS,
    API_HOOK_TRAMP_DEPRECATED,					// deprecated
    API_UNMOUNT_HIVES_DEPRECATED,               // deprecated
    API_QUERY_CONF,
    API_RELOAD_CONF,
    API_CREATE_DIR_OR_LINK,
    API_DUPLICATE_OBJECT,
    API_GET_INJECT_SAVE_AREA_DEPRECATED,        // deprecated
    API_RENAME_FILE,
    API_SET_USER_NAME,
    API_INIT_GUI,
    API_UNLOAD_DRIVER,
    API_GET_SET_DEVICE_MAP_DEPRECATED,          // deprecated
    API_SESSION_SET_LEADER_DEPRECATED,          // deprecated
    API_GLOBAL_FORCE_PROCESS_DEPRECATED,        // deprecated
    API_MONITOR_CONTROL,
    API_MONITOR_PUT_DEPRECATED,                 // deprecated
    API_MONITOR_GET_DEPRECATED,                 // deprecated
    API_GET_UNMOUNT_HIVE,
    API_GET_FILE_NAME,
    API_REFRESH_FILE_PATH_LIST,
    API_SET_LSA_AUTH_PKG,
    API_OPEN_FILE,
    API_SESSION_CHECK_LEADER_DEPRECATED,        // deprecated
    API_START_PROCESS,
    API_CHECK_INTERNET_ACCESS,
    API_GET_HOME_PATH,
    API_GET_BLOCKED_DLL,
    API_QUERY_LICENSE,
    API_ACTIVATE_LICENSE_DEPRECATED,            // deprecated
    API_OPEN_DEVICE_MAP,
    API_OPEN_PROCESS,
    API_QUERY_PROCESS_INFO,
    API_IS_BOX_ENABLED,
    API_SESSION_LEADER,
    API_QUERY_SYMBOLIC_LINK,
    API_OPEN_KEY,
    API_SET_LOW_LABEL_KEY,
    API_OVERRIDE_PROCESS_TOKEN_DEPRECATED,      // deprecated
    API_SET_SERVICE_PORT,
    API_INJECT_COMPLETE,
    API_QUERY_SYSCALLS,
    API_INVOKE_SYSCALL,
    API_GUI_CLIPBOARD,
    API_ALLOW_SPOOLER_PRINT_TO_FILE_DEPRECATED, // deprecated
    API_RELOAD_CONF2,                           // unused
    API_MONITOR_PUT2,
    API_GET_SPOOLER_PORT_DEPRECATED,            // deprecated
    API_GET_WPAD_PORT_DEPRECATED,               // deprecated
    API_SET_GAME_CONFIG_STORE_PORT_DEPRECATED,  // deprecated
    API_SET_SMART_CARD_PORT_DEPRECATED,         // deprecated
    API_MONITOR_GET_EX,
    API_GET_MESSAGE,
    API_PROCESS_EXEMPTION_CONTROL,
    API_GET_DYNAMIC_PORT_FROM_PID,
    API_OPEN_DYNAMIC_PORT,
    API_QUERY_DRIVER_INFO,
    API_FILTER_TOKEN,
    API_SET_SECURE_PARAM,
    API_GET_SECURE_PARAM,
    API_MONITOR_GET2,
    API_PROTECT_ROOT,
    API_UNPROTECT_ROOT,
	API_LAST
};


enum {
    SVC_FIRST = 0x23450000L,

    SVC_LOOKUP_SID,
    SVC_INJECT_PROCESS,
    SVC_CANCEL_PROCESS,
    SVC_UNMOUNT_HIVE,
    SVC_LOG_MESSAGE,
    SVC_CONFIG_UPDATED,
    SVC_MOUNTED_HIVE,

    SVC_LAST
};


API_ARGS_BEGIN(API_GET_HOME_PATH_ARGS)
API_ARGS_FIELD(UNICODE_STRING64*, nt_path)
API_ARGS_FIELD(UNICODE_STRING64*, dos_path)
API_ARGS_CLOSE(API_GET_HOME_PATH_ARGS)


API_ARGS_BEGIN(API_QUERY_SYMBOLIC_LINK_ARGS)
API_ARGS_FIELD(WCHAR*, name_buf)
API_ARGS_FIELD(ULONG, name_len)
API_ARGS_CLOSE(API_QUERY_SYMBOLIC_LINK_ARGS)

API_ARGS_BEGIN(API_CHECK_INTERNET_ACCESS_ARGS)
API_ARGS_FIELD(HANDLE, process_id)
API_ARGS_FIELD(WCHAR*, device_name)
API_ARGS_FIELD(BOOLEAN, issue_message)
API_ARGS_CLOSE(API_CHECK_INTERNET_ACCESS_ARGS)

API_ARGS_BEGIN(API_MONITOR_PUT2_ARGS)
API_ARGS_FIELD(ULONG, log_type)
API_ARGS_FIELD(ULONG, log_len)
API_ARGS_FIELD(WCHAR*, log_ptr)
API_ARGS_FIELD(BOOLEAN, check_object_exists)
API_ARGS_FIELD(BOOLEAN, is_message)
//API_ARGS_FIELD(ULONG, log_aux)
API_ARGS_CLOSE(API_MONITOR_PUT2_ARGS)


API_ARGS_BEGIN(API_GET_VERSION_ARGS)
API_ARGS_FIELD(WCHAR*, string)
API_ARGS_FIELD(ULONG*, abi_ver)
API_ARGS_CLOSE(API_GET_VERSION_ARGS)


API_ARGS_BEGIN(API_SECURE_PARAM_ARGS)
API_ARGS_FIELD(WCHAR*, param_name)
API_ARGS_FIELD(VOID*, param_data)
API_ARGS_FIELD(ULONG, param_size)
API_ARGS_FIELD(ULONG*, param_size_out)
API_ARGS_FIELD(BOOLEAN, param_verify)
API_ARGS_CLOSE(API_SECURE_PARAM_ARGS)

API_ARGS_BEGIN(API_GET_MESSAGE_ARGS)
API_ARGS_FIELD(ULONG*, msg_num)
API_ARGS_FIELD(ULONG, session_id)
API_ARGS_FIELD(ULONG*, msgid)
API_ARGS_FIELD(UNICODE_STRING64*, msgtext)
API_ARGS_FIELD(ULONG*, process_id)
API_ARGS_CLOSE(API_GET_MESSAGE_ARGS)