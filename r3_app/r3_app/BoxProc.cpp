#include "pch.h"
#include "BoxProc.h"
#include"../common/defines.h"
#include"../Dll/sbieapi.h"

BoxProc::BoxProc(const CString& name):m_name(name)
{
	memzero(m_pids, sizeof(m_pids));
    m_images = NULL;
    m_titles = NULL;
    m_icons = NULL;
    m_num = 0;
    m_max = 0;
    m_old_num = 0;
}

void BoxProc::RefreshProcesses()
{
    if (m_name.IsEmpty()) 
    {
        return;
    }
    m_old_num = m_num;

    WCHAR name[256];
    wcscpy(name, m_name);
    SbieApi_EnumProcess(name, m_pids);
    m_num = m_pids[0];

    //快捷方式案例：找不到进程
    if (m_num == 0) 
    {
        return;
    }
}
