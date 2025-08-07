#ifndef SSDT_H
#define SSDT_H

#include <ntddk.h>
#include "ds.h"

VOID LocateWin32kBase(PDRIVER_OBJECT pDriverObj);
VOID LocateSSDTShadowBase();
NTSTATUS EnumSSDTShadow(ULONG uRestoreAll, PSSDT_INFO SSDTInfo);
NTSTATUS EnumSSDT(ULONG uRestoreAll, PSSDT_INFO SSDTInfo);
PVOID LocateNtKrnlBase(CHAR *szKernelName);
ULONG RVAToRaw(PVOID pBase, ULONG ulAddress);

#endif
