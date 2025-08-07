#ifndef DS_H
#define DS_H

#ifdef _NTDDK_
#include <ntddk.h>
#else
#include <WINDOWS.H>
#endif

typedef struct _SSDT_INFO {
	ULONG Count;
	struct _SSDT_ITEM {
		ULONG Index;
		PVOID OrigAddr;
		PVOID CurAddr;
	} Item[1];
} SSDT_INFO, *PSSDT_INFO;

typedef struct _IDT_INFO {
	ULONG Type;
	ULONG Selector;
	PVOID Offset;
	ULONG DPL;
} IDT_INFO, *PIDT_INFO;

typedef struct _KERNEL_INFO {
	ULONG Count;
	struct _KERNEL_ITEM {
		PVOID Address;
		ULONG Length;
		UCHAR CurBytes[5];
		UCHAR OrigBytes[5];
	} Item[1];
} KERNEL_INFO, *PKERNEL_INFO;

#endif
