/*=======================================================================*/
/* HEADER FILES                                                          */
/*=======================================================================*/

#include <video_fb.h>
#include <platform/disp_drv_platform.h>
#include <target/board.h>
#include <platform/env.h>
#include <platform/mt_disp_drv.h>
#include "lcm_drv.h"
#include <string.h>
#include <stdlib.h>
#include <platform/mt_gpt.h>
#include <platform/primary_display.h>
#include "ddp_dsi.h"

#if HDMI_SUB_PATH_LK
#include <platform/hdmi_drv.h>
#endif

// ---------------------------------------------------------------------------
//  Export Functions - Display
// ---------------------------------------------------------------------------

static void  *fb_addr      = NULL;
static void  *logo_db_addr = NULL;
static UINT32 fb_size      = 0;
static UINT32 fb_offset_logo = 0; // counter of fb_size
static UINT32 fb_isdirty   = 0;

// ---------------------------------------------------------------------------
//  Local Variables
// ---------------------------------------------------------------------------
extern LCM_PARAMS *lcm_params;

DISP_STATUS DISP_PowerEnable(BOOL enable);
DISP_STATUS DISP_PanelEnable(BOOL enable);
DSI_STATUS DPI_WaitVsync(void);
DSI_STATUS LCD_WaitForNotBusy(void);
DSI_STATUS DSI_WaitVsync(void);
DSI_STATUS DSI_WaitForNotBusy(void);

UINT32 mt_disp_get_vram_size(void)
{
	return DISP_GetVRamSize();
}

extern void disp_log_enable(int enable);
extern void dbi_log_enable(int enable);
extern void * memset(void *,int,unsigned int);
#if 0
static disp_dfo_item_t disp_dfo_setting[] = {
	{"LCM_FAKE_WIDTH",  0},
	{"LCM_FAKE_HEIGHT", 0},
	{"DISP_DEBUG_SWITCH",   0}
};

#define MT_DISP_DFO_DEBUG
#ifdef MT_DISP_DFO_DEBUG
#define disp_dfo_printf(string, args...) dprintf(INFO,"[DISP_DFO]"string, ##args)
#else
#define disp_dfo_printf(string, args...) ()
#endif

unsigned int mt_disp_parse_dfo_setting(void)
{
	unsigned int i, j=0 ;
	char tmp[11];
	char *buffer = NULL;
	char *ptr = NULL;

	buffer = (char *)get_env("DFO");
	disp_dfo_printf("env buffer = %s\n", buffer);

	if (buffer != NULL) {
		for (i = 0; i< (sizeof(disp_dfo_setting)/sizeof(disp_dfo_item_t)); i++) {
			j = 0;

			memset((void*)tmp, 0, sizeof(tmp)/sizeof(tmp[0]));

			ptr = strstr(buffer, disp_dfo_setting[i].name);

			if (ptr == NULL) continue;

			disp_dfo_printf("disp_dfo_setting[%d].name = [%s]\n", i, ptr);

			do {} while ((*ptr++) != ',');

			do {tmp[j++] = *ptr++;}
			while (*ptr != ',' && j < sizeof(tmp)/sizeof(tmp[0]));

			disp_dfo_setting[i].value = atoi((const char*)tmp);

			disp_dfo_printf("disp_dfo_setting[%d].name = [%s|%d]\n", i, tmp, disp_dfo_setting[i].value);
		}
	} else {
		disp_dfo_printf("env buffer = NULL\n");
	}

	return 0;
}


int mt_disp_get_dfo_setting(const char *string, unsigned int *value)
{
	char *disp_name;
	int  disp_value;
	unsigned int i = 0;

	if (string == NULL)
		return -1;

	for (i=0; i<(sizeof(disp_dfo_setting)/sizeof(disp_dfo_item_t)); i++) {
		disp_name = disp_dfo_setting[i].name;
		disp_value = disp_dfo_setting[i].value;
		if (!strcmp(disp_name, string)) {
			*value = disp_value;
			disp_dfo_printf("%s = [DEC]%d [HEX]0x%08x\n", disp_name, disp_value, disp_value);
			return 0;
		}
	}

	return 0;
}
#endif
#define FB_LAYER            2
#define BOOT_MENU_LAYER     3


#if HDMI_SUB_PATH_LK
static unsigned int resolution_mode=HDMI_VIDEO_1920x1080p_60Hz;//HDMI_VIDEO_1280x720p_60Hz;


unsigned int disp_get_hdmi_resolution()
{
	unsigned lk_hdmi_res = 0;

#ifdef MTK_ALPS_BOX_SUPPORT

	lk_hdmi_res = resolution_mode;
#endif

	printf("[LK] %s, #%d hdmi_res %d\n", __func__, __LINE__,lk_hdmi_res);


	return lk_hdmi_res;
}

unsigned int disp_get_hdmi_width(unsigned res)
{
	if (res >= HDMI_VIDEO_RESOLUTION_NUM) {
		printf("[LK] error %s, #%d res %d\n", __func__, __LINE__,res);
	}

	return hdmi_res_param_table_lk[res][0];
}

unsigned int disp_get_hdmi_height(unsigned res)
{
	if (res >= HDMI_VIDEO_RESOLUTION_NUM) {
		printf("[LK] error %s, #%d res %d\n", __func__, __LINE__,res);
	}

	return hdmi_res_param_table_lk[res][1];
}


DISP_STATUS disp_set_hdmi_resolution(unsigned int lk_hdmi_res)
{
	DISP_STATUS r = DISP_STATUS_OK;

#ifdef MTK_ALPS_BOX_SUPPORT
	if (lk_hdmi_res >= HDMI_VIDEO_RESOLUTION_NUM) {
		lk_hdmi_res = HDMI_VIDEO_1920x1080p_60Hz;
		dprintf(0, "[LK] error %s, #%d hdmi_res %d\n", __func__, __LINE__,lk_hdmi_res);

	}
	dprintf(0,"[LK] %s, #%d hdmi_res %d\n", __func__, __LINE__,lk_hdmi_res);

	resolution_mode = lk_hdmi_res;
#endif

	return r;
}
#endif


static void _mtkfb_draw_block(unsigned int addr, unsigned int x, unsigned int y, unsigned int w, unsigned int h, unsigned int color)
{
	unsigned int i = 0;
	unsigned int j = 0;
	unsigned int start_addr = addr+ALIGN_TO(CFG_DISPLAY_WIDTH, MTK_FB_ALIGNMENT)*4*y+x*4;
	for (j=0; j<h; j++) {
		for (i = 0; i<w; i++) {
			*(unsigned int*)(start_addr + i*4 + j*ALIGN_TO(CFG_DISPLAY_WIDTH, MTK_FB_ALIGNMENT)*4) = color;
		}
	}
}


static int _mtkfb_internal_test(unsigned int va, unsigned int w, unsigned int h)
{
	// this is for debug, used in bring up day
	unsigned int i = 0;
	unsigned int color = 0;
	int _internal_test_block_size =120;
	for (i=0; i<w*h/_internal_test_block_size/_internal_test_block_size; i++) {
		color = (i&0x1)*0xff;
		//color += ((i&0x2)>>1)*0xff00;
		//color += ((i&0x4)>>2)*0xff0000;
		color += 0xff000000U;
		_mtkfb_draw_block(va,
		                  i%(w/_internal_test_block_size)*_internal_test_block_size,
		                  i/(w/_internal_test_block_size)*_internal_test_block_size,
		                  _internal_test_block_size,
		                  _internal_test_block_size,
		                  color);
	}
	mt_disp_update(0, 0, CFG_DISPLAY_WIDTH, CFG_DISPLAY_HEIGHT);
	mdelay(100);
	primary_display_diagnose();
	return 0;

	_internal_test_block_size = 20;
	for (i=0; i<w*h/_internal_test_block_size/_internal_test_block_size; i++) {
		color = (i&0x1)*0xff;
		color += ((i&0x2)>>1)*0xff00;
		color += ((i&0x4)>>2)*0xff0000;
		color += 0xff000000U;
		_mtkfb_draw_block(va,
		                  i%(w/_internal_test_block_size)*_internal_test_block_size,
		                  i/(w/_internal_test_block_size)*_internal_test_block_size,
		                  _internal_test_block_size,
		                  _internal_test_block_size,
		                  color);
	}

	mt_disp_update(0, 0, CFG_DISPLAY_WIDTH, CFG_DISPLAY_HEIGHT);
	mdelay(100);
	primary_display_diagnose();
	return 0;
}


void mt_disp_init(void *lcdbase)
{
	//unsigned int lcm_fake_width = 0;
	//unsigned int lcm_fake_height = 0;
	UINT32 boot_mode_addr = 0;
#if HDMI_SUB_PATH_LK
	unsigned int hdmi_width = 0;
	unsigned int hdmi_height = 0;

#endif
	fb_size = ALIGN_TO(CFG_DISPLAY_WIDTH, MTK_FB_ALIGNMENT) * ALIGN_TO(CFG_DISPLAY_HEIGHT, MTK_FB_ALIGNMENT) * CFG_DISPLAY_BPP / 8;
	boot_mode_addr = ((UINT32)lcdbase + fb_size);

	logo_db_addr = (void *)((UINT32)lcdbase - 4 * 1024 * 1024);
//    fb_addr      = (void *)((UINT32)lcdbase + fb_size);
	fb_addr  =   lcdbase;
	fb_offset_logo = 0;

	dprintf(0, "boot_mode_addr=0x%08x, lcdbase=0x%08x,fb_addr=0x%08x,fb_size=0x%08x\n", boot_mode_addr, (unsigned int)lcdbase, (unsigned int)fb_addr, fb_size);

#ifdef MTK_ALPS_BOX_SUPPORT
	primary_display_init(NULL);
#else

	primary_display_init(NULL);

#endif

#if HDMI_SUB_PATH_LK
	ext_disp_init(resolution_mode);
#endif

	memset((void*)lcdbase, 0x0, DISP_GetVRamSize());

	disp_input_config input;
	memset(&input, 0, sizeof(disp_input_config));
	input.layer     = BOOT_MENU_LAYER;
	input.layer_en  = 1;
	input.fmt       = eBGR565;
	input.addr      = boot_mode_addr;
	input.src_x     = 0;
	input.src_y     = 0;
	input.src_w     = CFG_DISPLAY_WIDTH;
	input.src_h     = CFG_DISPLAY_HEIGHT;
	// input.src_pitch = ALIGN_TO(CFG_DISPLAY_WIDTH, MTK_FB_ALIGNMENT)*2;
	if (CFG_DISPLAY_WIDTH % MTK_FB_ALIGNMENT == 0)
	input.src_pitch = ALIGN_TO(CFG_DISPLAY_WIDTH, MTK_FB_ALIGNMENT)*2;
	else
	input.src_pitch = CFG_DISPLAY_WIDTH*2;
	input.dst_x     = 0;
	input.dst_y     = 0;
	input.dst_w     = CFG_DISPLAY_WIDTH;
	input.dst_h     = CFG_DISPLAY_HEIGHT;
	input.aen       = 0;
	input.alpha     = 0xa0;
	input.keyEn     = 1;
	input.key       = 0xFF000000;

#if DISABLE_LCM_INIT || DISABLE_LCM_INIT_V1
#else
	// xuecheng, workaround for CMDQ not ready
	/* if(!primary_display_is_video_mode()) */
	{
		primary_display_config_input(&input);
	}
#endif

#if HDMI_SUB_PATH_LK
	/*boot menu layer ,rgb565,pitch bpp = 2*/
	hdmi_width = hdmi_get_width();
	hdmi_height = hdmi_get_height();

	input.src_w     = hdmi_width;
	input.src_h     = hdmi_height;
	input.src_pitch = hdmi_width*2;
	input.dst_w     = hdmi_width;
	input.dst_h     = hdmi_height;
	ext_disp_config_input(&input);

#endif
	memset(&input, 0, sizeof(disp_input_config));
	input.layer     = FB_LAYER;
	input.layer_en  = 1;
	input.fmt       = eBGRA8888;
	input.addr      = (unsigned int)fb_addr;
	input.src_x     = 0;
	input.src_y     = 0;
	input.src_w     = CFG_DISPLAY_WIDTH;
	input.src_h     = CFG_DISPLAY_HEIGHT;
	input.src_pitch = ALIGN_TO(CFG_DISPLAY_WIDTH, MTK_FB_ALIGNMENT)*4;
	input.dst_x     = 0;
	input.dst_y     = 0;
	input.dst_w     = CFG_DISPLAY_WIDTH;
	input.dst_h     = CFG_DISPLAY_HEIGHT;

	input.aen       = 1;
	input.alpha     = 0xff;

#if DISABLE_LCM_INIT || DISABLE_LCM_INIT_V1
#else
	primary_display_config_input(&input);
#endif

#if HDMI_SUB_PATH_LK
	/*boot logo layer ,argb8888,pitch bpp = 4*/

	input.src_w     = hdmi_width;
	input.src_h     = hdmi_height;
	input.src_pitch = hdmi_width*4;
	input.dst_w     = hdmi_width;
	input.dst_h     = hdmi_height;
	ext_disp_config_input(&input);

#endif
	//_mtkfb_internal_test(fb_addr, CFG_DISPLAY_WIDTH, CFG_DISPLAY_HEIGHT);

#if 0
	mt_disp_parse_dfo_setting();

	if ((0 == mt_disp_get_dfo_setting("LCM_FAKE_WIDTH", &lcm_fake_width)) && (0 == mt_disp_get_dfo_setting("LCM_FAKE_HEIGHT", &lcm_fake_height))) {
		if (DISP_STATUS_OK != DISP_Change_LCM_Resolution(lcm_fake_width, lcm_fake_height)) {
			dprintf(INFO,"[DISP_DFO]WARNING!!! Change LCM Resolution FAILED!!!\n");
		}
	}
#endif

}


void mt_disp_power(BOOL on)
{
	return;
#ifndef CFG_MT6577_FPGA
	if (on) {
//		disp_path_ddp_clock_on();
		DISP_PowerEnable(TRUE);
		DISP_PanelEnable(TRUE);
		mt_disp_update(0, 0, CFG_DISPLAY_WIDTH, CFG_DISPLAY_HEIGHT);
	} else {
		DISP_PanelEnable(FALSE);
		DISP_PowerEnable(FALSE);
//		disp_path_ddp_clock_off();
	}
#endif
}


void* mt_get_logo_db_addr(void)
{
	return logo_db_addr;
}


void* mt_get_fb_addr(void)
{
	fb_isdirty = 1;
	return (void*)((UINT32)fb_addr + fb_offset_logo * fb_size);
}

void* mt_get_tempfb_addr(void)
{
	//use offset = 2 as tempfb for decompress logo
	return (void*)((UINT32)fb_addr + 2*fb_size);
}

UINT32 mt_get_fb_size(void)
{
	return fb_size;
}

extern void arch_clean_cache_range(addr_t start, size_t len);
void mt_disp_update(UINT32 x, UINT32 y, UINT32 width, UINT32 height)
{
	{

		unsigned int va = (unsigned int)fb_addr;
		dprintf(0,"fb dump:fb_addr(va)=0x%x, 0x%08x, 0x%08x, 0x%08x, 0x%08x\n",va, *(unsigned int*)va, *(unsigned int*)(va+4), *(unsigned int*)(va+8), *(unsigned int*)(va+0xC));
	}
	arch_clean_cache_range((unsigned int)fb_addr, DISP_GetFBRamSize());

#if DISABLE_LCM_INIT
#else
	primary_display_trigger(TRUE);
#endif

#if HDMI_SUB_PATH_LK
	extd_disp_trigger(TRUE);
#endif

	/*
	    // TODO: Fixit!!!!!
	    if(fb_isdirty)
	    {
	        fb_isdirty = 0;
	        MASKREG32(0x1400E000, 0x1, 0x1); //Enable DISP MUTEX0
	        MASKREG32(0x1400E004, 0x1, 0x0);
	        LCD_CHECK_RET(LCD_LayerSetAddress(FB_LAYER - 1, (UINT32)fb_addr + fb_offset_logo * fb_size));
	        printk("[wwy] hardware address = %x, fb_offset_logo = %d\n",(UINT32)fb_addr + fb_offset_logo * fb_size,fb_offset_logo);
	        arch_clean_cache_range((unsigned int)fb_addr, DISP_GetFBRamSize());
	        DISP_CHECK_RET(DISP_UpdateScreen(x, y, width, height));
	        //wait reg update to set fb_offset_logo
	        DISP_WaitRegUpdate();
	        fb_offset_logo = fb_offset_logo ? 0 : 3;

	    }
	    else
	    {
	    arch_clean_cache_range((unsigned int)fb_addr, DISP_GetFBRamSize());
	    DISP_CHECK_RET(DISP_UpdateScreen(x, y, width, height));
	    }
	    */
}


void mt_disp_wait_idle(void)
{
	if (lcm_params->type==LCM_TYPE_DPI) {
		DPI_WaitVsync();
	} else if (lcm_params->type==LCM_TYPE_DBI) {
		LCD_WaitForNotBusy();
	} else if (lcm_params->type==LCM_TYPE_DSI && lcm_params->dsi.mode!=CMD_MODE) {
		DSI_WaitVsync();
		DSI_WaitVsync();
	} else if (lcm_params->type==LCM_TYPE_DSI && lcm_params->dsi.mode ==CMD_MODE) {
		DSI_WaitForNotBusy();
	}
}
static void mt_disp_adjusting_hardware_addr(void)
{
	printf("[wwy] mt_disp_adjusting_hardware_addr fb_offset_logo = %d\n",fb_offset_logo);
	if (fb_offset_logo == 0) {
		mt_get_fb_addr();
		memcpy(fb_addr,(void *)((UINT32)fb_addr + 3 * fb_size),fb_size);
		mt_disp_update(0, 0, CFG_DISPLAY_WIDTH, CFG_DISPLAY_HEIGHT);
	}
}
UINT32 mt_disp_get_lcd_time(void)
{
	unsigned int fps = 0;
	// TODO: will enable after te enabled
#if HDMI_SUB_PATH_LK && DISABLE_LCM_INIT
	fps = ext_disp_get_vsync_interval();
#else
	fps = primary_display_get_vsync_interval();
#endif
	dprintf(CRITICAL, "%s, fps=%d\n", __func__, fps);
	if (fps)
		return fps;
	else
		return 6000;
}

// Attention: this api indicates whether the lcm is connected
int DISP_IsLcmFound(void)
{
	return primary_display_is_lcm_connected();
}

const char* mt_disp_get_lcm_id(void)
{
	return primary_display_get_lcm_name();
}


void disp_get_fb_address(UINT32 *fbVirAddr, UINT32 *fbPhysAddr)
{
	*fbVirAddr = (UINT32)fb_addr;
	*fbPhysAddr = (UINT32)fb_addr;
}

// ---------------------------------------------------------------------------
//  Export Functions - Console
// ---------------------------------------------------------------------------

#ifdef CONFIG_CFB_CONSOLE
//  video_hw_init -- called by drv_video_init() for framebuffer console

extern UINT32 memory_size(void);

void *video_hw_init (void)
{
	static GraphicDevice s_mt65xx_gd;
	unsigned int disp_width = CFG_DISPLAY_WIDTH;
	unsigned int disp_height = CFG_DISPLAY_HEIGHT;

	memset(&s_mt65xx_gd, 0, sizeof(GraphicDevice));
	//xuecheng, use new calculate formula;
	s_mt65xx_gd.frameAdrs  = (UINT32)fb_addr+fb_size;//CFG_DISPLAY_WIDTH*2*(CFG_DISPLAY_HEIGHT-80);//fb_size;//memory_size() - mt_disp_get_vram_size() + fb_size;

#if HDMI_SUB_PATH_LK
	disp_width = hdmi_get_width();
	disp_height = hdmi_get_height();
#endif

	s_mt65xx_gd.winSizeX   = disp_width;
	s_mt65xx_gd.winSizeY   = disp_height;
	s_mt65xx_gd.gdfIndex   = CFB_565RGB_16BIT;
	dprintf(0, "s_mt65xx_gd.gdfIndex=%d", s_mt65xx_gd.gdfIndex);
	s_mt65xx_gd.gdfBytesPP = 2;//CFG_DISPLAY_BPP / 8; //logo format change to ARGB8888
	s_mt65xx_gd.memSize    = s_mt65xx_gd.winSizeX * s_mt65xx_gd.winSizeY * s_mt65xx_gd.gdfBytesPP;

	return &s_mt65xx_gd;
}


void video_set_lut(unsigned int index,  /* color number */
                   unsigned char r,     /* red */
                   unsigned char g,     /* green */
                   unsigned char b)     /* blue */
{
}

#endif  // CONFIG_CFB_CONSOLE
