#include "idt.h"
#include "misc.h"

#define MAKELONG(l, h)		((LONG)(((SHORT)(l)) | ((LONG)((SHORT)(h))) << 16))

NTSTATUS EnumIDT(PVOID pBuffer)
{
	NTSTATUS status;
	IDTR idtr;
	PIDTENTRY pEntry;
	PIDT_INFO pIDTInfo;
	int i;

	status = STATUS_SUCCESS;

	load_idt(&idtr);

	__try {
		ProbeForWrite(pBuffer, 0x20000, sizeof(ULONG));
	} __except (EXCEPTION_EXECUTE_HANDLER) {
		return STATUS_UNSUCCESSFUL;
	}

	pIDTInfo = (PIDT_INFO)pBuffer;
	pEntry = idtr.IDTBase;
	DbgPrint("%08x  %08x\n", pEntry, pIDTInfo);

	for (i = 0; i < 0x7ff; i++) {
		pIDTInfo[i].DPL = pEntry[i].DPL;
		pIDTInfo[i].Offset = (PVOID)MAKELONG(pEntry[i].LowOffset, pEntry[i].HiOffset);
		pIDTInfo[i].Selector = pEntry[i].selector;
		pIDTInfo[i].Type = pEntry[i].segment_type;
	}

	return status;
}
