#ifndef MISC_H
#define MISC_H

#include <ntddk.h>
#include "PE.h"
#pragma region struct

#include <PSHPACK1.H>

typedef struct _KSERVICE_TABLE_DESCRIPTOR {
	PULONG_PTR Base;
	PULONG Count;
	ULONG Limit;
	PUCHAR Number;
} KSERVICE_TABLE_DESCRIPTOR, *PKSERVICE_TABLE_DESCRIPTOR;

typedef struct _LDR_DATA_TABLE_ENTRY
{
	LIST_ENTRY InLoadOrderLinks;
	LIST_ENTRY InMemoryOrderLinks;
	LIST_ENTRY InInitializationOrderLinks;
	PVOID DllBase;
	PVOID EntryPoint;
	ULONG SizeOfImage;
	UNICODE_STRING FullDllName;
	UNICODE_STRING BaseDllName;
	ULONG Flags;
	USHORT LoadCount;
	USHORT TlsIndex;
	union {
		LIST_ENTRY HashLinks;
		struct
		{
			PVOID SectionPointer;
			ULONG CheckSum;
		};
	};
	union {
		struct
		{
			ULONG TimeDateStamp;
		};
		struct
		{
			PVOID LoadedImports;
		};
	};
	struct _ACTIVATION_CONTEXT * EntryPointActivationContext;
	PVOID PatchInformation;
} LDR_DATA_TABLE_ENTRY, *PLDR_DATA_TABLE_ENTRY;

typedef struct _IDTENTRY {
    USHORT LowOffset;
    USHORT selector;
    UCHAR unused_lo;
    UCHAR segment_type: 4;    //0x0E is an interrupt gate
    UCHAR system_segment_flag: 1;
    UCHAR DPL: 2;    // descriptor privilege level 
    UCHAR P: 1; /* present */
    USHORT HiOffset;
} IDTENTRY, *PIDTENTRY;

typedef struct _IDTR {
    USHORT IDTLimit;
    PIDTENTRY IDTBase;
} IDTR, *PIDTR;

//Ä£¿éÐÅÏ¢
typedef struct _SYSTEM_MODULE
{
    ULONG  Reserved1;                   // Should be 0xBAADF00D
    ULONG  Reserved2;                   // Should be zero
    PVOID  Base;
    ULONG  Size;
    ULONG  Flags;
    USHORT Index;
    USHORT Unknown;
    USHORT LoadCount;
    USHORT ModuleNameOffset;
    CHAR   ImageName[256];

} SYSTEM_MODULE, *PSYSTEM_MODULE;


typedef struct _SYSTEM_MODULE_INFORMATION
{
    ULONG         ModulesCount;
    SYSTEM_MODULE Modules[1];

} SYSTEM_MODULE_INFORMATION, *PSYSTEM_MODULE_INFORMATION;

#include <POPPACK.H>

#pragma endregion

typedef enum _SYSTEM_INFORMATION_CLASS
{
		SystemBasicInformation, 				// 0
		SystemProcessorInformation, 			// 1
		SystemPerformanceInformation, 			// 2
		SystemTimeOfDayInformation, 			// 3
		SystemNotImplemented1, 				    // 4
		SystemProcessesAndThreadsInformation, 	// 5
		SystemCallCounts, 					    // 6
		SystemConfigurationInformation, 		// 7
		SystemProcessorTimes, 				    // 8
		SystemGlobalFlag, 					    // 9
		SystemNotImplemented2, 				    // 10
		SystemModuleInformation, 				// 11
		SystemLockInformation, 				    // 12
		SystemNotImplemented3, 				    // 13
		SystemNotImplemented4, 				    // 14
		SystemNotImplemented5, 				    // 15
		SystemHandleInformation, 				// 16
		SystemObjectInformation, 				// 17
		SystemPagefileInformation, 				// 18
		SystemInstructionEmulationCounts, 		// 19
		SystemInvalidInfoClass1, 				// 20
		SystemCacheInformation, 				// 21
		SystemPoolTagInformation, 				// 22
		SystemProcessorStatistics, 				// 23
		SystemDpcInformation, 				    // 24
		SystemNotImplemented6, 				    // 25
		SystemLoadImage, 					    // 26
		SystemUnloadImage, 				        // 27
		SystemTimeAdjustment, 				    // 28
		SystemNotImplemented7, 				    // 29
		SystemNotImplemented8, 				    // 30
		SystemNotImplemented9, 				    // 31
		SystemCrashDumpInformation, 			// 32
		SystemExceptionInformation, 			// 33
		SystemCrashDumpStateInformation, 		// 34
		SystemKernelDebuggerInformation, 		// 35
		SystemContextSwitchInformation, 		// 36
		SystemRegistryQuotaInformation, 		// 37
		SystemLoadAndCallImage, 				// 38
		SystemPrioritySeparation, 				// 39
		SystemNotImplemented10, 				// 40
		SystemNotImplemented11, 				// 41
		SystemInvalidInfoClass2, 				// 42
		SystemInvalidInfoClass3, 				// 43
		SystemTimeZoneInformation, 				// 44
		SystemLookasideInformation, 			// 45
		SystemSetTimeSlipEvent, 				// 46
		SystemCreateSession, 				    // 47
		SystemDeleteSession, 				    // 48
		SystemInvalidInfoClass4, 				// 49
		SystemRangeStartInformation, 			// 50
		SystemVerifierInformation, 				// 51
		SystemAddVerifier, 				        // 52
		SystemSessionProcessesInformation 		// 53
}SYSTEM_INFORMATION_CLASS;

NTKERNELAPI
NTSTATUS
KeAddSystemServiceTable(
			IN PULONG_PTR Base,
			IN PULONG Count OPTIONAL,
			IN ULONG Limit,
			IN PUCHAR Number,
			IN ULONG Index
			);

NTKERNELAPI
NTSTATUS
ZwQuerySystemInformation(
			ULONG SystemInformationClass,
			PVOID SystemInformation,
			ULONG SystemInformationLength,
			PULONG ReturnLength
			);

extern PKSERVICE_TABLE_DESCRIPTOR KeServiceDescriptorTable;

VOID wp_off();
VOID wp_on();
ULONG check_pae();
VOID load_idt(PIDTR pIDTR);

#endif
