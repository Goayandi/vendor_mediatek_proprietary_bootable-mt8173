#ifndef BUILD_LK
#include <linux/string.h>
#else
#include <string.h>
#endif
#include "lcm_drv.h"

#ifdef BUILD_LK
	#include <platform/mt_gpio.h>
	#include <platform/mt_pmic.h>
	#include <platform/mt_i2c.h>
    #include <platform/upmu_common.h>	
#elif defined(BUILD_UBOOT)
#else
	#include <mach/mt_gpio.h>
	#include <mach/mt_pm_ldo.h> 
#endif

// ---------------------------------------------------------------------------
//  Local Constants
// ---------------------------------------------------------------------------
#define LCM_DSI_CMD_MODE									0
#define FRAME_WIDTH  										(720)
#define FRAME_HEIGHT 										(1280)
#define LCM_ID_lq101r1sx01a_wqxga_dsi_vdo                                     0x59


#define UDELAY(n) 											(lcm_util.udelay(n))
#define MDELAY(n) 											(lcm_util.mdelay(n))

#define REGFLAG_DELAY             								0xFC
#define REGFLAG_END_OF_TABLE      							0xFD   // END OF REGISTERS MARKER

#ifndef TRUE
    #define TRUE 1
#endif

#ifndef FALSE
    #define FALSE 0
#endif

//extern void DSI_clk_HS_mode(DISP_MODULE_ENUM module, void* cmdq, bool enter);

//GPIO124-> BLK_EN ->LCD_POWER
#ifdef GPIO_LCM_PWR_EN
#define GPIO_LCD_PWR_EN      GPIOEXT14
#else
#define GPIO_LCD_PWR_EN      GPIOEXT14	/* 0xFFFFFFFF */
#endif

#define GPIO_LCD_BL_EN      (GPIO102  | 0x80000000) 
#define GPIO_LCD_PWR2_EN     (GPIO103  | 0x80000000) 


// ---------------------------------------------------------------------------
//  Local Variables
// ---------------------------------------------------------------------------

static LCM_UTIL_FUNCS lcm_util = {
	.set_reset_pin = NULL,
	.udelay = NULL,
	.mdelay = NULL,
};

#define SET_RESET_PIN(v)    (lcm_util.set_reset_pin((v)))

#define UDELAY(n) (lcm_util.udelay(n))
#define MDELAY(n) (lcm_util.mdelay(n))


// ---------------------------------------------------------------------------
//  Local Functions
// ---------------------------------------------------------------------------

#define dsi_set_cmdq_V2(cmd, count, ppara, force_update)	lcm_util.dsi_set_cmdq_V2(cmd, count, ppara, force_update)
#define dsi_set_cmdq(pdata, queue_size, force_update)		lcm_util.dsi_set_cmdq(pdata, queue_size, force_update)
#define wrtie_cmd(cmd)										lcm_util.dsi_write_cmd(cmd)
#define write_regs(addr, pdata, byte_nums)					lcm_util.dsi_write_regs(addr, pdata, byte_nums)
#define read_reg(cmd)											lcm_util.dsi_dcs_read_lcm_reg(cmd)
#define read_reg_v2(cmd, buffer, buffer_size)   				lcm_util.dsi_dcs_read_lcm_reg_v2(cmd, buffer, buffer_size)      


#ifndef BUILD_LK
static void lcm_request_gpio_control(void)
{		
	gpio_request(GPIO_LCD_BL_EN, "GPIO_LCD_BL_EN");//GPIO_LCD_PWR_EN	
	gpio_request(GPIO_LCD_PWR_EN, "GPIO_LCD_PWR_EN");
	printk("[KE/LCM-lq101r1sx01a_wqxga_dsi_vdo] GPIO_LCD_BL_EN =   0x%x\n", GPIO_LCD_BL_EN);
	
}
#endif

static void lcm_set_gpio_output(unsigned int GPIO, unsigned int output)
{
	if (GPIO == 0xFFFFFFFF) {
#ifdef BUILD_LK
		printf("[LK/LCM] invaid gpio! \n");
#elif (defined BUILD_UBOOT)	/* do nothing in uboot */
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


static void init_lcm_registers(void)
{
	unsigned int data_array[16];

#ifdef BUILD_LK
	printf("%s, LK\n", __func__);
#endif

#if 1
    //DSI_clk_HS_mode(DISP_MODULE_DSI0, NULL, 1);

	data_array[0] = 0x00110500;	/* software reset */
	dsi_set_cmdq(data_array, 1, 1);
	MDELAY(80);

	data_array[0] = 0x00290500;
	dsi_set_cmdq(data_array, 1, 1);
	MDELAY(120);	
#endif
}



// ---------------------------------------------------------------------------
//  LCM Driver Implementations
// ---------------------------------------------------------------------------

static void lcm_set_util_funcs(const LCM_UTIL_FUNCS *util)
{
    memcpy(&lcm_util, util, sizeof(LCM_UTIL_FUNCS));
}


static void lcm_get_params(LCM_PARAMS *params)
{
#ifdef BUILD_LK	
		printf("[LK/LCM] lcm_get_params() enter\n");
#else
		printk("[Kernel/LCM] lcm_get_params() enter\n");
#endif
		memset(params, 0, sizeof(LCM_PARAMS));
	
		params->type   = LCM_TYPE_DSI;

		params->width  = FRAME_WIDTH;
		params->height = FRAME_HEIGHT;

#if (LCM_DSI_CMD_MODE)
	params->dsi.mode = CMD_MODE;
#else
	params->dsi.mode = SYNC_EVENT_VDO_MODE;
#endif

		params->lcm_if = LCM_INTERFACE_DSI0;
		params->lcm_cmd_if = LCM_INTERFACE_DSI0;
		// DSI
		/* Command mode setting */
		//1 Three lane or Four lane
		params->dsi.LANE_NUM				= LCM_FOUR_LANE;
		//The following defined the fomat for data coming from LCD engine.
		params->dsi.data_format.color_order = LCM_COLOR_ORDER_RGB;
		params->dsi.data_format.trans_seq = LCM_DSI_TRANS_SEQ_MSB_FIRST;
		params->dsi.data_format.padding = LCM_DSI_PADDING_ON_LSB;
		params->dsi.data_format.format		= LCM_DSI_FORMAT_RGB888;

	/* Highly depends on LCD driver capability. */
	/* Not support in MT6573 */
		params->dsi.packet_size = 256;
	
		/* Video mode setting */
		params->dsi.intermediat_buffer_num = 0;	/* This para should be 0 at video mode */
	
		params->dsi.PS = LCM_PACKED_PS_24BIT_RGB888;
		params->dsi.word_count = FRAME_WIDTH * 3;
		
		params->dsi.vertical_sync_active				= 4;// 3	 2
		params->dsi.vertical_backporch					= 4;// 20   1
		params->dsi.vertical_frontporch 				= 24; // 1   12 
		params->dsi.vertical_active_line				= FRAME_HEIGHT; 


		params->dsi.horizontal_sync_active				= 40; 
		params->dsi.horizontal_backporch				= 60;
		params->dsi.horizontal_frontporch			   	= 60;
		params->dsi.horizontal_active_pixel 			= FRAME_WIDTH;
		
	    params->dsi.PLL_CLOCK = 420;
	    params->dsi.ssc_disable = 1;
        params->dsi.cont_clock = 1;
	
}

static void lcm_init(void)
{
	
	#ifdef BUILD_LK

    printf("%s,lcm_init lq101r1sx01a_wqxga_dsi_vdo LK \n", __func__);
	#else
    printk("%s,lcm_init lq101r1sx01a_wqxga_dsi_vdo kernel", __func__);
	#endif
	
#ifdef BUILD_LK
	lcm_set_gpio_output(GPIO_LCD_BL_EN, GPIO_OUT_ONE);//GPIO_LCD_PWR_EN
	
 	//MDELAY(20); 
	lcm_set_gpio_output(GPIO_LCD_PWR2_EN, GPIO_OUT_ONE); 	
	lcm_set_gpio_output(GPIO_LCD_PWR_EN, GPIO_OUT_ONE);
	MDELAY(20);
	
#else
	lcm_request_gpio_control();
#endif
	
	 init_lcm_registers(); 
}

static void lcm_suspend(void)
{
	unsigned int data_array[16];
    //Back to MP.P7 baseline , solve LCD display abnormal On the right
    // when phone sleep , config output low, disable backlight drv chip  
#ifdef BUILD_LK
    printf("%s,lcm_suspend LK \n", __func__);
#else
    printk("%s,lcm_suspend kernel", __func__);
#endif
	data_array[0] = 0x00280500;	/* Display Off */
	dsi_set_cmdq(data_array, 1, 1);
	MDELAY(80);

	data_array[0] = 0x00111500;	/* Sleep In */
	dsi_set_cmdq(data_array, 1, 1);   
	MDELAY(34);	  
	lcm_set_gpio_output(GPIO_LCD_BL_EN, GPIO_OUT_ZERO);
	//MDELAY(10);	
	lcm_set_gpio_output(GPIO_LCD_PWR2_EN, GPIO_OUT_ZERO);	
	lcm_set_gpio_output(GPIO_LCD_PWR_EN, GPIO_OUT_ZERO);
	MDELAY(10);
}
static void lcm_resume(void)
{
    //enable VSP & VSN
#ifdef BUILD_LK
    printf("%s,lcm_resume LK \n", __func__);
#else
    printk("%s,lcm_resume kernel", __func__);
#endif	
	lcm_set_gpio_output(GPIO_LCD_BL_EN, GPIO_OUT_ONE);
 	//MDELAY(20); 
	lcm_set_gpio_output(GPIO_LCD_PWR2_EN, GPIO_OUT_ONE); 	
	lcm_set_gpio_output(GPIO_LCD_PWR_EN, GPIO_OUT_ONE);
	MDELAY(20);
	init_lcm_registers(); 	
}

LCM_DRIVER lq101r1sx01a_wqxga_dsi_vdo_lcm_drv = {
	.name               = "lq101r1sx01a_wqxga_dsi_vdo",
	.set_util_funcs     = lcm_set_util_funcs,
	.get_params         = lcm_get_params,
	.init               = lcm_init,
	.suspend            = lcm_suspend,
	.resume             = lcm_resume,
    //.compare_id         = lcm_compare_id,
};

