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
#define FRAME_WIDTH  										(1600)
#define FRAME_HEIGHT 										(2560)


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
//#ifdef GPIO_LCM_PWR_EN   //VCC 3.3V
//#define GPIO_LCD_PWR_EN_1      GPIOEXT14
//#else
//#define GPIO_LCD_PWR_EN_1      GPIOEXT14	/* 0xFFFFFFFF */
//#endif

#define GPIO_LCD_PWR_EN_1      GPIOEXT14
#define GPIO_LCD_RST_EN_1  	   (GPIO127  | 0x80000000) 

//#ifdef GPIO_LCM_RST
//#define GPIO_LCD_RST_EN_1		 GPIO_LCM_RST
//#else
//#define GPIO_LCD_RST_EN_1     (GPIO127  | 0x80000000)  
//#endif
//#define GPIO_LCD_RST_EN_1     (GPIO127  | 0x80000000)  
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
	
	
	//{0x10,2,{0x00,0x17}},
	//{0x00, 0, {} }, 
	//{REGFLAG_DELAY, 10, {} },
	//{0x10,2,{0x07,0x07}},
	//{0x00, 0, {} }, 
	//{REGFLAG_DELAY, 10, {} },
	
	{0x11, 0, {} }, /* sleep out */
	{REGFLAG_DELAY, 40, {} },
	{0x29, 0, {} }, /* display on */
	{REGFLAG_DELAY, 160, {} },
	{REGFLAG_END_OF_TABLE, 0x00, {}}
};

static struct LCM_setting_table lcm_suspend_setting[] = {
	{0x28, 0, {} },
	{REGFLAG_DELAY, 50, {} },
	{0x10, 0, {} },
	{REGFLAG_DELAY, 200, {} },
	{REGFLAG_END_OF_TABLE, 0x00, {}}
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
	printf("%s, LK init_lcm_registers\n", __func__);
#else
	pr_debug("%s, KE init_lcm_registers\n", __func__);
#endif

	data_array[0] = 0x00010500;
	dsi_set_cmdq(data_array, 1, 1);
	MDELAY(10);
	data_array[0] = 0x00361500;
	data_array[1] = 0x703A1500;
	data_array[2] = 0x01351500;
	dsi_set_cmdq(data_array, 3, 1);
	MDELAY(10);
	data_array[0] = 0x00290500;
	dsi_set_cmdq(data_array, 1, 1);
	MDELAY(120);
	data_array[0] = 0x00110500;
	dsi_set_cmdq(data_array, 1, 1);
	MDELAY(10);
	
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

	params->lcm_if 							= LCM_INTERFACE_DSI_DUAL;
	params->lcm_cmd_if 						= LCM_INTERFACE_DSI_DUAL;
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

	params->dsi.vertical_sync_active		= 4;//6;
	params->dsi.vertical_backporch			= 8;//37;
	params->dsi.vertical_frontporch 		= 12;//3;
	params->dsi.vertical_active_line		= FRAME_HEIGHT; 


	params->dsi.horizontal_sync_active		= 32;//32; 
	params->dsi.horizontal_backporch		= 80;//80;
	params->dsi.horizontal_frontporch		= 100;//48;
	params->dsi.horizontal_active_pixel 	= FRAME_WIDTH;

	params->dsi.PLL_CLOCK 					= 450;//400;//30Hz 600;50Hz
	params->dsi.ssc_disable 				= 1;
	params->dsi.cont_clock 					= 0;

	params->dsi.ufoe_enable 				= 1;
	params->dsi.ufoe_params.lr_mode_en 		= 1;
}

static void lcm_init(void)
{
	
  printf("%s,lcm_init tpf0842001n_wqxga_dsi_vdo LK GPIO_LCD_PWR_EN_1 = %x,GPIO_LCD_RST_EN_1 = %x\n", GPIO_LCD_PWR_EN_1,GPIO_LCD_RST_EN_1);

	lcm_set_gpio_output(GPIO_LCD_PWR_EN_1, GPIO_OUT_ONE);
	lcm_set_gpio_output(GPIO_LCD_RST_EN_1,GPIO_OUT_ZERO);
	MDELAY(30);
	lcm_set_gpio_output(GPIO_LCD_RST_EN_1,GPIO_OUT_ONE);
	MDELAY(10);
	init_lcm_registers();
	//lcm_set_gpio_output(GPIO_LCD_PWR_EN_1, GPIO_OUT_ONE); 	
	//MDELAY(130);//wisky gzy 170504
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
	lcm_set_gpio_output(GPIO_LCD_PWR_EN_1, GPIO_OUT_ZERO); 	//背光上电
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
	
	//lcm_set_gpio_output(GPIO_LCD_PWR_EN_1, GPIO_OUT_ZERO);

	
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
	/*
	lcm_set_gpio_output(GPIO_LCD_BL_EN, GPIO_OUT_ONE);
 	//MDELAY(20); 
	lcm_set_gpio_output(GPIO_LCD_PWR_EN_1, GPIO_OUT_ONE); 	
	lcm_set_gpio_output(GPIO_LCD_PWR_EN_1, GPIO_OUT_ONE);
	MDELAY(20);
	init_lcm_registers(); 	
	*/
	lcm_init();
}

LCM_DRIVER tpf0842001n_wqxga_dsi_vdo_lcm_drv = {
    	.name           	= "tpf0842001n_wqxga_dsi_vdo",
        .set_util_funcs     = lcm_set_util_funcs,
        .get_params         = lcm_get_params,
        .init               = lcm_init,
        .suspend            = lcm_suspend,
        .resume             = lcm_resume,
 //   .compare_id     = lcm_compare_id,
};

