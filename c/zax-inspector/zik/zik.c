#include <ntddk.h>
#include "ZaxIoCode.h"
#include "ssdt.h"
#include "idt.h"
#include "kernel.h"

#pragma region declaration

#define ZIK_DEVICE_NAME	L"\\Device\\zik"
#define ZIK_LINK_NAME	L"\\DosDevices\\zik"

VOID DriverUnload(PDRIVER_OBJECT pDriverObject);
NTSTATUS DeviceDispatch(PDEVICE_OBJECT pDeviceObject, PIRP pIrp);
NTSTATUS DriverEntry(PDRIVER_OBJECT pDriverObject, PUNICODE_STRING pRegistryPath);

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT, DriverEntry)
#pragma alloc_text(PAGE, DriverUnload)
#pragma alloc_text(PAGE, DeviceDispatch)
#endif

#pragma endregion

VOID DriverUnload(PDRIVER_OBJECT pDriverObject)
{
	UNICODE_STRING LinkString;
	PDEVICE_OBJECT pDeviceObject;

	PAGED_CODE();

	RtlInitUnicodeString(&LinkString, ZIK_LINK_NAME);
	IoDeleteSymbolicLink(&LinkString);

	if((pDeviceObject = pDriverObject->DeviceObject))
		IoDeleteDevice(pDeviceObject);

	// TODO:unload kernel
	DbgPrint("unload\n");
	return;
}

NTSTATUS DeviceDispatch(PDEVICE_OBJECT pDeviceObject, PIRP pIrp)
{
	NTSTATUS status;
	PIO_STACK_LOCATION pIrpStack;
	ULONG InputBufferLength;
	ULONG IoControlCode;

	PAGED_CODE();

	DbgPrint("Dispatch\n");
	status = STATUS_SUCCESS;
	pIrpStack = IoGetCurrentIrpStackLocation(pIrp);
	InputBufferLength = 0;
	if(pIrpStack->MajorFunction == IRP_MJ_DEVICE_CONTROL) {
		InputBufferLength = pIrpStack->Parameters.DeviceIoControl.InputBufferLength;
		switch(pIrpStack->Parameters.DeviceIoControl.IoControlCode) {
		case ZIK_IO_DRV_INIT:
			//STUB
			break;
		case ZIK_IO_DRV_UNLOAD:
			// STUB
			break;
		case ZIK_IO_SSDT_R:
			EnumSSDT(FALSE, (PSSDT_INFO)pIrp->UserBuffer);
			break;
		case ZIK_IO_SSDT_W:
			EnumSSDT(TRUE, NULL);
			break;
		case ZIK_IO_SSDTS_R:
			status = EnumSSDTShadow(FALSE, (PSSDT_INFO)pIrp->UserBuffer);
			break;
		case ZIK_IO_SSDTS_W:
			status = EnumSSDTShadow(TRUE, NULL);
			break;
		case ZIK_IO_IDT_R:
			status = EnumIDT((PIDT_INFO)pIrp->UserBuffer);
			break;
		case ZIK_IO_KERNEL_R:
			status = EnumKernel(FALSE, NULL);
			break;
		case ZIK_IO_KERNEL_W:
			status = EnumKernel(TRUE, NULL);
			break;
		default:
			status = STATUS_INVALID_DEVICE_REQUEST;
		}

	}
	pIrp->IoStatus.Information = InputBufferLength;
	pIrp->IoStatus.Status = status;
	IoCompleteRequest(pIrp, IO_NO_INCREMENT);
	return status;
}

NTSTATUS DriverEntry(PDRIVER_OBJECT pDriverObject, PUNICODE_STRING pRegistryPath)
{
	NTSTATUS status;
	UNICODE_STRING DeviceName, LinkName;
	PDEVICE_OBJECT pDeviceObject;

	//create device
	RtlInitUnicodeString(&DeviceName, ZIK_DEVICE_NAME);
	status = IoCreateDevice(
		pDriverObject,
		0,
		&DeviceName,
		FILE_DEVICE_UNKNOWN,
		0,
		TRUE,
		&pDeviceObject);

	if(NT_SUCCESS(status)) {
		pDriverObject->MajorFunction[IRP_MJ_CREATE] = (PDRIVER_DISPATCH)DeviceDispatch;
		pDriverObject->MajorFunction[IRP_MJ_CLOSE] = (PDRIVER_DISPATCH)DeviceDispatch;
		pDriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = (PDRIVER_DISPATCH)DeviceDispatch;
		pDriverObject->DriverUnload = (PDRIVER_UNLOAD)DriverUnload;
		//create symbolic
		RtlInitUnicodeString(&LinkName, ZIK_LINK_NAME);
		status = IoCreateSymbolicLink(&LinkName, &DeviceName);

		if(NT_SUCCESS(status)) {
			// TODO: stub ... initialize driver
			LocateSSDTShadowBase();
			LocateWin32kBase(pDriverObject);
			DbgPrint("Initialized\n");
			return status;
		} else
			IoDeleteDevice(pDeviceObject);
		
	}

	return status;
}
