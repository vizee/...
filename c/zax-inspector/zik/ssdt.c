#include "ssdt.h"
#include "misc.h"
#include "LDasm.h"

PKSERVICE_TABLE_DESCRIPTOR KeServiceDescriptorTableShadow = NULL;
PVOID pWin32kBase = NULL;

VOID LocateSSDTShadowBase()
{
	UCHAR *cPtr;
	UCHAR *pOpcode;
	ULONG Length;

	for (cPtr = (PUCHAR)KeAddSystemServiceTable; cPtr < (PUCHAR)KeAddSystemServiceTable + PAGE_SIZE; cPtr += Length) {
		Length = SizeOfCode(cPtr, &pOpcode);

		if (!Length)
			break;

		if ( *(PUSHORT)cPtr == 0x888D) {
			KeServiceDescriptorTableShadow = (PKSERVICE_TABLE_DESCRIPTOR)(*(ULONG *)((ULONG)pOpcode + 2));
			return;
		}
	}
}

VOID LocateWin32kBase(PDRIVER_OBJECT pDriverObj)
{
	PLIST_ENTRY pList = NULL;
	PLDR_DATA_TABLE_ENTRY Ldr = NULL;
	pList = ((PLIST_ENTRY)pDriverObj->DriverSection)->Flink;
	do {
		Ldr = CONTAINING_RECORD(pList, LDR_DATA_TABLE_ENTRY, InLoadOrderLinks);
		if (Ldr->EntryPoint && Ldr->FullDllName.Buffer) {
			if (!_wcsicmp(Ldr->FullDllName.Buffer, L"\\systemroot\\system32\\win32k.sys" )) {
				//比较模块名字，应该比较FullDllName，单单比较BaseDllName很可能会遇到同名文件。
				pWin32kBase = Ldr->DllBase;//保存模块基址
				KdPrint(("win32k %x\n", pWin32kBase));
				break;
			}
		}
		pList = pList->Flink;//下一个链表
	} while ( pList != ((LIST_ENTRY*)pDriverObj->DriverSection)->Flink);
}

PVOID LocateNtKrnlBase(CHAR *szKernelName)
{
	NTSTATUS status;
	ULONG ulNeedSize;
	PVOID pKernelBase;
	SYSTEM_MODULE_INFORMATION ModuleList;

	pKernelBase = NULL;

	status = ZwQuerySystemInformation(
		SystemModuleInformation,
		&ModuleList,
		sizeof(SYSTEM_MODULE_INFORMATION), &ulNeedSize);
	if (!NT_SUCCESS(status) || status == STATUS_INFO_LENGTH_MISMATCH) {
		pKernelBase = ModuleList.Modules[0].Base;
		strcpy(szKernelName, "\\SystemRoot\\System32\\");
		strcat(szKernelName, ModuleList.Modules[0].ModuleNameOffset + ModuleList.Modules[0].ImageName);
	}
	return pKernelBase;
}

NTSTATUS EnumSSDTShadow(ULONG uRestoreAll, PSSDT_INFO SSDTInfo)
{
	NTSTATUS status;
	HANDLE hFile;//文件句柄
	OBJECT_ATTRIBUTES ObjAttr;
	UNICODE_STRING ustrWin32k;
	IO_STATUS_BLOCK ioStatus;
	ULONG ulShadowRaw = 0;
	ULONG ulShadowBase = 0;
	PVOID PoolArea = NULL;
	FILE_POSITION_INFORMATION fpi;
	LARGE_INTEGER Offset;
	ULONG_PTR OrigAddress = 0;
	ULONG_PTR CurAddress = 0;
	ULONG i = 0;
	ULONG ulCount = 0;
	PULONG_PTR pAddr;

	if ( pWin32kBase == NULL || KeServiceDescriptorTableShadow == NULL)
		return STATUS_UNSUCCESSFUL;

	ulCount = KeServiceDescriptorTableShadow[1].Limit;//Linit就是表中函数的个数
	
	ulShadowBase = *(ULONG *)&KeServiceDescriptorTableShadow[1].Base;//得到基址

	ulShadowRaw = ulShadowBase - (ULONG)pWin32kBase;
	//ulShadowRaw = RVAToRaw(pWin32kBase,ulShadowBase);

	RtlInitUnicodeString(&ustrWin32k, L"\\SystemRoot\\System32\\win32k.sys");

	PoolArea = ExAllocatePool(PagedPool, sizeof(ULONG) * ulCount);
	//分配空间，用于保存读取到的数据，因为每个地址的长度sizeof(ULONG),个数是ulCount，所以相乘

	if (!PoolArea)
		return STATUS_UNSUCCESSFUL;

	InitializeObjectAttributes(
		&ObjAttr,
		&ustrWin32k,
		OBJ_KERNEL_HANDLE | OBJ_CASE_INSENSITIVE,
		NULL,
		NULL);
	//打开文件
	status = IoCreateFile(
		&hFile,
		FILE_READ_ATTRIBUTES,
		&ObjAttr,
		&ioStatus,
		0,
		FILE_ATTRIBUTE_NORMAL,
		FILE_SHARE_READ,
		FILE_OPEN,
		0,
		NULL,
		0,
		CreateFileTypeNone,
		NULL,
		IO_NO_PARAMETER_CHECKING);

	if (!NT_SUCCESS(status) ) {
		KdPrint(("open win32k failed\n"));
		goto __exit;
	}

	//设置文件偏移
	Offset.LowPart = ulShadowRaw;
	Offset.HighPart = 0;
	//开始读取数据
	status = ZwReadFile (
		hFile,
		NULL,
		NULL,
		NULL,
		&ioStatus,
		PoolArea,
		ulCount * sizeof(ULONG),
		&Offset,
		NULL);

	if ( !NT_SUCCESS(status) ) {
		KdPrint(("read win32k failed\n"));
		goto __exit;
	}

	pAddr = (PULONG_PTR)PoolArea;
	//比较原始地址与当前的地址并且输出调试
	if (SSDTInfo)
		SSDTInfo->Count = ulCount;
	if (uRestoreAll)
		wp_off();
	for (i = 0; i < ulCount; i++) {
		OrigAddress = *pAddr;//指向原始地址
		CurAddress = KeServiceDescriptorTableShadow[1].Base[i];//读取当前地址
		DbgPrint("%d :: %08X - %08X\n", i, CurAddress, OrigAddress);
		if (uRestoreAll) {
			if (OrigAddress != CurAddress)
				KeServiceDescriptorTableShadow[1].Base[i] = OrigAddress;
		} else if (SSDTInfo) {
			SSDTInfo->Item[i].Index = i;
			SSDTInfo->Item[i].OrigAddr = (PVOID)OrigAddress;
			SSDTInfo->Item[i].CurAddr = (PVOID)KeServiceDescriptorTableShadow[1].Base[i];
		}
		pAddr++;//指针指向下一个函数
	}
	if (uRestoreAll)
		wp_on();
__exit:
	if (PoolArea) {
		ExFreePool(PoolArea);
		//释放空间
	}
	if (hFile) {
		ZwClose(hFile);
		//关闭句柄
	}
	return status;
}

ULONG RVAToRaw(PVOID pBase, ULONG ulAddress)
{
	PIMAGE_DOS_HEADER pDOS;
	PIMAGE_NT_HEADERS pNT;
	PIMAGE_SECTION_HEADER pSection;
	ULONG ulSections;
	ULONG i;

	pDOS = (PIMAGE_DOS_HEADER)pBase;

	if (pDOS->e_magic != IMAGE_DOS_SIGNATURE)
		return 0;
	pNT = (PIMAGE_NT_HEADERS)((PUCHAR)pBase + pDOS->e_lfanew);
	ulSections = pNT->FileHeader.NumberOfSections;
	pSection = (PIMAGE_SECTION_HEADER)
		((PUCHAR)pNT + sizeof(ULONG) + sizeof(IMAGE_FILE_HEADER) + pNT->FileHeader.SizeOfOptionalHeader);
	ulAddress -= (ULONG)pBase;
	for (i = 0; i < ulSections; i++) {
		pSection = (PIMAGE_SECTION_HEADER)((PUCHAR)pSection + sizeof(IMAGE_SECTION_HEADER) * i);
		if (ulAddress > pSection->VirtualAddress
			&& ulAddress < pSection->VirtualAddress + pSection->SizeOfRawData) {
				ulAddress = ulAddress - pSection->VirtualAddress + pSection->PointerToRawData;
				return ulAddress;
		}

	}
	return 0;
}

NTSTATUS EnumSSDT(ULONG uRestoreAll, PSSDT_INFO SSDTInfo)
{
	NTSTATUS status;
	CHAR KernelName[256];
	ANSI_STRING asKernel;
	UNICODE_STRING usKernel;
	OBJECT_ATTRIBUTES ObjAttr;
	IO_STATUS_BLOCK ioStatus;
	LARGE_INTEGER Offset;
	HANDLE hFile;
	PVOID pBase;
	PVOID pSSDTBase;
	ULONG ulImage;
	ULONG ulRaw;
	ULONG ulCount;
	PVOID OrigAddress;
	PVOID CurAddress;
	PVOID *pOrig;
	ULONG i;

	status = STATUS_UNSUCCESSFUL;
	pBase = LocateNtKrnlBase(KernelName);
	pSSDTBase = KeServiceDescriptorTable->Base;
	ulCount = KeServiceDescriptorTable->Limit;
	ulRaw = RVAToRaw(pBase, (ULONG)pSSDTBase);
	ulImage = ((PIMAGE_NT_HEADERS)((PUCHAR)pBase + ((PIMAGE_DOS_HEADER)pBase)->e_lfanew))->OptionalHeader.ImageBase;
	if (ulRaw == 0)
		return status;
	
	pOrig = (PVOID *)ExAllocatePool(NonPagedPool, sizeof(PVOID) * ulCount);

	RtlInitAnsiString(&asKernel, KernelName);
	RtlAnsiStringToUnicodeString(&usKernel, &asKernel, TRUE);

	InitializeObjectAttributes(
		&ObjAttr,
		&usKernel,
		OBJ_KERNEL_HANDLE | OBJ_CASE_INSENSITIVE,
		NULL,
		NULL);
	//打开文件
	status = IoCreateFile(
		&hFile,
		FILE_READ_ATTRIBUTES,
		&ObjAttr,
		&ioStatus,
		0,
		FILE_ATTRIBUTE_NORMAL,
		FILE_SHARE_READ,
		FILE_OPEN,
		0,
		NULL,
		0,
		CreateFileTypeNone,
		NULL,
		IO_NO_PARAMETER_CHECKING);

	if (NT_SUCCESS(status)) {
		Offset.LowPart = ulRaw;
		Offset.HighPart = 0;
		status = ZwReadFile(
			hFile,
			NULL,
			NULL,
			NULL,
			&ioStatus,
			pOrig,
			sizeof(PVOID) * ulCount,
			&Offset,
			NULL);
		if (NT_SUCCESS(status)) {
			if (SSDTInfo)
				SSDTInfo->Count = ulCount;
			if (uRestoreAll)
				wp_off();
			for (i = 0; i < ulCount; i++) {
				CurAddress = (PVOID)((PULONG)pSSDTBase)[i];
				OrigAddress = (PUCHAR)pOrig[i] - ulImage + (ULONG)pBase;
				DbgPrint("%d :: %08X - %08X\n", i, CurAddress, OrigAddress);
				if (uRestoreAll) {
					if (CurAddress != OrigAddress)
						((PULONG)pSSDTBase)[i] = (ULONG)OrigAddress;
				} else if (SSDTInfo) {
					SSDTInfo->Item[i].Index = i;
					SSDTInfo->Item[i].OrigAddr = OrigAddress;
					SSDTInfo->Item[i].CurAddr = CurAddress;
				}
				
			}
			if (uRestoreAll)
				wp_on();
		}
		ZwClose(hFile);
	}

	RtlFreeUnicodeString(&usKernel);
	return status;
}