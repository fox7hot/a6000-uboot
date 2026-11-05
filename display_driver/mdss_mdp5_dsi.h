/**
 * @file mdss_mdp5_dsi.h
 * @brief Qualcomm MDSS MDP5 and MIPI-DSI Register Map for MSM8916
 */

#ifndef MDSS_MDP5_DSI_H
#define MDSS_MDP5_DSI_H

#include <stdint.h>
#include <stdbool.h>

/* Register Base Addresses on Snapdragon 410 (MSM8916) */
#define MDSS_BASE                   0x01A00000U
#define MDP5_BASE                   0x01A01000U
#define DSI0_BASE                   0x01A98000U
#define DSI0_PHY_BASE               0x01A98500U

/* MDP5 Control Registers */
#define MDP5_CTL0_BASE              (MDP5_BASE + 0x1000)
#define MDP5_CTL_FLUSH              0x018
#define MDP5_CTL_START              0x000

/* MDP5 Layer Mixer 0 Registers */
#define MDP5_LM0_BASE               (MDP5_BASE + 0x3200)
#define MDP5_LM_OUT_SIZE            0x004
#define MDP5_LM_BORDER_COLOR_0      0x008

/* MDP5 Pipe ViG0 / RGB0 (Source Surface) Registers */
#define MDP5_PIPE_VIG0_BASE         (MDP5_BASE + 0x5000)
#define MDP5_PIPE_SRC_SIZE          0x000
#define MDP5_PIPE_SRC_IMG_SIZE      0x004
#define MDP5_PIPE_SRC_XY            0x008
#define MDP5_PIPE_OUT_SIZE          0x00C
#define MDP5_PIPE_OUT_XY            0x010
#define MDP5_PIPE_SRC0_ADDR         0x014
#define MDP5_PIPE_SRC_YSTRIDE1      0x024
#define MDP5_PIPE_SRC_FORMAT        0x030

/* MDP5 INTF1 (MIPI-DSI Interface) Registers */
#define MDP5_INTF1_BASE             (MDP5_BASE + 0x12700)
#define MDP5_INTF_TIMING_ENGINE_EN  0x000
#define MDP5_INTF_HSYNC_CTL         0x008
#define MDP5_INTF_VSYNC_PERIOD_F0   0x00C
#define MDP5_INTF_VSYNC_PULSE_WIDTH 0x010
#define MDP5_INTF_ACTIVE_HCTL       0x03C
#define MDP5_INTF_ACTIVE_V_START_F0 0x040
#define MDP5_INTF_ACTIVE_V_END_F0   0x044
#define MDP5_INTF_FRAME_LINE_COUNT  0x048

/* DSI Controller Registers */
#define DSI_CTRL                    0x000
#define DSI_STATUS                  0x004
#define DSI_VIDEO_MODE_ACTIVE_H     0x020
#define DSI_VIDEO_MODE_ACTIVE_V     0x024
#define DSI_VIDEO_MODE_TOTAL        0x028
#define DSI_VIDEO_MODE_HSYNC        0x02C
#define DSI_VIDEO_MODE_VSYNC        0x030
#define DSI_COMMAND_MODE_DMA_CTRL   0x038
#define DSI_COMMAND_MODE_MDP_CTRL   0x03C
#define DSI_MISR_CMD_CTRL           0x040
#define DSI_LANE_CTRL               0x0A8
#define DSI_CLK_CTRL                0x118

/* API Prototypes */
void mdss_mdp5_init(uint32_t fb_phys_addr, uint32_t width, uint32_t height, uint32_t stride);
void dsi_controller_init(void);
void dsi_send_dcs_cmd(uint8_t cmd, const uint8_t *payload, uint8_t len);
void a6000_panel_power_on(void);
void a6000_panel_init_sequence(void);

#endif /* MDSS_MDP5_DSI_H */
