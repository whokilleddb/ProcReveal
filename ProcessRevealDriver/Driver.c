#include <ntddk.h>
#include "ProcRevealCommon.h"
#define DRIVER_PREFIX "PROCREVEAL: "

void ProcRevealUnload(PDRIVER_OBJECT);
NTSTATUS ProcRevealCreateClose(PDEVICE_OBJECT, PIRP);
NTSTATUS ProcRevealDeviceControl(PDEVICE_OBJECT, PIRP);
NTSTATUS CompleteRequest(PIRP, NTSTATUS, ULONG_PTR);

NTSTATUS DriverEntry(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegPath) {
	UNREFERENCED_PARAMETER(RegPath);
	NTSTATUS status;
	PDEVICE_OBJECT devObj = NULL;

	DriverObject->DriverUnload = ProcRevealUnload;
	DriverObject->MajorFunction[IRP_MJ_CREATE] =
		DriverObject->MajorFunction[IRP_MJ_CLOSE] = ProcRevealCreateClose;
	DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = ProcRevealDeviceControl;


	UNICODE_STRING name = RTL_CONSTANT_STRING(L"\\Device\\ProcReveal");
	status = IoCreateDevice(DriverObject, 0, &name, FILE_DEVICE_UNKNOWN, 0, FALSE, &devObj);
	if (!NT_SUCCESS(status)) {
		KdPrint((DRIVER_PREFIX "Failed to create device: 0x%X\n", status));
		return status;
	}

	UNICODE_STRING symLink = RTL_CONSTANT_STRING(DRIVER_SYM_LINK_NAME);
	status = IoCreateSymbolicLink(&symLink, &name);
	if (!NT_SUCCESS(status)) {
		KdPrint((DRIVER_PREFIX "Failed to create symbolic link: 0x%X\n", status));
		IoDeleteDevice(devObj);
		return status;
	}

	return STATUS_SUCCESS;
}

void ProcRevealUnload(PDRIVER_OBJECT DriverObject) {
	IoDeleteDevice(DriverObject->DeviceObject);
	UNICODE_STRING symLink = RTL_CONSTANT_STRING(DRIVER_SYM_LINK_NAME);
	IoDeleteSymbolicLink(&symLink);
}

NTSTATUS ProcRevealCreateClose(PDEVICE_OBJECT _DeviceObject, PIRP Irp) {
	UNREFERENCED_PARAMETER(_DeviceObject);
	HANDLE h_curr_pid = PsGetCurrentProcessId();
	ULONG u_curr_pid = HandleToULong(h_curr_pid);

	if (IoGetCurrentIrpStackLocation(Irp)->MajorFunction == IRP_MJ_CREATE) {
		KdPrint((DRIVER_PREFIX "Create called from process %u\n", u_curr_pid));
	}
	else {
		KdPrint((DRIVER_PREFIX "Close called from process %u\n", u_curr_pid));
	}

	return CompleteRequest(Irp, STATUS_SUCCESS, 0);
}

NTSTATUS CompleteRequest(PIRP Irp, NTSTATUS status, ULONG_PTR info) {
	Irp->IoStatus.Status = status;
	Irp->IoStatus.Information = info;
	IoCompleteRequest(Irp, IO_NO_INCREMENT);
	return status;
}

NTSTATUS ProcRevealDeviceControl(PDEVICE_OBJECT _DeviceObject, PIRP Irp) {
	UNREFERENCED_PARAMETER(_DeviceObject);
	
	ULONG len = 0;
	NTSTATUS status = STATUS_INVALID_DEVICE_REQUEST;
	PIO_STACK_LOCATION irpSp = IoGetCurrentIrpStackLocation(Irp);

	switch (irpSp->Parameters.DeviceIoControl.IoControlCode) {
		case IOCTL_OPEN_PROCESS:
			// Check size of input and output buffer sizes
			if (irpSp->Parameters.DeviceIoControl.InputBufferLength < sizeof(ProcessData) ||
				irpSp->Parameters.DeviceIoControl.OutputBufferLength < sizeof(HANDLE)) {
				status = STATUS_BUFFER_TOO_SMALL;
				break;
			}

			// Get process data sent by client
			ProcessData* cData = (ProcessData*)Irp->AssociatedIrp.SystemBuffer;
			if (NULL == cData) {
				status = STATUS_INVALID_PARAMETER;
				break;
			}

			CLIENT_ID cid = { 0 };
			cid.UniqueProcess = ULongToHandle(cData->ProcessId);
			// InitializeObjectAttributes(&objAttr, NULL, 0, NULL, NULL);
			OBJECT_ATTRIBUTES objAttr = RTL_CONSTANT_OBJECT_ATTRIBUTES(NULL, 0);
			HANDLE hProcess;
			status = ZwOpenProcess(&hProcess, cData->Access, &objAttr, &cid);
			if (NT_SUCCESS(status)) {
				len = sizeof(HANDLE);
				KdPrint((DRIVER_PREFIX "Escalated Handle:\t0x%p\n", hProcess));
				memcpy(cData, &hProcess, sizeof(hProcess));
				// *(HANDLE*)data = hProcess;
			}
			break;
	}

	return CompleteRequest(Irp, status, len);
}