#include "pch.h"
#include "Box.h"
#include"SbieIni.h"


static const CString _ConfigLevel(L"ConfigLevel");

Box::Box(const CString& name)
{
    m_name = name;
    m_BoxProc = NULL;
    m_BoxFile = NULL;

    if (m_name.IsEmpty()) 
    {
        m_expandedView = TRUE;
    }
    else 
    {
        m_expandedView = FALSE;
        SetDefaultSettings();
    }
    m_TemporarilyDisableQuickRecover = FALSE;
}

BoxProc& Box::GetBoxProc()
{
    if (!m_BoxProc)
        m_BoxProc = new BoxProc(m_name);
    return *m_BoxProc;
}

const CString& Box::GetName() const
{
    return m_name;
}

void Box::SetExpandedView(BOOL view)
{
    m_expandedView = view;
}

void Box::SetDefaultSettings()
{
    SbieIni& ini = SbieIni::GetInstance();
    int cfglvl;
    ini.GetNum(m_name, _ConfigLevel, cfglvl);

    if (cfglvl >= 10)
        return;
}
