# Lenovo A6000 Display Driver for Windows Phone 8 / 8.1 / 10 Mobile

## 1. Hardware Architecture Overview

- **Device**: Lenovo A6000 (Wingtech WT86518 / WT86528)
- **SoC**: Qualcomm Snapdragon 410 (MSM8916)
- **Display Engine**: Qualcomm MDSS v1.4 / MDP5
- **Panel Interface**: MIPI-DSI (4 data lanes, 1 clock lane) in Video Mode
- **LCD Panels**:
  - Ilitek ILI9881 / ILI9881C (720x1280 @ 60Hz) [Primary Qimei/Wingtech panel]
  - Himax HX8394D (720x1280 @ 60Hz) [Alternative Tianma panel]
- **Hardware Pins**:
  - Panel Reset: GPIO 25 (Active Low)
  - Power Enable: GPIO 12
  - Backlight PWM: GPIO 86 / PM8916 WLED

---

## 2. Display Pipeline on Windows Phone 8

Windows Phone 8 utilizes a two-tier display architecture:

```
[ UEFI Firmware / GOP Driver ]
            │ (Initializes MDSS MDP5, DSI PHY, & ILI9881 via DCS commands)
            ▼
[ Linear Framebuffer at 0x86000000 (720x1280 ARGB) ]
            │
            ├─► Windows Boot Manager (bootmgr.efi) [Shows Windows logo]
            ├─► Windows OS Loader (winload.efi)    [Shows boot spinner]
            │
            ▼ (ExitBootServices)
[ Windows NT Kernel (ntoskrnl.exe) ]
            │
    ┌───────┴───────────────────────────────┐
    ▼                                       ▼
[ Mode A: Framebuffer Driver ]      [ Mode B: Qualcomm WDDM Miniport ]
- basicdisplay.sys (MSBDD)          - qcdx8916.sys (Adreno 306 3D)
- a6000_disp_kmdod.c (Custom KMDOD) - Enumerated via ACPI DSDT "QCOM240D"
- Reads/writes 0x86000000           - Uses dsdt_disp_a6000.asl definitions
```

---

## 3. Deployment Instructions

### A. UEFI GOP Integration
1. Compile `uefi_gop_a6000.c` and `mdss_mdp5_dsi.c` into your 32-bit EDK2 or U-Boot build.
2. Ensure physical address `0x86000000` is reserved as `EfiBootServicesData` or `EfiReservedMemoryType` in the UEFI memory map so the OS does not allocate application RAM over the display buffer.

### B. ACPI DSDT Injection
1. Compile the ACPI ASL source to binary AML using Intel iASL:
   ```bash
   iasl -tc dsdt_disp_a6000.asl
   ```
2. Link the generated `dsdt_disp_a6000.aml` into the UEFI ACPI Table Protocol so Windows Phone 8 enumerates `ACPI\QCOM240D` during PNP boot.

### C. Driver Package Injection
To inject `qcdx8916_a6000.inf` and `qcdx8916.sys` into your Windows Phone 8.1 / 10 Mobile image (FFU / WIM):
```cmd
dism /Image:C:\Mount\WP8 /Add-Driver /Driver:display_driver\qcdx8916_a6000.inf /ForceUnsigned
```
