#include <Wdm.h>
#include <Wdf.h>
#include "SampleDriver.h"

NTSTATUS DriverEntry(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath
)
/*++

Routine Description:

    Installable driver initialization entry point.
    This entry point is called directly by the I/O system.

Arguments:

    DriverObject - pointer to the driver object

    RegistryPath - pointer to a unicode string representing the path,
                   to driver-specific key in the registry.

Return Value:

    STATUS_SUCCESS if successful,
    STATUS_UNSUCCESSFUL otherwise.

--*/
{
    NTSTATUS               status = STATUS_SUCCESS;
    WDF_DRIVER_CONFIG      config;
    WDF_OBJECT_ATTRIBUTES  attrib;
    WDFDRIVER              driver;
    PDRIVER_CONTEXT        driverContext;

    //
    // Initialize the Object Attributes structure.
    //
    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&attrib, DRIVER_CONTEXT);

    //
    // Initialize the Driver Config structure.
    //
    WDF_DRIVER_CONFIG_INIT(&config, SampleEvtDriverDeviceAdd);

    //
    // Create a WDFDRIVER object.
    //
    status = WdfDriverCreate(DriverObject,
        RegistryPath,
        &attrib,
        &config,
        &driver);

    if (!NT_SUCCESS(status)) {
        return status;
    }

    driverContext = GetDriverContext(driver);

    return STATUS_SUCCESS;
}

NTSTATUS SampleEvtDriverDeviceAdd(
    IN     WDFDRIVER Driver,
    IN OUT PWDFDEVICE_INIT DeviceInit
)
/*++
Routine Description:

    EvtDeviceAdd is called by the framework in response to AddDevice
    call from the PnP manager.

Arguments:

    Driver - Handle to a framework driver object created in DriverEntry

    DeviceInit - Pointer to a framework-allocated WDFDEVICE_INIT structure.

Return Value:

    NTSTATUS

--*/
{
    UNREFERENCED_PARAMETER(Driver);
    UNREFERENCED_PARAMETER(DeviceInit);

    return STATUS_SUCCESS;
}

