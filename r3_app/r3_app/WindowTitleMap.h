#pragma once
class WindowTitleMap:public CMapPtrToPtr
{
	static WindowTitleMap* m_instance;
	ULONG m_counter;
	ULONG m_pids[512];
	void* m_pGetWindowText;
	WindowTitleMap();
	BOOL ShouldIgnoreProcess(ULONG pid);
	static BOOL EnumProc(HWND hwnd, LPARAM lParam);
	void RefreshIcons();
public:
	~WindowTitleMap();
	void Refresh();
	static WindowTitleMap& GetInstance();
};

