#pragma once

#include <afxtempl.h>
#include"Box.h"
class Boxes:public CObArray
{	
	static Boxes* m_instance;
	Boxes();
	void Clear();
	bool m_AnyHiddenBoxes;
public:
	~Boxes();
	void ReloadBoxes();
	void RefreshProcesses();
	static Boxes& GetInstance();
	Box& GetBox(int index) const;
	Box& GetBox(const CString& name) const;
	void ReadExpandedView();
};

