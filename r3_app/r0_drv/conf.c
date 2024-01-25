#include"conf.h"
#include"mem.h"
#include"../common/list.h"
#include"../common/map.h"
#include"../common/stream.h"
#include"api_defs.h"
#include"api.h"
#include"util.h"
#include"api_flags.h"

//defines
	//��sbeiniwire����ͬ��
#define CONF_LINE_LEN               2000        
#define CONF_MAX_LINES              100000      
#define CONF_TMPL_LINE_BASE         0x01000000
//�ṹ��
typedef struct _CONF_DATA
{
	POOL* pool;
	LIST sections;
	HASH_MAP sections_map;
	ULONG home;
	WCHAR* path;
	ULONG encoding;
	volatile ULONG use_count;
}CONF_DATA;

typedef struct _CONF_SECTION {

	LIST_ELEM list_elem;
	WCHAR* name;
	LIST settings;      // CONF_SETTING
	HASH_MAP settings_map;
	BOOLEAN from_template;

} CONF_SECTION;

typedef struct _CONF_SETTING {

	LIST_ELEM list_elem;
	WCHAR* name;
	WCHAR* value;
	BOOLEAN from_template;
	BOOLEAN template_handled;

} CONF_SETTING;

//����
static CONF_DATA Conf_Data;
static PERESOURCE Conf_Lock = NULL;
static LIST Conf_Users;
static PERESOURCE Conf_Users_Lock = NULL;
static KEVENT* Conf_Users_Event = NULL;

static const WCHAR* Conf_GlobalSettings = L"GlobalSettings";
static const WCHAR* Conf_UserSettings_ = L"UserSettings_";
static const WCHAR* Conf_DefaultTemplates = L"DefaultTemplates";
static const WCHAR* Conf_Template_ = L"Template_";

static const WCHAR* Conf_Template = L"Template";
const WCHAR* Conf_Tmpl = L"Tmpl.";

static NTSTATUS Conf_Read(ULONG session_id);
static NTSTATUS Conf_Api_SetUserName(PROCESS* proc, ULONG64* parms);
static NTSTATUS Conf_Api_IsBoxEnabled(PROCESS* proc, ULONG64* parms);
static NTSTATUS Conf_Merge_Templates(CONF_DATA* data, ULONG session_id);
NTSTATUS Conf_Read_Line(STREAM* stream, WCHAR* line, int* linenum);
static NTSTATUS Conf_Read_Sections(
	STREAM* stream, CONF_DATA* data, int* linenum);
static NTSTATUS Conf_Read_Settings(
	STREAM* stream, CONF_DATA* data, CONF_SECTION* section,
	WCHAR* line, int* linenum);
CONF_SECTION* Conf_Get_Section(
	CONF_DATA* data, const WCHAR* section_name);
static NTSTATUS Conf_Merge_Global(
	CONF_DATA* data, ULONG session_id,
	CONF_SECTION* global);
static NTSTATUS Conf_Merge_Template(
	CONF_DATA* data, ULONG session_id,
	const WCHAR* tmpl_name, CONF_SECTION* section);

BOOLEAN Conf_Init(void)
{
	Conf_Data.pool = NULL;
	List_Init(&Conf_Data.sections);
	map_init(&Conf_Data.sections_map, NULL);
	Conf_Data.sections_map.func_key_size = NULL;
	Conf_Data.sections_map.func_match_key = &str_map_match;
	Conf_Data.sections_map.func_hash_key = &str_map_hash;
	Conf_Data.home = FALSE;
	Conf_Data.path = NULL;
	Conf_Data.encoding = 0;

	if (!Mem_GetLockResource(&Conf_Lock, TRUE))
		return FALSE;
	if (!Conf_Init_User())
		return FALSE;
	Conf_Read(-1);
	Api_SetFunction(API_RELOAD_CONF, Conf_Api_Reload);
	Api_SetFunction(API_QUERY_CONF, Conf_Api_Query);
}

BOOLEAN Conf_Init_User(void)
{
	List_Init(&Conf_Users);
	Conf_Users_Event = ExAllocatePoolWithTag(NonPagedPool, sizeof(KEVENT), tzuk);
	if (!Conf_Users_Event)
	{
		//Log_Msg0(MSG_1104);
		return FALSE;
	}
	KeInitializeEvent(Conf_Users_Event, SynchronizationEvent, FALSE);
	if (!Mem_GetLockResource(&Conf_Users_Lock, TRUE))
		return FALSE;
	Api_SetFunction(API_SET_USER_NAME, Conf_Api_SetUserName);
	Api_SetFunction(API_IS_BOX_ENABLED, Conf_Api_IsBoxEnabled);
	return TRUE;
}

NTSTATUS Conf_Api_Reload(PROCESS* proc, ULONG64* parms)
{
	//�´�����
	return STATUS_SUCCESS;
}

NTSTATUS Conf_Api_Query(PROCESS* proc, ULONG64* parms)
{
	//�´�����
	return STATUS_SUCCESS;
}

void Conf_AdjustUseCount(BOOLEAN increase)
{
	KIRQL irql;
	KeRaiseIrql(APC_LEVEL, &irql);
	ExAcquireResourceExclusiveLite(Conf_Lock, TRUE);

	if (increase)
		InterlockedIncrement(&Conf_Data.use_count);
	else
		InterlockedDecrement(&Conf_Data.use_count);

	ExReleaseResourceLite(Conf_Lock);
	KeLowerIrql(irql);
}

const WCHAR* Conf_Get(const WCHAR* section, const WCHAR* setting, ULONG index)
{
	const WCHAR* value;
	BOOLEAN have_section;
	BOOLEAN have_setting;
	BOOLEAN check_global;
	BOOLEAN skip_tmpl;
	KIRQL irql;

	value = NULL;
	have_section = (section && section[0]);
	have_setting = (setting && setting[0]);
	skip_tmpl = ((index & CONF_GET_NO_TEMPLS) != 0);

	KeRaiseIrql(APC_LEVEL, &irql);
	ExAcquireResourceSharedLite(Conf_Lock, TRUE);
	if ((!have_section) && have_setting &&
		_wcsicmp(setting, L"IniLocation") == 0) 
	{

	}
	else if ((!have_section) && have_setting &&
		_wcsicmp(setting, L"IniEncoding") == 0)
	{
	
	}
	else if (have_setting) 
	{
		check_global = ((index & CONF_GET_NO_GLOBAL) == 0);
		index &= 0xFFFF;
		if (section)
			value = Conf_Get_Helper(section, setting, &index, skip_tmpl);
		//���δ�ҵ������ڵ�ֵ���볢�Դ�ȫ�ֽڻ�ȡ��ֵ
		if ((!value) && check_global) 
		{
			value = Conf_Get_Helper(
				Conf_GlobalSettings, setting, &index, skip_tmpl);
		}
	}
	ExReleaseResourceLite(Conf_Lock);
	KeLowerIrql(irql);

	return value;
}

const WCHAR* Conf_Get_Helper(const WCHAR* section_name, const WCHAR* setting_name, ULONG* index, BOOLEAN skip_tmpl)
{
	WCHAR* value;
	CONF_SECTION* section;
	CONF_SETTING* setting;

	value = NULL;
	//���ҹ�ϣͼ�еĲ���
	section = map_get(&Conf_Data.sections_map, section_name);
	if (skip_tmpl && section && section->from_template)
		section = NULL;
	if (section) 
	{
		//ʹ�ô��������������������ƥ�������
		map_iter_t iter2 = map_key_iter(&section->settings_map, setting_name);
		while (map_next(&section->settings_map, &iter2))
		{
			setting = iter2.value;
			if (skip_tmpl && setting->from_template) 
			{
			
			}
			if (*index == 0) 
			{
				value = setting->value;
				break;
			}
		}
	}
	return value;
}

BOOLEAN Conf_Get_Boolean(const WCHAR* section, const WCHAR* setting, ULONG index, BOOLEAN def)
{
	const WCHAR* value;
	BOOLEAN retval;

	Conf_AdjustUseCount(TRUE);
	value = Conf_Get(section, setting, index);
	retval = def;
	if (value) 
	{
		if (*value == 'y' || *value == 'Y')
			retval = TRUE;
		else if (*value == 'n' || *value == 'N')
			retval = FALSE;
	}
	Conf_AdjustUseCount(FALSE);
	return retval;
}


NTSTATUS Conf_Read_Sections(STREAM* stream, CONF_DATA* data, int* linenum)
{
	const int line_len = (CONF_LINE_LEN + 2) * sizeof(WCHAR);
	NTSTATUS status;
	WCHAR* line;
	WCHAR* ptr;
	CONF_SECTION* section;

	line = Mem_Alloc(data->pool, line_len);
	if (!line)
		return STATUS_INSUFFICIENT_RESOURCES;
	status = Conf_Read_Line(stream, line, linenum);
	while (NT_SUCCESS(status))
	{
		//�ӽ���������ȡ������
		if (line[0] != L'[')
		{
			status = STATUS_INVALID_PARAMETER;
			break;
		}
		ptr = &line[1];
		while (*ptr && *ptr != L']')
			++ptr;
		if (*ptr != L']')
		{
			status = STATUS_INVALID_PARAMETER;
			break;
		}
		*ptr = L'\0';

		if (_wcsnicmp(&line[1], Conf_UserSettings_, 13) == 0) {
			if (!line[14]) {
				status = STATUS_INVALID_PARAMETER;
				break;
			}
		}
		else if (_wcsnicmp(&line[1], Conf_Template_, 9) == 0) {
			if (!line[10]) {
				status = STATUS_INVALID_PARAMETER;
				break;
			}
		}
		/*else if (!Box_IsValidName(&line[1])) {
			status = STATUS_INVALID_PARAMETER;
			break;
		}*/
		//�Ը����Ʋ������нڻ򴴽��½�
		section = map_get(&data->sections_map, &line[1]);
		if (!section)
		{
			section = Mem_Alloc(data->pool, sizeof(CONF_SECTION));
			if (!section)
			{
				status = STATUS_INSUFFICIENT_RESOURCES;
				break;
			}
			if ((*linenum) >= CONF_TMPL_LINE_BASE)
			{

			}
			else
			{
				section->from_template = FALSE;
			}
			section->name = Mem_AllocString(data->pool, &line[1]);
			if (!section->name)
			{
				status = STATUS_INSUFFICIENT_RESOURCES;
				break;
			}
			List_Init(&section->settings);
			map_init(&section->settings_map, data->pool);
			section->settings_map.func_key_size = NULL;
			section->settings_map.func_match_key = &str_map_match;
			section->settings_map.func_hash_key = &str_map_hash;
			map_resize(&section->settings_map, 16);
			List_Insert_After(&data->sections, NULL, section);
			if (map_insert(&data->sections_map, section->name, section, 0) == NULL)
			{
				status = STATUS_INSUFFICIENT_RESOURCES;
				break;
			}
		}
		//��ȡ�˷���������
		status = Conf_Read_Settings(stream, data, section, line, linenum);
	}
	Mem_Free(line, line_len);

	return status;
}

NTSTATUS Conf_Read_Settings(STREAM* stream, CONF_DATA* data, CONF_SECTION* section, WCHAR* line, int* linenum)
{
	NTSTATUS status;
	WCHAR* ptr;
	WCHAR* value;
	CONF_SETTING* setting;

	while (1)
	{
		status = Conf_Read_Line(stream, line, linenum);
		if (!NT_SUCCESS(status))
			break;

		if (line[0] == L'[' || line[0] == L']')
			break;

		//������������=ֵ
		ptr = wcschr(line, L'=');
		if ((!ptr) || ptr == line)
		{
			status = STATUS_INVALID_PARAMETER;
			break;
		}
		value = &ptr[1];
		//�������������е�β��հ�
		while (ptr > line)
		{
			--ptr;
			if (*ptr > 32)
			{
				++ptr;
				break;
			}
		}
		*ptr = L'\0';
		//����ֵ�е�ǰ����β��ո�
		while (*value <= 32) {
			if (!(*value))
				break;
			++value;
		}

		if (*value == L'\0') {
			status = STATUS_INVALID_PARAMETER;
			break;
		}

		ptr = value + wcslen(value);
		while (ptr > value) {
			--ptr;
			if (*ptr > 32) {
				++ptr;
				break;
			}
		}
		*ptr = L'\0';
		//����������
		setting = Mem_Alloc(data->pool, sizeof(CONF_SETTING));
		if (!setting) {
			status = STATUS_INSUFFICIENT_RESOURCES;
			break;
		}

		if ((*linenum) >= CONF_TMPL_LINE_BASE)
			setting->from_template = TRUE;
		else
			setting->from_template = FALSE;

		setting->template_handled = FALSE;
		setting->name = Mem_AllocString(data->pool, line);
		if (!setting->name) {
			status = STATUS_INSUFFICIENT_RESOURCES;
			break;
		}
		setting->value = Mem_AllocString(data->pool, value);
		if (!setting->value) {
			status = STATUS_INSUFFICIENT_RESOURCES;
			break;
		}

		List_Insert_After(&section->settings, NULL, setting);
		if (map_append(&section->settings_map, setting->name, setting, 0) == NULL) {
			status = STATUS_INSUFFICIENT_RESOURCES;
			break;
		}
	}
	return status;
}

CONF_SECTION* Conf_Get_Section(CONF_DATA* data, const WCHAR* section_name)
{
	return map_get(&data->sections_map, section_name);
}

NTSTATUS Conf_Merge_Global(CONF_DATA* data, ULONG session_id, CONF_SECTION* global)
{
	NTSTATUS status;
	CONF_SECTION* sandbox;
	CONF_SETTING* setting;

	//ɨ��ò����Բ���Template=Xxx����
	setting = List_Head(&global->settings);
	while (setting)
	{
		if (_wcsicmp(setting->name, Conf_Template) != 0)
		{
			setting = List_Next(setting);
			continue;
		}
		//ɨ������Բ���ɳ�з���
		sandbox = List_Head(&data->sections);
		while (sandbox)
		{
			CONF_SECTION* next_sandbox = List_Next(sandbox);
			//ģ�岿���������ж�
			if (sandbox->from_template)
			{
				//���ǿ����жϣ���Ϊģ�岿��λ�����з�ģ�岿��֮��
				break;
			}
			//����ȫ�ֲ��֡��κ�ģ�岿�ֺ��û����ò���
			if (_wcsicmp(sandbox->name, Conf_GlobalSettings) == 0 ||
				_wcsnicmp(sandbox->name, Conf_Template_, 9) == 0 ||
				_wcsnicmp(sandbox->name, Conf_UserSettings_, 13) == 0)
			{
				sandbox = next_sandbox;
				continue;
			}
			//��ģ��ϲ���ɳ�䲿��
			status = Conf_Merge_Template(data, session_id, setting->value, sandbox);
			if (!NT_SUCCESS(status))
				return status;
			sandbox = next_sandbox;
		}
		setting = List_Next(setting);
	}
	return STATUS_SUCCESS;
}

NTSTATUS Conf_Merge_Template(CONF_DATA* data, ULONG session_id, const WCHAR* tmpl_name, CONF_SECTION* section)
{
	CONF_SECTION* tmpl = NULL;

	WCHAR section_name[130]; // 128 + 2 // max regular section length is 64
	if (wcslen(tmpl_name) < 119)
	{
		wcscpy(section_name, Conf_Template_);
		wcscat(section_name, tmpl_name);
		tmpl = Conf_Get_Section(data, section_name);
	}
	//�����ô�ģ�岿�ָ��Ƶ�ɳ�䲿��
	if (tmpl)
	{
		CONF_SETTING* oset, * nset;
		oset = List_Head(&tmpl->settings);
		while (oset)
		{
			if (_wcsnicmp(oset->name, Conf_Tmpl, 5) == 0)
			{
				oset = List_Next(oset);
				continue;
			}
			nset = Mem_Alloc(data->pool, sizeof(CONF_SETTING));
			nset->from_template = TRUE;
			nset->template_handled = FALSE;
			if (!nset)
				return STATUS_INSUFFICIENT_RESOURCES;
			nset->name = Mem_AllocString(data->pool, oset->name);
			if (!nset->name)
				return STATUS_INSUFFICIENT_RESOURCES;
			nset->value = Mem_AllocString(data->pool, oset->value);
			if (!nset->value)
				return STATUS_INSUFFICIENT_RESOURCES;

			List_Insert_After(&section->settings, NULL, nset);
			if (map_append(&section->settings_map, nset->name, nset, 0) == NULL)
				return STATUS_INSUFFICIENT_RESOURCES;
			oset = List_Next(oset);
		}
	}
	else
	{

	}
	return STATUS_SUCCESS;
}

NTSTATUS Conf_Read(ULONG session_id)
{
	static const WCHAR* path_sandboxie = L"%s\\" SANDBOXIE_INI;
	static const WCHAR* path_templates = L"%s\\Templates.ini";
	static const WCHAR* SystemRoot = L"\\SystemRoot";
	NTSTATUS status;
	CONF_DATA data;
	int linenum;
	WCHAR linenum_str[32];
	ULONG path_len;
	WCHAR* path = NULL;
	ULONG path_home;
	STREAM* stream = NULL;
	POOL* pool;

	//Ϊ\SystemRoot\Sandboxie.ini����·����\Sandboxe.ini�����㹻��Ļ�����
	path_len = 260 * sizeof(WCHAR);
	if (path_len < wcslen(Driver_HomePathDos) * sizeof(WCHAR) + 64)
		path_len = wcslen(Driver_HomePathDos) * sizeof(WCHAR) + 64;
	pool = Pool_Create();
	if (!pool)
		return STATUS_INSUFFICIENT_RESOURCES;
	path = Mem_Alloc(pool, path_len);
	if (!path) {
		//Pool_Delete(pool);
		return STATUS_INSUFFICIENT_RESOURCES;
	}
	//���Դ��Զ��������ļ�����������ã�
	UNICODE_STRING IniPath = { 0, (USHORT)path_len - (4 * sizeof(WCHAR)), path };
	status = GetRegString(RTL_REGISTRY_ABSOLUTE, Driver_RegistryPath, L"IniPath", &IniPath);
	if (NT_SUCCESS(status))
	{

	}
	//�������ļ���ͬʱ��������λ�ã����ڼ�
	if (!NT_SUCCESS(status))
	{
		path_home = 1;
		RtlStringCbPrintfW(path, path_len, path_sandboxie, Driver_HomePathDos);
		status = Stream_Open(&stream, path, FILE_GENERIC_READ, 0, FILE_SHARE_READ, FILE_OPEN, 0);
	}
	if (status == STATUS_OBJECT_NAME_NOT_FOUND)
	{
		path_home = 0;
		RtlStringCbPrintfW(path, path_len, path_sandboxie, SystemRoot);
		status = Stream_Open(&stream, path, FILE_GENERIC_READ, 0, FILE_SHARE_READ, FILE_OPEN, 0);
	}
	if (!NT_SUCCESS(status))
	{

	}
	//���ļ��ж�ȡ����
	data.pool = pool;
	List_Init(&data.sections);
	map_init(&data.sections_map, data.pool);
	data.sections_map.func_key_size = NULL;
	data.sections_map.func_match_key = &str_map_match;
	data.sections_map.func_hash_key = &str_map_hash;
	map_resize(&data.sections_map, 16);
	data.home = path_home;
	if (path_home == 2)
	{

	}
	else
	{
		data.path = NULL;
	}
	data.use_count = 0;
	if (stream)
	{
		status = Stream_Read_BOM(stream, &data.encoding);
		linenum = 1;
		while (NT_SUCCESS(status))
		{
			status = Conf_Read_Sections(stream, &data, &linenum);
		}
		if (status == STATUS_END_OF_FILE)
		{
			status = STATUS_SUCCESS;
		}
	}
	if (stream) Stream_Close(stream);
	//��ȡ����·����\Templates.ini
	if (NT_SUCCESS(status))
	{
		RtlStringCbPrintfW(path, path_len, path_templates, Driver_HomePathDos);
		status = Stream_Open(
			&stream, path,
			FILE_GENERIC_READ, 0, FILE_SHARE_READ, FILE_OPEN, 0);
		if (!NT_SUCCESS(status))
		{

		}
		else
		{
			status = Stream_Read_BOM(stream, NULL);
			linenum = 1 + CONF_TMPL_LINE_BASE;
			while (NT_SUCCESS(status))
				status = Conf_Read_Sections(stream, &data, &linenum);
			if (status == STATUS_END_OF_FILE)
				status = STATUS_SUCCESS;
			Stream_Close(stream);

			linenum -= CONF_TMPL_LINE_BASE;
			if (!NT_SUCCESS(status))
			{

			}
		}
	}
	//�ϲ�ģ��
	if (NT_SUCCESS(status))
	{
		status = Conf_Merge_Templates(&data, session_id);
		linenum = 0;
	}
	//�����ȡ�ɹ������滻��������
	if (NT_SUCCESS(status))
	{
		BOOLEAN done = FALSE;
		while (!done)
		{
			KIRQL irql;
			KeRaiseIrql(APC_LEVEL, &irql);
			ExAcquireResourceExclusiveLite(Conf_Lock, TRUE);

			if (Conf_Data.use_count == 0)
			{
				pool = Conf_Data.pool;
				memcpy(&Conf_Data, &data, sizeof(CONF_DATA));
				done = TRUE;
			}
			ExReleaseResourceLite(Conf_Lock);
			KeLowerIrql(irql);

			if (!done)
			{
				ZwYieldExecution();
			}
		}
	}
	Mem_Free(path, path_len);
	if (pool)
	{
		//Pool_Delete(pool);
	}
	if (!NT_SUCCESS(status))
	{

	}
	return status;
}

NTSTATUS Conf_Api_SetUserName(PROCESS* proc, ULONG64* parms)
{
	//�´�ʵ��
	return STATUS_SUCCESS;
}

NTSTATUS Conf_Api_IsBoxEnabled(PROCESS* proc, ULONG64* parms)
{
	//�´�ʵ��
	return STATUS_SUCCESS;
}

NTSTATUS Conf_Merge_Templates(CONF_DATA* data, ULONG session_id)
{
	NTSTATUS status;
	CONF_SECTION* sandbox;
	CONF_SETTING* setting;

	//���ȴ���ȫ�ֲ���
	CONF_SECTION* global = Conf_Get_Section(data, Conf_GlobalSettings);
	if (global)
	{
		status = Conf_Merge_Global(data, session_id, global);
		if (!NT_SUCCESS(status))
			return status;
	}
	//�ڶ�������Ĭ��ģ��
	global = Conf_Get_Section(data, Conf_DefaultTemplates);
	if (global) {
		status = Conf_Merge_Global(data, session_id, global);
		if (!NT_SUCCESS(status))
			return status;
	}
	//ɨ������Բ���ɳ�з���
	sandbox = List_Head(&data->sections);
	while (sandbox)
	{
		CONF_SECTION* next_sandbox = List_Next(sandbox);
		//ģ�岿���������ж�
		if (sandbox->from_template)
		{

		}
		//����ȫ�ֲ��֣������κα���ģ�岿�ֺ��û����ò���
		if (_wcsicmp(sandbox->name, Conf_GlobalSettings) == 0 ||
			_wcsnicmp(sandbox->name, Conf_Template_, 9) == 0 || // Template_ or Template_Local_
			_wcsnicmp(sandbox->name, Conf_UserSettings_, 13) == 0)
		{
			sandbox = next_sandbox;
			continue;
		}
		//ʹ�ü��ص����������������Template=Xxx����
		map_iter_t iter2 = map_key_iter(&sandbox->settings_map, Conf_Template);
		while (map_next(&sandbox->settings_map, &iter2))
		{
			setting = iter2.value;
			if (setting->template_handled)
			{
			}
			//��ģ��ϲ���ɳ�䲿��
			status = Conf_Merge_Template(
				data, session_id, setting->value, sandbox);
			if (!NT_SUCCESS(status))
				return status;

			setting->template_handled = TRUE;
		}
		sandbox = next_sandbox;
	}
	return STATUS_SUCCESS;
}

NTSTATUS Conf_Read_Line(STREAM* stream, WCHAR* line, int* linenum)
{
	NTSTATUS status;
	WCHAR* ptr;
	USHORT ch;

	while (1)
	{
		//����ǰ���ؼ��Ϳհ��ַ�
		while (1)
		{
			status = Stream_Read_Wchar(stream, &ch);
			if ((!NT_SUCCESS(status)) || (ch > 32 && ch < 0xFE00))
				break;
			if (ch == L'\r')
				continue;
			if (ch == L'\n')
			{
				ULONG numlines = (++(*linenum));
				if (numlines >= CONF_TMPL_LINE_BASE)
					numlines -= CONF_TMPL_LINE_BASE;
				if (numlines > CONF_MAX_LINES)
				{
					status = STATUS_TOO_MANY_COMMANDS;
					break;
				}
			}
		}
		if (!NT_SUCCESS(status))
		{
			*line = L'\0';
			break;
		}
		//��ȡ�ַ�ֱ���ﵽ���з����
		ptr = line;
		while (1)
		{
			*ptr = ch;
			++ptr;
			if (ptr - line == CONF_LINE_LEN)
				status = STATUS_BUFFER_OVERFLOW;
			else
				status = Stream_Read_Wchar(stream, &ch);
			if ((!NT_SUCCESS(status)) || ch == L'\n' || ch == L'\r')
				break;
		}
		//ɾ������β��ؼ��Ϳհ��ַ�
		while (ptr > line)
		{
			--ptr;
			if (*ptr > 32)
			{
				++ptr;
				break;
			}
		}
		*ptr = L'\0';
		//�������������Ҫ���أ��벻Ҫ�����ļ���β
		if (ptr > line && status == STATUS_END_OF_FILE)
			status = STATUS_SUCCESS;
		//������Ǽ����ɹ����������У�
		//Ȼ��������У��Ӷ�����������
		if (status == STATUS_SUCCESS && *line == L'#')
			continue;
		break;
	}
	return status;
}