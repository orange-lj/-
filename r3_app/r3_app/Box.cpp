#include "pch.h"
#include "Box.h"

Box::Box(const CString& name)
{
    m_name = name;
    m_BoxProc = NULL;
    m_BoxFile = NULL;
}
