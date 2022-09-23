#include <Wdm.h>
#include <Wdf.h>
#include <ntstrsafe.h>
#include "SampleDriver.h"
#include "trace.h"

#if defined(EVENT_TRACING)
//
// The trace message header (.tmh) file must be included in a source file
// before any WPP macro calls and after defining a WPP_CONTROL_GUIDS
// macro (defined in toaster.h). During the compilation, WPP scans the source
// files for DoTraceMessage() calls and builds a .tmh file which stores a unique
// data GUID for each message, the text resource string for each message,
// and the data types of the variables passed in for each message.  This file
// is automatically generated and used during post-processing.
//
#include "SampleDriver.tmh"
#endif

//#define WPP_INIT_TRACING(DriverObject, RegistryPath)
//#define WPP_CLEANUP(DriverObject)



#define _DRIVER_NAME_ "PCIDRV"

#if !defined(TRACE_LEVEL_NONE)
#define TRACE_LEVEL_NONE        0
#define TRACE_LEVEL_CRITICAL    1
#define TRACE_LEVEL_FATAL       1
#define TRACE_LEVEL_ERROR       2
#define TRACE_LEVEL_WARNING     3
#define TRACE_LEVEL_INFORMATION 4
#define TRACE_LEVEL_VERBOSE     5
#define TRACE_LEVEL_RESERVED6   6
#define TRACE_LEVEL_RESERVED7   7
#define TRACE_LEVEL_RESERVED8   8
#define TRACE_LEVEL_RESERVED9   9
#endif

//
// Define Debug Flags
//
#define DBG_INIT                0x00000001
#define DBG_PNP                 0x00000002
#define DBG_POWER               0x00000004
#define DBG_WMI                 0x00000008
#define DBG_CREATE_CLOSE        0x00000010
#define DBG_IOCTLS              0x00000020
#define DBG_WRITE               0x00000040
#define DBG_READ                0x00000080
#define DBG_DPC                 0x00000100
#define DBG_INTERRUPT           0x00000200
#define DBG_LOCKS               0x00000400
#define DBG_QUEUEING            0x00000800
#define DBG_HW_ACCESS           0x00001000

VOID
TraceEvents(
   IN ULONG   DebugPrintLevel,
   IN ULONG   DebugPrintFlag,
   IN PCCHAR  DebugMessage,
   ...
);

NTSTATUS
SampleEvtDevicePrepareHardware(
    WDFDEVICE      Device,
    WDFCMRESLIST   Resources,
    WDFCMRESLIST   ResourcesTranslated
);

NTSTATUS
SerialMapHWResources(
    IN OUT PFDO_DATA FdoData,
    IN WDFCMRESLIST  ResourcesRaw,
    IN WDFCMRESLIST  ResourcesTranslated
);

//
// Global debug error level
//
#if !defined(EVENT_TRACING)
ULONG DebugLevel = TRACE_LEVEL_INFORMATION;
ULONG DebugFlag = 0x2f;//0x46;//0x4FF; //0x00000006;
#else
ULONG DebugLevel; // wouldn't be used to control the TRACE_LEVEL_VERBOSE
ULONG DebugFlag;
#endif


#ifdef ALLOC_PRAGMA
#pragma alloc_text (INIT, DriverEntry)
#pragma alloc_text (PAGE, SampleEvtDriverDeviceAdd)
#pragma alloc_text (PAGE, SampleEvtDevicePrepareHardware)
#endif


NTSTATUS DriverEntry(
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

    TraceEvents(TRACE_LEVEL_INFORMATION, DBG_INIT, "PCIDRV Sample - Driver Framework Edition \n");

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

    TraceEvents(TRACE_LEVEL_INFORMATION, DBG_PNP,
        "-->PciDrvEvtDeviceAdd routine. Driver: 0x%p\n", Driver);

    //
    // I/O type is Buffered by default. If required to use something else,
    // call WdfDeviceInitSetIoType with the appropriate type.
    //
    WdfDeviceInitSetIoType(DeviceInit, WdfDeviceIoDirect);

    //
    // Zero out the PnpPowerCallbacks structure.
    //
    WDF_PNPPOWER_EVENT_CALLBACKS_INIT(&pnpPowerCallbacks);

    //
    // Set Callbacks for any of the functions we are interested in.
    // If no callback is set, Framework will take the default action
    // by itself.  This sample provides many of the possible callbacks,
    // mostly because it's a fairly complex sample that drives full-featured
    // hardware.  Drivers derived from this sample will often be able to
    // provide only some of these.
    //

    //
    // These callback is invoked to tear down all the driver-managed state
    // that is set up in this function.  Many times, this callback won't do
    // much of anything, since many of the things that are set up here will
    // have their lifetimes automatically managed by the Framework.
    //


    //
    // These two callbacks set up and tear down hardware state,
    // specifically that which only has to be done once.
    //

    //pnpPowerCallbacks.EvtDevicePrepareHardware = SampleEvtDevicePrepareHardware;
    //pnpPowerCallbacks.EvtDeviceReleaseHardware = PciDrvEvtDeviceReleaseHardware;

    //
    // These two callbacks set up and tear down hardware state that must be
    // done every time the device moves in and out of the D0-working state.
    //

    //pnpPowerCallbacks.EvtDeviceD0Entry = PciDrvEvtDeviceD0Entry;
    //pnpPowerCallbacks.EvtDeviceD0Exit = PciDrvEvtDeviceD0Exit;

    //
    // These next two callbacks are for doing work at PASSIVE_LEVEL (low IRQL)
    // after all the interrupts are connected and before they are disconnected.
    //
    // Some drivers need to do device initialization and tear-down while the
    // interrupt is connected.  (This is a problem for these devices, since
    // it opens them up to taking interrupts before they are actually ready
    // to handle them, or to taking them after they have torn down too much
    // to be able to handle them.)  While this hardware design pattern is to
    // be discouraged, it is possible to handle it by doing device init and
    // tear down in these routines rather than in EvtDeviceD0Entry and
    // EvtDeviceD0Exit.
    //
    // In this sample these callbacks don't do anything.
    //

    //pnpPowerCallbacks.EvtDeviceD0EntryPostInterruptsEnabled = NICEvtDeviceD0EntryPostInterruptsEnabled;
    //pnpPowerCallbacks.EvtDeviceD0ExitPreInterruptsDisabled = NICEvtDeviceD0ExitPreInterruptsDisabled;

    //
    // This next group of five callbacks allow a driver to become involved in
    // starting and stopping operations within a driver as the driver moves
    // through various PnP/Power states.  These functions are not necessary
    // if the Framework is managing all the device's queues and there is no
    // activity going on that isn't queue-based.  This sample provides these
    // callbacks because it uses watchdog timer to monitor whether the device
    // is working or not and it needs to start and stop the timer when the device
    // is started or removed. It cannot start and stop the timers in the D0Entry
    // and D0Exit callbacks because if the device is surprise-removed, D0Exit
    // will not be called.
    //
    //pnpPowerCallbacks.EvtDeviceSelfManagedIoInit = PciDrvEvtDeviceSelfManagedIoInit;
    //pnpPowerCallbacks.EvtDeviceSelfManagedIoCleanup = PciDrvEvtDeviceSelfManagedIoCleanup;
    //pnpPowerCallbacks.EvtDeviceSelfManagedIoSuspend = PciDrvEvtDeviceSelfManagedIoSuspend;
    //pnpPowerCallbacks.EvtDeviceSelfManagedIoRestart = PciDrvEvtDeviceSelfManagedIoRestart;

    //
    // Register the PnP and power callbacks. Power policy related callbacks will be registered
    // later.
    //
    WdfDeviceInitSetPnpPowerEventCallbacks(DeviceInit, &pnpPowerCallbacks);

    //
    // Init the power policy callbacks
    //
    WDF_POWER_POLICY_EVENT_CALLBACKS_INIT(&powerPolicyCallbacks);

    //
    // This group of three callbacks allows this sample driver to manage
    // arming the device for wake from the S0 state.  Networking devices can
    // optionally be put into a low-power state when there is no networking
    // cable plugged into them.  This sample implements this feature.
    //
    //powerPolicyCallbacks.EvtDeviceArmWakeFromS0 = PciDrvEvtDeviceWakeArmS0;
    //powerPolicyCallbacks.EvtDeviceDisarmWakeFromS0 = PciDrvEvtDeviceWakeDisarmS0;
    //powerPolicyCallbacks.EvtDeviceWakeFromS0Triggered = PciDrvEvtDeviceWakeTriggeredS0;

    //
    // This group of three callbacks allows the device to be armed for wake
    // from Sx (S1, S2, S3 or S4.)  Networking devices can optionally be put
    // into a state where a packet sent to them will cause the device's wake
    // signal to be triggered, which causes the machine to wake, moving back
    // into the S0 state.
    //

    //powerPolicyCallbacks.EvtDeviceArmWakeFromSx = PciDrvEvtDeviceWakeArmSx;
    //powerPolicyCallbacks.EvtDeviceDisarmWakeFromSx = PciDrvEvtDeviceWakeDisarmSx;
    //powerPolicyCallbacks.EvtDeviceWakeFromSxTriggered = PciDrvEvtDeviceWakeTriggeredSx;

    //
    // Register the power policy callbacks.
    //
    WdfDeviceInitSetPowerPolicyEventCallbacks(DeviceInit, &powerPolicyCallbacks);

    // Since we are the function driver, we are now the power policy owner
    // for the device according to the default framework rule. We will register
    // our power policy callbacks after finding the wakeup capability of the device.

    //
    // Specify the context type and size for the device we are about to create.
    //
    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&fdoAttributes, FDO_DATA);

    //
    // ContextCleanup will be called by the framework when it deletes the device.
    // So you can defer freeing any resources allocated to Cleanup callback in the
    // event AddDevice returns any error after the device is created.
    //
    //fdoAttributes.EvtCleanupCallback = PciDrvEvtDeviceContextCleanup;

    status = WdfDeviceCreate(&DeviceInit, &fdoAttributes, &device);

    if (!NT_SUCCESS(status)) {
        TraceEvents(TRACE_LEVEL_ERROR, DBG_PNP,
            "WdfDeviceInitialize failed %!STATUS!\n", status);
        return status;
    }

    //
    // Device creation is complete.
    // Get the DeviceExtension and initialize it.
    //
    fdoData = FdoGetData(device);
    fdoData->WdfDevice = device;

    TraceEvents(TRACE_LEVEL_INFORMATION, DBG_PNP,
        "PDO(0x%p) FDO(0x%p), Lower(0x%p) DevExt (0x%p)\n",
        WdfDeviceWdmGetPhysicalDevice(device),
        WdfDeviceWdmGetDeviceObject(device),
        WdfDeviceWdmGetAttachedDevice(device),
        fdoData);

    //
    // Initialize the device extension and allocate all the software resources
    //
    //status = NICAllocateSoftwareResources(fdoData);
    //if (!NT_SUCCESS(status)) {
    //    TraceEvents(TRACE_LEVEL_ERROR, DBG_PNP,
    //        "NICAllocateSoftwareResources failed: %!STATUS!\n",
    //        status);
    //    return status;
    //}

    //
    // If our device supports wait-wake then we will set our power-policy and
    // update S0-Idle policy.
    //
    //if (IsPoMgmtSupported(fdoData)) {
    //    TraceEvents(TRACE_LEVEL_INFORMATION, DBG_PNP,
    //        "Device has wait-wake capability\n");
    //    status = PciDrvSetPowerPolicy(fdoData);
    //    if (!NT_SUCCESS(status)) {
    //        TraceEvents(TRACE_LEVEL_ERROR, DBG_PNP,
    //            "PciDrvSetPowerPolicy failed %!STATUS!\n", status);
    //        return status;
    //    }
    //}

    //
    // Tell the Framework that this device will need an interface so that
    // application can interact with it.
    //
    //status = WdfDeviceCreateDeviceInterface(
    //    device,
    //    (LPGUID)&GUID_DEVINTERFACE_PCIDRV,
    //    NULL
    //);

    if (!NT_SUCCESS(status)) {
        TraceEvents(TRACE_LEVEL_ERROR, DBG_PNP,
            "WdfDeviceCreateDeviceInterface failed %!STATUS!\n", status);
        return status;
    }

    //status = PciDrvWmiRegistration(device);

    TraceEvents(TRACE_LEVEL_INFORMATION, DBG_PNP, "<-- PciDrvEvtDeviceAdd  \n");

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

    TraceEvents(TRACE_LEVEL_INFORMATION, DBG_PNP,
        "--> PciDrvEvtDevicePrepareHardware\n");

    fdoData = FdoGetData(Device);

    status = SerialMapHWResources(fdoData, Resources, ResourcesTranslated);
    if (!NT_SUCCESS(status)) {
        TraceEvents(TRACE_LEVEL_ERROR, DBG_PNP,
            "NICMapHWResources failed: %!STATUS!\n", status);
        return status;
    }

    TraceEvents(TRACE_LEVEL_INFORMATION, DBG_PNP,
        "<-- PciDrvEvtDevicePrepareHardware\n");

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
            TraceEvents(TRACE_LEVEL_ERROR, DBG_INIT, "WdfResourceCmGetDescriptor");
            return STATUS_DEVICE_CONFIGURATION_ERROR;
        }

        switch (descriptor->Type) {

        case CmResourceTypePort:
            numberOfBARs++;

            TraceEvents(TRACE_LEVEL_VERBOSE, DBG_INIT,
                "numberOfBARs: %d\n",
                numberOfBARs);
            break;

        case CmResourceTypeMemory:
            numberOfBARs++;

            TraceEvents(TRACE_LEVEL_VERBOSE, DBG_INIT,
                "numberOfBARs: %d\n",
                numberOfBARs);
            break;

        case CmResourceTypeInterrupt:
            numberOfBARs++;

            TraceEvents(TRACE_LEVEL_VERBOSE, DBG_INIT,
                "numberOfBARs: %d\n",
                numberOfBARs);
            break;

        default:
            TraceEvents(TRACE_LEVEL_VERBOSE, DBG_INIT, "Unhandled resource type (0x%x)\n",
                descriptor->Type);
            break;
        }
    }
    return status;
}


VOID
TraceEvents(
    IN ULONG   TraceEventsLevel,
    IN ULONG   TraceEventsFlag,
    IN PCCHAR  DebugMessage,
    ...
)
{
#define     TEMP_BUFFER_SIZE        512
    va_list    list;
    CHAR       debugMessageBuffer[TEMP_BUFFER_SIZE];
    NTSTATUS   status;

    va_start(list, DebugMessage);

    if (DebugMessage) {

        //
        // Using new safe string functions instead of _vsnprintf.
        // This function takes care of NULL terminating if the message
        // is longer than the buffer.
        //
        status = RtlStringCbVPrintfA(debugMessageBuffer,
            sizeof(debugMessageBuffer),
            DebugMessage,
            list);
        if (!NT_SUCCESS(status)) {

            DbgPrint(_DRIVER_NAME_": RtlStringCbVPrintfA failed %x\n",
                status);
            return;
        }
        if (TraceEventsLevel <= TRACE_LEVEL_INFORMATION ||
            (TraceEventsLevel <= DebugLevel &&
                ((TraceEventsFlag & DebugFlag) == TraceEventsFlag))) {
            DbgPrint(debugMessageBuffer);
        }
    }
    va_end(list);

    return;

}

