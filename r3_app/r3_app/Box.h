#pragma once
#include"BoxProc.h"
#include"BoxFile.h"

class Box
{
    CString m_name;
    BoxProc* m_BoxProc;
    BoxFile* m_BoxFile;
    BOOL m_expandedView;
    BOOL m_TemporarilyDisableQuickRecover;
public:

    Box(const CString& name);

    const CString& GetName() const;
    void SetExpandedView(BOOL view);

    //Settings
    void SetDefaultSettings();
};

