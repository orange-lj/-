#pragma once
class BoxProc
{
	const CString& m_name;
	ULONG m_pids[512];
	CString* m_images;
	CString* m_titles;
	HICON* m_icons;
	int m_num, m_max;
	int m_old_num;
public:
	BoxProc(const CString& name);
	void RefreshProcesses();
};

