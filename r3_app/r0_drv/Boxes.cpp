#include "Boxes.h"

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
		
		}
		Box* box = new Box(name);
	}
}

Boxes& Boxes::GetInstance()
{
	if (!m_instance)
		m_instance = new Boxes();
	return *m_instance;
}
