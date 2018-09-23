#ifndef BUILD_LK
#include <linux/string.h>
#include <linux/string.h>
#include <linux/regulator/consumer.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/spinlock.h>
#include <linux/syscore_ops.h>
#include <linux/timer.h>
#include <linux/types.h>
#include <linux/sched.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/regulator/consumer.h>
#include <linux/gpio.h>
#include <linux/pinctrl/consumer.h>
#else
#include <string.h>
#endif

#include "lcm_drv.h"

#ifdef BUILD_LK
#include <platform/mt_i2c.h>
#include <platform/mt_gpio.h>
#include <platform/mt_pmic.h>
#elif defined(BUILD_UBOOT)
#else
#include <mach/mt_gpio.h>
#include <mach/mt_pm_ldo.h>
#endif

/* --------------------------------------------------------------------------- */
/* Local Constants */
/* --------------------------------------------------------------------------- */

#define FRAME_WIDTH  (1536)
#define FRAME_HEIGHT (2048)

#define GPIO_TPS65132_EN_P GPIO102
#define GPIO_TPS65132_EN_N GPIO103
#define GPIO_LCD_RST_EN GPIO127
#define GPIO_BL_EN GPIOEXT14

#define REGFLAG_DELAY             							0xFC
#define REGFLAG_END_OF_TABLE      							0xFD

/* --------------------------------------------------------------------------- */
/* Local Variables */
/* --------------------------------------------------------------------------- */

static LCM_UTIL_FUNCS lcm_util = {
	.set_reset_pin = NULL,
	.udelay = NULL,
	.mdelay = NULL,
};

#define SET_RESET_PIN(v)    (lcm_util.set_reset_pin((v)))

#define UDELAY(n) (lcm_util.udelay(n))
#define MDELAY(n) (lcm_util.mdelay(n))


/* --------------------------------------------------------------------------- */
/* Local Functions */
/* --------------------------------------------------------------------------- */

#define dsi_set_cmdq_V2(cmd, count, ppara, force_update) lcm_util.dsi_set_cmdq_V2(cmd, count, ppara, force_update)
#define dsi_set_cmdq(pdata, queue_size, force_update)    lcm_util.dsi_set_cmdq(pdata, queue_size, force_update)
#define wrtie_cmd(cmd)                                   lcm_util.dsi_write_cmd(cmd)
#define write_regs(addr, pdata, byte_nums)               lcm_util.dsi_write_regs(addr, pdata, byte_nums)
#define read_reg(cmd)                         lcm_util.dsi_dcs_read_lcm_reg(cmd)
#define read_reg_v2(cmd, buffer, buffer_size) lcm_util.dsi_dcs_read_lcm_reg_v2(cmd, buffer, buffer_size)

#define   LCM_DSI_CMD_MODE                                  0

/* #define  PUSH_TABLET_USING */
#define REGFLAG_DELAY                                       0xFFFC
#define REGFLAG_END_OF_TABLE                                0xFFFD

struct LCM_setting_table {
    unsigned char cmd;
    unsigned char count;
    unsigned char para_list[64];
};

static struct LCM_setting_table lcm_initialization_setting[] = {

    {0x36, 1, {0x40}},
    {0x3A, 1, {0x70}},
    {0xB0, 1, {0x04}},
    {0xD6, 1, {0x01}},

    // PWM and CABC setting
    {0xB8, 6, {0x07, 0x90, 0x1E, 0x10, 0x1E, 0x32}},
    {0xB9, 6, {0x07, 0x82, 0x3C, 0x10, 0x3C, 0x87}},
    {0xBA, 6, {0x07, 0x78, 0x64, 0x10, 0x64, 0xB4}},
    {0xCE, 23, {0x75, 0x40, 0x43, 0x49, 0x55, 0x62, 0x71, 0x82, 0x94, 0xA8,
                     0xB9, 0xCB, 0xDB, 0xE9, 0xF5, 0xFC, 0xFF, 0xbb, 0x00, 0x04,
                     0x04, 0x44, 0x20}},
    {0x51, 1, {0xFF}},
    {0x53, 1, {0x24}},
    {0x55, 1, {0x00}},

/*
    // BIST mode
    {0xB0, 1, {0x04}},
    {0xD6, 1, {0x01}},
    {0xDE, 4, {0x01, 0x3F, 0xFF, 0x10}},
    {0x51, 1, {0xFF}},
    {0x53, 1, {0x24}},
    {0x55, 1, {0x03}},
    {0x11, 0, {}}, // sleep out
    {REGFLAG_DELAY, 120, {}},
    {0x29, 0, {}}, // display on
    {REGFLAG_DELAY, 120, {}},
*/

    {0x29, 0, {}}, // display on
    {REGFLAG_DELAY, 120, {}},
    {0x11, 0, {}}, // sleep out
    {REGFLAG_DELAY, 120, {}},

};

static struct LCM_setting_table lcm_sleep_out_setting[] = {
//Sleep Out
    {0x11, 1, {0x00}},
    {REGFLAG_DELAY, 120, {}},

// Display ON
    {0x29, 1, {0x00}},
    {REGFLAG_DELAY, 20, {}},
    {REGFLAG_END_OF_TABLE, 0x00, {}}
};

static struct LCM_setting_table lcm_suspend_setting[] = {
    {0x28, 0, {}},
    {0x10, 0, {}},
    {REGFLAG_DELAY, 120, {}}
};

static void push_table(struct LCM_setting_table *table, unsigned int count,
                       unsigned char force_update)
{
    unsigned int i;

    for (i = 0; i < count; i++) {
        unsigned cmd;
        cmd = table[i].cmd;

        dprintf(0, "%s: count=%d\n", __func__, i);

        switch (cmd) {

        case REGFLAG_DELAY:
            if (table[i].count <= 10)
                MDELAY(table[i].count);
            else
                MDELAY(table[i].count);
            break;

        case REGFLAG_END_OF_TABLE:
            break;

        default:
            dsi_set_cmdq_V2(cmd, table[i].count, table[i].para_list,
                            force_update);
        }
    }
}

// ---------------------------------------------------------------------------
//  LCM Driver Implementations
// ---------------------------------------------------------------------------

static void lcm_set_util_funcs(const LCM_UTIL_FUNCS * util)
{
    memcpy(&lcm_util, util, sizeof(LCM_UTIL_FUNCS));
}

static void lcm_set_gpio_output(unsigned int GPIO, unsigned int output)
{
	if (GPIO == 0xFFFFFFFF) {
#ifdef BUILD_LK
		dprintf(INFO, "[LK/LCM] GPIO_TPS65132_EN_P = 0x%x\n", GPIO_TPS65132_EN_P);
		dprintf(INFO, "[LK/LCM] GPIO_TPS65132_EN_N = 0x%x\n", GPIO_TPS65132_EN_N);
		dprintf(INFO, "[LK/LCM] GPIO_BL_EN = 0x%x\n", GPIO_BL_EN);
		//dprintf(INFO, "[LK/LCM] GPIO_LCM_LED_EN =  0x%x\n", GPIO_LCM_LED_EN);
#elif (defined BUILD_UBOOT)
#else
#endif
		return;
	}

#ifdef BUILD_LK
	mt_set_gpio_mode(GPIO, GPIO_MODE_00);
	mt_set_gpio_dir(GPIO, GPIO_DIR_OUT);
	mt_set_gpio_out(GPIO, (output > 0) ? GPIO_OUT_ONE : GPIO_OUT_ZERO);
#else
	gpio_set_value(GPIO, (output > 0) ? GPIO_OUT_ONE : GPIO_OUT_ZERO);
#endif
}

static void lcm_get_params(LCM_PARAMS * params)
{
    memset(params, 0, sizeof(LCM_PARAMS));

    params->type = LCM_TYPE_DSI;

    params->width = FRAME_WIDTH;
    params->height = FRAME_HEIGHT;
    params->lcm_if = LCM_INTERFACE_DSI_DUAL;
    params->lcm_cmd_if = LCM_INTERFACE_DSI0;
    params->dsi.cont_clock = 0;
    params->dsi.clk_lp_per_line_enable = 1;

#if (LCM_DSI_CMD_MODE)
    params->dsi.mode = CMD_MODE;
#else
    params->dsi.mode = BURST_VDO_MODE;
#endif

// DSI
/* Command mode setting */
    params->dsi.LANE_NUM = LCM_FOUR_LANE;
//The following defined the fomat for data coming from LCD engine.
    params->dsi.data_format.color_order = LCM_COLOR_ORDER_RGB;
    params->dsi.data_format.trans_seq = LCM_DSI_TRANS_SEQ_MSB_FIRST;
    params->dsi.data_format.padding = LCM_DSI_PADDING_ON_LSB;
    params->dsi.data_format.format = LCM_DSI_FORMAT_RGB888;

// Highly depends on LCD driver capability.
    params->dsi.packet_size = 256;
    params->dsi.ssc_disable = 1;
//video mode timing

    params->dsi.PS = LCM_PACKED_PS_24BIT_RGB888;

    params->dsi.vertical_sync_active = 2;
    params->dsi.vertical_backporch = 6; //4;
    params->dsi.vertical_frontporch = 30;
    params->dsi.vertical_active_line = FRAME_HEIGHT;

    params->dsi.horizontal_sync_active = 8;
    params->dsi.horizontal_backporch = 100;
    params->dsi.horizontal_frontporch = 100;
    params->dsi.horizontal_active_pixel = FRAME_WIDTH;
    params->dsi.PLL_CLOCK = 430; //this value must be in MTK suggested table
    params->dsi.ufoe_enable = 1;
    params->dsi.ufoe_params.lr_mode_en = 1;

}

#define TPS65132_SLAVE_ADDR_WRITE  0x7C
static struct mt_i2c_t TPS65132_i2c;

static int TPS65132_write_byte(kal_uint8 addr, kal_uint8 value)
{
    kal_uint32 ret_code = I2C_OK;
    kal_uint8 write_data[2];
    kal_uint16 len;

    write_data[0] = addr;
    write_data[1] = value;

    TPS65132_i2c.id = 4; // I2C4

/* Since i2c will left shift 1 bit, we need to set FAN5405 I2C address to >>1 */
    TPS65132_i2c.addr = (TPS65132_SLAVE_ADDR_WRITE >> 1);
    TPS65132_i2c.mode = ST_MODE;
    TPS65132_i2c.speed = 100;
    len = 2;

    ret_code = i2c_write(&TPS65132_i2c, write_data, len);
    return ret_code;
}

#define RT4532_SLAVE_ADDR  0x22
static struct mt_i2c_t RT4532_i2c;

static int RT4532_write_byte(kal_uint8 addr, kal_uint8 value)
{
    kal_uint32 ret_code = I2C_OK;
    kal_uint8 write_data[2];
    kal_uint16 len;

    write_data[0] = addr;
    write_data[1] = value;

    RT4532_i2c.id = 2; // I2C2

    RT4532_i2c.addr = RT4532_SLAVE_ADDR;
    RT4532_i2c.mode = ST_MODE;
    RT4532_i2c.speed = 100;
    len = 2;

    ret_code = i2c_write(&RT4532_i2c, write_data, len);
    return ret_code;
}

static void lcm_init_power(void)
{
    unsigned char cmd = 0x0;
    unsigned char data = 0xFF;
    int ret = 0;
    cmd = 0x00;
    data = 0x0e;

    dprintf(0, "%s+, LK\n", __func__);

    /* set reset pin to 0 before +-5.4V is up*/
    lcm_set_gpio_output(GPIO_LCD_RST_EN, GPIO_OUT_ZERO);
    MDELAY(50);

    /* 1.8v */
    upmu_set_rg_vgp4_vosel(3);
    upmu_set_rg_vgp4_sw_en(1);

    MDELAY(10);

    lcm_set_gpio_output(GPIO_TPS65132_EN_P, GPIO_OUT_ONE);
    lcm_set_gpio_output(GPIO_TPS65132_EN_N, GPIO_OUT_ONE);

    ret = TPS65132_write_byte(cmd, data);
    if (ret)
        dprintf(0,
                "[LK] tps6132 cmd=0x%x, data=0x%x --i2c write error--\n",
                cmd, data);
    else
        dprintf(0,
                "[LK] tps6132 cmd=0x%x, data=0x%x --i2c write success--\n",
                cmd, data);

    MDELAY(20);

    cmd = 0x01;
    data = 0x0e;

    ret = TPS65132_write_byte(cmd, data);
    if (ret)
        dprintf(0,
                "[LK] tps6132 cmd=0x%x, data=0x%x --i2c write error--\n",
                cmd, data);
    else
        dprintf(0,
                "[LK] tps6132 cmd=0x%x, data=0x%x --i2c write success--\n",
                cmd, data);

    dprintf(0, "%s-, LK\n", __func__);
}

static void lcm_resume_power(void)
{
    unsigned char cmd = 0x0;
    unsigned char data = 0xFF;
    int ret = 0;
    cmd = 0x00;
    data = 0x0e;

    printf("%s+, LK\n", __func__);

    lcm_set_gpio_output(GPIO_TPS65132_EN_P, GPIO_OUT_ONE);
    lcm_set_gpio_output(GPIO_TPS65132_EN_N, GPIO_OUT_ONE);

    ret = TPS65132_write_byte(cmd, data);
    if (ret)
        dprintf(0,
                "[LK]r63319----tps6132----cmd=%0x--i2c write error----\n",
                cmd);
    else
        dprintf(0,
                "[LK]r63319----tps6132----cmd=%0x--i2c write success----\n",
                cmd);

    MDELAY(20);

    cmd = 0x01;
    data = 0x0e;

    ret = TPS65132_write_byte(cmd, data);
    if (ret)
        dprintf(0,
                "[LK]r63319----tps6132----cmd=%0x--i2c write error----\n",
                cmd);
    else
        dprintf(0,
                "[LK]r63319----tps6132----cmd=%0x--i2c write success----\n",
                cmd);

    printf("%s-, LK\n", __func__);
}

static void lcm_suspend_power(void)
{
    printf("%s+, LK\n", __func__);

    lcm_set_gpio_output(GPIO_LCD_RST_EN, GPIO_OUT_ZERO);
    MDELAY(50);

    lcm_set_gpio_output(GPIO_TPS65132_EN_P, GPIO_OUT_ZERO);
    lcm_set_gpio_output(GPIO_TPS65132_EN_N, GPIO_OUT_ZERO);

    printf("%s-, LK\n", __func__);
}

static void lcm_init(struct platform_device *pdev)
{
    unsigned char cmd, data;
    int ret;

    dprintf(0, "%s+, LK\n", __func__);

    /* set reset pin to 1 */
    MDELAY(10);
    lcm_set_gpio_output(GPIO_LCD_RST_EN, GPIO_OUT_ONE);
    MDELAY(10);

    push_table(lcm_initialization_setting,
               sizeof(lcm_initialization_setting) /
               sizeof(struct LCM_setting_table), 1);

    lcm_set_gpio_output(GPIO_BL_EN, GPIO_OUT_ONE);

    /* Set backlight IC feedback current to 40mA */
    cmd = 0x02;
    data = 0xCF;

    ret = RT4532_write_byte(cmd, data);
    if (ret)
        dprintf(0,
                "[LK]rt4532----cmd=%0x--i2c write error----\n",
                cmd);
    else
        dprintf(0,
                "[LK]rt4532----cmd=%0x--i2c write success----\n",
                cmd);

    dprintf(0, "%s-, LK\n", __func__);
}


static void lcm_suspend(void)
{
    printf("%s+, LK\n", __func__);

    lcm_set_gpio_output(GPIO_BL_EN, GPIO_OUT_ZERO);
    MDELAY(10);

    push_table(lcm_suspend_setting,
               sizeof(lcm_suspend_setting) /
               sizeof(struct LCM_setting_table), 1);

    lcm_set_gpio_output(GPIO_LCD_RST_EN, GPIO_OUT_ZERO);
    MDELAY(10);

    printf("%s-, LK\n", __func__);
}

static void lcm_resume(void)
{
    printf("%s+, LK\n", __func__);

    lcm_set_gpio_output(GPIO_LCD_RST_EN, GPIO_OUT_ONE);
    MDELAY(10);
    lcm_set_gpio_output(GPIO_LCD_RST_EN, GPIO_OUT_ZERO);
    MDELAY(10);
    lcm_set_gpio_output(GPIO_LCD_RST_EN, GPIO_OUT_ONE);
    MDELAY(10);

    push_table(lcm_sleep_out_setting,
               sizeof(lcm_sleep_out_setting) /
               sizeof(struct LCM_setting_table), 1);

    lcm_set_gpio_output(GPIO_BL_EN, GPIO_OUT_ONE);

    printf("%s-, LK\n", __func__);
}

LCM_DRIVER r63319_qxga_dsi_vdo_truly_lcm_drv = {
    .name = "r63319_qxga_dsi_vdo_truly",
    .set_util_funcs = lcm_set_util_funcs,
    .get_params = lcm_get_params,
    .init = lcm_init,
    .suspend = lcm_suspend,
    .resume = lcm_resume,
	/*.compare_id     = lcm_compare_id, */
    .init_power = lcm_init_power,
    .resume_power = lcm_resume_power,
    .suspend_power = lcm_suspend_power,
};
