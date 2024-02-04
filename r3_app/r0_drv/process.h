#pragma once
//#include"pool.h"
#include"main.h"

extern volatile BOOLEAN Process_ReadyToSandbox;

#ifdef _WIN64
#define PROCESS_TERMINATED      ((PROCESS *) 0xFFFFFFFFFFFFFFFF)
#else
#define PROCESS_TERMINATED      ((PROCESS *) 0xFFFFFFFF)
#endif

struct _PROCESS {

    // 通过Process_ListLock上的排他锁来同步对PROCESS块的链表的更改
    LIST_ELEM list_elem;

    // process id

    HANDLE pid;
    HANDLE starter_id;

    // process pool.  在进程创建时创建。当进程终止时，它的全部被释放

    POOL* pool;

    // box parameters

    //BOX* box;

    // 可执行映像的全名和扩展名，以及映像是否从复制系统中加载的指示

    WCHAR* image_path;

    WCHAR* image_name;
    ULONG   image_name_len;             // in bytes, including NULL

    BOOLEAN image_from_box;
    BOOLEAN image_sbie;

    // 流程创建时间和完整性级别

    ULONG64 create_time;

    ULONG integrity_level;

    ULONG ntdll32_base;

    ULONG detected_image_type;

    // 原始进程主访问令牌

    void* primary_token;

    PSID* SandboxieLogonSid;

    // thread data

    PERESOURCE threads_lock;

#ifdef USE_PROCESS_MAP
    HASH_MAP thread_map;
#else
    LIST threads;
#endif

    // flags

    volatile BOOLEAN initialized;       // process initialized once?
    volatile BOOLEAN terminated;        // process termination started?
    volatile ULONG   reason;            // process termination reason

    BOOLEAN untouchable;                // protected process?

    BOOLEAN drop_rights;                // admin rights should be dropped?
    BOOLEAN rights_dropped;             // admin rights were dropped?

    BOOLEAN forced_process;

    BOOLEAN sbiedll_loaded;
    BOOLEAN sbielow_loaded;

    BOOLEAN is_start_exe;
    BOOLEAN parent_was_start_exe;
    BOOLEAN parent_was_sandboxed;

    BOOLEAN change_notify_token_flag;

    BOOLEAN bAppCompartment;

    BOOLEAN in_pca_job;
    BOOLEAN can_use_jobs;

    UCHAR   create_console_flag;

    BOOLEAN disable_monitor;

    BOOLEAN always_close_for_boxed;
    BOOLEAN dont_open_for_boxed;
    BOOLEAN protect_host_images;
    BOOLEAN use_security_mode;
    BOOLEAN is_locked_down;
#ifdef USE_MATCH_PATH_EX
    BOOLEAN restrict_devices;
    BOOLEAN use_rule_specificity;
    BOOLEAN use_privacy_mode;
#endif
    BOOLEAN confidential_box;

    ULONG call_trace;

    // file-related

    PERESOURCE file_lock;
#ifdef USE_MATCH_PATH_EX
    LIST normal_file_paths;             // PATTERN elements
#endif
    LIST open_file_paths;               // PATTERN elements
    LIST closed_file_paths;             // PATTERN elements
    LIST read_file_paths;               // PATTERN elements
    LIST write_file_paths;              // PATTERN elements
    BOOLEAN file_block_network_files;
    LIST blocked_dlls;
    ULONG file_trace;
    ULONG pipe_trace;
    BOOLEAN disable_file_flt;
    BOOLEAN file_warn_internet;
    BOOLEAN file_warn_direct_access;
    BOOLEAN AllowInternetAccess;
    BOOLEAN file_open_devapi_cmapi;

    // key-related

    PERESOURCE key_lock;
    //KEY_MOUNT* key_mount;
#ifdef USE_MATCH_PATH_EX
    LIST normal_key_paths;              // PATTERN elements
#endif
    LIST open_key_paths;                // PATTERN elements
    LIST closed_key_paths;              // PATTERN elements
    LIST read_key_paths;                // PATTERN elements
    LIST write_key_paths;               // PATTERN elements
    ULONG key_trace;
    BOOLEAN disable_key_flt;

    // ipc-related

    PERESOURCE ipc_lock;
#ifdef USE_MATCH_PATH_EX
    LIST normal_ipc_paths;              // PATTERN elements
#endif
    LIST open_ipc_paths;                // PATTERN elements
    LIST closed_ipc_paths;              // PATTERN elements
    LIST read_ipc_paths;                // PATTERN elements
    ULONG ipc_trace;
    BOOLEAN disable_object_flt;
    BOOLEAN ipc_namespace_isoaltion;
    BOOLEAN ipc_warn_startrun;
    BOOLEAN ipc_warn_open_proc;
    BOOLEAN ipc_block_password;
    BOOLEAN ipc_open_lsa_endpoint;
    BOOLEAN ipc_open_sam_endpoint;
    BOOLEAN ipc_allowSpoolerPrintToFile;
    BOOLEAN ipc_openPrintSpooler;

    // gui-related

    PERESOURCE gui_lock;
    BOOLEAN open_all_win_classes;
    void* block_fake_input_hwnd;
    void* gui_user_area;
    WCHAR* gui_class_name;
    LIST open_win_classes;
    ULONG gui_trace;

    BOOLEAN bHostInject;

};



BOOLEAN Process_Init(void);

//低层系统接口（process_low.c）
BOOLEAN Process_Low_Init(void);

//返回进程句柄或进程ID号指定的进程的SID字符串和会话ID。
//若要释放返回的SidString，请使用RtlFreeUnicodeString
NTSTATUS Process_GetSidStringAndSessionId(
    HANDLE ProcessHandle, HANDLE ProcessId,
    UNICODE_STRING* SidString, ULONG* SessionId);

//返回从指定池分配的idProcess的ProcessName.exe。返回时，* out_buf指向UNICODE_STRING结构，该结构指向紧挨在该结构后面的以NULL终止的进程名称。
//*out_len包含*out_buf的长度。
//*out_ptr接收out_buf->Buffer。
//使用Mem_Free（*out_buf，*out_len）释放缓冲区。
//出错时，out_buf、out_len、out_ptr接收NULL
void Process_GetProcessName(
    POOL* pool, ULONG_PTR idProcess,
    void** out_buf, ULONG* out_len, WCHAR** out_ptr);

//process_Find查找指定进程id的PROCESS块。
//如果ProcessId为NULL，则返回当前沙盒进程。
//如果ProcessId为NULL且当前沙箱进程计划为终止，则返回值将终止
PROCESS* Process_Find(HANDLE ProcessId, KIRQL* out_irql);

extern PERESOURCE Process_ListLock;