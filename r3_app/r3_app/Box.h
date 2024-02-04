#pragma once
class Box
{
    CString m_name;
    BoxProc* m_BoxProc;
    BoxFile* m_BoxFile;
public:

    Box(const CString& name);
};

