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


NTOS_NTSTATUS   ZwQueryInformationProcess(
    IN HANDLE           ProcessHandle,
    IN PROCESSINFOCLASS ProcessInformationClass,
    OUT PVOID           ProcessInformation,
    IN ULONG            ProcessInformationLength,
    OUT PULONG          ReturnLength OPTIONAL);


typedef struct _OBJECT_DUMP_CONTROL {
    PVOID Stream;
    ULONG Detail;
} OB_DUMP_CONTROL, * POB_DUMP_CONTROL;


typedef enum _OB_OPEN_REASON {
    ObCreateHandle,
    ObOpenHandle,
    ObDuplicateHandle,
    ObInheritHandle,
    ObMaxOpenReason
} OB_OPEN_REASON;

typedef VOID(*OB_DUMP_METHOD)(
    IN PVOID Object,
    IN POB_DUMP_CONTROL Control OPTIONAL);


typedef NTSTATUS(*OB_OPEN_METHOD)(
    IN OB_OPEN_REASON OpenReason,
    IN PEPROCESS Process OPTIONAL,
    IN PVOID Object,
    IN ACCESS_MASK GrantedAccess,
    IN ULONG HandleCount);


typedef NTSTATUS(*OB_OPEN_METHOD_VISTA)(
    IN OB_OPEN_REASON OpenReason,
    IN ACCESS_MASK GrantedAccess,
    IN PEPROCESS Process OPTIONAL,
    IN PVOID Object,
    PVOID Unknown1,
    IN ULONG HandleCount);


typedef VOID(*OB_CLOSE_METHOD)(
    IN PEPROCESS Process OPTIONAL,
    IN PVOID Object,
    IN ACCESS_MASK GrantedAccess,
    IN ULONG ProcessHandleCount,
    IN ULONG SystemHandleCount);


typedef VOID(*OB_DELETE_METHOD)(
    IN PVOID Object);


typedef NTSTATUS(*OB_PARSE_METHOD)(
    IN PVOID ParseObject,
    IN PVOID ObjectType,
    IN OUT PACCESS_STATE AccessState,
    IN KPROCESSOR_MODE AccessMode,
    IN ULONG Attributes,
    IN OUT PUNICODE_STRING CompleteName,
    IN OUT PUNICODE_STRING RemainingName,
    IN OUT PVOID Context OPTIONAL,
    IN PSECURITY_QUALITY_OF_SERVICE SecurityQos OPTIONAL,
    OUT PVOID* Object);


typedef NTSTATUS(*OB_SECURITY_METHOD)(
    IN PVOID Object,
    IN SECURITY_OPERATION_CODE OperationCode,
    IN PSECURITY_INFORMATION SecurityInformation,
    IN OUT PSECURITY_DESCRIPTOR SecurityDescriptor,
    IN OUT PULONG CapturedLength,
    IN OUT PSECURITY_DESCRIPTOR* ObjectsSecurityDescriptor,
    IN POOL_TYPE PoolType,
    IN PGENERIC_MAPPING GenericMapping);


typedef NTSTATUS(*OB_QUERYNAME_METHOD)(
    IN PVOID Object,
    IN BOOLEAN HasObjectName,
    OUT POBJECT_NAME_INFORMATION ObjectNameInfo,
    IN ULONG Length,
    OUT PULONG ReturnLength);


typedef BOOLEAN(*OB_OKAYTOCLOSE_METHOD)(
    IN PEPROCESS Process OPTIONAL,
    IN PVOID Object,
    IN HANDLE Handle);

typedef struct _OBJECT_TYPE_INITIALIZER_VISTA_SP1 {
    USHORT Length;
    BOOLEAN ObjectTypeFlags;
    ULONG ObjectTypeCode;
    ULONG InvalidAttributes;
    GENERIC_MAPPING GenericMapping;
    ULONG ValidAccessMask;
    ULONG RetainAccess;
    POOL_TYPE PoolType;
    ULONG DefaultPagedPoolCharge;
    ULONG DefaultNonPagedPoolCharge;
    OB_DUMP_METHOD DumpProcedure;
    OB_OPEN_METHOD OpenProcedure;
    OB_CLOSE_METHOD CloseProcedure;
    OB_DELETE_METHOD DeleteProcedure;
    OB_PARSE_METHOD ParseProcedure;
    OB_SECURITY_METHOD SecurityProcedure;
    OB_QUERYNAME_METHOD QueryNameProcedure;
    OB_OKAYTOCLOSE_METHOD OkayToCloseProcedure;
} OBJECT_TYPE_INITIALIZER_VISTA_SP1, * POBJECT_TYPE_INITIALIZER_VISTA_SP1;


typedef struct _OBJECT_TYPE_VISTA_SP1 {
    LIST_ENTRY TypeList;
    UNICODE_STRING Name;            // Copy from object header for convenience
    PVOID DefaultObject;
    ULONG Index;
    ULONG TotalNumberOfObjects;
    ULONG TotalNumberOfHandles;
    ULONG HighWaterNumberOfObjects;
    ULONG HighWaterNumberOfHandles;
    OBJECT_TYPE_INITIALIZER_VISTA_SP1 TypeInfo;
    ERESOURCE Mutex;
    EX_PUSH_LOCK TypeLock;
    ULONG Key;
    EX_PUSH_LOCK ObjectLocks[32];
    LIST_ENTRY CallbackList;
} OBJECT_TYPE_VISTA_SP1, * POBJECT_TYPE_VISTA_SP1;





typedef struct _OBJECT_TYPE_INITIALIZER {
    USHORT Length;
    BOOLEAN UseDefaultObject;
    BOOLEAN Reserved;
    ULONG InvalidAttributes;
    GENERIC_MAPPING GenericMapping;
    ULONG ValidAccessMask;
    BOOLEAN SecurityRequired;
    BOOLEAN MaintainHandleCount;
    BOOLEAN MaintainTypeList;
    POOL_TYPE PoolType;
    ULONG DefaultPagedPoolCharge;
    ULONG DefaultNonPagedPoolCharge;
    OB_DUMP_METHOD DumpProcedure;
    OB_OPEN_METHOD OpenProcedure;
    OB_CLOSE_METHOD CloseProcedure;
    OB_DELETE_METHOD DeleteProcedure;
    OB_PARSE_METHOD ParseProcedure;
    OB_SECURITY_METHOD SecurityProcedure;
    OB_QUERYNAME_METHOD QueryNameProcedure;
    OB_OKAYTOCLOSE_METHOD OkayToCloseProcedure;
} OBJECT_TYPE_INITIALIZER, * POBJECT_TYPE_INITIALIZER;

typedef struct _OBJECT_TYPE 
{
    //ERESOURCE Mutex;              //在7或更高版本中不存在是在xp中吗？
    LIST_ENTRY TypeList;
    UNICODE_STRING Name;            //为方便起见，从对象标题复制
    PVOID DefaultObject;
    ULONG Index;
    ULONG TotalNumberOfObjects;
    ULONG TotalNumberOfHandles;
    ULONG HighWaterNumberOfObjects;
    ULONG HighWaterNumberOfHandles;
    OBJECT_TYPE_INITIALIZER TypeInfo;
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

NTOS_API(ULONG) PsGetProcessSessionId(PEPROCESS EProcess);


typedef void* PINITIAL_TEB;


#define PROCESS_QUERY_INFORMATION (0x0400)


typedef struct _MODULE_INFO {
    ULONG_PTR   Reserved1;
    ULONG_PTR   MappedBase;
    ULONG_PTR   ImageBaseAddress;
    ULONG       ImageSize;
    ULONG       Flags;
    USHORT      LoadCount;
    USHORT      LoadOrderIndex;
    USHORT      InitOrderIndex;
    USHORT      NameOffset;
    UCHAR       Path[256];
} MODULE_INFO;


typedef struct _SYSTEM_MODULE_INFORMATION {
    ULONG       ModuleCount;
    ULONG       Reserved;
    MODULE_INFO ModuleInfo[0];
} SYSTEM_MODULE_INFORMATION;

NTOS_NTSTATUS   ZwQuerySystemInformation(
    IN  SYSTEM_INFORMATION_CLASS    SystemInformationClass,
    OUT PVOID                       SystemInformation,
    IN  ULONG                       SystemInformationLength,
    OUT PULONG                      ReturnLength OPTIONAL);