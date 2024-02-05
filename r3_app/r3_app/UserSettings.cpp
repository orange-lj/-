#include "pch.h"
#include "UserSettings.h"
#include"../common/my_version.h"
#include"SbieIni.h"

void UserSettings::GetText(const CString& Setting, CString& Value, const CString& Default) const
{
    CString section = GetUserSectionName(Setting);
    SbieIni::GetInstance().GetText(
        section, WithPrefix(Setting), Value, Default);
}

void UserSettings::GetBool(const CString& Setting, BOOL& Value, BOOL Default) const
{
    CString section = GetUserSectionName(Setting);
    SbieIni::GetInstance().GetBool(
        section, WithPrefix(Setting), Value, Default);
}

UserSettings& UserSettings::GetInstance()
{
    if (!m_instance) 
    {
        m_instance = new UserSettings();
    }
    return *m_instance;
}

void UserSettings::GetTextList(const CString& Setting, CStringList& Value) const
{
    CString section = GetUserSectionName(Setting);
    SbieIni::GetInstance().GetTextList(
        section, WithPrefix(Setting), Value);
}

void UserSettings::GetTextCsv(const CString& Setting, CStringList& ValueList) const
{
    while (!ValueList.IsEmpty()) 
    {
        ValueList.RemoveHead();
    }
    CStringList ValueListRead;
    GetTextList(Setting, ValueListRead);

    while (!ValueListRead.IsEmpty()) 
    {
        CString text = ValueListRead.RemoveHead();
        while (!text.IsEmpty()) 
        {
            CString NextValue;
            int comma = text.Find(L',');
            if (comma == -1) 
            {
                NextValue = text;
                text = CString();
            }
            else 
            {
                NextValue = text.Left(comma);
                text = text.Mid(comma + 1);
            }
            NextValue.TrimLeft();
            NextValue.TrimRight();
            if (!NextValue.IsEmpty())
                ValueList.AddTail(NextValue);
        }
    }
}


UserSettings::UserSettings() 
{
    m_SettingPrefix = CString(SBIECTRL_);
    SbieIni::GetInstance().GetUser(m_Section, m_UserName, m_IsAdmin);
    m_CanEdit = TRUE;
    m_CanDisableForce = TRUE;

    if (!m_IsAdmin) 
    {
    
    }
    //确保存在UserName设置
    static const WCHAR* _UserName = L"UserName";
    static const WCHAR* _QuestionMark = L"?";
    CString TempUserName;
    GetText(_UserName, TempUserName, _QuestionMark);
    if (TempUserName == _QuestionMark) 
    {
    
    }
}

CString UserSettings::WithPrefix(const CString& Setting) const
{
    return m_SettingPrefix + Setting;
}

CString UserSettings::GetUserSectionName(const CString& Setting) const
{
    static const CString _invalid(L"|!@#$%^&*(),.?|");
    static const CString _default(L"UserSettings_Default");

    CString section = m_Section;
    CString value;
    SbieIni::GetInstance().
        GetText(section, WithPrefix(Setting), value, _invalid);
    if (value == _invalid)
        section = _default;
    return section;
}
