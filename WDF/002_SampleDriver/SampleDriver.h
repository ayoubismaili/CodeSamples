
#ifndef SAMPLE_DRIVER_H
#define SAMPLE_DRIVER_H

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

#endif //SAMPLE_DRIVER_H

