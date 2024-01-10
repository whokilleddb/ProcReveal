#include <stdio.h>
#include <Windows.h>
#include "..\ProcessRevealDriver\ProcRevealCommon.h"

int main(int argc, char * argv[]) {
	char* __t;
	HANDLE hProcess;
	DWORD bytes = 0;
	ProcessData data = { 0 };
	
	if (argc != 2) {
		fprintf(stderr, "\nUSAGE:\n\t%s\t<PID>\n", argv[0]);
		return -1;
	}

	ULONG pid = strtoul(argv[1], &__t, 10);
	printf("[i] Trying to open handle to process:\t%lu\n", pid);

	hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);
	if (hProcess != NULL) {
		printf("[i] Opened target process with `PROCESS_ALL_ACCESS` using OpenProcess()\n");
		CloseHandle(hProcess);
		return 1;
	}

	printf("[?] OpenProcess() failed with:\t\t0x%x\n[i] Trying with the ProcReveal Driver now\n", GetLastError());
	HANDLE hDevice = CreateFile(USER_DEVICE_SYM_LINK, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
	if (INVALID_HANDLE_VALUE == hDevice) {
		fprintf(stderr, "[!] CreateFile() failed with:\t\t0x%x\n", GetLastError());
		return -1;
	}

	printf("[i] Opened a handle to Kernel Device\n");

	data.ProcessId = pid;
	data.Access = PROCESS_ALL_ACCESS;

	BOOL bResult = DeviceIoControl(hDevice, IOCTL_OPEN_PROCESS, &data, sizeof(data), &hProcess, sizeof(hProcess), &bytes, NULL);
	if (!bResult) {
		fprintf(stderr, "[!] DeviceIoControl() failed with:\t\t0x%x\n", GetLastError());
		CloseHandle(hDevice);
		return -1;
	}
	CloseHandle(hDevice);

	if (hProcess) {
		printf("[i] Successfully opened handle to target:\t0x%p\n", hProcess);
		CloseHandle(hProcess);
		return 0;
	} 
	
	printf("[!] Still could not open handle to process :(\n");
	return -1;
}