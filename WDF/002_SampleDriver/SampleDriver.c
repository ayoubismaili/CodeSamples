/*++

Copyright (c) Ayoub Ismaili.  All rights reserved.

SPDX-License-Identifier: MIT


Module Name:

    SampleDriver.c   

Abstract:

    Sample kernel mode driver that enumerates Base Adress Registers (BARs) for a Serial PCI Device (PCI\VEN_1B36&DEV_0002).

--*/
#include <stdio.h>
#include <ntddk.h>
#include <Wdf.h>
//#include "drvioctl.h"
#include "SampleDriver.h"
#include "SampleDriver.tmh"      //  this is the file that will be auto generated


#ifdef ALLOC_PRAGMA
    #pragma alloc_text( INIT, DriverEntry )
    #pragma alloc_text( PAGE, SampleEvtDriverDeviceAdd )
    #pragma alloc_text( PAGE, SampleEvtDevicePrepareHardware )
#endif // ALLOC_PRAGMA


#define MAXEVENTS 3


NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath
    )
{
    NTSTATUS               status = STATUS_SUCCESS;
    WDF_DRIVER_CONFIG      config;
    WDF_OBJECT_ATTRIBUTES  attrib;
    WDFDRIVER              driver;
    PDRIVER_CONTEXT        driverContext;

    //
    // Initialize WPP Tracing
    //
    WPP_INIT_TRACING(DriverObject, RegistryPath);

    DoTraceLevelMessage(TRACE_LEVEL_INFORMATION, DBG_INIT, "002_SampleDriver - Enumerates Base Adress Registers (BARs) for a PCI Device \n");

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
{
    NTSTATUS                        status = STATUS_SUCCESS;
    WDF_PNPPOWER_EVENT_CALLBACKS    pnpPowerCallbacks;
    WDF_POWER_POLICY_EVENT_CALLBACKS powerPolicyCallbacks;
    WDF_OBJECT_ATTRIBUTES           fdoAttributes;
    WDFDEVICE                       device;
    PFDO_DATA                       fdoData = NULL;

    PAGED_CODE();

    DoTraceLevelMessage(TRACE_LEVEL_INFORMATION, DBG_PNP,
        "-->SampleEvtDriverDeviceAdd routine. Driver: 0x%p\n", Driver);

    //
    // I/O type is Buffered by default. If required to use something else,
    // call WdfDeviceInitSetIoType with the appropriate type.
    //
    WdfDeviceInitSetIoType(DeviceInit, WdfDeviceIoDirect);

    //
    // Zero out the PnpPowerCallbacks structure.
    //
    WDF_PNPPOWER_EVENT_CALLBACKS_INIT(&pnpPowerCallbacks);

    pnpPowerCallbacks.EvtDevicePrepareHardware = SampleEvtDevicePrepareHardware;

	//
    // Register the PNP and Power callbacks.
    //
    WdfDeviceInitSetPnpPowerEventCallbacks(DeviceInit, &pnpPowerCallbacks);

    //
    // Init the power policy callbacks
    //
    WDF_POWER_POLICY_EVENT_CALLBACKS_INIT(&powerPolicyCallbacks);

    //
    // Register the power policy callbacks.
    //
    WdfDeviceInitSetPowerPolicyEventCallbacks(DeviceInit, &powerPolicyCallbacks);

    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&fdoAttributes, FDO_DATA);

    status = WdfDeviceCreate(&DeviceInit, &fdoAttributes, &device);

    if (!NT_SUCCESS(status)) {
        DoTraceLevelMessage(TRACE_LEVEL_ERROR, DBG_PNP,
            "WdfDeviceInitialize failed %!STATUS!\n", status);
        return status;
    }

    //
    // Device creation is complete.
    // Get the DeviceExtension and initialize it.
    //
    fdoData = FdoGetData(device);
    fdoData->WdfDevice = device;

    DoTraceLevelMessage(TRACE_LEVEL_INFORMATION, DBG_PNP,
        "PDO(0x%p) FDO(0x%p), Lower(0x%p) DevExt (0x%p)\n",
        WdfDeviceWdmGetPhysicalDevice(device),
        WdfDeviceWdmGetDeviceObject(device),
        WdfDeviceWdmGetAttachedDevice(device),
        fdoData);

    if (!NT_SUCCESS(status)) {
        DoTraceLevelMessage(TRACE_LEVEL_ERROR, DBG_PNP,
            "WdfDeviceCreateDeviceInterface failed %!STATUS!\n", status);
        return status;
    }

    DoTraceLevelMessage(TRACE_LEVEL_INFORMATION, DBG_PNP, "<-- SampleEvtDriverDeviceAdd  \n");

    return status;
}


NTSTATUS
SampleEvtDevicePrepareHardware(
    WDFDEVICE      Device,
    WDFCMRESLIST   Resources,
    WDFCMRESLIST   ResourcesTranslated
)
{
    NTSTATUS     status = STATUS_SUCCESS;
    PFDO_DATA    fdoData = NULL;

    PAGED_CODE();

    DoTraceLevelMessage(TRACE_LEVEL_INFORMATION, DBG_PNP,
        "--> SampleEvtDevicePrepareHardware\n");

    fdoData = FdoGetData(Device);

    status = SerialMapHWResources(fdoData, Resources, ResourcesTranslated);
    if (!NT_SUCCESS(status)) {
        DoTraceLevelMessage(TRACE_LEVEL_ERROR, DBG_PNP,
            "SerialMapHWResources failed: %!STATUS!\n", status);
        return status;
    }

    DoTraceLevelMessage(TRACE_LEVEL_INFORMATION, DBG_PNP,
        "<-- SampleEvtDevicePrepareHardware\n");

    return status;
}


NTSTATUS
SerialMapHWResources(
    IN OUT PFDO_DATA FdoData,
    IN WDFCMRESLIST  ResourcesRaw,
    IN WDFCMRESLIST  ResourcesTranslated
)
{
    PCM_PARTIAL_RESOURCE_DESCRIPTOR descriptor;
    ULONG       i;
    NTSTATUS    status = STATUS_SUCCESS;
    BOOLEAN     bResPort = FALSE;
    BOOLEAN     bResInterrupt = FALSE;
    BOOLEAN     bResMemory = FALSE;
    ULONG       numberOfBARs = 0;

    UNREFERENCED_PARAMETER(FdoData);
    UNREFERENCED_PARAMETER(ResourcesRaw);
    UNREFERENCED_PARAMETER(bResPort);
    UNREFERENCED_PARAMETER(bResInterrupt);
    UNREFERENCED_PARAMETER(bResMemory);

    PAGED_CODE();

    for (i = 0; i < WdfCmResourceListGetCount(ResourcesTranslated); i++) {

        descriptor = WdfCmResourceListGetDescriptor(ResourcesTranslated, i);

        if (!descriptor) {
            DoTraceLevelMessage(TRACE_LEVEL_ERROR, DBG_INIT, "WdfResourceCmGetDescriptor");
            return STATUS_DEVICE_CONFIGURATION_ERROR;
        }

        switch (descriptor->Type) {

        case CmResourceTypePort:
            numberOfBARs++;

            DoTraceLevelMessage(TRACE_LEVEL_VERBOSE, DBG_INIT,
                "numberOfBARs: %d -> Port\n",
                numberOfBARs);
            break;

        case CmResourceTypeMemory:
            numberOfBARs++;

            DoTraceLevelMessage(TRACE_LEVEL_VERBOSE, DBG_INIT,
                "numberOfBARs: %d -> Memory\n",
                numberOfBARs);
            break;

        case CmResourceTypeInterrupt:
            numberOfBARs++;

            DoTraceLevelMessage(TRACE_LEVEL_VERBOSE, DBG_INIT,
                "numberOfBARs: %d -> Interrupt\n",
                numberOfBARs);
            break;

        default:
            DoTraceLevelMessage(TRACE_LEVEL_VERBOSE, DBG_INIT, "Unhandled resource type (0x%x)\n",
                descriptor->Type);
            break;
        }
    }
    return status;
}

