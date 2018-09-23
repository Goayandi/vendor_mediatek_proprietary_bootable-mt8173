#include "platform/ddp_info.h"


#include <platform/mt_typedefs.h>
#include <platform/sync_write.h>

#include <platform/disp_drv_platform.h>
#include <platform/disp_drv_log.h>
//#include <debug.h>
//#include <platform/ddp_path.h>

#include <platform/ddp_manager.h>
//#include <platform/ddp_dump.h>



#include <platform/ddp_reg.h>
#include <platform/ddp_dpi.h>
#include <platform/ddp_dpi_reg.h>

#include <platform/mt_gpt.h>

#include <debug.h>
#include <string.h>


#undef  LOG_TAG
#define LOG_TAG "DPI"
#define ENABLE_DPI_INTERRUPT        0
#define DPI_INTERFACE_NUM           2
//#define DPI_IDX(module)             ((module==DISP_MODULE_DPI0)?0:1)
#define DPI_IDX(module)             0


#define K2_SMT

#undef LCD_BASE
#define LCD_BASE (0xF4024000)
#define DPI_REG_OFFSET(r)       offsetof(DPI_REGS, r)
#define REG_ADDR(base, offset)  (((BYTE *)(base)) + (offset))
#define msleep(x)    mdelay(x)


//#ifdef INREG32
//#undef INREG32
//#define INREG32(x)          (__raw_readl((unsigned long*)(x)))
//#endif

#if 0
static int dpi_reg_op_debug = 0;

#define DPI_OUTREG32(cmdq, addr, val) \
    {\
        if(dpi_reg_op_debug) \
            printk("[dsi/reg]0x%08x=0x%08x, cmdq:0x%08x\n", addr, val, cmdq);\
        if(cmdq) \
            cmdqRecWrite(cmdq, (unsigned int)(addr)&0x1fffffff, val, ~0); \
        else \
            mt65xx_reg_sync_writel(val, addr);}
#else
#define DPI_OUTREG32(cmdq, addr, val) mt_reg_sync_writel(val, addr)
#endif

/*#undef DISP_LOG_PRINT
#define DISP_LOG_PRINT(level, sub_module, fmt, arg...)  \
    do {                                                    \
        xlog_printk(level, "DISP/"sub_module, fmt, ##arg);  \
    }while(0)*/

static BOOL s_isDpiPowerOn = FALSE;
static BOOL s_isDpiStart   = FALSE;
static BOOL s_isDpiConfig  = FALSE;
static int dpi_vsync_irq_count[DPI_INTERFACE_NUM];
static int dpi_undflow_irq_count[DPI_INTERFACE_NUM];
static DPI_REGS regBackup;
static PDPI_REGS DPI_REG[DPI_INTERFACE_NUM];
static LCM_UTIL_FUNCS lcm_utils_dpi;

const UINT32 BACKUP_DPI_REG_OFFSETS[] = {
	DPI_REG_OFFSET(INT_ENABLE),
	DPI_REG_OFFSET(CNTL),
	DPI_REG_OFFSET(SIZE),

	DPI_REG_OFFSET(TGEN_HWIDTH),
	DPI_REG_OFFSET(TGEN_HPORCH),
	DPI_REG_OFFSET(TGEN_VWIDTH_LODD),
	DPI_REG_OFFSET(TGEN_VPORCH_LODD),

	DPI_REG_OFFSET(BG_HCNTL),
	DPI_REG_OFFSET(BG_VCNTL),
	DPI_REG_OFFSET(BG_COLOR),

	DPI_REG_OFFSET(TGEN_VWIDTH_LEVEN),
	DPI_REG_OFFSET(TGEN_VPORCH_LEVEN),
	DPI_REG_OFFSET(TGEN_VWIDTH_RODD),

	DPI_REG_OFFSET(TGEN_VPORCH_RODD),
	DPI_REG_OFFSET(TGEN_VWIDTH_REVEN),

	DPI_REG_OFFSET(TGEN_VPORCH_REVEN),
	DPI_REG_OFFSET(ESAV_VTIM_LOAD),
	DPI_REG_OFFSET(ESAV_VTIM_ROAD),
	DPI_REG_OFFSET(ESAV_FTIM),
};

/*the static functions declare*/
static void lcm_udelay(UINT32 us)
{
	udelay(us);
}

static void lcm_mdelay(UINT32 ms)
{
	msleep(ms);
}

static void lcm_set_reset_pin(UINT32 value)
{
#ifndef K2_SMT
	DPI_OUTREG32(0, MMSYS_CONFIG_BASE+0x150, value);
#endif
}

static void lcm_send_cmd(UINT32 cmd)
{
#ifndef K2_SMT
	DPI_OUTREG32(0, LCD_BASE+0x0F80, cmd);
#endif
}

static void lcm_send_data(UINT32 data)
{
#ifndef K2_SMT
	DPI_OUTREG32(0, LCD_BASE+0x0F90, data);
#endif
}

static void _BackupDPIRegisters(DISP_MODULE_ENUM module)
{
	UINT32 i;
	DPI_REGS *reg = &regBackup;

	for (i = 0; i < ARY_SIZE(BACKUP_DPI_REG_OFFSETS); ++i) {
		DPI_OUTREG32(0, REG_ADDR(reg, BACKUP_DPI_REG_OFFSETS[i]),
		             AS_UINT32(REG_ADDR(DPI_REG[DPI_IDX(module)], BACKUP_DPI_REG_OFFSETS[i])));
	}
}

static void _RestoreDPIRegisters(DISP_MODULE_ENUM module)
{
	UINT32 i;
	DPI_REGS *reg = &regBackup;

	for (i = 0; i < ARY_SIZE(BACKUP_DPI_REG_OFFSETS); ++i) {
		DPI_OUTREG32(0, REG_ADDR(DPI_REG[DPI_IDX(module)], BACKUP_DPI_REG_OFFSETS[i]),
		             AS_UINT32(REG_ADDR(reg, BACKUP_DPI_REG_OFFSETS[i])));
	}
}

/*the fuctions declare*/
/*DPI clock setting - use TVDPLL provide DPI clock*/
DPI_STATUS ddp_dpi_ConfigPclk(DISP_MODULE_ENUM module, cmdqRecHandle cmdq, unsigned int clk_req, DPI_POLARITY polarity)
{
	UINT32 dpickpol = 1, dpickoutdiv = 1, dpickdut = 1;
	UINT32 pcw = 0, postdiv = 0;
	DPI_REG_OUTPUT_SETTING ctrl = DPI_REG[DPI_IDX(module)]->OUTPUT_SETTING;
	DPI_REG_CLKCNTL clkcon = DPI_REG[DPI_IDX(module)]->DPI_CLKCON;

	switch (clk_req) {
		case DPI_VIDEO_720x480p_60Hz:
		case DPI_VIDEO_720x576p_50Hz: {
			pcw = 0xc7627;
			postdiv = 1;
			break;
		}

		case DPI_VIDEO_1920x1080p_30Hz:
		case DPI_VIDEO_1280x720p_50Hz:
		case DPI_VIDEO_1920x1080i_50Hz:
		case DPI_VIDEO_1920x1080p_25Hz:
		case DPI_VIDEO_1920x1080p_24Hz:
		case DPI_VIDEO_1920x1080p_50Hz:
		case DPI_VIDEO_1280x720p3d_50Hz:
		case DPI_VIDEO_1920x1080i3d_50Hz:
		case DPI_VIDEO_1920x1080p3d_24Hz: {
			pcw = 0x112276;
			postdiv = 0;
			break;
		}

		case DPI_VIDEO_1280x720p_60Hz:
		case DPI_VIDEO_1920x1080i_60Hz:
		case DPI_VIDEO_1920x1080p_23Hz:
		case DPI_VIDEO_1920x1080p_29Hz:
		case DPI_VIDEO_1920x1080p_60Hz:
		case DPI_VIDEO_1280x720p3d_60Hz:
		case DPI_VIDEO_1920x1080i3d_60Hz:
		case DPI_VIDEO_1920x1080p3d_23Hz: {
			pcw = 0x111e08;
			postdiv = 0;
			break;
		}

		default: {
			DISP_LOG_PRINT(ANDROID_LOG_WARN, "DPI", "unknown clock frequency: %d \n", clk_req);
			break;
		}
	}

	switch (clk_req) {
		case DPI_VIDEO_720x480p_60Hz:
		case DPI_VIDEO_720x576p_50Hz:
		case DPI_VIDEO_1920x1080p3d_24Hz:
		case DPI_VIDEO_1280x720p_60Hz: {
			dpickpol = 0;
			dpickdut = 0;
			break;
		}

		case DPI_VIDEO_1920x1080p_30Hz:
		case DPI_VIDEO_1280x720p_50Hz:
		case DPI_VIDEO_1920x1080i_50Hz:
		case DPI_VIDEO_1920x1080p_25Hz:
		case DPI_VIDEO_1920x1080p_24Hz:
		case DPI_VIDEO_1920x1080p_50Hz:
		case DPI_VIDEO_1280x720p3d_50Hz:
		case DPI_VIDEO_1920x1080i3d_50Hz:
		case DPI_VIDEO_1920x1080i_60Hz:
		case DPI_VIDEO_1920x1080p_23Hz:
		case DPI_VIDEO_1920x1080p_29Hz:
		case DPI_VIDEO_1920x1080p_60Hz:
		case DPI_VIDEO_1280x720p3d_60Hz:
		case DPI_VIDEO_1920x1080i3d_60Hz:
		case DPI_VIDEO_1920x1080p3d_23Hz: {
			break;
		}

		default: {
			DISP_LOG_PRINT(ANDROID_LOG_WARN, "DPI", "unknown clock frequency: %d \n", clk_req);
			break;
		}
	}

	//DISP_LOG_PRINT(ANDROID_LOG_WARN, "DPI", "TVDPLL clock setting clk %d, clksrc: %d\n", clk_req,  clksrc);

#ifndef CONFIG_FPGA_EARLY_PORTING    //FOR BRING_UP
	DPI_OUTREG32(cmdq, 0x10209000 + 0x270, (postdiv << 4)|(0x01 << 0)); // TVDPLL enable
	DPI_OUTREG32(cmdq, 0x10209000 + 0x274, pcw|(1 << 31));     // set TVDPLL output clock frequency
#endif

	/*IO driving setting*/
	MASKREG32(DISPSYS_IO_DRIVING, 0x3C00, 0x0); // 0x1400 for 8mA, 0x0 for 4mA

	/*DPI output clock polarity*/
	ctrl.CLK_POL = (DPI_POLARITY_FALLING == polarity) ? 1 : 0;
	DPI_OUTREG32(cmdq, &DPI_REG[DPI_IDX(module)]->OUTPUT_SETTING, AS_UINT32(&ctrl));

	clkcon.DPI_CKOUT_DIV = dpickoutdiv;
	clkcon.DPI_CK_POL = dpickpol;
	clkcon.DPI_CK_DUT = dpickdut;
	DPI_OUTREG32(cmdq, &DPI_REG[DPI_IDX(module)]->DPI_CLKCON , AS_UINT32(&clkcon));

	return DPI_STATUS_OK;
}

DPI_STATUS ddp_dpi_ConfigDE(DISP_MODULE_ENUM module, cmdqRecHandle cmdq, DPI_POLARITY polarity)
{
	DPI_REG_OUTPUT_SETTING pol = DPI_REG[DPI_IDX(module)]->OUTPUT_SETTING;

	pol.DE_POL = (DPI_POLARITY_FALLING == polarity) ? 1 : 0;
	DPI_OUTREG32(cmdq, &DPI_REG[DPI_IDX(module)]->OUTPUT_SETTING, AS_UINT32(&pol));

	return DPI_STATUS_OK;
}

DPI_STATUS ddp_dpi_ConfigVsync(DISP_MODULE_ENUM module, cmdqRecHandle cmdq, DPI_POLARITY polarity, UINT32 pulseWidth, UINT32 backPorch, UINT32 frontPorch)
{
	DPI_REG_TGEN_VWIDTH_LODD vwidth_lodd  = DPI_REG[DPI_IDX(module)]->TGEN_VWIDTH_LODD;
	DPI_REG_TGEN_VPORCH_LODD vporch_lodd  = DPI_REG[DPI_IDX(module)]->TGEN_VPORCH_LODD;
	DPI_REG_OUTPUT_SETTING pol = DPI_REG[DPI_IDX(module)]->OUTPUT_SETTING;
	DPI_REG_CNTL VS = DPI_REG[DPI_IDX(module)]->CNTL;

	pol.VSYNC_POL = (DPI_POLARITY_FALLING == polarity) ? 1 : 0;
	vwidth_lodd.VPW_LODD = pulseWidth;
	vporch_lodd.VBP_LODD= backPorch;
	vporch_lodd.VFP_LODD= frontPorch;

	VS.VS_LODD_EN = 1;
	VS.VS_LEVEN_EN = 0;
	VS.VS_RODD_EN = 0;
	VS.VS_REVEN_EN = 0;

	DPI_OUTREG32(cmdq, &DPI_REG[DPI_IDX(module)]->OUTPUT_SETTING, AS_UINT32(&pol));
	DPI_OUTREG32(cmdq, &DPI_REG[DPI_IDX(module)]->TGEN_VWIDTH_LODD, AS_UINT32(&vwidth_lodd));
	DPI_OUTREG32(cmdq, &DPI_REG[DPI_IDX(module)]->TGEN_VPORCH_LODD, AS_UINT32(&vporch_lodd));
	DPI_OUTREG32(cmdq, &DPI_REG[DPI_IDX(module)]->CNTL, AS_UINT32(&VS));

	return DPI_STATUS_OK;
}

DPI_STATUS ddp_dpi_ConfigHsync(DISP_MODULE_ENUM module, cmdqRecHandle cmdq, DPI_POLARITY polarity, UINT32 pulseWidth, UINT32 backPorch, UINT32 frontPorch)
{
	DPI_REG_TGEN_HPORCH hporch = DPI_REG[DPI_IDX(module)]->TGEN_HPORCH;
	DPI_REG_OUTPUT_SETTING pol = DPI_REG[DPI_IDX(module)]->OUTPUT_SETTING;

	hporch.HBP = backPorch;
	hporch.HFP = frontPorch;
	pol.HSYNC_POL = (DPI_POLARITY_FALLING == polarity) ? 1 : 0;
	DPI_REG[DPI_IDX(module)]->TGEN_HWIDTH = pulseWidth;

	DPI_OUTREG32(cmdq, &DPI_REG[DPI_IDX(module)]->TGEN_HWIDTH,pulseWidth);
	DPI_OUTREG32(cmdq, &DPI_REG[DPI_IDX(module)]->TGEN_HPORCH, AS_UINT32(&hporch));
	DPI_OUTREG32(cmdq, &DPI_REG[DPI_IDX(module)]->OUTPUT_SETTING, AS_UINT32(&pol));

	return DPI_STATUS_OK;
}

DPI_STATUS ddp_dpi_ConfigDualEdge(DISP_MODULE_ENUM module, cmdqRecHandle cmdq, bool enable, UINT32 mode)
{
	DPI_OUTREGBIT(cmdq, DPI_REG_OUTPUT_SETTING, DPI_REG[DPI_IDX(module)]->OUTPUT_SETTING, DUAL_EDGE_SEL, enable);

#ifndef CONFIG_FPGA_EARLY_PORTING
	DPI_OUTREGBIT(cmdq, DPI_REG_DDR_SETTING, DPI_REG[DPI_IDX(module)]->DDR_SETTING, DDR_4PHASE, 1);
	DPI_OUTREGBIT(cmdq, DPI_REG_DDR_SETTING, DPI_REG[DPI_IDX(module)]->DDR_SETTING, DDR_EN, 1);
#endif

	return DPI_STATUS_OK;
}

DPI_STATUS ddp_dpi_ConfigBG(DISP_MODULE_ENUM module, cmdqRecHandle cmdq, bool enable, int BG_W, int BG_H)
{
	if (enable == false) {
		DPI_OUTREGBIT(cmdq, DPI_REG_CNTL, DPI_REG[DPI_IDX(module)]->CNTL, BG_EN, 0);
	} else if (BG_W || BG_H) {
		DPI_OUTREGBIT(cmdq, DPI_REG_CNTL, DPI_REG[DPI_IDX(module)]->CNTL, BG_EN, 1);
		DPI_OUTREGBIT(cmdq, DPI_REG_BG_HCNTL, DPI_REG[DPI_IDX(module)]->BG_HCNTL, BG_RIGHT, BG_W/4);
		DPI_OUTREGBIT(cmdq, DPI_REG_BG_HCNTL, DPI_REG[DPI_IDX(module)]->BG_HCNTL, BG_LEFT, BG_W - BG_W/4);
		DPI_OUTREGBIT(cmdq, DPI_REG_BG_VCNTL, DPI_REG[DPI_IDX(module)]->BG_VCNTL, BG_BOT, BG_H/4);
		DPI_OUTREGBIT(cmdq, DPI_REG_BG_VCNTL, DPI_REG[DPI_IDX(module)]->BG_VCNTL, BG_TOP, BG_H - BG_H/4);
		DPI_OUTREGBIT(cmdq, DPI_REG_CNTL, DPI_REG[DPI_IDX(module)]->CNTL, BG_EN, 1);
		DPI_OUTREG32(cmdq, &DPI_REG[DPI_IDX(module)]->BG_COLOR, 0);
	}

	return DPI_STATUS_OK;
}

DPI_STATUS ddp_dpi_ConfigSize(DISP_MODULE_ENUM module, cmdqRecHandle cmdq, UINT32 width, UINT32 height)
{
	DPI_REG_SIZE size = DPI_REG[DPI_IDX(module)]->SIZE;
	size.WIDTH  = width;
	size.HEIGHT = height;
	DPI_OUTREG32(cmdq, &DPI_REG[DPI_IDX(module)]->SIZE, AS_UINT32(&size));

	return DPI_STATUS_OK;
}

DPI_STATUS ddp_dpi_EnableColorBar(DISP_MODULE_ENUM module)
{
	/*enable internal pattern - color bar*/
	if (module == DISP_MODULE_DPI0)
		DPI_OUTREG32(0, DISPSYS_DPI0_BASE + 0xF00, 0x41);
	else
		DPI_OUTREG32(0, DISPSYS_DPI1_BASE + 0xF00, 0x41);

	return DPI_STATUS_OK;
}

int ddp_dpi_power_on(DISP_MODULE_ENUM module, void *cmdq_handle)
{
	int ret = 0;
	DISP_LOG_PRINT(ANDROID_LOG_WARN, "DPI", "ddp_dpi_power_on, s_isDpiPowerOn %d\n", s_isDpiPowerOn);
	if (!s_isDpiPowerOn) {
#if 0 //ndef DIABLE_CLOCK_API
		ret += clk_prepare(ddp_clk_map[MM_CLK_DPI_PIXEL]);
		ret += clk_enable(ddp_clk_map[MM_CLK_DPI_PIXEL]);
		ret += clk_prepare(ddp_clk_map[MM_CLK_DPI_ENGINE]);
		ret += clk_enable(ddp_clk_map[MM_CLK_DPI_ENGINE]);
#endif
		if (ret > 0) {
			DISP_LOG_PRINT(ANDROID_LOG_ERROR, "DPI", "power manager API return FALSE\n");
		}
		//_RestoreDPIRegisters();
		s_isDpiPowerOn = TRUE;
	}

	return 0;
}

int ddp_dpi_power_off(DISP_MODULE_ENUM module, void *cmdq_handle)
{
	int ret = 0;
	DISP_LOG_PRINT(ANDROID_LOG_WARN, "DPI", "ddp_dpi_power_off, s_isDpiPowerOn %d\n", s_isDpiPowerOn);
	if (s_isDpiPowerOn) {
#if 0 //ndef DIABLE_CLOCK_API
		_BackupDPIRegisters();
		ret += clk_disable(ddp_clk_map[MM_CLK_DPI_PIXEL]);
		clk_unprepare(ddp_clk_map[MM_CLK_DPI_PIXEL]);
		ret += clk_disable(ddp_clk_map[MM_CLK_DPI_ENGINE]);
		clk_unprepare(ddp_clk_map[MM_CLK_DPI_ENGINE]);
#endif
		if (ret >0) {
			DISP_LOG_PRINT(ANDROID_LOG_ERROR, "DPI", "power manager API return FALSE\n");
		}
		s_isDpiPowerOn = FALSE;
	}

	return 0;

}

DPI_STATUS ddp_dpi_yuv422_setting(DISP_MODULE_ENUM module, cmdqRecHandle cmdq, UINT32 uvsw)
{
	DPI_REG_YUV422_SETTING uvset = DPI_REG[DPI_IDX(module)]->YUV422_SETTING;

	uvset.UV_SWAP = uvsw;
	DPI_OUTREG32(cmdq, &DPI_REG[DPI_IDX(module)]->YUV422_SETTING, AS_UINT32(&uvset));

	return DPI_STATUS_OK;
}

DPI_STATUS ddp_dpi_CLPFSetting(DISP_MODULE_ENUM module, cmdqRecHandle cmdq, UINT8 clpfType, BOOL roundingEnable)
{
	DPI_REG_CLPF_SETTING setting = DPI_REG[DPI_IDX(module)]->CLPF_SETTING;

	setting.CLPF_TYPE = clpfType;
	setting.ROUND_EN = roundingEnable ? 1 : 0;
	DPI_OUTREG32(cmdq, &DPI_REG[DPI_IDX(module)]->CLPF_SETTING, AS_UINT32(&setting));

	return DPI_STATUS_OK;
}

DPI_STATUS ddp_dpi_ConfigHDMI(DISP_MODULE_ENUM module, cmdqRecHandle cmdq, UINT32 yuv422en, UINT32 rgb2yuven, UINT32 ydfpen, UINT32 r601sel, UINT32 clpfen)
{
	DPI_REG_CNTL ctrl = DPI_REG[DPI_IDX(module)]->CNTL;

	ctrl.YUV422_EN = yuv422en;
	ctrl.RGB2YUV_EN = rgb2yuven;
	ctrl.TDFP_EN = ydfpen;
	ctrl.R601_SEL = r601sel;
	ctrl.CLPF_EN = clpfen;

	DPI_OUTREG32(cmdq, &DPI_REG[DPI_IDX(module)]->CNTL, AS_UINT32(&ctrl));

	return DPI_STATUS_OK;
}

DPI_STATUS ddp_dpi_ConfigVsync_LEVEN(DISP_MODULE_ENUM module, cmdqRecHandle cmdq, UINT32 pulseWidth, UINT32 backPorch, UINT32 frontPorch, BOOL fgInterlace)
{
	DPI_REG_TGEN_VWIDTH_LEVEN vwidth_leven  = DPI_REG[DPI_IDX(module)]->TGEN_VWIDTH_LEVEN;
	DPI_REG_TGEN_VPORCH_LEVEN vporch_leven  = DPI_REG[DPI_IDX(module)]->TGEN_VPORCH_LEVEN;

	vwidth_leven.VPW_LEVEN = pulseWidth;
	vwidth_leven.VPW_HALF_LEVEN = fgInterlace;
	vporch_leven.VBP_LEVEN = backPorch; //vporch_leven.VFP_HALF_LEVEN = fgInterlace;
	vporch_leven.VFP_LEVEN = frontPorch;

	DPI_OUTREG32(cmdq, &DPI_REG[DPI_IDX(module)]->TGEN_VWIDTH_LEVEN, AS_UINT32(&vwidth_leven));
	DPI_OUTREG32(cmdq, &DPI_REG[DPI_IDX(module)]->TGEN_VPORCH_LEVEN, AS_UINT32(&vporch_leven));

	return DPI_STATUS_OK;
}

DPI_STATUS ddp_dpi_ConfigVsync_RODD(DISP_MODULE_ENUM module, cmdqRecHandle cmdq, UINT32 pulseWidth, UINT32 backPorch, UINT32 frontPorch)
{
	DPI_REG_TGEN_VWIDTH_RODD vwidth_rodd  = DPI_REG[DPI_IDX(module)]->TGEN_VWIDTH_RODD;
	DPI_REG_TGEN_VPORCH_RODD vporch_rodd  = DPI_REG[DPI_IDX(module)]->TGEN_VPORCH_RODD;

	vwidth_rodd.VPW_RODD = pulseWidth;
	vporch_rodd.VBP_RODD = backPorch;
	vporch_rodd.VFP_RODD = frontPorch;

	DPI_OUTREG32(cmdq, &DPI_REG[DPI_IDX(module)]->TGEN_VWIDTH_RODD, AS_UINT32(&vwidth_rodd));
	DPI_OUTREG32(cmdq, &DPI_REG[DPI_IDX(module)]->TGEN_VPORCH_RODD, AS_UINT32(&vporch_rodd));

	return DPI_STATUS_OK;
}

int ddp_dpi_config(DISP_MODULE_ENUM module, disp_ddp_path_config *config, void *cmdq_handle)
{
	if (s_isDpiConfig == FALSE) {
		LCM_DPI_PARAMS *dpi_config = &(config->dispif_config.dpi);
		DISP_LOG_PRINT(ANDROID_LOG_WARN, "DPI", "ddp_dpi_config DPI status:%x, cmdq:%x\n", INREG32(&DPI_REG[DPI_IDX(module)]->STATUS), (unsigned int)cmdq_handle);

		ddp_dpi_ConfigPclk(module, cmdq_handle, dpi_config->dpi_clock, dpi_config->clk_pol);
		ddp_dpi_ConfigSize(module, cmdq_handle, dpi_config->width, dpi_config->height);
		ddp_dpi_ConfigBG(module, cmdq_handle, true, dpi_config->bg_width, dpi_config->bg_height);
		ddp_dpi_ConfigDE(module, cmdq_handle, dpi_config->de_pol);
		ddp_dpi_ConfigVsync(module, cmdq_handle, dpi_config->vsync_pol, dpi_config->vsync_pulse_width,
		                    dpi_config->vsync_back_porch, dpi_config->vsync_front_porch );
		ddp_dpi_ConfigHsync(module, cmdq_handle, dpi_config->hsync_pol, dpi_config->hsync_pulse_width,
		                    dpi_config->hsync_back_porch, dpi_config->hsync_front_porch );

		ddp_dpi_ConfigDualEdge(module, cmdq_handle, dpi_config->i2x_en, dpi_config->i2x_edge);

		s_isDpiConfig = TRUE;
		DISP_LOG_PRINT(ANDROID_LOG_WARN, "DPI", "ddp_dpi_config done\n");
	}

	return 0;
}

int ddp_dpi_reset( DISP_MODULE_ENUM module, void *cmdq_handle)
{
	DISP_LOG_PRINT(ANDROID_LOG_WARN, "DPI", "ddp_dpi_reset\n");

	DPI_OUTREGBIT(NULL, DPI_REG_RST, DPI_REG[DPI_IDX(module)]->DPI_RST, RST, 1);
	DPI_OUTREGBIT(NULL, DPI_REG_RST, DPI_REG[DPI_IDX(module)]->DPI_RST, RST, 0);

	return 0;
}

int ddp_dpi_start(DISP_MODULE_ENUM module, void *cmdq)
{
	return 0;
}

int ddp_dpi_trigger(DISP_MODULE_ENUM module, void *cmdq)
{
	if (s_isDpiStart == FALSE) {
		DISP_LOG_PRINT(ANDROID_LOG_WARN, "DPI", "ddp_dpi_start\n");
		ddp_dpi_reset(module, cmdq);
		/*enable DPI*/
		DPI_OUTREG32(cmdq, &DPI_REG[DPI_IDX(module)]->DPI_EN, 0x00000001);

		s_isDpiStart = TRUE;
	}
	return 0;
}

int ddp_dpi_stop(DISP_MODULE_ENUM module, void *cmdq_handle)
{
	DISP_LOG_PRINT(ANDROID_LOG_WARN, "DPI", "ddp_dpi_stop\n");

	/*disable DPI and background, and reset DPI*/
	DPI_OUTREG32(cmdq_handle, &DPI_REG[DPI_IDX(module)]->DPI_EN, 0x00000000);
	ddp_dpi_ConfigBG(module, cmdq_handle, false, 0, 0);
	ddp_dpi_reset(module, cmdq_handle);

	s_isDpiStart  = FALSE;
	s_isDpiConfig = FALSE;
	dpi_vsync_irq_count[0]   = 0;
	dpi_vsync_irq_count[1]   = 0;
	dpi_undflow_irq_count[0] = 0;
	dpi_undflow_irq_count[1] = 0;

	return 0;
}

int ddp_dpi_is_busy(DISP_MODULE_ENUM module)
{
	unsigned int status = INREG32(&DPI_REG[DPI_IDX(module)]->STATUS);

	return (status & (0x1<<16) ? 1 : 0);
}

int ddp_dpi_is_idle(DISP_MODULE_ENUM module)
{
	return !ddp_dpi_is_busy(module);
}

unsigned int ddp_dpi_get_cur_addr(bool rdma_mode, int layerid )
{
	if (rdma_mode)
		return (INREG32(DISP_REG_RDMA_MEM_START_ADDR+DISP_INDEX_OFFSET*2));
	else {
		if (INREG32(DISP_INDEX_OFFSET+DISP_REG_OVL_RDMA0_CTRL+layerid* 0x20 ) & 0x1)
			return (INREG32(DISP_INDEX_OFFSET+DISP_REG_OVL_L0_ADDR+layerid * 0x20));
		else
			return 0;
	}
}


#if ENABLE_DPI_INTERRUPT
static irqreturn_t _DPI_InterruptHandler(DISP_MODULE_ENUM module, unsigned int param)
{
	static int counter = 0;
	DPI_REG_INTERRUPT status = DPI_REG[DPI_IDX(module)]->INT_STATUS;

	if (status.VSYNC) {
		dpi_vsync_irq_count[DPI_IDX(module)]++;
		if (dpi_vsync_irq_count[DPI_IDX(module)] > 30) {
			//printk("dpi vsync %d\n", dpi_vsync_irq_count[DPI_IDX(module)]);
			dpi_vsync_irq_count[DPI_IDX(module)] = 0;
		}

		if (counter) {
			DISP_LOG_PRINT(ANDROID_LOG_ERROR, "DPI", "[Error] DPI FIFO is empty, "
			               "received %d times interrupt !!!\n", counter);
			counter = 0;
		}
	}

	DPI_OUTREG32(0, &DPI_REG[DPI_IDX(module)]->INT_STATUS, 0);

	return IRQ_HANDLED;
}
#endif

int ddp_dpi_init(DISP_MODULE_ENUM module, void *cmdq)
{
	UINT32 i;

	DISP_LOG_PRINT(ANDROID_LOG_WARN, "DPI", "ddp_dpi_init- %x\n", (unsigned int)cmdq);

#ifdef CONFIG_FPGA_EARLY_PORTING
	DPI_OUTREG32(0, DISPSYS_DPI0_BASE, 0x1);
	DPI_OUTREG32(0, DISPSYS_DPI0_BASE + 0xE0, 0x404);
#endif

	DPI_REG[0] = (PDPI_REGS) (DISPSYS_DPI0_BASE);
	DPI_REG[1] = (PDPI_REGS) (DISPSYS_DPI1_BASE);

	ddp_dpi_power_on(DISP_MODULE_DPI0, cmdq);
	ddp_dpi_power_on(DISP_MODULE_DPI1, cmdq);


#if ENABLE_DPI_INTERRUPT
	for (i = 0; i < DPI_INTERFACE_NUM; i++) {
		DPI_OUTREGBIT(cmdq, DPI_REG_INTERRUPT, DPI_REG[i]->INT_ENABLE, VSYNC, 1);
	}

	disp_register_module_irq_callback(DISP_MODULE_DPI0, _DPI_InterruptHandler);
	disp_register_module_irq_callback(DISP_MODULE_DPI1, _DPI_InterruptHandler);

#endif
	for (i = 0; i < DPI_INTERFACE_NUM; i++) {
		DISPCHECK("dsi%d init finished\n", i);
	}

	return 0;
}

int ddp_dpi_deinit(DISP_MODULE_ENUM module, void *cmdq_handle)
{
	DISP_LOG_PRINT(ANDROID_LOG_WARN, "DPI", "ddp_dpi_deinit- %x\n", (unsigned int)cmdq_handle);

	ddp_dpi_stop(module, cmdq_handle);
	ddp_dpi_power_off(module, cmdq_handle);

	return 0;
}

int ddp_dpi_set_lcm_utils(DISP_MODULE_ENUM module, LCM_DRIVER *lcm_drv)
{
	DISPFUNC();
	LCM_UTIL_FUNCS *utils = NULL;

	if (lcm_drv == NULL) {
		DISP_LOG_PRINT(ANDROID_LOG_ERROR, "DPI", "lcm_drv is null!\n");
		return -1;
	}

	utils = &lcm_utils_dpi;

	utils->set_reset_pin = lcm_set_reset_pin;
	utils->udelay        = lcm_udelay;
	utils->mdelay        = lcm_mdelay;
	utils->send_cmd      = lcm_send_cmd,
	       utils->send_data     = lcm_send_data,

	              lcm_drv->set_util_funcs(utils);

	return 0;
}

int ddp_dpi_build_cmdq(DISP_MODULE_ENUM module, void *cmdq_trigger_handle, CMDQ_STATE state)
{
	return 0;
}

int ddp_dpi_dump(DISP_MODULE_ENUM module, int level)
{
	UINT32 i;
	dprintf(0,"---------- Start dump DPI registers ----------\n");

	for (i = 0; i <= 0x40; i += 4) {
		dprintf(0,"DPI+%04x : 0x%08x\n", i, INREG32(DISPSYS_DPI0_BASE + i));
	}

	for (i = 0x68; i <= 0x7C; i += 4) {
		dprintf(0,"DPI+%04x : 0x%08x\n", i, INREG32(DISPSYS_DPI0_BASE + i));
	}

	dprintf(0,"DPI+Color Bar : %04x : 0x%08x\n", 0xF00, INREG32(DISPSYS_DPI0_BASE + 0xF00));
#if 0
	dprintf(0,"DPI Addr IO Driving : 0x%08x\n", INREG32(DISPSYS_IO_DRIVING));

	dprintf(0,"DPI TVDPLL CON0 : 0x%08x\n",  INREG32(DDP_REG_TVDPLL_CON0));
	dprintf(0,"DPI TVDPLL CON1 : 0x%08x\n",  INREG32(DDP_REG_TVDPLL_CON1));

	dprintf(0,"DPI TVDPLL CON6 : 0x%08x\n",  INREG32(DDP_REG_TVDPLL_CON6));
	dprintf(0,"DPI MMSYS_CG_CON1:0x%08x\n",  INREG32(DISP_REG_CONFIG_MMSYS_CG_CON1));
#endif

	return 0;
}
/*
int ddp_dpi_ioctl(DISP_MODULE_ENUM module, void *cmdq_handle, unsigned int ioctl_cmd, unsigned long *params)
{
    DISPFUNC();

    int ret = 0;
    DDP_IOCTL_NAME ioctl = (DDP_IOCTL_NAME)ioctl_cmd;
    DISP_LOG_PRINT(ANDROID_LOG_DEBUG, "DPI", "DPI ioctl: %d \n", ioctl);

    switch(ioctl)
    {
        case DDP_DPI_FACTORY_TEST:
        {
            disp_ddp_path_config *config_info = (disp_ddp_path_config *)params;

            ddp_dpi_power_on(module, NULL);
            ddp_dpi_stop(module, NULL);
            ddp_dpi_config(module, config_info, NULL);
            ddp_dpi_EnableColorBar(module);

            ddp_dpi_trigger(module, NULL);
            ddp_dpi_start(module, NULL);
            ddp_dpi_dump(module, 1);
            break;
        }
        default:
            break;
    }

    return ret;
}
*/
static int ddp_dpi_clock_on(DISP_MODULE_ENUM module,void * handle)
{
	ddp_enable_module_clock(module);
	return 0;
}

static int ddp_dpi_clock_off(DISP_MODULE_ENUM module,void * handle)
{
	ddp_disable_module_clock(module);
	return 0;
}
DDP_MODULE_DRIVER ddp_driver_dpi0 = {
	.module        = DISP_MODULE_DPI0,
#if HDMI_MAIN_PATH_LK || HDMI_SUB_PATH_LK

	.init          = ddp_dpi_clock_on,
	.deinit        = ddp_dpi_clock_off,
#else
	.init          = ddp_dpi_init,
	.deinit        = ddp_dpi_deinit,
	.config        = ddp_dpi_config,
	.build_cmdq    = ddp_dpi_build_cmdq,
	.trigger       = ddp_dpi_trigger,
	.start         = ddp_dpi_start,
	.stop          = ddp_dpi_stop,
	.reset         = ddp_dpi_reset,
	.power_on      = ddp_dpi_power_on,
	.power_off     = ddp_dpi_power_off,
	.is_idle       = ddp_dpi_is_idle,
	.is_busy       = ddp_dpi_is_busy,
	.dump_info     = ddp_dpi_dump,
	.set_lcm_utils = ddp_dpi_set_lcm_utils,
	//.ioctl         = ddp_dpi_ioctl
#endif
};

DDP_MODULE_DRIVER ddp_driver_dpi1 = {
	.module        = DISP_MODULE_DPI1,
	.init          = ddp_dpi_init,
	.deinit        = ddp_dpi_deinit,
	.config        = ddp_dpi_config,
	.build_cmdq    = ddp_dpi_build_cmdq,
	.trigger       = ddp_dpi_trigger,
	.start         = ddp_dpi_start,
	.stop          = ddp_dpi_stop,
	.reset         = ddp_dpi_reset,
	.power_on      = ddp_dpi_power_on,
	.power_off     = ddp_dpi_power_off,
	.is_idle       = ddp_dpi_is_idle,
	.is_busy       = ddp_dpi_is_busy,
	.dump_info     = ddp_dpi_dump,
	.set_lcm_utils = ddp_dpi_set_lcm_utils,
	//.ioctl         = ddp_dpi_ioctl
};