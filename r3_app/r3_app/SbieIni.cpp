#include "pch.h"
#include "SbieIni.h"
#include"../svc/sbieiniwire.h"
#include"../r0_drv/api_flags.h"
#include"../Dll/sbieapi.h"


#define REQUEST_LEN                 4096

SbieIni& SbieIni::GetInstance()
{
    // TODO: 在此处插入 return 语句
    if (!m_instance) 
    {
        m_instance = new SbieIni();
    }
    return *m_instance;
}

BOOL SbieIni::GetUser(CString& Section, CString& Name, BOOL& IsAdmin)
{
    Section = CString();
    Name = CString();
    IsAdmin = FALSE;

    SBIE_INI_GET_USER_REQ* req =
        (SBIE_INI_GET_USER_REQ*)malloc(REQUEST_LEN);
    if (req) 
    {
        req->h.msgid = MSGID_SBIE_INI_GET_USER;
        req->h.length = sizeof(SBIE_INI_GET_USER_REQ);
        SBIE_INI_GET_USER_RPL* rpl =
            (SBIE_INI_GET_USER_RPL*)SbieDll_CallServer(&req->h);
        if (rpl) 
        {
            if (rpl->h.status == 0) 
            {
                if (rpl->admin) 
                {
                    IsAdmin = TRUE;
                }
                Section = CString(rpl->section);
                Name = CString(rpl->name);
            }
            SbieDll_FreeMem(rpl);
        }
        free(req);
    }
    if (Section.IsEmpty()) 
    {
    
    }
    return (!(Section.IsEmpty() || Name.IsEmpty()));
}

void SbieIni::GetText(const CString& Section, const CString& Setting, CString& Value, const CString& Default) const
{
    if (Section.IsEmpty()) 
    {
        Value = Default;
        return;
    }
    ULONG buf_len = sizeof(WCHAR) * CONF_LINE_LEN;
    WCHAR* buf = (WCHAR*)malloc(buf_len);
    int flags = Section.IsEmpty() ? 0 : CONF_GET_NO_GLOBAL;
    SbieApi_QueryConfAsIs(
        Section, Setting, flags, buf, buf_len - 4);
    if (buf[0]) 
    {
        Value = buf;
    }
    else 
    {
        Value = Default;
    }
    free(buf);
}

void SbieIni::GetNum(const CString& Section, const CString& Setting, int& Value, int Default) const
{
    CString ValueStr;
    GetText(Section, Setting, ValueStr);
    Value = _wtoi(ValueStr);
    if (!Value)
        Value = Default;
}

void SbieIni::GetBool(const CString& Section, const CString& Setting, BOOL& Value, BOOL Default) const
{
    CString ValueStr;
    GetText(Section, Setting, ValueStr);
    WCHAR ch = 0;
    if (!ValueStr.IsEmpty())
        ch = ValueStr.GetAt(0);
    if (ch == L'y' || ch == L'Y')
        Value = TRUE;
    else if (ch == L'n' || ch == L'N')
        Value = FALSE;
    else
        Value = Default;
}

void SbieIni::GetTextList(const CString& Section, const CString& Setting, CStringList& Value, BOOL withBrackets) const
{
    if (Section.IsEmpty())
        return;

    ULONG buf_len = sizeof(WCHAR) * CONF_LINE_LEN;
    WCHAR* buf = (WCHAR*)malloc(buf_len);

    int index = 0;
    int flags = Section.IsEmpty() ? 0 : CONF_GET_NO_GLOBAL;
    if (withBrackets)
        flags |= CONF_GET_NO_TEMPLS;
    while (1) 
    {
        SbieApi_QueryConfAsIs(
            Section, Setting, index | flags, buf, buf_len - 4);
        if (!buf[0])
            break;
        ++index;
        Value.AddTail(buf);
    }
    if (withBrackets) 
    {
    
    }
    free(buf);
}


SbieIni::SbieIni() 
{
    //分配并初始化SbieSvc请求缓冲区
    m_ExpandsInit = FALSE;

    m_Password[0] = L'\0';
}