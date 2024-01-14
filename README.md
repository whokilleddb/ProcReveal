# ProcReveal 
This repository contains a Proof-of-Concept code, as well as the code walkthrough of a client-driver code which retirieves a handle to any process using a kernel driver. Usually, userland code can request handle of a process using the `OpenProcess()` function, but it may fail in case of certain process like protected/privileged processes. However, no such restrictions are present in the Kernel Land. 

In this case, the client takes in a target process's PID and tries to open a handle to it with `PROCESS_ALL_ACCESS` permissions using the `OpenProcess()` function. Incase that fails, we then make a call to our driver using `DeviceIoControl()` to retrieve a handle from kernel land. 

This repository is a part of my ongoing attempt to document my journey into Windows Kernel Land. I really recommend reading through the [previous post](https://github.com/whokilleddb/BoosterDriver) first in case you haven't already as I will skip some sections of the code which have been previously discussed back there.

# Usage 

```
.\ProcessRevealClient <PID>
```

# Code Walkthrough

The code consists of two parts a client and a driver. We would first look at the Driver code and then the client code.

## Driver

The Driver has the following functions:

- `DriverEntry()` - Driver entry point function
- `ProcRevealUnload()` - This function is called when the system unloads our driver
- `ProcRevealCreateClose()` - This function handles Create/Close dispatch routines issued by the Client
- `ProcRevealDeviceControl()` - This function handles the DeviceControl dispatch routines issued by the Client

### DriverEntry, ProcRevealCreateClose and ProcRevealUnload

I am going to speedrun through these functions as they are almost the same as last time. 

#### DriverEntry()

Just like before we fill the major function array entries with the associated functions responsible for handling the respective dispatch routines. Then we create a Kernel device using `IoCreateDevice()`. Clients will be interacting with this device object. Finally, we create a symlink pointing to that device with `IoCreateSymbolicLink()`. That's it.

#### ProcRevealCreateClose()

This function is responsible for handling all Create and Close dispatch routines. This is a good time to introduce the `CompleteRequest()` function - which is just a convinience function we have so that we dont have to write the same repetative code to complete IRPs.

#### ProcRevealUnload()

This function is called when the driver is unloaded by the system. It performs the necessary cleanups like deleting the Device object we previously created as well as the corresponding symlink.

### ProcRevealDeviceControl

This function is responsible for handling any Device Control dispatch requests. The code function looks like:

```c
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
```

The first thing we need to understand before diving into the function is the idea of Control Codes. According to [MSDN](https://learn.microsoft.com/en-us/windows-hardware/drivers/kernel/introduction-to-i-o-control-codes):

> I/O control codes (IOCTLs) are used for communication between user-mode applications and drivers, or for communication internally among drivers in a stack. I/O control codes are sent using IRPs.

These codes are help the driver to define the functionality which the client seeks out of it. For example, a dispatch routine can have functionality to support various control codes, each of which caters to a specific condition. 

For example, our driver uses just one control code - `IOCTL_OPEN_PROCESS`. This cannot be an arbitrary number. Infact, there is a very specific way of constructing a Control Code. An I/O control code is a 32-bit value that consists of several fields. The following figure illustrates the layout of I/O control codes:

![](https://learn.microsoft.com/en-us/windows-hardware/drivers/kernel/images/ioctl-1.png)

Windows makes it easier for us to define a control code using the `CTL_CODE` macro.

## Client 
