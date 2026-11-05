# Windows Phone 8 on Lenovo A6000 (Snapdragon 410 / MSM8916) Master Porting Guide

This document provides the complete architectural breakdown, implementation details, and step-by-step instructions to boot **Windows Phone 8 / 8.1 / 10 Mobile** on the **Lenovo A6000** (Wingtech WT86518 / MSM8916), including resolving the 32-bit UEFI incompatibility, implementing the EL2 SMC Sandbox for Android/Linux TrustZone, and driving the device's MIPI-DSI LCD panel.

---

## 1. System Architecture & Boot Flow

```
┌────────────────────────────────────────────────────────────┐
│                    Qualcomm Primary Boot                   │
│             PBL ──► SBL1 ──► Android QSEE / TZ             │
└─────────────────────────────┬──────────────────────────────┘
                              ▼
┌────────────────────────────────────────────────────────────┐
│                      Stage 2 Bootloader                    │
│                 lk2nd (Little Kernel Stage 2)              │
└─────────────────────────────┬──────────────────────────────┘
                              ▼
┌────────────────────────────────────────────────────────────┐
│                 U-Boot (32-bit ARMv7 Target)               │
│          Exposes 32-bit UEFI Boot Services & Shell         │
└─────────────────────────────┬──────────────────────────────┘
                              ▼
┌────────────────────────────────────────────────────────────┐
│         EL2 SMC Sandbox & Translation Layer (Hypervisor)   │
│         - Configures HCR_EL2.TSC = 1 (Trap SMC calls)      │
│         - Emulates WinSecApp, fTPM 2.0, & OEM Fuses        │
│         - Translates Windows SCM (PEP/PWR) to Android SCM  │
│         - Bridges Secondary CPU Bringup via ARM PSCI       │
└─────────────────────────────┬──────────────────────────────┘
                              ▼
┌────────────────────────────────────────────────────────────┐
│             Windows Phone 8 UEFI Boot Manager              │
│                 EFI\Boot\bootarm.efi / bootmgr.efi         │
└─────────────────────────────┬──────────────────────────────┘
                              ▼
┌────────────────────────────────────────────────────────────┐
│                Windows OS Loader (winload.efi)             │
│        Renders Boot Logo via UEFI GOP (0x86000000)         │
└─────────────────────────────┬──────────────────────────────┘
                              ▼ (ExitBootServices)
┌────────────────────────────────────────────────────────────┐
│            Windows Phone NT Kernel (ntoskrnl.exe)          │
│   ├── HAL / PEP: qcpep.sys (Power/Clocks via SMC Sandbox)  │
│   ├── Security:  qctz.sys (Intercepted by WinSecApp Emu)   │
│   └── Display:   qcdx8916.sys / MSBDD / a6000disp.sys     │
│                  - Drives 720x1280 ILI9881 IPS Panel       │
│                  - Enumerated via ACPI DSDT Device(DISP)   │
└────────────────────────────────────────────────────────────┘
```

---

## 2. Problem 1: UEFI Shell "Not Compatible" Error

### Cause
- Your device was running a 64-bit (**AArch64**) UEFI shell.
- Windows Phone 8 is an **ARMv7-A 32-bit Thumb-2** operating system (`IMAGE_FILE_MACHINE_ARMNT = 0x01C4`).
- An AArch64 UEFI `LoadImage` service explicitly checks the PE machine header and aborts with `EFI_UNSUPPORTED` on 32-bit ARM binaries.
- The Cortex-A53 core in 64-bit state cannot execute 32-bit Thumb-2 code without an architectural execution state switch (`ERET` with `HCR_EL2.RW = 0`).

### Solution
1. **32-bit U-Boot / 32-bit EDK2**:
   - Stock LK (`aboot`) and `lk2nd` run in 32-bit ARM mode.
   - Compiling U-Boot for 32-bit (`ARCH=arm CROSS_COMPILE=arm-none-eabi- make qcom_defconfig`) gives a native 32-bit UEFI Shell where `bootarm.efi` loads without error.
2. **Architectural Drop Stub**:
   - If chainloading from an AArch64 environment, `uefi_boot_fix/aarch64_to_aarch32_drop.S` switches the CPU execution state to AArch32 EL1 (SVC mode) and branches to `bootarm.efi`.

---

## 3. Problem 2: SMC Sandbox & Protocol Translation

### The Issue
- Windows Phone Qualcomm drivers (`qcpep.sys`, `qccrypt.sys`, `qctz.sys`) issue Secure Monitor Calls (`SMC`) expecting Qualcomm's Windows TrustZone firmware (`winsecapp.mbn`).
- The Lenovo A6000 contains Android Qualcomm TrustZone (`tz.mbn`).
- When WP8 issues Windows Phone SMCs to Android TZ:
  - Android TZ rejects unknown service IDs (`SMC_UNK = -1`).
  - Missing `WINSECAPP` triggers NT BugCheck `0x00000100` (`SECURE_BOOT_VIOLATION`).
  - Windows PEP power calls fail, locking the CPU.

### The Sandbox Implementation (`smc_sandbox/`)
- Runs at EL2 (Hypervisor).
- Sets `HCR_EL2.TSC = 1`: Hardware intercepts all `SMC` instructions executed by Windows Phone at EL1.
- **Trap Dispatcher (`smc_trap_handler.c`)**:
  - Traps exceptions with syndrome `EC = 0x13` (AArch32 SMC).
  - Categorizes call by Function ID / Service ID.
- **WinSecApp Emulator (`winsecapp_emulator.c`)**:
  - Fakes `winsecapp` presence and version handshake.
  - Returns simulated TPM 2.0 PCR capabilities and validation nonces.
  - Returns unlocked / production OEM fuse state.
- **SCM Translator (`scm_translator.c`)**:
  - Translates Windows Phone PEP power collapse and CPR calls into Android SCM v1 / v2 packet format.
  - Issues real `smc #0` to Android TZ from EL2 and returns mapped status codes.
- **PSCI Bridge (`psci_bridge.c`)**:
  - Translates WP8 secondary core startup into standard ARM PSCI `CPU_ON` calls (`0x84000003`) to wake up Cortex-A53 cores 1, 2, and 3.

---

## 4. Problem 3: Lenovo A6000 Display Driver for WP8

### The Issue
- Windows Phone 8 requires a display output mechanism to render the lockscreen, tiles, and applications.
- Lenovo A6000 uses a 5.0" 720x1280 IPS panel (Ilitek ILI9881 / Himax HX8394) connected via Qualcomm MDSS 1.4 / DSI.

### The Implementation (`display_driver/`)
1. **UEFI GOP Driver (`uefi_gop_a6000.c` & `mdss_mdp5_dsi.c`)**:
   - Programs PM8916 regulators and toggles LCD Reset (GPIO 25).
   - Initializes DSI Controller 0 with 4 data lanes @ 420-450 MHz.
   - Sends DCS command packet table to configure ILI9881 internal timing, gamma, and exits sleep (`0x11` -> `0x29`).
   - Sets up MDP5 Layer Mixer 0, Source Pipe ViG0, and INTF1 for 720x1280 @ 60Hz.
   - Maps continuous linear framebuffer at physical address `0x86000000`.
   - Windows `bootmgr.efi`, `winload.efi`, and `basicdisplay.sys` bind to this GOP framebuffer immediately.
2. **ACPI DSDT Device Object (`dsdt_disp_a6000.asl`)**:
   - Declares `Device(DISP)` (`QCOM240D`) with MDSS, DSI0, DSI PHY memory maps, IRQ 104, GPIO 25 (Reset), and panel timing properties (`PANI`).
   - Supplies brightness methods `_BCM` and `_BQC` for backlight modulation.
3. **Qualcomm WDDM Miniport Driver Package (`qcdx8916_a6000.inf`)**:
   - Binds `qcdx8916.sys` to `ACPI\QCOM240D` on Windows Phone 8.1 / 10 Mobile.
4. **Kernel-Mode Display-Only Driver (`a6000_disp_kmdod.c`)**:
   - Fallback WDDM 1.3 KMDOD driver ensuring reliable 2D framebuffer rendering in kernel mode.

---

## 5. File Inventory

| Path | Purpose |
| :--- | :--- |
| `uefi_boot_fix/aarch64_to_aarch32_drop.S` | Assembly stub switching CPU state from AArch64 EL2 to AArch32 EL1 (SVC mode) |
| `uefi_boot_fix/arm32_uefi_loader.c` | 32-bit UEFI chainloader validating PE ARMNT images and calling `bootarm.efi` |
| `uefi_boot_fix/README_UEFI_32BIT_FIX.md` | Guide on compiling 32-bit U-Boot and fixing the architecture mismatch |
| `smc_sandbox/smc_sandbox.h` | Core data structures and SMC service ID definitions |
| `smc_sandbox/smc_trap_handler.c` | EL2 trap dispatcher and register virtualization |
| `smc_sandbox/winsecapp_emulator.c` | Emulation of Windows Phone WinSecApp, fTPM, and OEM fuses |
| `smc_sandbox/scm_translator.c` | Translates Windows SCM PEP/Power calls to Android QSEE SCM |
| `smc_sandbox/psci_bridge.c` | ARM PSCI CPU bringup and power bridge |
| `smc_sandbox/el2_entry.S` | Assembly EL2 exception vector table and context switcher |
| `smc_sandbox/Makefile` & `smc_sandbox.ld` | Build script and memory linker script for EL2 hypervisor payload |
| `display_driver/a6000_panel_ili9881.h` | Panel timings, GPIO pins, and ILI9881 DCS command sequences |
| `display_driver/mdss_mdp5_dsi.h` & `.c` | Low-level Snapdragon 410 MDSS MDP5 and DSI initialization engine |
| `display_driver/uefi_gop_a6000.c` | UEFI Graphics Output Protocol driver for linear framebuffer |
| `display_driver/dsdt_disp_a6000.asl` | ACPI DSDT Device(DISP) definition for Qualcomm display driver |
| `display_driver/qcdx8916_a6000.inf` | Driver INF installation package for Windows Phone 8 |
| `display_driver/a6000_disp_kmdod.c` | WDDM 1.3 Kernel-Mode Display-Only Driver source |
| `tools/verify_wp8_image.py` | Python diagnostic tool to inspect PE/COFF architecture of WP8 binaries |
| `tools/dsi_cmd_builder.py` | Tool to encode DCS panel command sequences into ASL and C formats |
