/*++

Copyright (c) Ayoub Ismaili.  All rights reserved.

SPDX-License-Identifier: MIT

Module Name:

    SampleDriver.h

Abstract:

    SampleDriver.h defines:
    - The provider GUID for the driver. 
    - Macros for tracing with levels and flags
    - Tracing enumerations using custom type
    - Trace macro that incorporates PRE/POST macros

Environment:

    Kernel mode

--*/


#define TRACEDRV_NT_DEVICE_NAME     L"\\Device\\TraceKmp"
#define TRACEDRV_WIN32_DEVICE_NAME  L"\\DosDevices\\TRACEKMP"

//
// Software Tracing Definitions 
//

#define WPP_CONTROL_GUIDS \
    WPP_DEFINE_CONTROL_GUID(CtlGuid,(d58c126f, b309, 11d1, 969e, 0000f875a5bc),  \
        WPP_DEFINE_BIT(DBG_INIT)             /* bit  0 = 0x00000001 */ \
        WPP_DEFINE_BIT(DBG_PNP)              /* bit  1 = 0x00000002 */ \
        WPP_DEFINE_BIT(DBG_POWER)            /* bit  2 = 0x00000004 */ \
        WPP_DEFINE_BIT(DBG_WMI)              /* bit  3 = 0x00000008 */ \
        WPP_DEFINE_BIT(DBG_CREATE_CLOSE)     /* bit  4 = 0x00000010 */ \
        WPP_DEFINE_BIT(DBG_IOCTLS)           /* bit  5 = 0x00000020 */ \
        WPP_DEFINE_BIT(DBG_WRITE)            /* bit  6 = 0x00000040 */ \
        WPP_DEFINE_BIT(DBG_READ)             /* bit  7 = 0x00000080 */ \
        WPP_DEFINE_BIT(DBG_DPC)              /* bit  8 = 0x00000100 */ \
        WPP_DEFINE_BIT(DBG_INTERRUPT)        /* bit  9 = 0x00000200 */ \
        WPP_DEFINE_BIT(DBG_LOCKS)            /* bit 10 = 0x00000400 */ \
        WPP_DEFINE_BIT(DBG_QUEUEING)         /* bit 11 = 0x00000800 */ \
        WPP_DEFINE_BIT(DBG_HW_ACCESS)        /* bit 12 = 0x00001000 */ \
        /* You can have up to 32 defines. If you want more than that,\
           you have to provide another trace control GUID */\
		)
		

//
// DoTraceLevelMessage is a custom macro that adds support for levels to the 
// default DoTraceMessage, which supports only flags. In this version, both
// flags and level are conditions for generating the trace message. 
// The preprocessor is told to recognize the function by using the -func argument
// in the RUN_WPP line on the source file. In the source file you will find
// -func:DoTraceLevelMessage(LEVEL,FLAGS,MSG,...). The conditions for triggering 
// this event in the macro are the Levels defined in evntrace.h and the flags 
// defined above and are evaluated by the macro WPP_LEVEL_FLAGS_ENABLED below. 
// 
#define WPP_LEVEL_FLAGS_LOGGER(level,flags) WPP_LEVEL_LOGGER(flags)
#define WPP_LEVEL_FLAGS_ENABLED(level, flags) (WPP_LEVEL_ENABLED(flags) && WPP_CONTROL(WPP_BIT_ ## flags).Level >= level)

typedef enum _MachineState {
	Offline = 2,
	Online = 1,
	Failed = 0xFF000001,
	Stalled = 0xFF000002
} MachineState;
//
// Configuration block to scan the enumeration definition MachineState. Used when  
// viewing the trace to display names instead of the integer values that users must decode
//
// begin_wpp config
// CUSTOM_TYPE(state, ItemEnum(_MachineState));
// end_wpp


// MACRO: TRACE_RETURN
// Configuration block that defines trace macro. It uses the PRE/POST macros to include
// code as part of the trace macro expansion. TRACE_MACRO is equivalent to the code below:
//
// {if (Status != STATUS_SUCCESS){  // This is the code in the PRE macro
//     DoTraceMessage(FLAG_ONE, "Function Return = %!STATUS!", Status)
// ;}}                              // This is the code in the POST macro
//                                 
// 
// USEPREFIX statement: Defines a format string prefix to be used when logging the event, 
// below the STDPREFIX is used. The first value is the trace function name with out parenthesis
// and the second value is the format string to be used.
// 
// USESUFFIX statement: Defines a suffix format string that gets logged with the event. 
// 
// FUNC statement: Defines the name and signature of the trace function. The function defined 
// below takes one argument, no format string, and predefines the flag equal to FLAG_ONE.
//
//
//begin_wpp config
//USEPREFIX (TRACE_RETURN, "%!STDPREFIX!");
//FUNC TRACE_RETURN{FLAG=FLAG_ONE}(EXP);
//USESUFFIX (TRACE_RETURN, "Function Return=%!STATUS!",EXP);
//end_wpp

//
// PRE macro: The name of the macro includes the condition arguments FLAGS and EXP
//            define in FUNC above
//
#define WPP_FLAG_EXP_PRE(FLAGS, HR) {if (HR != STATUS_SUCCESS) {

//
// POST macro
// The name of the macro includes the condition arguments FLAGS and EXP
//            define in FUNC above
#define WPP_FLAG_EXP_POST(FLAGS, HR) ;}}

// 
// The two macros below are for checking if the event should be logged and for 
// choosing the logger handle to use when calling the ETW trace API
//
#define WPP_FLAG_EXP_ENABLED(FLAGS, HR) WPP_FLAG_ENABLED(FLAGS)
#define WPP_FLAG_EXP_LOGGER(FLAGS, HR) WPP_FLAG_LOGGER(FLAGS)


//==========================================================================


typedef struct _DRIVER_CONTEXT
{
	int value;
} DRIVER_CONTEXT, *PDRIVER_CONTEXT;

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(DRIVER_CONTEXT, GetDriverContext)

typedef struct _FDO_DATA
{
    WDFDEVICE               WdfDevice;
}  FDO_DATA, * PFDO_DATA;

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(FDO_DATA, FdoGetData)

//
// Function prototypes
//
DRIVER_INITIALIZE DriverEntry;

EVT_WDF_DRIVER_DEVICE_ADD SampleEvtDriverDeviceAdd;

NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath
);

NTSTATUS SampleEvtDriverDeviceAdd(
    IN     WDFDRIVER Driver,
    IN OUT PWDFDEVICE_INIT DeviceInit
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