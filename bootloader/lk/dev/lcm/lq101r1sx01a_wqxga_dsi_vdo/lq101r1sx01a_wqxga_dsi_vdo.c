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
 //#define PUSH_TABLET_USING
#define LCM_DSI_CMD_MODE									0
#define FRAME_WIDTH  										(720)
#define FRAME_HEIGHT 										(1280)


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
#define GPIO_LED     (GPIO103  | 0x80000000) 
#define GPIO_RESET    (GPIO127  | 0x80000000) 

// ---------------------------------------------------------------------------
//  Local Variables
// ---------------------------------------------------------------------------

static LCM_UTIL_FUNCS lcm_util = {
	.set_reset_pin = NULL,
	.udelay = NULL,
	.mdelay = NULL,
};

#ifdef PUSH_TABLET_USING
struct LCM_setting_table {
    unsigned char cmd;
    unsigned char count;
    unsigned char para_list[128];
};
#endif
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

#ifdef PUSH_TABLET_USING

static struct LCM_setting_table lcm_initialization_setting[] = {
	
		{0xFF,3,{0x98,0x81,0x03}},			//PAGE3
	{0x01,1,{0x00}},				//GIP_1
	{0x02,1,{0x00}},
	{0x03,1,{0x53}},
	{0x04,1,{0x13}},
	{0x05,1,{0x13}},
	{0x06,1,{0x06}},
	{0x07,1,{0x00}},
	{0x08,1,{0x04}},
	{0x09,1,{0x04}},
	{0x0A,1,{0x03}},
	{0x0B,1,{0x03}},
	{0x0C,1,{0x00}},
	{0x0D,1,{0x00}},
	{0x0E,1,{0x00}},
	{0x0F,1,{0x04}},
	{0x10,1,{0x04}},
	{0x11,1,{0x00}},
	{0x12,1,{0x00}},
	{0x13,1,{0x00}},
	{0x14,1,{0x00}},
	{0x15,1,{0x00}},
	{0x16,1,{0x00}},
	{0x17,1,{0x00}},
	{0x18,1,{0x00}},
	{0x19,1,{0x00}},
	{0x1A,1,{0x00}},
	{0x1B,1,{0x00}},
	{0x1C,1,{0x00}},
	{0x1D,1,{0x00}},
	{0x1E,1,{0xC0}},
	{0x1F,1,{0x80}},
	{0x20,1,{0x04}},
	{0x21,1,{0x0B}},
	{0x22,1,{0x00}},
	{0x23,1,{0x00}},
	{0x24,1,{0x00}},
	{0x25,1,{0x00}},
	{0x26,1,{0x00}},
	{0x27,1,{0x00}},
	{0x28,1,{0x55}},
	{0x29,1,{0x03}},
	{0x2A,1,{0x00}},
	{0x2B,1,{0x00}},
	{0x2C,1,{0x06}},
	{0x2D,1,{0x00}},
	{0x2E,1,{0x00}},
	{0x2F,1,{0x00}},
	{0x30,1,{0x00}},
	{0x31,1,{0x00}},
	{0x32,1,{0x00}},
	{0x33,1,{0x30}},
	{0x34,1,{0x04}},
	{0x35,1,{0x05}},
	{0x36,1,{0x05}},
	{0x37,1,{0x00}},
	{0x38,1,{0x00}},
	{0x39,1,{0x00}},
	{0x3A,1,{0x40}},
	{0x3B,1,{0x40}},
	{0x3C,1,{0x00}},
	{0x3D,1,{0x00}},
	{0x3E,1,{0x00}},
	{0x3F,1,{0x00}},
	{0x40,1,{0x00}},
	{0x41,1,{0x00}},
	{0x42,1,{0x00}},
	{0x43,1,{0x00}},
	{0x44,1,{0x00}},
	{0x50,1,{0x01}},				//GIP_2
	{0x51,1,{0x23}},
	{0x52,1,{0x45}},
	{0x53,1,{0x67}},
	{0x54,1,{0x89}},
	{0x55,1,{0xAB}},
	{0x56,1,{0x01}},
	{0x57,1,{0x23}},
	{0x58,1,{0x45}},
	{0x59,1,{0x67}},
	{0x5A,1,{0x89}},
	{0x5B,1,{0xAB}},
	{0x5C,1,{0xCD}},
	{0x5D,1,{0xEF}},
	{0x5E,1,{0x01}},			//GIP_3
	{0x5F,1,{0x14}},
	{0x60,1,{0x15}},
	{0x61,1,{0x0C}},
	{0x62,1,{0x0D}},
	{0x63,1,{0x0E}},
	{0x64,1,{0x0F}},
	{0x65,1,{0x10}},
	{0x66,1,{0x11}},
	{0x67,1,{0x08}},
	{0x68,1,{0x02}},
	{0x69,1,{0x0A}},
	{0x6A,1,{0x02}},
	{0x6B,1,{0x02}},
	{0x6C,1,{0x02}},
	{0x6D,1,{0x02}},
	{0x6E,1,{0x02}},
	{0x6F,1,{0x02}},
	{0x70,1,{0x02}},
	{0x71,1,{0x02}},
	{0x72,1,{0x06}},
	{0x73,1,{0x02}},
	{0x74,1,{0x02}},
	{0x75,1,{0x14}},
	{0x76,1,{0x15}},
	{0x77,1,{0x11}},
	{0x78,1,{0x10}},
	{0x79,1,{0x0F}},
	{0x7A,1,{0x0E}},
	{0x7B,1,{0x0D}},
	{0x7C,1,{0x0C}},
	{0x7D,1,{0x06}},
	{0x7E,1,{0x02}},
	{0x7F,1,{0x0A}},
	{0x80,1,{0x02}},
	{0x81,1,{0x02}},
	{0x82,1,{0x02}},
	{0x83,1,{0x02}},
	{0x84,1,{0x02}},
	{0x85,1,{0x02}},
	{0x86,1,{0x02}},
	{0x87,1,{0x02}},
	{0x88,1,{0x08}},
	{0x89,1,{0x02}},
	{0x8A,1,{0x02}},

	{0xFF,3,{0x98,0x81,0x04}},		
	{0x6C,1,{0x15}},
	{0x6E,1,{0x3B}},				
	{0x6F,1,{0x53}},				
	{0x3A,1,{0xA4}},
	{0x8D,1,{0x15}},				
	{0x87,1,{0xBA}},
	
		{0xb2,1,{0xd1}},
		{0x26,1,{0x76}},
		{0x88,1,{0x0b}},
		{0xb2,1,{0xd1}},
		
	{0xFF,3,{0x98,0x81,0x01}},			
	{0x22,1,{0x0A}},				
	{0x31,1,{0x00}},				
	{0x53,1,{0x8A}},				//VCOM1
	{0x55,1,{0x88}},				//VCOM2
	{0x50,1,{0xA6}},				//VREG1OUT
	{0x51,1,{0xA6}},			//VREG2OUT
	{0x60,1,{0x14}},			//SDT
	{0xA0,1,{0x08}},
	{0xA1,1,{0x27}},			//GammaP
	{0xA2,1,{0x36}},
	{0xA3,1,{0x15}},
	{0xA4,1,{0x17}},
	{0xA5,1,{0x2B}},
	{0xA6,1,{0x1E}},
	{0xA7,1,{0x1F}},
	{0xA8,1,{0x96}},
	{0xA9,1,{0x1C}},
	{0xAA,1,{0x28}},
	{0xAB,1,{0x7C}},
	{0xAC,1,{0x1B}},
	{0xAD,1,{0x1A}},
	{0xAE,1,{0x4D}},
	{0xAF,1,{0x23}},
	{0xB0,1,{0x29}},
	{0xB1,1,{0x4B}},
	{0xB2,1,{0x5A}},
	{0xB3,1,{0x2C}},
	{0xC0,1,{0x08}},			//GAMMAN
	{0xC1,1,{0x26}},
	{0xC2,1,{0x36}},
	{0xC3,1,{0x15}},
	{0xC4,1,{0x17}},
	{0xC5,1,{0x2B}},
	{0xC6,1,{0x1F}},
	{0xC7,1,{0x1F}},
	{0xC8,1,{0x96}},
	{0xC9,1,{0x1C}},
	{0xCA,1,{0x29}},
	{0xCB,1,{0x7C}},
	{0xCC,1,{0x1A}},
	{0xCD,1,{0x19}},
	{0xCE,1,{0x4D}},
	{0xCF,1,{0x22}},
	{0xD0,1,{0x29}},
	{0xD1,1,{0x4B}},
	{0xD2,1,{0x59}},
	{0xD3,1,{0x2C}},

	{0xFF,3,{0x98,0x81,0x00}},
	{0x35,1,{0x00}},		     
	{0x11, 1, {0x00}},       
	{REGFLAG_DELAY, 150, {}},
	// Display ON            
	{0x29, 1, {0x00}},       
	{REGFLAG_DELAY, 200, {}},
	{REGFLAG_END_OF_TABLE, 0x00, {}}
	// {0x10,2,{0x00,0x17}},
	// {0x00, 0, {} }, 
	// {REGFLAG_DELAY, 10, {} },
	// {0x10,2,{0x07,0x07}},
	// {0x00, 0, {} }, 
	// {REGFLAG_DELAY, 10, {} },
	
	// {0x11, 0, {} }, /* sleep out */
	// {REGFLAG_DELAY, 40, {} },
	// {0x29, 0, {} }, /* display on */
	// {REGFLAG_DELAY, 160, {} },
	// {REGFLAG_END_OF_TABLE, 0x00, {}}
};

static struct LCM_setting_table lcm_suspend_setting[] = {
	// {0x28, 0, {} },
	// {REGFLAG_DELAY, 50, {} },
	// {0x10, 0, {} },
	// {REGFLAG_DELAY, 200, {} },
	// {REGFLAG_END_OF_TABLE, 0x00, {}}
};

static void push_table(struct LCM_setting_table *table, unsigned int count, unsigned char force_update)
{
    unsigned int i,j,buffer[6];
   
    for(i = 0; i < count; i++)
    {
        unsigned cmd;
        cmd = table[i].cmd;

        switch (cmd) {
            case REGFLAG_DELAY :
                if(table[i].count <= 10)
                    MDELAY(table[i].count);
                else
                    MDELAY(table[i].count);
                break;

            case REGFLAG_END_OF_TABLE :
                break;

            default:
                dsi_set_cmdq_V2(cmd, table[i].count, table[i].para_list, force_update);
        }
    }
}



#endif

// ---------------------------------------------------------------------------
//  LCM Driver Implementations
// ---------------------------------------------------------------------------

static void lcm_set_util_funcs(const LCM_UTIL_FUNCS *util)
{
    memcpy(&lcm_util, util, sizeof(LCM_UTIL_FUNCS));
}

static void init_lcm_registers(void)
{
	unsigned int data_array[16];

#ifdef BUILD_LK
	printf("%s, LK\n", __func__);
#else
	pr_debug("%s, KE\n", __func__);
#endif

#ifdef PUSH_TABLET_USING
	push_table(lcm_initialization_setting,
		   sizeof(lcm_initialization_setting) / sizeof(struct LCM_setting_table), 1);
#else
	/*
	data_array[0] = 0x00023902;
	data_array[1] = 0x00170010;
	dsi_set_cmdq(data_array, 2, 1);
	
	data_array[0] = 0x00000500;
	dsi_set_cmdq(data_array, 1, 1);
	MDELAY(10);	
	
	data_array[0] = 0x00023902;
	data_array[1] = 0x00070710;
	dsi_set_cmdq(data_array, 2, 1);

	data_array[0] = 0x00000500;
	dsi_set_cmdq(data_array, 1, 1);
	MDELAY(10);
				
	data_array[0] = 0x00110500;
	dsi_set_cmdq(data_array, 1, 1);
	MDELAY(120);
	
	data_array[0] = 0x00290500;
	dsi_set_cmdq(data_array, 1, 1);
	MDELAY(20);
	*/
	
data_array[0] = 0x00110500;
	dsi_set_cmdq(data_array, 1, 1);
	MDELAY(50);
	
	data_array[0] = 0x00290500;
	dsi_set_cmdq(data_array, 1, 1);
	MDELAY(30);
	
#endif
}

static void lcm_get_params(LCM_PARAMS *params)
{
#ifdef BUILD_LK	
	printf("[LK/LCM] lcm_get_params() enter\n");
#else
	printk("[Kernel/LCM] lcm_get_params() enter\n");
#endif
	memset(params, 0, sizeof(LCM_PARAMS));
	
	params->type   							= LCM_TYPE_DSI;

	params->width  							= FRAME_WIDTH;
	params->height 							= FRAME_HEIGHT;
	
	params->dsi.mode 						= SYNC_PULSE_VDO_MODE;//CMD_MODE,SYNC_PULSE_VDO_MODE,SYNC_EVENT_VDO_MODE,BURST_VDO_MODE

	params->lcm_if 							= LCM_INTERFACE_DSI0;
	// params->lcm_cmd_if 					= LCM_INTERFACE_DSI_DUAL;
	// DSI
	/* Command mode setting */
	//1 Three lane or Four lane
	params->dsi.LANE_NUM					= LCM_FOUR_LANE;
	//The following defined the fomat for data coming from LCD engine.
	params->dsi.data_format.color_order 	= LCM_COLOR_ORDER_RGB;
	params->dsi.data_format.trans_seq 		= LCM_DSI_TRANS_SEQ_MSB_FIRST;
	params->dsi.data_format.padding 		= LCM_DSI_PADDING_ON_LSB;
	params->dsi.data_format.format			= LCM_DSI_FORMAT_RGB888;


	/* Highly depends on LCD driver capability. */
	/* Not support in MT6573 */
	params->dsi.packet_size = 256;

	/* Video mode setting */
	params->dsi.intermediat_buffer_num 		= 0;	/* This para should be 0 at video mode */

	params->dsi.PS = LCM_PACKED_PS_24BIT_RGB888;
	params->dsi.word_count = FRAME_WIDTH * 3;
/*
	params->dsi.esd_check_enable = 1;
	params->dsi.customization_esd_check_enable = 1;
	params->dsi.lcm_esd_check_table[0].cmd = 0x0A;
	params->dsi.lcm_esd_check_table[0].count = 1;
	params->dsi.lcm_esd_check_table[0].para_list[0] = 0x9C;
	*/
	params->dsi.vertical_sync_active		= 2;//6;
	params->dsi.vertical_backporch			= 16;//37;
	params->dsi.vertical_frontporch 		= 14;//3;
	params->dsi.vertical_active_line		= FRAME_HEIGHT; 


	params->dsi.horizontal_sync_active		= 5;//32; 
	params->dsi.horizontal_backporch		= 42;//80;
	params->dsi.horizontal_frontporch		= 40;//48;
	params->dsi.horizontal_active_pixel 	= FRAME_WIDTH;

	params->dsi.PLL_CLOCK					= 203;
	params->dsi.ssc_disable 				= 1;
	params->dsi.cont_clock 					= 0;

	params->dsi.ufoe_enable 				= 0;
	params->dsi.ufoe_params.lr_mode_en 		= 0;
}

static void lcm_init_power(void)
{
	printf("lcm_init_power\n");
	lcm_set_gpio_output(GPIO_RESET,GPIO_OUT_ZERO);
	MDELAY(20);
	upmu_set_rg_vgp4_vosel(3);
	upmu_set_rg_vgp4_sw_en(1);
	MDELAY(15);
	lcm_set_gpio_output(GPIO_RESET,GPIO_OUT_ONE);
	MDELAY(80);
	lcm_set_gpio_output(GPIO_RESET,GPIO_OUT_ZERO);
	MDELAY(20);
	lcm_set_gpio_output(GPIO_RESET,GPIO_OUT_ONE);
	MDELAY(20);
	lcm_set_gpio_output(GPIO_LCD_PWR_EN, GPIO_OUT_ONE);
	lcm_set_gpio_output(GPIO_LED,GPIO_OUT_ONE);
	MDELAY(270);	
}
static void lcm_init(void)
{
	// upmu_set_rg_vgp4_vosel(3);
	// upmu_set_rg_vgp4_sw_en(1);
	// mt_set_gpio_out(GPIO_LED,GPIO_OUT_ZERO);
	// lcm_set_gpio_output(992, GPIO_OUT_ZERO);
	
	printf("%s,lcm_init lq101r1sx01a_wqxga_dsi_vdo LK \n", __func__);
	init_lcm_registers();//bypass 11 29等
	MDELAY(130);
	//lcm_set_gpio_output(GPIO_LCD_PWR2_EN, GPIO_OUT_ONE); 	

}

static void lcm_suspend(void)
{
	unsigned int data_array[16];
    printf("%s,lcm_suspend LK \n", __func__);
	//1.背光关闭 
	//2.停止传输数据 20ms 
	//3.disp off 0x28 50ms ; sleep in 0x10 20ms 
	//4.200ms
	//5.VCC off
//	lcm_set_gpio_output(GPIO_LCD_PWR2_EN, GPIO_OUT_ZERO); 	//背光断电
	MDELAY(20);
	//disp off 0x28 50ms ; sleep in 0x10 20ms 
	MDELAY(200);
	#ifdef PUSH_TABLET_USING
	push_table(lcm_suspend_setting,
		   sizeof(lcm_suspend_setting) / sizeof(struct LCM_setting_table), 1);
	#else
	data_array[0] = 0x00280500;
	dsi_set_cmdq(data_array, 1, 1);
	MDELAY(20);

	data_array[0] = 0x00100500;
	dsi_set_cmdq(data_array, 1, 1);
	MDELAY(100);
  #endif
		upmu_set_rg_vgp4_sw_en(0);
	lcm_set_gpio_output(GPIO_LCD_PWR_EN, GPIO_OUT_ZERO);

	
}
static void lcm_resume(void)
{
    //enable VSP & VSN
	//enable VSP & VSN
#ifdef BUILD_LK
    printf("%s,lcm_resume LK \n", __func__);
#else
    printk("%s,lcm_resume kernel", __func__);
#endif	
		// MDELAY(210);
		// lcm_set_gpio_output(GPIO_LCD_PWR_EN, GPIO_OUT_ZERO);
	/*
	lcm_set_gpio_output(GPIO_LCD_BL_EN, GPIO_OUT_ONE);
 	//MDELAY(20); 
	lcm_set_gpio_output(GPIO_LCD_PWR2_EN, GPIO_OUT_ONE); 	
	lcm_set_gpio_output(GPIO_LCD_PWR_EN, GPIO_OUT_ONE);
	MDELAY(20);
	init_lcm_registers(); 	
	*/
	lcm_init();
}

LCM_DRIVER lq101r1sx01a_wqxga_dsi_vdo_lcm_drv = {
    	.name           	= "lq101r1sx01a_wqxga_dsi_vdo",
        .set_util_funcs     = lcm_set_util_funcs,
        .get_params         = lcm_get_params,
        .init               = lcm_init,
        .suspend            = lcm_suspend,
        .resume             = lcm_resume,
 			  .init_power     		= lcm_init_power,			  
};

