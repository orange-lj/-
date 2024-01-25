#include "util.h"
#include"../common/defines.h"
NTSTATUS GetRegString(ULONG RelativeTo, const WCHAR* Path, const WCHAR* ValueName, UNICODE_STRING* pData)
{
	NTSTATUS status;
	RTL_QUERY_REGISTRY_TABLE qrt[2];

	memzero(qrt, sizeof(qrt));
	qrt[0].Flags = RTL_QUERY_REGISTRY_REQUIRED |
		RTL_QUERY_REGISTRY_DIRECT |
		RTL_QUERY_REGISTRY_TYPECHECK | // fixes security violation but causes STATUS_OBJECT_TYPE_MISMATCH when buffer to small
		RTL_QUERY_REGISTRY_NOVALUE |
		RTL_QUERY_REGISTRY_NOEXPAND;
	qrt[0].Name = (WCHAR*)ValueName;
	qrt[0].EntryContext = pData;
	qrt[0].DefaultType = (REG_SZ << RTL_QUERY_REGISTRY_TYPECHECK_SHIFT) | REG_NONE;

	status = RtlQueryRegistryValues(
		RelativeTo, Path, qrt, NULL, NULL);
	if (status == STATUS_OBJECT_TYPE_MISMATCH) 
	{
	
	}
	return status;
}
