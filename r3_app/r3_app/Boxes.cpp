#include "Boxes.h"
#include"../Dll/sbieapi.h"
#include"UserSettings.h"


static const CString _BoxExpandedView(L"BoxExpandedView");


Boxes::Boxes()
{
	ReloadBoxes();
}

void Boxes::Clear()
{
	while (GetSize()) 
	{
	//Box*
	}
	m_AnyHiddenBoxes = false;
}


Boxes::~Boxes()
{
}

void Boxes::ReloadBoxes()
{
	Clear();
	WCHAR name[64];
	int index = -1;
	while (1) 
	{
		if (GetSize() == 0)
			name[0] = L'\0';
		else 
		{
			index = SbieApi_EnumBoxesEx(index, name, TRUE);
			if (index == -1)
				break;
			LONG rc = SbieApi_IsBoxEnabled(name);
			if (rc == STATUS_ACCOUNT_RESTRICTION)
				m_AnyHiddenBoxes = true;
			if (rc != STATUS_SUCCESS)
				continue;
		}
		Box* box = new Box(name);
		int index2;
		for (index2 = 1; index2 < GetSize(); ++index2) 
		{
			const Box& oldBox = GetBox(index2);
			if (oldBox.GetName().CompareNoCase(name) > 0)
				break;
		}
		if (index2 > GetSize()) 
		{
			Add((CObject*)box);
		}
		else 
		{
			InsertAt(index2, (CObject*)box);
		}
	}
	//如果缺少DefaultBox，请添加它
	bool doWriteExpandedView = false;
	if (GetSize() == 1 ) 
	{
		//CBox* box = new CBox(m_DefaultBox);
		//Add((CObject*)box);
		//CBox::SetEnabled(box->GetName());
		//box->SetExpandedView(TRUE);
		//doWriteExpandedView = true;
	}
	ReadExpandedView();
	if (doWriteExpandedView) 
	{
	
	}
}

void Boxes::RefreshProcesses()
{

}

Boxes& Boxes::GetInstance()
{
	if (!m_instance)
		m_instance = new Boxes();
	return *m_instance;
}

Box& Boxes::GetBox(int index) const
{
	if (index < 0 || index > GetSize())
		index = 0;
	return *(Box*)GetAt(index);
}

Box& Boxes::GetBox(const CString& name) const
{
	for (int i = 0; i < GetSize(); ++i) {
		Box& box = GetBox(i);
		if (box.GetName().CompareNoCase(name) == 0)
			return box;
	}
	return GetBox(0);
}

void Boxes::ReadExpandedView()
{
	UserSettings& ini = UserSettings::GetInstance();
	//读取旧的BoxExpandedView_boxname设置
	for (int i = 0; i < GetSize(); ++i) 
	{
		Box& box = GetBox(i);
		CString boxname = box.GetName();
		if (!boxname.IsEmpty()) 
		{
			CString setting(_BoxExpandedView);
			setting += L"_";
			setting += boxname;
			BOOL value;
			ini.GetBool(setting, value, FALSE);
			if (value) 
			{
				//box.SetExpandedView(TRUE);
			}
		}
	}
	//读取新设置BoxExpandedView=boxname1，boxname2
	CStringList list;
	ini.GetTextCsv(_BoxExpandedView, list);
	while (!list.IsEmpty()) 
	{
		CString boxname = list.RemoveHead();
		if (!boxname.IsEmpty()) {
			Box& box = GetBox(boxname);
			if (!box.GetName().IsEmpty()) 
			{
				//box.SetExpandedView(TRUE);
			}
		}
	}
}
