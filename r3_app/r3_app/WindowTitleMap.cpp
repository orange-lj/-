#include "pch.h"
#include "WindowTitleMap.h"
#include"../Dll/sbieapi.h"

typedef int (*P_InternalGetWindowText)(
    HWND hWnd, LPWSTR lpString, int nMaxCount);

struct MapEntry 
{
    WCHAR name[100];
    HICON icon;
    FILETIME time;
    BOOL valid;
};


BOOL WindowTitleMap::ShouldIgnoreProcess(ULONG pid)
{
    ULONG i, num;

    num = m_pids[0];
    if (num > 500)
        return FALSE;

    for (i = 1; i <= num; ++i)
        if (m_pids[i] == pid)
            return FALSE;

    return TRUE;
}

BOOL WindowTitleMap::EnumProc(HWND hwnd, LPARAM lParam)
{
    if (GetParent(hwnd) || GetWindow(hwnd, GW_OWNER))
        return TRUE;
    ULONG style = GetWindowLong(hwnd, GWL_STYLE);
    if ((style & (WS_CAPTION | WS_SYSMENU)) != (WS_CAPTION | WS_SYSMENU))
        return TRUE;
    if (!IsWindowVisible(hwnd))
        return TRUE;
    /*
    if ((style & WS_OVERLAPPEDWINDOW) != WS_OVERLAPPEDWINDOW &&
        (style & WS_POPUPWINDOW)      != WS_POPUPWINDOW)
        return TRUE;
    */

    WindowTitleMap& map = *(WindowTitleMap*)lParam;

    ULONG64 pid;
    ULONG pid32;
    GetWindowThreadProcessId(hwnd, &pid32);
    pid = pid32;


    if (map.ShouldIgnoreProcess((ULONG)(ULONG_PTR)pid))
        return TRUE;

    MapEntry* entry;
    void* ptr;
    BOOL ok = map.Lookup((void*)pid, ptr);
    if (ok && ptr) {
        entry = (MapEntry*)ptr;
        if ((map.m_counter % 5) == 0)
            entry->name[0] = L'\0';
    }
    else {
        entry = new MapEntry();
        entry->name[0] = L'\0';
        entry->icon = NULL;
        entry->time.dwLowDateTime = 0;
        entry->time.dwHighDateTime = 0;
    }
    entry->valid = TRUE;

    if (!entry->name[0]) {

        ((P_InternalGetWindowText)map.m_pGetWindowText)(hwnd, entry->name, 99);
        entry->name[99] = L'\0';

        map.SetAt((void*)pid, entry);
    }

    return TRUE;
}

void WindowTitleMap::RefreshIcons()
{
    WCHAR path[300];
    void* key, * ptr;
    POSITION pos = GetStartPosition();
    while (pos) 
    {
    
    }
}

void WindowTitleMap::Refresh()
{
    ++m_counter;
    SbieApi_EnumProcess(NULL, m_pids);
    EnumWindows(WindowTitleMap::EnumProc, (LPARAM)this);
    RefreshIcons();
}

WindowTitleMap& WindowTitleMap::GetInstance()
{
    return *m_instance;
}
