/**
 * @file a6000_disp_kmdod.c
 * @brief Windows Phone Kernel-Mode Display-Only Driver (KMDOD) for Lenovo A6000
 * 
 * Target: Windows Phone 8.1 / Windows 10 Mobile ARM (ARMv7 / ARMNT)
 * Model: WDDM 1.2 / 1.3 KMDOD
 */

#include <ntddk.h>
#include <dispmprt.h>

#define A6000_FB_WIDTH              720
#define A6000_FB_HEIGHT             1280
#define A6000_FB_BPP                32
#define A6000_FB_PITCH              (A6000_FB_WIDTH * 4)
#define A6000_FB_PHYSICAL_BASE      0x86000000ULL
#define A6000_FB_SIZE               (A6000_FB_PITCH * A6000_FB_HEIGHT)

typedef struct _A6000_DEVICE_EXTENSION {
    PVOID               FrameBufferVirtualAddress;
    PHYSICAL_ADDRESS    FrameBufferPhysicalAddress;
    ULONG               FrameBufferSize;
    ULONG               Width;
    ULONG               Height;
    ULONG               Pitch;
    DEVICE_POWER_STATE  PowerState;
} A6000_DEVICE_EXTENSION, *PA6000_DEVICE_EXTENSION;

/* WDDM KMDOD DDI Callbacks */
NTSTATUS A6000DdiAddDevice(
    IN_CONST_PDEVICE_OBJECT     PhysicalDeviceObject,
    OUT_PPVOID                  MiniportDeviceContext
)
{
    PA6000_DEVICE_EXTENSION pDevExt = (PA6000_DEVICE_EXTENSION)ExAllocatePoolWithTag(
        NonPagedPoolNx,
        sizeof(A6000_DEVICE_EXTENSION),
        '06Aa'
    );

    if (!pDevExt) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlZeroMemory(pDevExt, sizeof(A6000_DEVICE_EXTENSION));
    pDevExt->Width = A6000_FB_WIDTH;
    pDevExt->Height = A6000_FB_HEIGHT;
    pDevExt->Pitch = A6000_FB_PITCH;
    pDevExt->FrameBufferPhysicalAddress.QuadPart = A6000_FB_PHYSICAL_BASE;
    pDevExt->FrameBufferSize = A6000_FB_SIZE;
    pDevExt->PowerState = PowerDeviceD0;

    *MiniportDeviceContext = pDevExt;
    return STATUS_SUCCESS;
}

NTSTATUS A6000DdiStartDevice(
    IN_CONST_PVOID          MiniportDeviceContext,
    IN_PDXGK_START_INFO     DxgkStartInfo,
    IN_PDXGKRNL_INTERFACE  DxgkInterface,
    OUT_PULONG              NumberOfVideoPresentSources,
    OUT_PULONG              NumberOfChildren
)
{
    PA6000_DEVICE_EXTENSION pDevExt = (PA6000_DEVICE_EXTENSION)MiniportDeviceContext;
    (void)DxgkStartInfo;
    (void)DxgkInterface;

    /* Map linear framebuffer into kernel virtual memory */
    pDevExt->FrameBufferVirtualAddress = MmMapIoSpace(
        pDevExt->FrameBufferPhysicalAddress,
        pDevExt->FrameBufferSize,
        MmWriteCombined
    );

    if (!pDevExt->FrameBufferVirtualAddress) {
        return STATUS_UNSUCCESSFUL;
    }

    *NumberOfVideoPresentSources = 1;
    *NumberOfChildren = 1;

    return STATUS_SUCCESS;
}

NTSTATUS A6000DdiStopDevice(
    IN_CONST_PVOID MiniportDeviceContext
)
{
    PA6000_DEVICE_EXTENSION pDevExt = (PA6000_DEVICE_EXTENSION)MiniportDeviceContext;

    if (pDevExt->FrameBufferVirtualAddress) {
        MmUnmapIoSpace(pDevExt->FrameBufferVirtualAddress, pDevExt->FrameBufferSize);
        pDevExt->FrameBufferVirtualAddress = NULL;
    }

    return STATUS_SUCCESS;
}

NTSTATUS A6000DdiRemoveDevice(
    IN_CONST_PVOID MiniportDeviceContext
)
{
    PA6000_DEVICE_EXTENSION pDevExt = (PA6000_DEVICE_EXTENSION)MiniportDeviceContext;
    if (pDevExt) {
        ExFreePoolWithTag(pDevExt, '06Aa');
    }
    return STATUS_SUCCESS;
}

/* DriverEntry for Windows Phone 8 Display Driver */
NTSTATUS DriverEntry(
    IN PDRIVER_OBJECT   DriverObject,
    IN PUNICODE_STRING  RegistryPath
)
{
    (void)RegistryPath;
    DRIVER_INITIALIZATION_DATA DriverInitData;
    RtlZeroMemory(&DriverInitData, sizeof(DriverInitData));

    DriverInitData.Version = DXGKDDI_INTERFACE_VERSION_WDDM1_3;
    DriverInitData.DxgkDdiAddDevice = A6000DdiAddDevice;
    DriverInitData.DxgkDdiStartDevice = A6000DdiStartDevice;
    DriverInitData.DxgkDdiStopDevice = A6000DdiStopDevice;
    DriverInitData.DxgkDdiRemoveDevice = A6000DdiRemoveDevice;

    return DxgkInitializeDisplayOnlyDriver(DriverObject, RegistryPath, &DriverInitData);
}
