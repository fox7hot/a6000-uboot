/**
 * @file uefi_gop_a6000.c
 * @brief UEFI Graphics Output Protocol (GOP) Driver for Lenovo A6000
 * 
 * Sets up 720x1280 linear framebuffer for Windows Phone 8 bootmgr/winload/basicdisplay.
 */

#include "mdss_mdp5_dsi.h"
#include "a6000_panel_ili9881.h"

typedef int32_t EFI_STATUS;
typedef void *  EFI_HANDLE;

#define EFI_SUCCESS                 0
#define EFI_UNSUPPORTED             3
#define EFI_INVALID_PARAMETER       2

typedef enum {
    PixelRedGreenBlueReserved8BitPerColor,
    PixelBlueGreenRedReserved8BitPerColor,
    PixelBitMask,
    PixelBltOnly,
    PixelFormatMax
} EFI_GRAPHICS_PIXEL_FORMAT;

typedef struct {
    uint32_t RedMask;
    uint32_t GreenMask;
    uint32_t BlueMask;
    uint32_t ReservedMask;
} EFI_PIXEL_BITMASK;

typedef struct {
    uint32_t                    Version;
    uint32_t                    HorizontalResolution;
    uint32_t                    VerticalResolution;
    EFI_GRAPHICS_PIXEL_FORMAT   PixelFormat;
    EFI_PIXEL_BITMASK           PixelInformation;
    uint32_t                    PixelsPerScanLine;
} EFI_GRAPHICS_OUTPUT_MODE_INFORMATION;

typedef struct {
    uint32_t                                MaxMode;
    uint32_t                                Mode;
    EFI_GRAPHICS_OUTPUT_MODE_INFORMATION    *Info;
    uint32_t                                SizeOfInfo;
    uint64_t                                FrameBufferBase;
    uint32_t                                FrameBufferSize;
} EFI_GRAPHICS_OUTPUT_PROTOCOL_MODE;

typedef struct _EFI_GRAPHICS_OUTPUT_PROTOCOL EFI_GRAPHICS_OUTPUT_PROTOCOL;

typedef EFI_STATUS (*EFI_GRAPHICS_OUTPUT_PROTOCOL_QUERY_MODE)(
    EFI_GRAPHICS_OUTPUT_PROTOCOL          *This,
    uint32_t                              ModeNumber,
    uint32_t                             *SizeOfInfo,
    EFI_GRAPHICS_OUTPUT_MODE_INFORMATION **Info
);

typedef EFI_STATUS (*EFI_GRAPHICS_OUTPUT_PROTOCOL_SET_MODE)(
    EFI_GRAPHICS_OUTPUT_PROTOCOL *This,
    uint32_t                      ModeNumber
);

typedef enum {
    EfiBltVideoFill,
    EfiBltVideoToBltBuffer,
    EfiBltBufferToVideo,
    EfiBltVideoToVideo,
    EfiGraphicsOutputBltOperationMax
} EFI_GRAPHICS_OUTPUT_BLT_OPERATION;

typedef struct {
    uint8_t Blue;
    uint8_t Green;
    uint8_t Red;
    uint8_t Reserved;
} EFI_GRAPHICS_OUTPUT_BLT_PIXEL;

typedef EFI_STATUS (*EFI_GRAPHICS_OUTPUT_PROTOCOL_BLT)(
    EFI_GRAPHICS_OUTPUT_PROTOCOL      *This,
    EFI_GRAPHICS_OUTPUT_BLT_PIXEL     *BltBuffer,
    EFI_GRAPHICS_OUTPUT_BLT_OPERATION  BltOperation,
    uint32_t                           SourceX,
    uint32_t                           SourceY,
    uint32_t                           DestinationX,
    uint32_t                           DestinationY,
    uint32_t                           Width,
    uint32_t                           Height,
    uint32_t                           Delta
);

struct _EFI_GRAPHICS_OUTPUT_PROTOCOL {
    EFI_GRAPHICS_OUTPUT_PROTOCOL_QUERY_MODE QueryMode;
    EFI_GRAPHICS_OUTPUT_PROTOCOL_SET_MODE   SetMode;
    EFI_GRAPHICS_OUTPUT_PROTOCOL_BLT        Blt;
    EFI_GRAPHICS_OUTPUT_PROTOCOL_MODE      *Mode;
};

/* Static GOP instances */
static EFI_GRAPHICS_OUTPUT_MODE_INFORMATION mGopModeInfo = {
    .Version              = 0,
    .HorizontalResolution = A6000_PANEL_WIDTH,
    .VerticalResolution   = A6000_PANEL_HEIGHT,
    .PixelFormat          = PixelBlueGreenRedReserved8BitPerColor,
    .PixelsPerScanLine    = A6000_PANEL_WIDTH
};

static EFI_GRAPHICS_OUTPUT_PROTOCOL_MODE mGopMode = {
    .MaxMode         = 1,
    .Mode            = 0,
    .Info            = &mGopModeInfo,
    .SizeOfInfo      = sizeof(EFI_GRAPHICS_OUTPUT_MODE_INFORMATION),
    .FrameBufferBase = A6000_DEFAULT_FB_PHYS_ADDR,
    .FrameBufferSize = A6000_PANEL_FB_SIZE
};

static EFI_STATUS GopQueryMode(
    EFI_GRAPHICS_OUTPUT_PROTOCOL          *This,
    uint32_t                              ModeNumber,
    uint32_t                             *SizeOfInfo,
    EFI_GRAPHICS_OUTPUT_MODE_INFORMATION **Info)
{
    (void)This;
    if (ModeNumber != 0 || Info == NULL || SizeOfInfo == NULL) {
        return EFI_INVALID_PARAMETER;
    }
    *SizeOfInfo = sizeof(EFI_GRAPHICS_OUTPUT_MODE_INFORMATION);
    *Info = &mGopModeInfo;
    return EFI_SUCCESS;
}

static EFI_STATUS GopSetMode(
    EFI_GRAPHICS_OUTPUT_PROTOCOL *This,
    uint32_t                      ModeNumber)
{
    (void)This;
    if (ModeNumber != 0) {
        return EFI_UNSUPPORTED;
    }
    return EFI_SUCCESS;
}

static EFI_STATUS GopBlt(
    EFI_GRAPHICS_OUTPUT_PROTOCOL      *This,
    EFI_GRAPHICS_OUTPUT_BLT_PIXEL     *BltBuffer,
    EFI_GRAPHICS_OUTPUT_BLT_OPERATION  BltOperation,
    uint32_t                           SourceX,
    uint32_t                           SourceY,
    uint32_t                           DestinationX,
    uint32_t                           DestinationY,
    uint32_t                           Width,
    uint32_t                           Height,
    uint32_t                           Delta)
{
    (void)This; (void)BltBuffer; (void)BltOperation;
    (void)SourceX; (void)SourceY; (void)DestinationX; (void)DestinationY;
    (void)Width; (void)Height; (void)Delta;
    return EFI_SUCCESS;
}

static EFI_GRAPHICS_OUTPUT_PROTOCOL mGop = {
    .QueryMode = GopQueryMode,
    .SetMode   = GopSetMode,
    .Blt       = GopBlt,
    .Mode      = &mGopMode
};

/**
 * @brief Initialize Lenovo A6000 hardware display and publish GOP protocol.
 */
EFI_STATUS A6000DisplayGopInit(EFI_GRAPHICS_OUTPUT_PROTOCOL **OutGop)
{
    /* 1. Power on panel and reset sequence */
    a6000_panel_power_on();

    /* 2. Initialize DSI Controller */
    dsi_controller_init();

    /* 3. Send ILI9881 DCS command table to turn on panel */
    a6000_panel_init_sequence();

    /* 4. Configure MDSS MDP5 pipeline for 720x1280 ARGB linear framebuffer */
    mdss_mdp5_init(A6000_DEFAULT_FB_PHYS_ADDR, A6000_PANEL_WIDTH, A6000_PANEL_HEIGHT, A6000_PANEL_STRIDE);

    if (OutGop) {
        *OutGop = &mGop;
    }

    return EFI_SUCCESS;
}
