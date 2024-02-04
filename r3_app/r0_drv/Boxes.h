#pragma once

#include <afxtempl.h>
#include"box.h"
class Boxes:public CObArray
{	
	static Boxes* m_instance;
	Boxes();
	void Clear();
	bool m_AnyHiddenBoxes;
public:
	~Boxes();
	void ReloadBoxes();
	static Boxes& GetInstance();
};

