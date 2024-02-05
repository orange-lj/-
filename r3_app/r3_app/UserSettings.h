#pragma once
class UserSettings
{
	static UserSettings* m_instance;
	CString m_SettingPrefix;
	CString m_Section;
	CString m_UserName;
	BOOL    m_IsAdmin;
	BOOL    m_CanEdit;
	BOOL    m_CanDisableForce;
	UserSettings();
	CString WithPrefix(const CString& Setting) const;
	CString GetUserSectionName(const CString& Setting) const;
public:
	~UserSettings();
	void GetText(const CString& Setting, CString& Value,
		const CString& Default = CString()) const;
	void GetBool(
		const CString& Setting, BOOL& Value, BOOL Default = 0) const;
	static UserSettings& GetInstance();
	void GetTextList(const CString& Setting, CStringList& Value) const;
	void GetTextCsv(const CString& Setting, CStringList& ValueList) const;
};

