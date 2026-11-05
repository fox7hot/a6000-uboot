/**
 * @file dsdt_disp_a6000.asl
 * @brief ACPI DSDT Definition Block for Lenovo A6000 (MSM8916) Display Subsystem
 * 
 * Used by Qualcomm WDDM Display Miniport Driver (qcdx8916.sys) on Windows Phone 8 / 10 Mobile.
 */

DefinitionBlock ("dsdt_disp_a6000.aml", "SSDT", 2, "LENOVO", "A6000DIS", 0x00001000)
{
    Scope (\_SB)
    {
        Device (DISP)
        {
            Name (_HID, "QCOM240D")
            Name (_CID, "ACPI\QCOM240D")
            Name (_UID, 0x00)
            Name (_DEP, Package() { \_SB.PEP0, \_SB.GIO0 })

            Method (_STA, 0, NotSerialized)
            {
                Return (0x0F) /* Device present, enabled, functioning */
            }

            Name (_CRS, ResourceTemplate ()
            {
                /* MDSS Base: 0x01A00000 */
                Memory32Fixed (ReadWrite, 0x01A00000, 0x00030000)

                /* DSI0 Base: 0x01A98000 */
                Memory32Fixed (ReadWrite, 0x01A98000, 0x00001000)

                /* DSI0 PHY Base: 0x01A98500 */
                Memory32Fixed (ReadWrite, 0x01A98500, 0x00001000)

                /* MDSS Core Interrupt (GIC SPI 72 + 32 = 104) */
                Interrupt (ResourceConsumer, Level, ActiveHigh, Shared)
                {
                    104
                }

                /* LCD Reset GPIO: GPIO 25 */
                GpioIo (Exclusive, PullNone, 0, 0, IoRestrictionOutputOnly, "\\_SB.GIO0", 0, ResourceConsumer)
                {
                    25
                }

                /* LCD Power Enable GPIO: GPIO 12 */
                GpioIo (Exclusive, PullNone, 0, 0, IoRestrictionOutputOnly, "\\_SB.GIO0", 0, ResourceConsumer)
                {
                    12
                }
            })

            /* Enumerate child display devices (0x0100 = Built-in LCD Panel) */
            Method (_DOD, 0, NotSerialized)
            {
                Return (Package () { 0x0100 })
            }

            Device (LCD0)
            {
                Name (_ADR, 0x0100)

                /* Supported Brightness Levels (0% to 100%) */
                Name (_BCL, Package ()
                {
                    100, /* Full AC power level */
                    50,  /* DC power level */
                    0, 5, 10, 15, 20, 25, 30, 35, 40, 45, 50, 55, 60, 65, 70, 75, 80, 85, 90, 95, 100
                })

                Name (CBRT, 80) /* Current Brightness level cache */

                /* Set Brightness Level */
                Method (_BCM, 1, NotSerialized)
                {
                    Store (Arg0, CBRT)
                    /* Program PM8916 WLED or PWM Backlight */
                }

                /* Query Current Brightness Level */
                Method (_BQC, 0, NotSerialized)
                {
                    Return (CBRT)
                }

                /* Query Device Current Status (Bit 0: Connector exists, Bit 1: Active) */
                Method (_DCS, 0, NotSerialized)
                {
                    Return (0x1F)
                }

                /* Query Graphics Status */
                Method (_DGS, 0, NotSerialized)
                {
                    Return (0x01)
                }

                /* Panel Timing and Identification Object */
                Name (PANI, Buffer ()
                {
                    /* Width: 720, Height: 1280, Refresh: 60Hz */
                    0xD0, 0x02, /* 720 */
                    0x00, 0x05, /* 1280 */
                    0x3C, 0x00, /* 60Hz */
                    0x04,       /* 4 DSI Data Lanes */
                    0x18,       /* 24 BPP */
                    0x00        /* Video Mode */
                })
            }
        }
    }
}
