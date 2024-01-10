#pragma once
#define DEVICE_REVEAL 0x8011
#define USER_DEVICE_SYM_LINK L"\\\\.\\ProcReveal"
#define DRIVER_SYM_LINK_NAME L"\\??\\ProcReveal"

#define IOCTL_OPEN_PROCESS CTL_CODE(DEVICE_REVEAL, 0x800, METHOD_BUFFERED, FILE_ANY_ACCESS)

typedef struct _ProcessData {
    ULONG ProcessId;
    ACCESS_MASK Access;
} ProcessData;
