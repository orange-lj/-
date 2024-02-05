#pragma once

#include"main.h"

typedef struct _BOX             BOX;

struct _BOX {

    //ɳ�������������ʶ��
    // 1.������ӵ�����

    WCHAR name[BOXNAME_COUNT];
    ULONG name_len;                     // in bytes, including NULL

    // 2.������ɳ����û��ʻ�

    WCHAR* sid;
    ULONG sid_len;                      // in bytes, including NULL

    // 3.�ն˷���Ự���

    ULONG session_id;

    // ���ڴ˿�ʱConf_Expand�Ĳ���

    CONF_EXPAND_ARGS* expand_args;

    // ɳ��������·����
    // 1.����ɳ����ļ�ϵͳ��ڵ�
    // default:  \??\C:\Sandbox\%SID%\BoxName

    WCHAR* file_path;
    ULONG file_path_len;                // in bytes, including NULL

    // ���ļ�·�������·������ض���ʱ�����Ǳ���ԭʼ·��

    WCHAR* file_raw_path;
    ULONG file_raw_path_len;            // in bytes, including NULL

    // 2.����ɳ���ע�����ڵ�
    // ������ע�⣬Registry.dat�ļ�λ��file_path�·���
    // default:  HKEY_CURRENT_USER\Sandbox\BoxName

    WCHAR* key_path;
    ULONG key_path_len;                 // in bytes, including NULL

    // 3.ɳ��Ļ������������Ŀ¼
    // default:  \Sandbox\%SID%\Session_%SESSION%\BoxName

    WCHAR* ipc_path;
    ULONG ipc_path_len;                 // in bytes, including NULL

    // 4.��ipc·����ͬ������б��ת��Ϊ�»���

    WCHAR* pipe_path;
    ULONG pipe_path_len;                // in bytes, including NULL

    WCHAR* spooler_directory;
    ULONG spooler_directory_len;

    WCHAR* system_temp_path;
    ULONG system_temp_path_len;

    WCHAR* user_temp_path;
    ULONG user_temp_path_len;
};

//���ָ����������Ϊ��������Ч���򷵻�TRUE��
//�Ⲣ��һ����ζ�Ŵ�������һ�����ӡ�
BOOLEAN Box_IsValidName(const WCHAR* name);