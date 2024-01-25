#pragma once
//#include"pool.h"
#include"main.h"

BOOLEAN Conf_Init(void);
BOOLEAN Conf_Init_User(void);


NTSTATUS Conf_Api_Reload(PROCESS* proc, ULONG64* parms);

NTSTATUS Conf_Api_Query(PROCESS* proc, ULONG64* parms);

//Conf_AdjustUseCount���ڵ���Conf_Get����֮ǰ��֮��ʹ�ã���ȷ�����ͬʱ����Conf_Api_Reload��
//�򷵻ص��ַ�����������
void Conf_AdjustUseCount(BOOLEAN increase);

//Conf_Get������һ��ָ���ַ����������ݵ�ָ�롣ʹ��
//ʹ��Conf_AdjustUseCount��ȷ�����ص�ָ����Ч
const WCHAR* Conf_Get(
    const WCHAR* section, const WCHAR* setting, ULONG index);


static const WCHAR* Conf_Get_Helper(
    const WCHAR* section_name, const WCHAR* setting_name,
    ULONG* index, BOOLEAN skip_tmpl);


//Conf_Get_Boolean������y/n���á��ú�������ʹ��Conf_AdjustUseCount���б���
BOOLEAN Conf_Get_Boolean(
    const WCHAR* section, const WCHAR* setting, ULONG index, BOOLEAN def);
