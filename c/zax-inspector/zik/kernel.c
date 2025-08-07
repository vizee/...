#include "kernel.h"
#include "ssdt.h"
#include "misc.h"

NTSTATUS EnumKernel(ULONG uRestoreAll, PKERNEL_INFO KernelInfo)
{
	NTSTATUS status;
	CHAR KernelName[256];
	PVOID pBase;
	PVOID pSSDTBase;
	ULONG ulCount;
	BYTE OrigData[5] = {0x8b, 0xff, 0x55, 0x8b, 0xec};
	BYTE CurData[5];
	PVOID *pOrig;
	ULONG i;

	status = STATUS_UNSUCCESSFUL;
	pBase = LocateNtKrnlBase(KernelName);
	pSSDTBase = KeServiceDescriptorTable->Base;
	ulCount = KeServiceDescriptorTable->Limit;
	
	if (NT_SUCCESS(status)) {
		if (NT_SUCCESS(status)) {
			if (KernelInfo)
				KernelInfo->Count = 0;
			if (uRestoreAll)
				wp_off();
			for (i = 0; i < ulCount; i++) {
				RtlCopyMemory(CurData, (PVOID)((PULONG)pSSDTBase)[i], 5);
				if (memcmp(CurData, OrigData, 5) != 0)
					if (KernelInfo) {
						KernelInfo->Item[i].Address = (PVOID)((PULONG)pSSDTBase)[i];
						KernelInfo->Item[i].Length = 5;
						RtlCopyMemory(KernelInfo->Item[i].CurBytes, CurData, 5);
						RtlCopyMemory(KernelInfo->Item[i].OrigBytes, OrigData, 5);
						KernelInfo->Count++;
					}
			}
			if (uRestoreAll)
				wp_on();
		}
	}

	return status;
}


