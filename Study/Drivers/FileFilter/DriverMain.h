#ifndef DRIVER_MAIN_H_
#define DRIVER_MAIN_H_

#include <ntddk.h>

/*************************************************************************
    Prototypes for the startup and unload routines used for
    this Filter.

    Implementation in nullFilter.c
*************************************************************************/

DRIVER_INITIALIZE DriverEntry;
NTSTATUS
DriverEntry(
    _In_ PDRIVER_OBJECT DriverObject,
    _In_ PUNICODE_STRING RegistryPath
);

NTSTATUS
sfPassThrough(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp);

#endif // DRIVER_MAIN_H_
