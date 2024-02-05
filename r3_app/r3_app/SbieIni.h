#pragma once
class SbieIni
{
	static SbieIni* m_instance;
	BOOL m_ExpandsInit;

	WCHAR m_Password[66];
	SbieIni();

public:
	static SbieIni& GetInstance();
	BOOL GetUser(CString& Section, CString& Name, BOOL& IsAdmin);
	//»ù±¾±à¼­
	void GetText(
		const CString& Section, const CString& Setting,
		CString& Value, const CString& Default = CString()) const;
	void GetNum(
		const CString& Section, const CString& Setting,
		int& Value, int Default = 0) const;
	void GetBool(
		const CString& Section, const CString& Setting,
		BOOL& Value, BOOL Default = 0) const;
	void GetTextList(
		const CString& Section, const CString& Setting,
		CStringList& Value, BOOL withBrackets = FALSE) const;
};

