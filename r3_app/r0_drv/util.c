#include "util.h"
#include"mem.h"
#include"../common/defines.h"
#include"process.h"
#include"../common/my_version.h"
#include"util.h"
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

typedef struct _SYSTEM_CODEINTEGRITY_INFORMATION
{
	ULONG Length;
	ULONG CodeIntegrityOptions;
} SYSTEM_CODEINTEGRITY_INFORMATION, * PSYSTEM_CODEINTEGRITY_INFORMATION;

BOOLEAN MyIsTestSigning(void)
{
	SYSTEM_CODEINTEGRITY_INFORMATION sci = { sizeof(SYSTEM_CODEINTEGRITY_INFORMATION) };
	if (NT_SUCCESS(ZwQuerySystemInformation(/*SystemCodeIntegrityInformation*/ 103, &sci, sizeof(sci), NULL)))
	{
		//BOOLEAN bCodeIntegrityEnabled = !!(sci.CodeIntegrityOptions & /*CODEINTEGRITY_OPTION_ENABLED*/ 0x1);
		BOOLEAN bTestSigningEnabled = !!(sci.CodeIntegrityOptions & /*CODEINTEGRITY_OPTION_TESTSIGN*/ 0x2);

		//DbgPrint("Test Signing: %d; Code Integrity: %d\r\n", bTestSigningEnabled, bCodeIntegrityEnabled);

		//if (bTestSigningEnabled || !bCodeIntegrityEnabled)
		if (bTestSigningEnabled)
			return TRUE;
	}
	return FALSE;
}



BOOLEAN MyIsCallerMyServiceProcess(void)
{
	BOOLEAN ok = FALSE;

	if (MyIsCurrentProcessRunningAsLocalSystem()) {

		void* nbuf;
		ULONG nlen;
		WCHAR* nptr;

		const ULONG ProcessId = (ULONG)(ULONG_PTR)PsGetCurrentProcessId();
		Process_GetProcessName(Driver_Pool, ProcessId, &nbuf, &nlen, &nptr);
		if (nbuf) {

			UNICODE_STRING* uni = (UNICODE_STRING*)nbuf;

			if ((uni->Length > Driver_HomePathNt_Len * sizeof(WCHAR)) &&
				(0 == _wcsnicmp(uni->Buffer, Driver_HomePathNt,
					Driver_HomePathNt_Len))) {

				if (_wcsicmp(nptr, SBIESVC_EXE) == 0) {

					ok = TRUE;
				}
			}

			Mem_Free(nbuf, nlen);
		}
	}

	return ok;
}

BOOLEAN MyIsCurrentProcessRunningAsLocalSystem(void)
{
	extern const WCHAR* Driver_S_1_5_18;
	UNICODE_STRING SidString;
	ULONG SessionId;
	BOOLEAN s_1_5_18 = FALSE;
	NTSTATUS status = Process_GetSidStringAndSessionId(
		NtCurrentProcess(), NULL, &SidString, &SessionId);
	if (NT_SUCCESS(status)) {
		if (_wcsicmp(SidString.Buffer, Driver_S_1_5_18) == 0)
			s_1_5_18 = TRUE;
		RtlFreeUnicodeString(&SidString);
	}
	return s_1_5_18;
}

BOOLEAN MyIsCallerSigned(void)
{
	NTSTATUS status;

	//在测试签名模式下，不验证签名
	if (MyIsTestSigning())
		return TRUE;

	//status = KphVerifyCurrentProcess();
	//
	////DbgPrint("Image Signature Verification result: 0x%08x\r\n", status);
	//
	//if (!NT_SUCCESS(status)) {
	//
	//	//Log_Status(MSG_1330, 0, status);
	//
	//	return FALSE;
	//}

	return TRUE;
}

NTSTATUS MyGetSessionId(ULONG* SessionId)
{
	NTSTATUS status;
	PROCESS_SESSION_INFORMATION info;
	ULONG len;
	len = sizeof(info);
	status = ZwQueryInformationProcess(
		NtCurrentProcess(), ProcessSessionInformation,
		&info, sizeof(info), &len);
	if (NT_SUCCESS(status))
		*SessionId = info.SessionId;
	else
		*SessionId = 0;

	return status;

}

//extern wchar_t g_uuid_str[40];
//void InitFwUuid();
//NTSTATUS MyValidateCertificate(void) 
//{
//	if (!*g_uuid_str)
//		InitFwUuid();
//
//	NTSTATUS status = KphValidateCertificate();
//
//	if (status == STATUS_ACCOUNT_EXPIRED)
//		status = STATUS_SUCCESS;
//
//	return status;
//}