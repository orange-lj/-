#pragma once

#include"main.h"

typedef struct _BOX             BOX;

struct _BOX {

    //沙箱由三个组件标识：
    // 1.这个盒子的名字

    WCHAR name[BOXNAME_COUNT];
    ULONG name_len;                     // in bytes, including NULL

    // 2.启动此沙箱的用户帐户

    WCHAR* sid;
    ULONG sid_len;                      // in bytes, including NULL

    // 3.终端服务会话编号

    ULONG session_id;

    // 用于此框时Conf_Expand的参数

    CONF_EXPAND_ARGS* expand_args;

    // 沙盒有四条路径：
    // 1.进入沙箱的文件系统入口点
    // default:  \??\C:\Sandbox\%SID%\BoxName

    WCHAR* file_path;
    ULONG file_path_len;                // in bytes, including NULL

    // 当文件路径被重新分析点重定向时，我们保留原始路径

    WCHAR* file_raw_path;
    ULONG file_raw_path_len;            // in bytes, including NULL

    // 2.进入沙箱的注册表入口点
    // （但请注意，Registry.dat文件位于file_path下方）
    // default:  HKEY_CURRENT_USER\Sandbox\BoxName

    WCHAR* key_path;
    ULONG key_path_len;                 // in bytes, including NULL

    // 3.沙箱的基于命名对象的目录
    // default:  \Sandbox\%SID%\Session_%SESSION%\BoxName

    WCHAR* ipc_path;
    ULONG ipc_path_len;                 // in bytes, including NULL

    // 4.与ipc路径相同，但反斜杠转换为下划线

    WCHAR* pipe_path;
    ULONG pipe_path_len;                // in bytes, including NULL

    WCHAR* spooler_directory;
    ULONG spooler_directory_len;

    WCHAR* system_temp_path;
    ULONG system_temp_path_len;

    WCHAR* user_temp_path;
    ULONG user_temp_path_len;
};

//如果指定的名称作为框名称有效，则返回TRUE。
//这并不一定意味着存在这样一个盒子。
BOOLEAN Box_IsValidName(const WCHAR* name);