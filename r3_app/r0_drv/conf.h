#pragma once
//#include"pool.h"
#include"main.h"

BOOLEAN Conf_Init(void);
BOOLEAN Conf_Init_User(void);


NTSTATUS Conf_Api_Reload(PROCESS* proc, ULONG64* parms);

NTSTATUS Conf_Api_Query(PROCESS* proc, ULONG64* parms);

//Conf_AdjustUseCount：在调用Conf_Get序列之前和之后使用，以确保如果同时调用Conf_Api_Reload，
//则返回的字符串不会蒸发
void Conf_AdjustUseCount(BOOLEAN increase);

//Conf_Get：返回一个指向字符串配置数据的指针。使用
//使用Conf_AdjustUseCount来确保返回的指针有效
const WCHAR* Conf_Get(
    const WCHAR* section, const WCHAR* setting, ULONG index);


static const WCHAR* Conf_Get_Helper(
    const WCHAR* section_name, const WCHAR* setting_name,
    ULONG* index, BOOLEAN skip_tmpl);


//Conf_Get_Boolean：解析y/n设置。该函数不必使用Conf_AdjustUseCount进行保护
BOOLEAN Conf_Get_Boolean(
    const WCHAR* section, const WCHAR* setting, ULONG index, BOOLEAN def);
