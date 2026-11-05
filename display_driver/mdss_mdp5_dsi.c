/**
 * @file mdss_mdp5_dsi.c
 * @brief MDSS MDP5 and DSI Initialization for Lenovo A6000
 */

#include "mdss_mdp5_dsi.h"
#include "a6000_panel_ili9881.h"

static inline void reg_write(uint32_t addr, uint32_t val)
{
    *(volatile uint32_t *)(uintptr_t)addr = val;
}

static inline uint32_t reg_read(uint32_t addr)
{
    return *(volatile uint32_t *)(uintptr_t)addr;
}

static void delay_ms(uint32_t ms)
{
    /* Calibration loop for Cortex-A53 running @ 1.2GHz */
    volatile uint32_t count = ms * 120000;
    while (count--) {
        __asm__ volatile ("nop");
    }
}

/* GPIO register access (TLMM block on MSM8916: 0x01000000) */
#define TLMM_BASE                   0x01000000U
#define TLMM_GPIO_CFG(n)            (TLMM_BASE + 0x1000 * (n))
#define TLMM_GPIO_IN_OUT(n)         (TLMM_BASE + 0x1000 * (n) + 0x04)

static void gpio_set_dir_out(uint32_t gpio)
{
    reg_write(TLMM_GPIO_CFG(gpio), 0x00000000); /* Function 0 = GPIO, output */
}

static void gpio_set_val(uint32_t gpio, bool high)
{
    reg_write(TLMM_GPIO_IN_OUT(gpio), high ? 0x02 : 0x00);
}

void a6000_panel_power_on(void)
{
    /* Configure Reset and Power GPIOs */
    gpio_set_dir_out(A6000_GPIO_LCD_POWER_EN);
    gpio_set_dir_out(A6000_GPIO_LCD_RESET);

    /* Assert power enable */
    gpio_set_val(A6000_GPIO_LCD_POWER_EN, true);
    delay_ms(10);

    /* Hardware Reset sequence: High -> Low (10ms) -> High (20ms) */
    gpio_set_val(A6000_GPIO_LCD_RESET, true);
    delay_ms(5);
    gpio_set_val(A6000_GPIO_LCD_RESET, false);
    delay_ms(15);
    gpio_set_val(A6000_GPIO_LCD_RESET, true);
    delay_ms(25);
}

void dsi_controller_init(void)
{
    /* Enable DSI clocks and controller */
    reg_write(DSI0_BASE + DSI_CTRL, 0x00000000);
    delay_ms(2);

    /* 4 DSI Data Lanes enabled, Clock Lane enabled */
    reg_write(DSI0_BASE + DSI_LANE_CTRL, 0x0000001F);

    /* Video mode timing settings */
    reg_write(DSI0_BASE + DSI_VIDEO_MODE_ACTIVE_H, A6000_TIMING_H_ACTIVE);
    reg_write(DSI0_BASE + DSI_VIDEO_MODE_ACTIVE_V, A6000_TIMING_V_ACTIVE);
    reg_write(DSI0_BASE + DSI_VIDEO_MODE_TOTAL, 
              ((A6000_TIMING_V_ACTIVE + A6000_TIMING_V_FRONT_PORCH + A6000_TIMING_V_BACK_PORCH + A6000_TIMING_V_SYNC_WIDTH) << 16) |
              (A6000_TIMING_H_ACTIVE + A6000_TIMING_H_FRONT_PORCH + A6000_TIMING_H_BACK_PORCH + A6000_TIMING_H_SYNC_WIDTH));

    /* Enable DSI Video Mode and DMA */
    reg_write(DSI0_BASE + DSI_CTRL, 0x00000003);
}

void dsi_send_dcs_cmd(uint8_t cmd, const uint8_t *payload, uint8_t len)
{
    /* Wait for DMA/Command FIFO idle */
    while ((reg_read(DSI0_BASE + DSI_STATUS) & 0x01) == 0) {
        /* wait */
    }

    uint32_t fifo_data = cmd;
    if (len >= 1) fifo_data |= ((uint32_t)payload[0] << 8);
    if (len >= 2) fifo_data |= ((uint32_t)payload[1] << 16);
    if (len >= 3) fifo_data |= ((uint32_t)payload[2] << 24);

    reg_write(DSI0_BASE + DSI_COMMAND_MODE_DMA_CTRL, fifo_data);
    reg_write(DSI0_BASE + DSI_COMMAND_MODE_MDP_CTRL, 0x00000001); /* Trigger send */
}

void a6000_panel_init_sequence(void)
{
    for (size_t i = 0; i < A6000_INIT_SEQ_COUNT; i++) {
        const dsi_cmd_packet_t *pkt = &a6000_ili9881_init_seq[i];
        dsi_send_dcs_cmd(pkt->cmd, pkt->payload, pkt->payload_len);
        if (pkt->delay_ms > 0) {
            delay_ms(pkt->delay_ms);
        }
    }

    /* Enable Backlight */
    gpio_set_dir_out(A6000_GPIO_BACKLIGHT_PWM);
    gpio_set_val(A6000_GPIO_BACKLIGHT_PWM, true);
}

void mdss_mdp5_init(uint32_t fb_phys_addr, uint32_t width, uint32_t height, uint32_t stride)
{
    /* 1. Configure Layer Mixer 0 Output Size (720x1280) */
    reg_write(MDP5_LM0_BASE + MDP5_LM_OUT_SIZE, (height << 16) | width);

    /* 2. Configure Source Pipe ViG0 */
    reg_write(MDP5_PIPE_VIG0_BASE + MDP5_PIPE_SRC_SIZE, (height << 16) | width);
    reg_write(MDP5_PIPE_VIG0_BASE + MDP5_PIPE_SRC_IMG_SIZE, (height << 16) | width);
    reg_write(MDP5_PIPE_VIG0_BASE + MDP5_PIPE_OUT_SIZE, (height << 16) | width);
    reg_write(MDP5_PIPE_VIG0_BASE + MDP5_PIPE_SRC0_ADDR, fb_phys_addr);
    reg_write(MDP5_PIPE_VIG0_BASE + MDP5_PIPE_SRC_YSTRIDE1, stride);
    reg_write(MDP5_PIPE_VIG0_BASE + MDP5_PIPE_SRC_FORMAT, 0x0002243F); /* 32-bit ARGB8888 */

    /* 3. Configure INTF1 Timing Engine for 720x1280 @ 60Hz */
    reg_write(MDP5_INTF1_BASE + MDP5_INTF_HSYNC_CTL, (A6000_TIMING_H_SYNC_WIDTH << 16) | (width + A6000_TIMING_H_FRONT_PORCH + A6000_TIMING_H_BACK_PORCH));
    reg_write(MDP5_INTF1_BASE + MDP5_INTF_VSYNC_PERIOD_F0, (height + A6000_TIMING_V_FRONT_PORCH + A6000_TIMING_V_BACK_PORCH + A6000_TIMING_V_SYNC_WIDTH));
    reg_write(MDP5_INTF1_BASE + MDP5_INTF_VSYNC_PULSE_WIDTH, A6000_TIMING_V_SYNC_WIDTH);
    reg_write(MDP5_INTF1_BASE + MDP5_INTF_TIMING_ENGINE_EN, 1);

    /* 4. Flush and trigger MDP5 CTL0 */
    reg_write(MDP5_CTL0_BASE + MDP5_CTL_FLUSH, 0x00000007);
    reg_write(MDP5_CTL0_BASE + MDP5_CTL_START, 0x00000001);
}
