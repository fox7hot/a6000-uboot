/**
 * @file a6000_panel_ili9881.h
 * @brief Lenovo A6000 (MSM8916) MIPI-DSI Panel Configuration & Commands
 * 
 * Target Panel: Ilitek ILI9881 / ILI9881C 720x1280 IPS LCD (Qimei/Wingtech)
 * Interface: MIPI DSI 4 Data Lanes + 1 Clock Lane, Video Mode
 */

#ifndef A6000_PANEL_ILI9881_H
#define A6000_PANEL_ILI9881_H

#include <stdint.h>

/* Display Dimensions */
#define A6000_PANEL_WIDTH           720
#define A6000_PANEL_HEIGHT          1280
#define A6000_PANEL_BPP             32      /* 32-bit ARGB8888 in Framebuffer */
#define A6000_PANEL_STRIDE          (A6000_PANEL_WIDTH * 4)
#define A6000_PANEL_FB_SIZE         (A6000_PANEL_STRIDE * A6000_PANEL_HEIGHT)

/* Default Framebuffer Memory Carveout */
#define A6000_DEFAULT_FB_PHYS_ADDR  0x86000000U

/* Video Timings (720x1280 @ 60Hz) */
#define A6000_TIMING_H_ACTIVE       720
#define A6000_TIMING_H_FRONT_PORCH  100
#define A6000_TIMING_H_BACK_PORCH   40
#define A6000_TIMING_H_SYNC_WIDTH   20

#define A6000_TIMING_V_ACTIVE       1280
#define A6000_TIMING_V_FRONT_PORCH  20
#define A6000_TIMING_V_BACK_PORCH   14
#define A6000_TIMING_V_SYNC_WIDTH   4

#define A6000_REFRESH_RATE_HZ       60
#define A6000_PIXEL_CLOCK_KHZ       69600   /* ~69.6 MHz */

/* Hardware GPIOs for Lenovo A6000 (Wingtech WT86518) */
#define A6000_GPIO_LCD_RESET        25      /* Active Low reset */
#define A6000_GPIO_LCD_POWER_EN     12      /* Panel LDO / Boost enable */
#define A6000_GPIO_BACKLIGHT_PWM    86      /* Backlight PWM */

/* MIPI DSI DCS Commands */
#define DCS_CMD_SOFT_RESET          0x01
#define DCS_CMD_EXIT_SLEEP_MODE     0x11
#define DCS_CMD_ENTER_SLEEP_MODE    0x10
#define DCS_CMD_SET_DISPLAY_OFF     0x28
#define DCS_CMD_SET_DISPLAY_ON      0x29
#define DCS_CMD_SET_PIXEL_FORMAT    0x3A
#define DCS_CMD_SET_PAGE_PAGE       0xFF

/* DSI Command Packet descriptor */
typedef struct {
    uint8_t  cmd;
    uint8_t  payload_len;
    uint8_t  payload[16];
    uint16_t delay_ms;
} dsi_cmd_packet_t;

/* ILI9881C Initialization Command Sequence */
static const dsi_cmd_packet_t a6000_ili9881_init_seq[] = {
    /* 1. Software Reset */
    { DCS_CMD_SOFT_RESET, 0, {0}, 15 },

    /* 2. Page 3 selection for internal analog control */
    { DCS_CMD_SET_PAGE_PAGE, 3, { 0x98, 0x81, 0x03 }, 5 },
    { 0x01, 1, { 0x00 }, 0 },
    { 0x02, 1, { 0x00 }, 0 },
    { 0x03, 1, { 0x73 }, 0 },
    { 0x04, 1, { 0x00 }, 0 },
    { 0x05, 1, { 0x00 }, 0 },
    { 0x06, 1, { 0x0A }, 0 },
    { 0x07, 1, { 0x00 }, 0 },
    { 0x08, 1, { 0x00 }, 0 },

    /* 3. Page 4 selection for timing configuration */
    { DCS_CMD_SET_PAGE_PAGE, 3, { 0x98, 0x81, 0x04 }, 5 },
    { 0x00, 1, { 0x80 }, 0 },
    { 0x01, 1, { 0x40 }, 0 },
    { 0x02, 1, { 0x00 }, 0 },
    { 0x03, 1, { 0x00 }, 0 },

    /* 4. Page 1 selection for Gamma correction */
    { DCS_CMD_SET_PAGE_PAGE, 3, { 0x98, 0x81, 0x01 }, 5 },
    { 0x22, 1, { 0x0A }, 0 },
    { 0x31, 1, { 0x00 }, 0 },
    { 0x40, 1, { 0x53 }, 0 },
    { 0x41, 1, { 0x33 }, 0 },
    { 0x42, 1, { 0x22 }, 0 },
    { 0x43, 1, { 0x02 }, 0 },

    /* 5. Return to Page 0 */
    { DCS_CMD_SET_PAGE_PAGE, 3, { 0x98, 0x81, 0x00 }, 5 },

    /* 6. Pixel Format: 24-bit RGB888 */
    { DCS_CMD_SET_PIXEL_FORMAT, 1, { 0x77 }, 10 },

    /* 7. Exit Sleep */
    { DCS_CMD_EXIT_SLEEP_MODE, 0, {0}, 120 },

    /* 8. Display ON */
    { DCS_CMD_SET_DISPLAY_ON, 0, {0}, 20 },
};

#define A6000_INIT_SEQ_COUNT (sizeof(a6000_ili9881_init_seq) / sizeof(a6000_ili9881_init_seq[0]))

#endif /* A6000_PANEL_ILI9881_H */
