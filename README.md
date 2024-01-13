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

- `DriverEntry()` 
- `ProcRevealUnload()`
- `ProcRevealCreateClose()`
- ` ProcRevealDeviceControl()`

## Client 
