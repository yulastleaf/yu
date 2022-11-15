#include "DriverMain.h"

// 全局变量，记录DriverObject
PDRIVER_OBJECT gSFilterControlDeviceObject;

typedef struct _SFILTER_DEVICE_EXTENSION {

    ULONG TypeFlag;

    //
    //  Pointer to the file system device object we are attached to
    //

    PDEVICE_OBJECT AttachedToDeviceObject;

    //
    //  Pointer to the real (disk) device object that is associated with
    //  the file system device object we are attached to
    //

    PDEVICE_OBJECT StorageStackDeviceObject;

    //
    //  Name for this device.  If attached to a Volume Device Object it is the
    //  name of the physical disk drive.  If attached to a Control Device
    //  Object it is the name of the Control Device Object.
    //

    UNICODE_STRING DeviceName;

    //
    //  Buffer used to hold the above unicode strings
    //

    WCHAR DeviceNameBuffer[MAX_DEVNAME_LENGTH];

    // 
    // The extension used by other user.
    //

    UCHAR UserExtension[1];

} SFILTER_DEVICE_EXTENSION, * PSFILTER_DEVICE_EXTENSION;



NTSTATUS
DriverEntry(
    _In_ PDRIVER_OBJECT DriverObject,
    _In_ PUNICODE_STRING RegistryPath
) {

    // 定义一个Unicode字符串

    UNICODE_STRING nameString;
    RtlInitUnicodeString(&nameString, L"\\FileSystem\\Filters\\SFilter");


    // 生成控制设备
    NTSTATUS status = IoCreateDevice(DriverObject, 0, &nameString, FILE_DEVICE_DISK_FILE_SYSTEM, FILE_DEVICE_SECURE_OPEN, FALSE, &gSFilterControlDeviceObject);

    do {

        // 如果路径没找到而生成失败
        if (status == STATUS_OBJECT_PATH_NOT_FOUND) {
            // 因为低版本可能没有\FileSystem\Filter\这个目录
            // 如果没有，则改变位置，生成到\FileSystem\下

            RtlInitUnicodeString(&nameString, L"\\FileSystem\\SFilterCDO");

            status = IoCreateDevice(DriverObject, 0, &nameString, FILE_DEVICE_DISK_FILE_SYSTEM, FILE_DEVICE_SECURE_OPEN, FALSE, &gSFilterControlDeviceObject);

            // 成功后，用KdPrint打印一个log.
            if (!NT_SUCCESS(status)) {
                KdPrint(("SFilterIDriverEntry: Error creating control device object \"%wZ\", status = % 08xln", &nameString, status));
                return status;
            }
        }
        else if (!NT_SUCCESS(status)) {
            // 失败也打印一个。并直接返回错误
            KdPrint(("SFilterlDriverEntry: Error creating control device object \"%wZ\",status=%08xln", &nameString, status));
            break;
        }

        int i = 0;
        for (i = 0; i <= IRP_MJ_MAXIMUM_FUNCTION; i++) {
            DriverObject->MajorFunction[i] = sfPassThrough;
        }

        DriverObject->MajorFunction[IRP_MJ_CREATE] = sfCreate;
        DriverObject->MajorFunction[IRP_MJ_CREATE_NAMED_PIPE] = SfCreate; 
        DriverObject->MajorFunction[IRP_MJ_CREATE_MAILSLOT] = sfCreate;
        DriverObject->MajorFunction[IRP_MJ_CLEANUP] = SfCleanupClose;
        DriverObject->MajorFunction[IRP_MJ_FILE_SYSTEM_CONTROL] = SfFsControl;
        DriverObject->MajorFunction[IRP_MJ_CLOSE] = sfCleanupClose;

        PFAST_IO_DISPATCH fastIoDispatch;
        fastIoDispatch = ExAllocatePoolWithTag(NonPagedPool, sizeof(FAST_IO_DISPATCH), SFLT_POOL_TAG);
        if (!fastIoDispatch) {
            // 分配失败的情况，删除我们先生成的控制设备IoDeleteDevice( gSFilterControlDeviceObject ); return STATUS_INSUFFICIENT_RESOURCES;
        }
        // 内存清零。
        RtlZeroMemory(fastIoDispatch, sizeof(FAST_IO_DISPATCH)); fastIoDispatch->SizeOfFastIoDispatch = sizeof(FAST_IO_DISPATCH);

        //我们过滤以下所有的函数:
        fastIoDispatch->FastIoCheckIfPossible = SfFastIoCheckIfPossible; 
        fastIoDispatch->FastIoRead = SfFastIoRead;
        fastIoDispatch->FastIoWrite = SfFastIoWrite;
        fastIoDispatch->FastIoQueryBasicInfo = SfFastIoQueryBasicInfo; 
        fastIoDispatch->FastIoQueryStandardInfo = SfFastIoQueryStandardInfo; 
        fastIoDispatch->FastIoLock = SfFastIoLock;
        fastIoDispatch->FastIoUnlockSingle = SfFastIoUnlockSingle; 
        fastIoDispatch->FastIoUnlockAll = SfFastIoUnlockAll; 
        fastIoDispatch->FastIoUnlockAllByKey = SfFastIoUnlockAllByKey; 
        fastIoDispatch->FastIoDeviceControl = SfFastIoDeviceControl; 
        fastIoDispatch->FastIoDetachDevice = SfFastIoDetachDevice;
        fastIoDispatch->FastIoQueryNetworkOpenInfo = SfFastIoQueryNetworkOpenInfo; 
        fastIoDispatch->MdlRead = SfFastIoMdlRead;
        fastIoDispatch->MdlReadComplete = SfFastIoMdlReadComplete; 
        fastIoDispatch->PrepareMdlWrite = SfFastIoPrepareMdlWrite; 
        fastIoDispatch->MdlWriteComplete = SfFastIoMdlWriteComplete; 
        fastIoDispatch->FastIoReadCompressed = SfFastIoReadCompressed; 
        fastIoDispatch->FastIoWriteCompressed = SfFastIoWriteCompressed;
        fastIoDispatch->MdlReadCompleteCompressed = SfFastIoMdlReadCompleteCompressed; 
        fastIoDispatch->MdlWriteCompleteCompressed = SfFastIoMdlWriteCompleteCompressed;

        fastIoDispatch->FastIoQueryOpen = SfFastIoQueryOpen;
        // 最后指定给 DriverObject.
        DriverObject->FastIoDispatch = fastIoDispatch;

    } while (0);


    return status;

}

NTSTATUS
sfPassThrough(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp)
{
    // 对于我们认为“不能”的事情，我们采用ASSERT进行调试模式下的确认。 
    // 而不加多余的判断来消耗我们的效率。这些宏在调试模式下不被编译。
    ASSERT(!IS_MY_CONTROL_DEVICE_OBJECT(DeviceObject)); 
    ASSERT(IS_MY_DEVICE_OBJECT(DeviceObject));

    IoSkipCurrentIrpStacklocation(Irp);
    
    return IoCallDriver(((PSFILTER_DEVICE_EXTENSION)DeviceObject->DeviceExtension)->AttachedToDeviceObject, Irp);
}

