#pragma once
#include <ntifs.h>

#define NTOS_API(type)  NTSYSAPI type NTAPI
#define NTOS_NTSTATUS   NTOS_API(NTSTATUS)

NTOS_NTSTATUS   RtlSetSaclSecurityDescriptor(
    IN PSECURITY_DESCRIPTOR SecurityDescriptor,
    IN BOOLEAN SaclPresent,
    IN PACL Sacl,
    IN BOOLEAN SaclDefaulted);

NTOS_NTSTATUS   ObOpenObjectByName(
    IN POBJECT_ATTRIBUTES   ObjectAttributes,
    IN POBJECT_TYPE         ObjectType OPTIONAL,
    IN KPROCESSOR_MODE      AccessMode,
    IN OUT PACCESS_STATE    AccessState OPTIONAL,
    IN ACCESS_MASK          DesiredAccess OPTIONAL,
    IN OUT PVOID            ParseContext OPTIONAL,
    OUT PHANDLE             Handle);


typedef struct _OBJECT_TYPE_VISTA_SP1 {
    //LIST_ENTRY TypeList;
    UNICODE_STRING Name;            // Copy from object header for convenience
    //PVOID DefaultObject;
    //ULONG Index;
    //ULONG TotalNumberOfObjects;
    //ULONG TotalNumberOfHandles;
    //ULONG HighWaterNumberOfObjects;
    //ULONG HighWaterNumberOfHandles;
    //OBJECT_TYPE_INITIALIZER_VISTA_SP1 TypeInfo;
    //ERESOURCE Mutex;
    //EX_PUSH_LOCK TypeLock;
    ULONG Key;
    //EX_PUSH_LOCK ObjectLocks[32];
    //LIST_ENTRY CallbackList;
} OBJECT_TYPE_VISTA_SP1, * POBJECT_TYPE_VISTA_SP1;

typedef struct _OBJECT_TYPE 
{
    ULONG Index;
}OBJECT_TYPE,*POBJECT_TYPE;

typedef enum _SYSTEM_INFORMATION_CLASS {
    SystemBasicInformation,
    SystemProcessorInformation,
    SystemPerformanceInformation,
    SystemTimeOfDayInformation,
    SystemNotImplemented1,
    SystemProcessesAndThreadsInformation,
    SystemCallCounts,
    SystemConfigurationInformation,
    SystemProcessorTimes,
    SystemGlobalFlag,
    SystemNotImplemented2,
    SystemModuleInformation,
    SystemLockInformation,
    SystemNotImplemented3,
    SystemNotImplemented4,
    SystemNotImplemented5,
    SystemHandleInformation,
    SystemObjectInformation,
    SystemPagefileInformation,
    SystemInstructionEmulationCounts,
    SystemInvalidInfoClass1,
    SystemCacheInformation,
    SystemPoolTagInformation,
    SystemProcessorStatistics,
    SystemDpcInformation,
    SystemNotImplemented6,
    SystemLoadImage,
    SystemUnloadImage,
    SystemTimeAdjustment,
    SystemNotImplemented7,
    SystemNotImplemented8,
    SystemNotImplemented9,
    SystemCrashDumpInformation,
    SystemExceptionInformation,
    SystemCrashDumpStateInformation,
    SystemKernelDebuggerInformation,
    SystemContextSwitchInformation,
    SystemRegistryQuotaInformation,
    SystemLoadAndCallImage,
    SystemPrioritySeparation,
    SystemNotImplemented10,
    SystemNotImplemented11,
    SystemInvalidInfoClass2,
    SystemInvalidInfoClass3,
    SystemTimeZoneInformation,
    SystemLookasideInformation,
    SystemSetTimeSlipEvent,
    SystemSessionCreate,
    SystemSessionDetach,
    SystemInvalidInfoClass4,
    SystemRangeStartInformation,
    SystemVerifierInformation,
    SystemAddVerifier,
    SystemSessionProcessesInformation
} SYSTEM_INFORMATION_CLASS;

typedef enum _JOBOBJECTINFOCLASS {
    JobObjectBasicAccountingInformation = 1,
    JobObjectBasicLimitInformation,
    JobObjectBasicProcessIdList,
    JobObjectBasicUIRestrictions,
    JobObjectSecurityLimitInformation,
    JobObjectEndOfJobTimeInformation,
    JobObjectAssociateCompletionPortInformation,
    JobObjectBasicAndIoAccountingInformation,
    JobObjectExtendedLimitInformation,
    JobObjectJobSetInformation,
    MaxJobObjectInfoClass
} JOBOBJECTINFOCLASS;

//与对象相关
typedef NTSTATUS(*P_ObRegisterCallbacks)(
    __in POB_CALLBACK_REGISTRATION CallbackRegistration,
    __deref_out PVOID* RegistrationHandle);


typedef NTSTATUS(*P_ObUnRegisterCallbacks)(
    __in PVOID RegistrationHandle);


// 与注册表相关
typedef NTSTATUS(*P_CmRegisterCallbackEx)(
    IN PEX_CALLBACK_FUNCTION  Function,
    IN PCUNICODE_STRING  Altitude,
    IN PVOID  Driver,
    IN PVOID  Context,
    OUT PLARGE_INTEGER  Cookie,
    PVOID  Reserved
    );


NTOS_NTSTATUS   ZwYieldExecution(void);

NTOS_NTSTATUS   ZwUnloadKey(
    IN HANDLE               KeyHandle);


typedef void* PINITIAL_TEB;