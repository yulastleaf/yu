#include "DriverMain.h"

// ȫ�ֱ�������¼DriverObject
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

    // ����һ��Unicode�ַ���

    UNICODE_STRING nameString;
    RtlInitUnicodeString(&nameString, L"\\FileSystem\\Filters\\SFilter");


    // ���ɿ����豸
    NTSTATUS status = IoCreateDevice(DriverObject, 0, &nameString, FILE_DEVICE_DISK_FILE_SYSTEM, FILE_DEVICE_SECURE_OPEN, FALSE, &gSFilterControlDeviceObject);

    do {

        // ���·��û�ҵ�������ʧ��
        if (status == STATUS_OBJECT_PATH_NOT_FOUND) {
            // ��Ϊ�Ͱ汾����û��\FileSystem\Filter\���Ŀ¼
            // ���û�У���ı�λ�ã����ɵ�\FileSystem\��

            RtlInitUnicodeString(&nameString, L"\\FileSystem\\SFilterCDO");

            status = IoCreateDevice(DriverObject, 0, &nameString, FILE_DEVICE_DISK_FILE_SYSTEM, FILE_DEVICE_SECURE_OPEN, FALSE, &gSFilterControlDeviceObject);

            // �ɹ�����KdPrint��ӡһ��log.
            if (!NT_SUCCESS(status)) {
                KdPrint(("SFilterIDriverEntry: Error creating control device object \"%wZ\", status = % 08xln", &nameString, status));
                return status;
            }
        }
        else if (!NT_SUCCESS(status)) {
            // ʧ��Ҳ��ӡһ������ֱ�ӷ��ش���
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
            // ����ʧ�ܵ������ɾ�����������ɵĿ����豸IoDeleteDevice( gSFilterControlDeviceObject ); return STATUS_INSUFFICIENT_RESOURCES;
        }
        // �ڴ����㡣
        RtlZeroMemory(fastIoDispatch, sizeof(FAST_IO_DISPATCH)); fastIoDispatch->SizeOfFastIoDispatch = sizeof(FAST_IO_DISPATCH);

        //���ǹ����������еĺ���:
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
        // ���ָ���� DriverObject.
        DriverObject->FastIoDispatch = fastIoDispatch;

    } while (0);


    return status;

}

NTSTATUS
sfPassThrough(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp)
{
    // ����������Ϊ�����ܡ������飬���ǲ���ASSERT���е���ģʽ�µ�ȷ�ϡ� 
    // �����Ӷ�����ж����������ǵ�Ч�ʡ���Щ���ڵ���ģʽ�²������롣
    ASSERT(!IS_MY_CONTROL_DEVICE_OBJECT(DeviceObject)); 
    ASSERT(IS_MY_DEVICE_OBJECT(DeviceObject));

    IoSkipCurrentIrpStacklocation(Irp);
    
    return IoCallDriver(((PSFILTER_DEVICE_EXTENSION)DeviceObject->DeviceExtension)->AttachedToDeviceObject, Irp);
}

