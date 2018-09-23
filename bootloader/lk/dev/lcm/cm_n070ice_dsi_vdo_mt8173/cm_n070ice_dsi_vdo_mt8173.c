#ifndef BUILD_LK
#include <linux/string.h>
#else
#include <string.h>
#endif
#include "lcm_drv.h"
//#include "lcm_drv_mt8135.h"

#ifdef BUILD_LK
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

#if defined(MTK_ALPS_BOX_SUPPORT)
#define HDMI_SUB_PATH_LK 1
#else
#define HDMI_SUB_PATH_LK 0
#endif

#if HDMI_SUB_PATH_LK
#define FRAME_WIDTH  (1920)
#define FRAME_HEIGHT (1080)
#else
#define FRAME_WIDTH  (800)
#define FRAME_HEIGHT (1280)
#endif

#define LCM_ID_NT35590 (0x90)
/* TODO. This LCM ID is NT51012 not 35590. */

/* GPIO103        LCM_PMU_EN(1.8V) */
#ifdef GPIO_LCM_PWR_EN
#define GPIO_LCD_PWR_EN      GPIO_LCM_PWR_EN
#else
#define GPIO_LCD_PWR_EN      GPIO103	/* 0xFFFFFFFF */
#endif

/* GPIO102       LCDPANEL_EN */
#ifdef GPIO_LCM_PWR2_EN
#define GPIO_LCD_PWR2_EN      GPIO_LCM_PWR2_EN
#else
#define GPIO_LCD_PWR2_EN      GPIO102	/* 0xFFFFFFFF */
#endif

/* GPIO127       LCM_RST_SOC(3.7V) for panel */
#ifdef GPIO_LCM_RST
#define GPIO_LCD_RST_EN		GPIO_LCM_RST
#else
#define GPIO_LCD_RST_EN		GPIO127		/* 0xFFFFFFFF */
#endif

/* GPIO87       DISP_PWM0 for backlight panel */
#ifdef GPIO_LCM_BL_EN
#define GPIO_LCD_BL_EN		GPIO_LCM_BL_EN
#else
#define GPIO_LCD_BL_EN		0xFFFFFFFF
#endif

#ifdef GPIO_LCM_LED_EN
#define GPIO_LCD_LED_EN		GPIO_LCM_LED_EN
#else
#define GPIO_LCD_LED_EN		GPIOEXT14	/* 0xFFFFFFFF */
#endif

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
#define dsi_set_cmdq(pdata, queue_size, force_update)	 lcm_util.dsi_set_cmdq(pdata, queue_size, force_update)
#define wrtie_cmd(cmd)									 lcm_util.dsi_write_cmd(cmd)
#define write_regs(addr, pdata, byte_nums)				 lcm_util.dsi_write_regs(addr, pdata, byte_nums)
#define read_reg(cmd)						  lcm_util.dsi_dcs_read_lcm_reg(cmd)
#define read_reg_v2(cmd, buffer, buffer_size) lcm_util.dsi_dcs_read_lcm_reg_v2(cmd, buffer, buffer_size)

#define   LCM_DSI_CMD_MODE							0

static void init_lcm_registers(void)
{
	unsigned int data_array[16];

#ifdef BUILD_LK
	printf("%s, LK\n", __func__);
#endif

#if 1
	data_array[0] = 0x00011500;	/* software reset */
	dsi_set_cmdq(data_array, 1, 1);
	MDELAY(20);

	data_array[0] = 0x0bae1500;
	data_array[1] = 0xeaee1500;
	data_array[2] = 0x5fef1500;
	data_array[3] = 0x68f21500;
	data_array[4] = 0x00ee1500;
	data_array[5] = 0x00ef1500;
	dsi_set_cmdq(data_array, 6, 1);
#endif
}

/* --------------------------------------------------------------------------- */
/* LCM Driver Implementations */
/* --------------------------------------------------------------------------- */

static void lcm_set_util_funcs(const LCM_UTIL_FUNCS *util)
{
	memcpy(&lcm_util, util, sizeof(LCM_UTIL_FUNCS));
}

#ifndef BUILD_LK
static void lcm_request_gpio_control(void)
{		
	gpio_request(GPIO_LCD_PWR_EN, "GPIO_LCD_PWR_EN");	
	printk("[KE/LCM] GPIO_LCD_PWR_EN =   0x%x\n", GPIO_LCD_PWR_EN);
	
	gpio_request(GPIO_LCD_PWR2_EN, "GPIO_LCD_PWR2_EN");
	printk("[KE/LCM] GPIO_LCD_PWR2_EN =  0x%x\n", GPIO_LCD_PWR2_EN);
	
	gpio_request(GPIO_LCD_RST_EN, "GPIO_LCD_RST_EN");
	printk("[KE/LCM] GPIO_LCD_RST_EN =  0x%x\n", GPIO_LCD_RST_EN);
	
	gpio_request(GPIO_LCD_LED_EN, "GPIO_LCD_LED_EN");
	printk("[KE/LCM] GPIO_LCD_LED_EN =   0x%x\n", GPIO_LCD_LED_EN);
}
#endif

static void lcm_set_gpio_output(unsigned int GPIO, unsigned int output)
{
	if (GPIO == 0xFFFFFFFF) {
#ifdef BUILD_LK
		printf("[LK/LCM] GPIO_LCD_PWR_EN =   0x%x\n", GPIO_LCD_PWR_EN);
		printf("[LK/LCM] GPIO_LCD_PWR2_EN =  0x%x\n", GPIO_LCD_PWR2_EN);
		printf("[LK/LCM] GPIO_LCD_RST_EN =  0x%x\n", GPIO_LCD_RST_EN);
		printf("[LK/LCM] GPIO_LCD_LED_EN =   0x%x\n", GPIO_LCD_LED_EN);
		printf("[LK/LCM] GPIO_LCD_BL_EN =   0x%x\n", GPIO_LCD_BL_EN);
#elif (defined BUILD_UBOOT)	/* do nothing in uboot */
#else
#endif

		return;
	}

#ifdef BUILD_LK	
	mt_set_gpio_mode(GPIO, GPIO_MODE_00);
	mt_set_gpio_dir(GPIO, GPIO_DIR_OUT);
	mt_set_gpio_out(GPIO, (output>0)? GPIO_OUT_ONE: GPIO_OUT_ZERO);
#else
	gpio_set_value(GPIO, (output>0)? GPIO_OUT_ONE: GPIO_OUT_ZERO);
#endif	
}

static void lcm_get_params(LCM_PARAMS *params)
{
	memset(params, 0, sizeof(LCM_PARAMS));

	params->type = LCM_TYPE_DSI;

	params->width = FRAME_WIDTH;
	params->height = FRAME_HEIGHT;

#if (LCM_DSI_CMD_MODE)
	params->dsi.mode = CMD_MODE;
#else
	params->dsi.mode = SYNC_EVENT_VDO_MODE;
#endif

	/* DSI */
	/* Command mode setting */
	/* 1 Three lane or Four lane */
	params->dsi.LANE_NUM = LCM_FOUR_LANE;
	/* The following defined the fomat for data coming from LCD engine. */
	params->dsi.data_format.color_order = LCM_COLOR_ORDER_RGB;
	params->dsi.data_format.trans_seq = LCM_DSI_TRANS_SEQ_MSB_FIRST;
	params->dsi.data_format.padding = LCM_DSI_PADDING_ON_LSB;
	params->dsi.data_format.format = LCM_DSI_FORMAT_RGB888;

	/* Highly depends on LCD driver capability. */
	/* Not support in MT6573 */
	params->dsi.packet_size = 256;

	/* Video mode setting */
	params->dsi.intermediat_buffer_num = 0;	/* This para should be 0 at video mode */

	params->dsi.PS = LCM_PACKED_PS_24BIT_RGB888;
	params->dsi.word_count = 720 * 3;

	params->dsi.vertical_sync_active = 1;
	params->dsi.vertical_backporch = 10;
	params->dsi.vertical_frontporch = 20;
	params->dsi.vertical_active_line = FRAME_HEIGHT;

	params->dsi.horizontal_sync_active = 1;
	params->dsi.horizontal_backporch = 30;	//57;
	params->dsi.horizontal_frontporch = 14;	//32;
	params->dsi.horizontal_active_pixel = FRAME_WIDTH;

	/* Bit rate calculation */
	/* 1 Every lane speed */
	/* params->dsi.pll_div1=0;               // div1=0,1,2,3;div1_real=1,2,4,4 ----0: 546Mbps  1:273Mbps */
	/* params->dsi.pll_div2=1;               // div2=0,1,2,3;div1_real=1,2,4,4 */
	/* params->dsi.fbk_div =31;    		// fref=26MHz, fvco=fref*(fbk_div+1)*2/(div1_real*div2_real) */
	params->dsi.PLL_CLOCK = 215;
	params->dsi.ssc_disable = 1;

	params->dsi.CLK_ZERO = 47;
	params->dsi.HS_ZERO = 36;
}

static void lcm_init(void)
{
#ifdef BUILD_LK
	printf("%s, LK\n", __func__);
#endif

#ifdef BUILD_LK
	lcm_set_gpio_output(GPIO_LCD_RST_EN, GPIO_OUT_ZERO);
	MDELAY(20);

	lcm_set_gpio_output(GPIO_LCD_PWR_EN, GPIO_OUT_ZERO);
	MDELAY(20);

	lcm_set_gpio_output(GPIO_LCD_RST_EN, GPIO_OUT_ONE);
	MDELAY(5);

	lcm_set_gpio_output(GPIO_LCD_PWR_EN, GPIO_OUT_ONE);
	MDELAY(10);

	lcm_set_gpio_output(GPIO_LCD_PWR2_EN, GPIO_OUT_ONE);
	MDELAY(10);
	
	lcm_set_gpio_output(GPIO_LCD_LED_EN, GPIO_OUT_ONE);	
	MDELAY(10);

	lcm_set_gpio_output(GPIO_LCD_BL_EN, GPIO_OUT_ONE);	
#else
	lcm_request_gpio_control();
#endif

	init_lcm_registers();
}

static void lcm_suspend(void)
{
	unsigned int data_array[16];

#ifdef BUILD_LK
	printf("%s, LK\n", __func__);
#else
	pr_debug("%s, kernel", __func__);
#endif

	lcm_set_gpio_output(GPIO_LCD_BL_EN, GPIO_OUT_ZERO);
	MDELAY(5);

	lcm_set_gpio_output(GPIO_LCD_LED_EN, GPIO_OUT_ZERO);	
	MDELAY(10);

	lcm_set_gpio_output(GPIO_LCD_PWR_EN, GPIO_OUT_ZERO);
	MDELAY(200);

	lcm_set_gpio_output(GPIO_LCD_PWR2_EN, GPIO_OUT_ZERO);
	MDELAY(5);

	data_array[0] = 0x00280500;	/* Display Off */
	dsi_set_cmdq(data_array, 1, 1);
	MDELAY(20);

	data_array[0] = 0x00111500;	/* Sleep In */
	dsi_set_cmdq(data_array, 1, 1);
	MDELAY(160);

	lcm_set_gpio_output(GPIO_LCD_RST_EN, GPIO_OUT_ZERO);
	MDELAY(20);	
}

static void lcm_resume(void)
{
	unsigned int data_array[16];
	
#ifdef BUILD_LK
	printf("%s, LK\n", __func__);
#else
	pr_debug("%s, kernel", __func__);
#endif

	lcm_set_gpio_output(GPIO_LCD_RST_EN, GPIO_OUT_ONE);
	MDELAY(5);

	lcm_set_gpio_output(GPIO_LCD_PWR_EN, GPIO_OUT_ONE);
	MDELAY(10);

	lcm_set_gpio_output(GPIO_LCD_PWR2_EN, GPIO_OUT_ONE);
	MDELAY(10);

	lcm_set_gpio_output(GPIO_LCD_LED_EN, GPIO_OUT_ONE);	
	MDELAY(10);

	lcm_set_gpio_output(GPIO_LCD_BL_EN, GPIO_OUT_ONE);	

	init_lcm_registers();

	data_array[0] = 0x00101500;	/* Sleep Out */
	dsi_set_cmdq(data_array, 1, 1);
	MDELAY(20);

	data_array[0] = 0x00290500;	/* Display On */
	dsi_set_cmdq(data_array, 1, 1);
}

#if (LCM_DSI_CMD_MODE)
static void lcm_update(unsigned int x, unsigned int y, unsigned int width, unsigned int height)
{
	unsigned int x0 = x;
	unsigned int y0 = y;
	unsigned int x1 = x0 + width - 1;
	unsigned int y1 = y0 + height - 1;

	unsigned char x0_MSB = ((x0 >> 8) & 0xFF);
	unsigned char x0_LSB = (x0 & 0xFF);
	unsigned char x1_MSB = ((x1 >> 8) & 0xFF);
	unsigned char x1_LSB = (x1 & 0xFF);
	unsigned char y0_MSB = ((y0 >> 8) & 0xFF);
	unsigned char y0_LSB = (y0 & 0xFF);
	unsigned char y1_MSB = ((y1 >> 8) & 0xFF);
	unsigned char y1_LSB = (y1 & 0xFF);

	unsigned int data_array[16];

	data_array[0] = 0x00053902;
	data_array[1] = (x1_MSB << 24) | (x0_LSB << 16) | (x0_MSB << 8) | 0x2a;
	data_array[2] = (x1_LSB);
	dsi_set_cmdq(data_array, 3, 1);

	data_array[0] = 0x00053902;
	data_array[1] = (y1_MSB << 24) | (y0_LSB << 16) | (y0_MSB << 8) | 0x2b;
	data_array[2] = (y1_LSB);
	dsi_set_cmdq(data_array, 3, 1);

	data_array[0] = 0x00290508;	/* HW bug, so need send one HS packet */
	dsi_set_cmdq(data_array, 1, 1);

	data_array[0] = 0x002c3909;
	dsi_set_cmdq(data_array, 1, 0);

}
#endif
#if 0
static unsigned int lcm_compare_id(void)
{
	unsigned int id = 0;
	unsigned char buffer[2];
	unsigned int array[16];

	SET_RESET_PIN(1);
	SET_RESET_PIN(0);
	MDELAY(1);

	SET_RESET_PIN(1);
	MDELAY(20);

	array[0] = 0x00023700;	/* read id return two byte,version and id */
	dsi_set_cmdq(array, 1, 1);

	read_reg_v2(0xF4, buffer, 2);
	id = buffer[0];		/* we only need ID */
#ifdef BUILD_LK
	printf("%s, LK nt35590 debug: nt35590 id = 0x%08x\n", __func__, id);
#else
	pr_info("%s, kernel nt35590 horse debug: nt35590 id = 0x%08x\n", __func__, id);
#endif

	if (id == LCM_ID_NT35590)
		return 1;
	else
		return 0;


}
#endif

LCM_DRIVER cm_n070ice_dsi_vdo_mt8173_lcm_drv = {
	.name = "cm_n070ice_dsi_vdo_mt8173",
	.set_util_funcs = lcm_set_util_funcs,
	.get_params = lcm_get_params,
	.init = lcm_init,
	.suspend = lcm_suspend,
	.resume = lcm_resume,
	/* .compare_id     = lcm_compare_id, */
#if (LCM_DSI_CMD_MODE)
	.update = lcm_update,
#endif
};
