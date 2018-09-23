#include <platform/ddp_info.h>
#include <platform/ddp_log.h>
#include <platform/ddp_path.h>
#include <platform/ddp_dump.h>

#include "platform/ddp_reg.h"


static char* ddp_signal_0(int bit)
{
	switch (bit) {
		case 31:
			return "gamma0__gamma_mout";
		case 30:
			return "path1_sel__path1_sout";
		case 29:
			return "split0__path1_sel2";
		case 28:
			return "split0__ufoe_sel1";
		case 27:
			return "ufoe_sel__ufoe0";
		case 26:
			return "path0_sout0__ufoe_sel0";
		case 25:
			return "path0_sout1__split0";
		case 24:
			return "aal_sel__aal0";
		case 23:
			return "color0_sout1__merge0";
		case 22:
			return "color0_sout0__aal_sel0";
		case 21:
			return "color1_sout0__gamma0";
		case 20:
			return "color1__color1_sout";
		case 19:
			return "color1_sout1__merge0";
		case 18:
			return "merge0__aal_sel1";
		case 17:
			return "rdma2__rdma2_sout";
		case 16:
			return "rdma2_sout1__dpi_sel2";
		case 15:
			return "rdma2_sout0__dsi1_sel2";
		case 14:
			return "dpi_sel__dpi0";
		case 13:
			return "split1__dsi1_sel0";
		case 12:
			return "dsi0_sel__dsi0";
		case 11:
			return "ufoe0__ufoe_mout";
		case 10:
			return "aal0__od";
		case 9 :
			return "path1_sout0__dsi0_sel2";
		case 8 :
			return "dsi1_sel__dsi1";
		case 7 :
			return "ovl1_mout0__color1_sel1";
		case 6 :
			return "color1_sel__color1";
		case 5 :
			return "ovl1__ovl1_mout";
		case 4 :
			return "ovl1_mout1__wdma1_sel0";
		case 3 :
			return "color0_sel__color0";
		case 2 :
			return "color0__color0_sout";
		case 1 :
			return "ovl0__ovl0_mout";
		case 0 :
			return "ovl0_mout1__wdma0_sel0";
		default:
			DDPERR("ddp_signal_0, unknown bit=%d \n", bit);
			return "unknown";
	}
}

static char* ddp_signal_1(int bit)
{
	switch (bit) {
		case 23:
			return "od__od_mout";
		case 22:
			return "ufoe_mout3__wdma0_sel2";
		case 21:
			return "rdma1_sout0__path1_sel0";
		case 20:
			return "gamma_mout1__path1_sel1";
		case 19:
			return "gamma_mout0__rdma1";
		case 18:
			return "wdma1_sel__wdma1";
		case 17:
			return "gamma_mout2__wdma1_sel1";
		case 16:
			return "rdma1_sout1__color1_sel0";
		case 15:
			return "rdma1__rdma1_sout";
		case 14:
			return "od_mout2__wdma0_sel1";
		case 13:
			return "wdma0_sel__wdma0";
		case 12:
			return "path0_sel__path0_sout";
		case 11:
			return "rdma0_sout0__path0_sel0";
		case 10:
			return "od_mout0__rdma0";
		case 9 :
			return "od_mout1__path0_sel1";
		case 8 :
			return "rdma0_sout1__color0_sel0";
		case 7 :
			return "ovl0_mout0__color0_sel1";
		case 6 :
			return "rdma0__rdma0_sout";
		case 5 :
			return "path1_sout1__dsi1_sel1";
		case 4 :
			return "path1_sout2__dpi_sel1";
		case 3 :
			return "ufoe_mout2__dpi_sel0";
		case 2 :
			return "ufoe_mout1__split1";
		case 1 :
			return "ufoe_mout0__dsi0_sel0";
		case 0 :
			return "split1__dsi0_sel1";
		default:
			DDPERR("ddp_signal_1, unknown bit=%d \n", bit);
			return "unknown";
	}
}

/*
example:
ovl0 -> ovl0_mout_ready=1 means engines after ovl_mout are ready for receiving data
        ovl0_mout_ready=0 means ovl0_mout can not receive data, maybe ovl0_mout or after engines config error
ovl0 -> ovl0_mout_valid=1 means engines before ovl0_mout is OK,
        ovl0_mout_valid=0 means ovl can not transfer data to ovl0_mout, means ovl0 or before engines are not ready.
*/
int ddp_check_signal(DDP_SCENARIO_ENUM scenario)
{
	unsigned int signal0 = 0;
	unsigned int signal1 = 0;
	unsigned int i = 0;
	char * scenario_name = ddp_get_scenario_name(scenario);
	DISP_MODULE_ENUM dst_module = ddp_get_dst_module(scenario);
	DDPMSG("ddp check signal: scenario=%s\n", scenario_name);
	switch (scenario) {
		case DDP_SCENARIO_PRIMARY_DISP   :
			signal0 = (1 << DDP_SIGNAL_OVL0__OVL0_MOUT) |
			          (1 << DDP_SIGNAL_COLOR0_SEL__COLOR0) |
			          (1 << DDP_SIGNAL_COLOR0__COLOR0_SOUT) |
			          (1 << DDP_SIGNAL_COLOR0_SOUT0__AAL_SEL0) |
			          (1 << DDP_SIGNAL_AAL_SEL__AAL0) |
			          (1 << DDP_SIGNAL_AAL0__OD) |
			          (1 << DDP_SIGNAL_PATH0_SOUT0__UFOE_SEL0) |
			          (1 << DDP_SIGNAL_UFOE_SEL__UFOE0) |
			          (1 << DDP_SIGNAL_UFOE0__UFOE_MOUT);
			signal1 = (1 << DDP_SIGNAL_OVL0_MOUT0__COLOR0_SEL1) |
			          (1 << DDP_SIGNAL_OD__OD_MOUT) |
			          (1 << DDP_SIGNAL_OD_MOUT0__RDMA0) |
			          (1 << DDP_SIGNAL_RDMA0__RDMA0_SOUT) |
			          (1 << DDP_SIGNAL_RDMA0_SOUT0__PATH0_SEL0);
			if (dst_module == DISP_MODULE_DSI0) {
				signal0 = signal0 |
				          (1 << DDP_SIGNAL_DSI0_SEL__DSI0);
				signal1 = signal1 |
				          (1 << DDP_SIGNAL_UFOE_MOUT0__DSI0_SEL0);
			}

			if (dst_module == DISP_MODULE_DSI1) {
				DDPMSG("No signal information about UFOE_MOUT TO DSI1\n");
			}

			if (dst_module == DISP_MODULE_DPI0) {
				signal0 = signal0 |
				          (1 << DDP_SIGNAL_DPI_SEL__DPI0);
				signal1 = signal1 |
				          (1 << DDP_SIGNAL_UFOE_MOUT2__DPI_SEL0);
			}
			break;

		case DDP_SCENARIO_PRIMARY_OVL_MEMOUT :
			signal0 = (1 << DDP_SIGNAL_OVL0__OVL0_MOUT) |
			          (1 << DDP_SIGNAL_OVL0_MOUT1__WDMA0_SEL0);
			signal1 = (1 << DDP_SIGNAL_WDMA0_SEL__WDMA0);
			break;

		case DDP_SCENARIO_PRIMARY_ALL    :
			signal0 = (1 << DDP_SIGNAL_OVL0__OVL0_MOUT) |
			          (1 << DDP_SIGNAL_OVL0_MOUT1__WDMA0_SEL0) |
			          (1 << DDP_SIGNAL_COLOR0_SEL__COLOR0) |
			          (1 << DDP_SIGNAL_COLOR0__COLOR0_SOUT) |
			          (1 << DDP_SIGNAL_COLOR0_SOUT0__AAL_SEL0) |
			          (1 << DDP_SIGNAL_AAL_SEL__AAL0) |
			          (1 << DDP_SIGNAL_AAL0__OD) |
			          (1 << DDP_SIGNAL_PATH0_SOUT0__UFOE_SEL0) |
			          (1 << DDP_SIGNAL_UFOE_SEL__UFOE0) |
			          (1 << DDP_SIGNAL_UFOE0__UFOE_MOUT); // todo: add dst module related signal
			signal1 = (1 << DDP_SIGNAL_OVL0_MOUT0__COLOR0_SEL1) |
			          (1 << DDP_SIGNAL_OD__OD_MOUT) |
			          (1 << DDP_SIGNAL_OD_MOUT0__RDMA0) |
			          (1 << DDP_SIGNAL_RDMA0__RDMA0_SOUT) |
			          (1 << DDP_SIGNAL_RDMA0_SOUT0__PATH0_SEL0) |
			          (1 << DDP_SIGNAL_WDMA0_SEL__WDMA0);
			break;

		case DDP_SCENARIO_SUB_DISP       :
			signal0 = (1 << DDP_SIGNAL_OVL1__OVL1_MOUT) |
			          (1 << DDP_SIGNAL_OVL1_MOUT0__COLOR1_SEL1) |
			          (1 << DDP_SIGNAL_COLOR1_SEL__COLOR1) |
			          (1 << DDP_SIGNAL_COLOR1__COLOR1_SOUT) |
			          (1 << DDP_SIGNAL_COLOR1_SOUT0__GAMMA0) |
			          (1 << DDP_SIGNAL_GAMMA0__GAMMA_MOUT) |
			          (1 << DDP_SIGNAL_PATH1_SEL__PATH1_SOUT); // todo: add dst module related signal
			signal1 = (1 << DDP_SIGNAL_GAMMA_MOUT0__RDMA1) |
			          (1 << DDP_SIGNAL_RDMA1__RDMA1_SOUT) |
			          (1 << DDP_SIGNAL_RDMA1_SOUT0__PATH1_SEL0);
			break;

		case DDP_SCENARIO_SUB_OVL_MEMOUT     :
			signal0 = (1 << DDP_SIGNAL_OVL1__OVL1_MOUT) |
			          (1 << DDP_SIGNAL_OVL1_MOUT1__WDMA1_SEL0);
			signal1 = (1 << DDP_SIGNAL_WDMA1_SEL__WDMA1);
			break;

		case DDP_SCENARIO_SUB_ALL        :
			signal0 = (1 << DDP_SIGNAL_OVL1__OVL1_MOUT) |
			          (1 << DDP_SIGNAL_OVL1_MOUT0__COLOR1_SEL1) |
			          (1 << DDP_SIGNAL_OVL1_MOUT1__WDMA1_SEL0) |
			          (1 << DDP_SIGNAL_COLOR1_SEL__COLOR1) |
			          (1 << DDP_SIGNAL_COLOR1__COLOR1_SOUT) |
			          (1 << DDP_SIGNAL_COLOR1_SOUT0__GAMMA0) |
			          (1 << DDP_SIGNAL_GAMMA0__GAMMA_MOUT) |
			          (1 << DDP_SIGNAL_PATH1_SEL__PATH1_SOUT); // todo: add dst module related signal
			signal1 = (1 << DDP_SIGNAL_GAMMA_MOUT0__RDMA1) |
			          (1 << DDP_SIGNAL_RDMA1__RDMA1_SOUT) |
			          (1 << DDP_SIGNAL_RDMA1_SOUT0__PATH1_SEL0) |
			          (1 << DDP_SIGNAL_WDMA1_SEL__WDMA1);
			break;
		case DDP_SCENARIO_PRIMARY_RDMA0_DISP :
			signal0 = (1 << DDP_SIGNAL_PATH0_SOUT0__UFOE_SEL0) |
			          (1 << DDP_SIGNAL_UFOE_SEL__UFOE0) |
			          (1 << DDP_SIGNAL_UFOE0__UFOE_MOUT);
			signal1 = (1 << DDP_SIGNAL_RDMA0__RDMA0_SOUT) |
			          (1 << DDP_SIGNAL_RDMA0_SOUT0__PATH0_SEL0);
			if (dst_module == DISP_MODULE_DSI0) {
				signal0 = signal0 |
				          (1 << DDP_SIGNAL_DSI0_SEL__DSI0);
				signal1 = signal1 |
				          (1 << DDP_SIGNAL_UFOE_MOUT0__DSI0_SEL0);
			}
			if (dst_module == DISP_MODULE_DSI1) {
				DDPMSG("No signal information about UFOE_MOUT TO DSI1\n");
			}

			if (dst_module == DISP_MODULE_DPI0) {
				signal0 = signal0 |
				          (1 << DDP_SIGNAL_DPI_SEL__DPI0);
				signal1 = signal1 |
				          (1 << DDP_SIGNAL_UFOE_MOUT2__DPI_SEL0);
			}
			break;
		case DDP_SCENARIO_SUB_RDMA1_DISP :
			signal0 = 0;
			signal1 = (1 << DDP_SIGNAL_RDMA1__RDMA1_SOUT) |
			          (1 << DDP_SIGNAL_RDMA1_SOUT0__PATH1_SEL0);

			if (dst_module == DISP_MODULE_DSI0) {
				signal0 = signal0 |
				          (1 << DDP_SIGNAL_PATH1_SEL__PATH1_SOUT) |
				          (1 << DDP_SIGNAL_PATH1_SOUT0__DSI0_SEL2) |
				          (1 << DDP_SIGNAL_DSI0_SEL__DSI0);
			}
			if (dst_module == DISP_MODULE_DSI1) {
				signal0 = signal0 |
				          (1 << DDP_SIGNAL_PATH1_SEL__PATH1_SOUT) |
				          (1 << DDP_SIGNAL_DSI1_SEL__DSI1);
				signal1 = signal1 |
				          (1 << DDP_SIGNAL_PATH1_SOUT1__DSI1_SEL1);
			}
			if (dst_module == DISP_MODULE_DPI0) {
				signal0 = signal0 |
				          (1 << DDP_SIGNAL_PATH1_SEL__PATH1_SOUT) |
				          (1 << DDP_SIGNAL_DPI_SEL__DPI0);
				signal1 = signal1 |
				          (1 << DDP_SIGNAL_PATH1_SOUT2__DPI_SEL1);
			}
			break;
		case DDP_SCENARIO_SUB_RDMA2_DISP :
			break;

		case DDP_SCENARIO_PRIMARY_OD_MEMOUT    :
			signal0 = (1 << DDP_SIGNAL_OVL0__OVL0_MOUT) |
			          (1 << DDP_SIGNAL_COLOR0_SEL__COLOR0) |
			          (1 << DDP_SIGNAL_COLOR0__COLOR0_SOUT) |
			          (1 << DDP_SIGNAL_COLOR0_SOUT0__AAL_SEL0) |
			          (1 << DDP_SIGNAL_AAL_SEL__AAL0) |
			          (1 << DDP_SIGNAL_AAL0__OD);
			signal1 = (1 << DDP_SIGNAL_OVL0_MOUT0__COLOR0_SEL1) |
			          (1 << DDP_SIGNAL_OD__OD_MOUT) |
			          (1 << DDP_SIGNAL_OD_MOUT2__WDMA0_SEL1) |
			          (1 << DDP_SIGNAL_WDMA0_SEL__WDMA0);
			break;

		default:
			DDPERR("ddp_check_signal, unknown scenario=%d\n", scenario);
	}

	for (i = 0; i < 32; i++) {
		if ((signal0 & (1 << i)) != 0 &&
		        (DISP_REG_GET(DISPSYS_CONFIG_BASE + 0x8b0) & (1 << i)) == 0) {
			DDPMSG("%s not valid on scenario %s, address 0x%x, value 0x%x\n",
			       ddp_signal_0(i),scenario_name,DISPSYS_CONFIG_BASE + 0x8b0,DISP_REG_GET(DISPSYS_CONFIG_BASE + 0x8b0));
		}
	}
	for (i = 0; i < 24; i++) {
		if ((signal1 & (1 << i)) != 0 &&
		        (DISP_REG_GET(DISPSYS_CONFIG_BASE + 0x8b4) & (1 << i)) == 0) {
			DDPMSG("%s not valid on scenario %s, address 0x%x, value 0x%x\n",
			       ddp_signal_1(i),scenario_name,DISPSYS_CONFIG_BASE + 0x8b4,DISP_REG_GET(DISPSYS_CONFIG_BASE + 0x8b4));
		}
	}

	for (i = 0; i < 32; i++) {
		if ((signal0 & (1 << i)) != 0 &&
		        (DISP_REG_GET(DISPSYS_CONFIG_BASE + 0x8b8) & (1 << i)) == 0) {
			DDPMSG("%s not ready on scenario %s, address 0x%x, value 0x%x\n",
			       ddp_signal_0(i),scenario_name,DISPSYS_CONFIG_BASE + 0x8b8,DISP_REG_GET(DISPSYS_CONFIG_BASE + 0x8b8));
		}
	}
	for (i = 0; i < 24; i++) {
		if ((signal1 & (1 << i)) != 0 &&
		        (DISP_REG_GET(DISPSYS_CONFIG_BASE + 0x8bc) & (1 << i)) == 0) {
			DDPMSG(" %s not ready on scenario %s, address 0x%x, value 0x%x\n",
			       ddp_signal_1(i),scenario_name,DISPSYS_CONFIG_BASE + 0x8bc,DISP_REG_GET(DISPSYS_CONFIG_BASE + 0x8bc));
		}
	}
	return 0;
}

static char* ddp_get_mutex_module_name(unsigned int bit)
{
	switch (bit) {
		case 11:
			return "ovl0";
		case 12:
			return "ovl1";
		case 13:
			return "rdma0";
		case 14:
			return "rdma1";
		case 15:
			return "rdma2";
		case 16:
			return "wdma0";
		case 17:
			return "wdma1";
		case 18:
			return "color0";
		case 19:
			return "color1";
		case 20:
			return "aal";
		case 21:
			return "gamma";
		case 22:
			return "ufoe";
		case 23:
			return "pwm0";
		case 24:
			return "pwm1";
		case 25:
			return "od";
		default:
			return "mutex-unknown";
	}
}

char *ddp_ovl_get_status(unsigned int status)
{
	switch (status) {
		case 0x1:
			return "idle";
		case 0x2:
			return "wait_SOF";
		case 0x4:
			return "prepare";
		case 0x8:
			return "reg_update";
		case 0x10:
			return "eng_clr";
		case 0x20:
			return "processing";
		case 0x40:
			return "s_wait_w_rst";
		case 0x80:
			return "s_w_rst";
		case 0x100:
			return "h_wait_w_rst";
		case 0x200:
			return "h_w_rst";
		default:
			return "unknown";
	}
}

char* ddp_wdma_get_status(unsigned int status)
{
	switch (status) {
		case 0x1:
			return "idle";
		case 0x2:
			return "clear";
		case 0x4:
			return "prepare";
		case 0x8:
			return "prepare";
		case 0x10:
			return "data_running";
		case 0x20:
			return "eof_wait";
		case 0x40:
			return "soft_reset_wait";
		case 0x80:
			return "eof_done";
		case 0x100:
			return "soft_reset_done";
		case 0x200:
			return "frame_complete";
		default:
			return "unknown";
	}

}

char* ddp_get_fmt_name(DISP_MODULE_ENUM module, unsigned int fmt)
{
	if (module==DISP_MODULE_WDMA0 || module==DISP_MODULE_WDMA1) {
		switch (fmt) {
			case 0:
				return "rgb565";
			case 1:
				return "rgb888";
			case 2:
				return "rgba8888";
			case 3:
				return "argb8888";
			case 4:
				return "uyvy";
			case 5:
				return "yuy2";
			case 7:
				return "y-only";
			case 8:
				return "iyuv";
			case 12:
				return "nv12";
			default:
				DDPMSG("ddp_get_fmt_name, unknown fmt=%d, module=%d \n", fmt, module);
				return "unknown";
		}
	} else if (module==DISP_MODULE_OVL0 || module==DISP_MODULE_OVL1) {
		switch (fmt) {
			case 0:
				return "rgb565";
			case 1:
				return "rgb888";
			case 2:
				return "rgba8888";
			case 3:
				return "argb8888";
			case 4:
				return "uyvy";
			case 5:
				return "yuyv";
			default:
				DDPMSG("ddp_get_fmt_name, unknown fmt=%d, module=%d \n", fmt, module);
				return "unknown";
		}
	} else if (module==DISP_MODULE_RDMA0 || module==DISP_MODULE_RDMA1 || module==DISP_MODULE_RDMA2) { // todo: confirm with designers
		switch (fmt) {
			case 0:
				return "rgb565";
			case 1:
				return "rgb888";
			case 2:
				return "rgba8888";
			case 3:
				return "argb8888";
			case 4:
				return "uyvy";
			case 5:
				return "yuyv";
			default:
				DDPMSG("ddp_get_fmt_name, unknown fmt=%d, module=%d \n", fmt, module);
				return "unknown";
		}
	} else if (module==DISP_MODULE_MUTEX) {
		switch (fmt) {
			case 0:
				return "single";
			case 1:
				return "dsi0_vdo";
			case 2:
				return "dsi1_vdo";
			case 3:
				return "dpi";
			default:
				DDPMSG("ddp_get_fmt_name, unknown fmt=%d, module=%d \n", fmt, module);
				return "unknown";
		}
	} else {
		DDPMSG("ddp_get_fmt_name, unknown module=%d \n", module);
	}

	return "unknown";
}

static char* ddp_clock_0(int bit)
{
	switch (bit) {
		case 0:
			return "smi_common";
		case 1:
			return "smi_larb0";
		case 14:
			return "fake_eng";
		case 15:
			return "mutex_32k";
		case 16 :
			return "ovl0";
		case 17 :
			return "ovl1";
		case 18 :
			return "rdma0";
		case 19 :
			return "rdma1";
		case 20 :
			return "rdma2";
		case 21 :
			return "wdma0";
		case 22 :
			return "wdma1";
		case 23 :
			return "color0";
		case 24 :
			return "color1";
		case 25 :
			return "aal";
		case 26 :
			return "gamma";
		case 27 :
			return "ufoe";
		case 28 :
			return "split0";
		case 29 :
			return "split1";
		case 30 :
			return "merge";
		case 31 :
			return "od";
		default :
			return "";
	}
}

static char* ddp_clock_1(int bit)
{
	switch (bit) {
		case 0:
			return "pwm0_mm";
		case 1:
			return "pwm0_26m";
		case 2:
			return "pwm1_mm";
		case 3:
			return "pwm1_26m";
		case 4:
			return "dsi0_eng";
		case 5:
			return "dsi0_dig";
		case 6:
			return "dsi1_eng";
		case 7:
			return "dsi1_dig";
		case 8:
			return "dpi_pixel";
		case 9:
			return "dpi_eng";
		default :
			return "";
	}

}

int ddp_dump_info(DISP_MODULE_ENUM module)
{
	//unsigned int size;
	//unsigned int reg_base;
	unsigned int index;
	unsigned int i=0;
	unsigned int j=0;

	switch (module) {
		case DISP_MODULE_WDMA0:
		case DISP_MODULE_WDMA1:
			if (DISP_MODULE_WDMA0==module) {
				index = 0;
			} else if (DISP_MODULE_WDMA1==module) {
				index = 1;
			}
			DDPMSG("wdma%d: en=%d, w=%d, h=%d, clip=(%d, %d, %d, %d), pitch=(%d,%d), addr=(0x%x,0x%x,0x%x), fmt=%s \n",
			       index,
			       DISP_REG_GET(DISP_REG_WDMA_EN+DISP_INDEX_OFFSET*index),
			       DISP_REG_GET(DISP_REG_WDMA_SRC_SIZE+DISP_INDEX_OFFSET*index)&0x3fff,
			       (DISP_REG_GET(DISP_REG_WDMA_SRC_SIZE+DISP_INDEX_OFFSET*index)>>16)&0x3fff,
			       DISP_REG_GET(DISP_REG_WDMA_CLIP_COORD+DISP_INDEX_OFFSET*index)&0x3fff,
			       (DISP_REG_GET(DISP_REG_WDMA_CLIP_COORD+DISP_INDEX_OFFSET*index)>>16)&0x3fff,
			       DISP_REG_GET(DISP_REG_WDMA_CLIP_SIZE+DISP_INDEX_OFFSET*index)&0x3fff,
			       (DISP_REG_GET(DISP_REG_WDMA_CLIP_SIZE+DISP_INDEX_OFFSET*index)>>16)&0x3fff,
			       DISP_REG_GET(DISP_REG_WDMA_DST_W_IN_BYTE+DISP_INDEX_OFFSET*index),
			       DISP_REG_GET(DISP_REG_WDMA_DST_UV_PITCH+DISP_INDEX_OFFSET*index),
			       DISP_REG_GET(DISP_REG_WDMA_DST_ADDR0+DISP_INDEX_OFFSET*index),
			       DISP_REG_GET(DISP_REG_WDMA_DST_ADDR1+DISP_INDEX_OFFSET*index),
			       DISP_REG_GET(DISP_REG_WDMA_DST_ADDR2+DISP_INDEX_OFFSET*index),
			       ddp_get_fmt_name(DISP_MODULE_WDMA0, (DISP_REG_GET(DISP_REG_WDMA_CFG+DISP_INDEX_OFFSET*index)>>4)&0xf));
			DDPMSG("wdma%d: status=%s, in_req=%d, in_ack=%d, total_pix=%d, output_pixel=(l:%d, p:%d), input_pixel=(l:%d, p:%d) \n",
			       index,
			       ddp_wdma_get_status(DISP_REG_GET_FIELD(FLOW_CTRL_DBG_FLD_WDMA_STA_FLOW_CTRL, DISP_REG_WDMA_FLOW_CTRL_DBG+DISP_INDEX_OFFSET*index)),
			       DISP_REG_GET_FIELD(EXEC_DBG_FLD_WDMA_IN_REQ, DISP_REG_WDMA_FLOW_CTRL_DBG+DISP_INDEX_OFFSET*index),
			       DISP_REG_GET_FIELD(EXEC_DBG_FLD_WDMA_IN_ACK, DISP_REG_WDMA_FLOW_CTRL_DBG+DISP_INDEX_OFFSET*index),
			       (DISP_REG_GET(DISP_REG_WDMA_SRC_SIZE+DISP_INDEX_OFFSET*index)&0x3fff)*((DISP_REG_GET(DISP_REG_WDMA_SRC_SIZE+DISP_INDEX_OFFSET*index)>>16)&0x3fff),
			       (DISP_REG_GET(DISP_REG_WDMA_EXEC_DBG+DISP_INDEX_OFFSET*index)>>16)&0xffff,
			       DISP_REG_GET(DISP_REG_WDMA_EXEC_DBG+DISP_INDEX_OFFSET*index)&0xffff,
			       (DISP_REG_GET(DISP_REG_WDMA_CT_DBG+DISP_INDEX_OFFSET*index)>>16)&0xffff,
			       DISP_REG_GET(DISP_REG_WDMA_CT_DBG+DISP_INDEX_OFFSET*index)&0xffff);
			break;
		case DISP_MODULE_RDMA0:
		case DISP_MODULE_RDMA1:
		case DISP_MODULE_RDMA2:
			if (DISP_MODULE_RDMA0==module) {
				index = 0;
			} else if (DISP_MODULE_RDMA1==module) {
				index = 1;
			} else if (DISP_MODULE_RDMA2==module) {
				index = 2;
			}
			DDPMSG("rdma%d: en=%d, w=%d, h=%d, pitch=%d, addr=0x%x, fmt=%s, fifo_min=%d \n",
			       index,
			       DISP_REG_GET(DISP_REG_RDMA_GLOBAL_CON+DISP_INDEX_OFFSET*index)&0x1,
			       DISP_REG_GET(DISP_REG_RDMA_SIZE_CON_0+DISP_INDEX_OFFSET*index)&0xfff,
			       DISP_REG_GET(DISP_REG_RDMA_SIZE_CON_1+DISP_INDEX_OFFSET*index)&0xfffff,
			       DISP_REG_GET(DISP_REG_RDMA_MEM_SRC_PITCH+DISP_INDEX_OFFSET*index),
			       DISP_REG_GET(DISP_REG_RDMA_MEM_START_ADDR+DISP_INDEX_OFFSET*index),
			       ((DISP_REG_GET(DISP_REG_RDMA_GLOBAL_CON+DISP_INDEX_OFFSET*index)&0x2)==0) ? "rgb888" : ddp_get_fmt_name(DISP_MODULE_RDMA0, (DISP_REG_GET(DISP_REG_RDMA_MEM_CON+DISP_INDEX_OFFSET*index)>>4)&0xf),
			       DISP_REG_GET(DISP_REG_RDMA_FIFO_LOG+DISP_INDEX_OFFSET*index));
			break;

		/* Dump OVL Reg */
		case DISP_MODULE_OVL0:
		case DISP_MODULE_OVL1:
			if (DISP_MODULE_OVL0==module) {
				index = 0;
			} else if (DISP_MODULE_OVL1==module) {
				index = 1;
			}
			DDPMSG("ovl%d: en=%d, layer(%d,%d,%d,%d), bg_w=%d, bg_h=%d, status=%s, ovl_rdy=%d, next_eng_rdy=%d, cur_x=%d, cur_y=%d \n",
			       index,
			       DISP_REG_GET(DISP_REG_OVL_EN+DISP_INDEX_OFFSET*index),
			       DISP_REG_GET(DISP_REG_OVL_SRC_CON+DISP_INDEX_OFFSET*index)&0x1,
			       (DISP_REG_GET(DISP_REG_OVL_SRC_CON+DISP_INDEX_OFFSET*index)>>1)&0x1,
			       (DISP_REG_GET(DISP_REG_OVL_SRC_CON+DISP_INDEX_OFFSET*index)>>2)&0x1,
			       (DISP_REG_GET(DISP_REG_OVL_SRC_CON+DISP_INDEX_OFFSET*index)>>3)&0x1,
			       DISP_REG_GET(DISP_REG_OVL_ROI_SIZE+DISP_INDEX_OFFSET*index)&0xfff,
			       (DISP_REG_GET(DISP_REG_OVL_ROI_SIZE+DISP_INDEX_OFFSET*index)>>16)&0xfff,
			       ddp_ovl_get_status(DISP_REG_GET_FIELD(FLOW_CTRL_DBG_FLD_FSM_STATE, DISP_REG_OVL_FLOW_CTRL_DBG+DISP_INDEX_OFFSET*index)),
			       DISP_REG_GET_FIELD(FLOW_CTRL_DBG_FLD_OUT_VALID, DISP_REG_OVL_FLOW_CTRL_DBG+DISP_INDEX_OFFSET*index),
			       DISP_REG_GET_FIELD(FLOW_CTRL_DBG_FLD_OUT_READY, DISP_REG_OVL_FLOW_CTRL_DBG+DISP_INDEX_OFFSET*index),
			       DISP_REG_GET_FIELD(ADDCON_DBG_FLD_ROI_X, DISP_REG_OVL_ADDCON_DBG+DISP_INDEX_OFFSET*index),
			       DISP_REG_GET_FIELD(ADDCON_DBG_FLD_ROI_Y, DISP_REG_OVL_ADDCON_DBG+DISP_INDEX_OFFSET*index));

			if (DISP_REG_GET(DISP_REG_OVL_SRC_CON+DISP_INDEX_OFFSET*index)&0x1) {
				DDPMSG("layer=%d, w=%d, h=%d, x=%d, y=%d, pitch=%d, addr=0x%x, fmt=%s \n",
				       0,
				       DISP_REG_GET(DISP_REG_OVL_L0_SRC_SIZE+DISP_INDEX_OFFSET*index)&0xfff,
				       (DISP_REG_GET(DISP_REG_OVL_L0_SRC_SIZE+DISP_INDEX_OFFSET*index)>>16)&0xfff,
				       DISP_REG_GET(DISP_REG_OVL_L0_OFFSET+DISP_INDEX_OFFSET*index)&0xfff,
				       (DISP_REG_GET(DISP_REG_OVL_L0_OFFSET+DISP_INDEX_OFFSET*index)>>16)&0xfff,
				       DISP_REG_GET(DISP_REG_OVL_L0_PITCH+DISP_INDEX_OFFSET*index),
				       DISP_REG_GET(DISP_REG_OVL_L0_ADDR+DISP_INDEX_OFFSET*index),
				       ddp_get_fmt_name(DISP_MODULE_OVL0, (DISP_REG_GET(DISP_REG_OVL_L0_CON+DISP_INDEX_OFFSET*index)>>12)&0xf));
				DDPMSG("smi_greq_request=%d, rdy_to_transfer=%d, next_eng_rdy=%d, smi_busy=%d, out_data=0x%x, rst_cs=%d, smi_layer_request=%d \n",
				       DISP_REG_GET_FIELD(RDMA0_DBG_FLD_RDMA0_SMI_GREQ, DISP_REG_OVL_RDMA0_DBG+DISP_INDEX_OFFSET*index),
				       DISP_REG_GET_FIELD(RDMA0_DBG_FLD_RDMA0_SMI_BUSY, DISP_REG_OVL_RDMA0_DBG+DISP_INDEX_OFFSET*index),
				       DISP_REG_GET_FIELD(RDMA0_DBG_FLD_RDMA0_OUT_VALID, DISP_REG_OVL_RDMA0_DBG+DISP_INDEX_OFFSET*index),
				       DISP_REG_GET_FIELD(RDMA0_DBG_FLD_RDMA0_OUT_READY, DISP_REG_OVL_RDMA0_DBG+DISP_INDEX_OFFSET*index),
				       DISP_REG_GET_FIELD(RDMA0_DBG_FLD_RDMA0_OUT_DATA, DISP_REG_OVL_RDMA0_DBG+DISP_INDEX_OFFSET*index),
				       DISP_REG_GET_FIELD(RDMA0_DBG_FLD_RDMA0_WRAM_RST_CS, DISP_REG_OVL_RDMA0_DBG+DISP_INDEX_OFFSET*index),
				       DISP_REG_GET_FIELD(RDMA0_DBG_FLD_RDMA0_LAYER_GREQ, DISP_REG_OVL_RDMA0_DBG+DISP_INDEX_OFFSET*index));
			}
			if (DISP_REG_GET(DISP_REG_OVL_SRC_CON+DISP_INDEX_OFFSET*index)&0x2) {
				DDPMSG("layer=%d, w=%d, h=%d, x=%d, y=%d, pitch=%d, addr=0x%x, fmt=%s \n",
				       1,
				       DISP_REG_GET(DISP_REG_OVL_L1_SRC_SIZE+DISP_INDEX_OFFSET*index)&0xfff,
				       (DISP_REG_GET(DISP_REG_OVL_L1_SRC_SIZE+DISP_INDEX_OFFSET*index)>>16)&0xfff,
				       DISP_REG_GET(DISP_REG_OVL_L1_OFFSET+DISP_INDEX_OFFSET*index)&0xfff,
				       (DISP_REG_GET(DISP_REG_OVL_L1_OFFSET+DISP_INDEX_OFFSET*index)>>16)&0xfff,
				       DISP_REG_GET(DISP_REG_OVL_L1_PITCH+DISP_INDEX_OFFSET*index),
				       DISP_REG_GET(DISP_REG_OVL_L1_ADDR+DISP_INDEX_OFFSET*index),
				       ddp_get_fmt_name(DISP_MODULE_OVL0, (DISP_REG_GET(DISP_REG_OVL_L1_CON+DISP_INDEX_OFFSET*index)>>12)&0xf));
				DDPMSG("smi_greq_request=%d, rdy_to_transfer=%d, next_eng_rdy=%d, smi_busy=%d, out_data=0x%x, rst_cs=%d, smi_layer_request=%d \n",
				       DISP_REG_GET_FIELD(RDMA1_DBG_FLD_RDMA1_SMI_GREQ, DISP_REG_OVL_RDMA1_DBG+DISP_INDEX_OFFSET*index),
				       DISP_REG_GET_FIELD(RDMA1_DBG_FLD_RDMA1_SMI_BUSY, DISP_REG_OVL_RDMA1_DBG+DISP_INDEX_OFFSET*index),
				       DISP_REG_GET_FIELD(RDMA1_DBG_FLD_RDMA1_OUT_VALID, DISP_REG_OVL_RDMA1_DBG+DISP_INDEX_OFFSET*index),
				       DISP_REG_GET_FIELD(RDMA1_DBG_FLD_RDMA1_OUT_READY, DISP_REG_OVL_RDMA1_DBG+DISP_INDEX_OFFSET*index),
				       DISP_REG_GET_FIELD(RDMA1_DBG_FLD_RDMA1_OUT_DATA, DISP_REG_OVL_RDMA1_DBG+DISP_INDEX_OFFSET*index),
				       DISP_REG_GET_FIELD(RDMA1_DBG_FLD_RDMA1_WRAM_RST_CS, DISP_REG_OVL_RDMA1_DBG+DISP_INDEX_OFFSET*index),
				       DISP_REG_GET_FIELD(RDMA1_DBG_FLD_RDMA1_LAYER_GREQ, DISP_REG_OVL_RDMA1_DBG+DISP_INDEX_OFFSET*index));
			}
			if (DISP_REG_GET(DISP_REG_OVL_SRC_CON+DISP_INDEX_OFFSET*index)&0x4) {
				DDPMSG("layer=%d, w=%d, h=%d, x=%d, y=%d, pitch=%d, addr=0x%x, fmt=%s \n",
				       2,
				       DISP_REG_GET(DISP_REG_OVL_L2_SRC_SIZE+DISP_INDEX_OFFSET*index)&0xfff,
				       (DISP_REG_GET(DISP_REG_OVL_L2_SRC_SIZE+DISP_INDEX_OFFSET*index)>>16)&0xfff,
				       DISP_REG_GET(DISP_REG_OVL_L2_OFFSET+DISP_INDEX_OFFSET*index)&0xfff,
				       (DISP_REG_GET(DISP_REG_OVL_L2_OFFSET+DISP_INDEX_OFFSET*index)>>16)&0xfff,
				       DISP_REG_GET(DISP_REG_OVL_L2_PITCH+DISP_INDEX_OFFSET*index),
				       DISP_REG_GET(DISP_REG_OVL_L2_ADDR+DISP_INDEX_OFFSET*index),
				       ddp_get_fmt_name(DISP_MODULE_OVL0, (DISP_REG_GET(DISP_REG_OVL_L2_CON+DISP_INDEX_OFFSET*index)>>12)&0xf));
				DDPMSG("smi_greq_request=%d, rdy_to_transfer=%d, next_eng_rdy=%d, smi_busy=%d, out_data=0x%x, rst_cs=%d, smi_layer_request=%d \n",
				       DISP_REG_GET_FIELD(RDMA2_DBG_FLD_RDMA2_SMI_GREQ, DISP_REG_OVL_RDMA2_DBG+DISP_INDEX_OFFSET*index),
				       DISP_REG_GET_FIELD(RDMA2_DBG_FLD_RDMA2_SMI_BUSY, DISP_REG_OVL_RDMA2_DBG+DISP_INDEX_OFFSET*index),
				       DISP_REG_GET_FIELD(RDMA2_DBG_FLD_RDMA2_OUT_VALID, DISP_REG_OVL_RDMA2_DBG+DISP_INDEX_OFFSET*index),
				       DISP_REG_GET_FIELD(RDMA2_DBG_FLD_RDMA2_OUT_READY, DISP_REG_OVL_RDMA2_DBG+DISP_INDEX_OFFSET*index),
				       DISP_REG_GET_FIELD(RDMA2_DBG_FLD_RDMA2_OUT_DATA, DISP_REG_OVL_RDMA2_DBG+DISP_INDEX_OFFSET*index),
				       DISP_REG_GET_FIELD(RDMA2_DBG_FLD_RDMA2_WRAM_RST_CS, DISP_REG_OVL_RDMA2_DBG+DISP_INDEX_OFFSET*index),
				       DISP_REG_GET_FIELD(RDMA2_DBG_FLD_RDMA2_LAYER_GREQ, DISP_REG_OVL_RDMA2_DBG+DISP_INDEX_OFFSET*index));
			}
			if (DISP_REG_GET(DISP_REG_OVL_SRC_CON+DISP_INDEX_OFFSET*index)&0x8) {
				DDPMSG("layer=%d, w=%d, h=%d, x=%d, y=%d, pitch=%d, addr=0x%x, fmt=%s \n",
				       3,
				       DISP_REG_GET(DISP_REG_OVL_L3_SRC_SIZE+DISP_INDEX_OFFSET*index)&0xfff,
				       (DISP_REG_GET(DISP_REG_OVL_L3_SRC_SIZE+DISP_INDEX_OFFSET*index)>>16)&0xfff,
				       DISP_REG_GET(DISP_REG_OVL_L3_OFFSET+DISP_INDEX_OFFSET*index)&0xfff,
				       (DISP_REG_GET(DISP_REG_OVL_L3_OFFSET+DISP_INDEX_OFFSET*index)>>16)&0xfff,
				       DISP_REG_GET(DISP_REG_OVL_L3_PITCH+DISP_INDEX_OFFSET*index),
				       DISP_REG_GET(DISP_REG_OVL_L3_ADDR+DISP_INDEX_OFFSET*index),
				       ddp_get_fmt_name(DISP_MODULE_OVL0, (DISP_REG_GET(DISP_REG_OVL_L3_CON+DISP_INDEX_OFFSET*index)>>12)&0xf));
				DDPMSG("smi_greq_request=%d, rdy_to_transfer=%d, next_eng_rdy=%d, smi_busy=%d, out_data=0x%x, rst_cs=%d, smi_layer_request=%d \n",
				       DISP_REG_GET_FIELD(RDMA3_DBG_FLD_RDMA3_SMI_GREQ, DISP_REG_OVL_RDMA3_DBG+DISP_INDEX_OFFSET*index),
				       DISP_REG_GET_FIELD(RDMA3_DBG_FLD_RDMA3_SMI_BUSY, DISP_REG_OVL_RDMA3_DBG+DISP_INDEX_OFFSET*index),
				       DISP_REG_GET_FIELD(RDMA3_DBG_FLD_RDMA3_OUT_VALID, DISP_REG_OVL_RDMA3_DBG+DISP_INDEX_OFFSET*index),
				       DISP_REG_GET_FIELD(RDMA3_DBG_FLD_RDMA3_OUT_READY, DISP_REG_OVL_RDMA3_DBG+DISP_INDEX_OFFSET*index),
				       DISP_REG_GET_FIELD(RDMA3_DBG_FLD_RDMA3_OUT_DATA, DISP_REG_OVL_RDMA3_DBG+DISP_INDEX_OFFSET*index),
				       DISP_REG_GET_FIELD(RDMA3_DBG_FLD_RDMA3_WRAM_RST_CS, DISP_REG_OVL_RDMA3_DBG+DISP_INDEX_OFFSET*index),
				       DISP_REG_GET_FIELD(RDMA3_DBG_FLD_RDMA3_LAYER_GREQ, DISP_REG_OVL_RDMA3_DBG+DISP_INDEX_OFFSET*index));
			}
			break;
		case DISP_MODULE_GAMMA:
			DDPMSG("gamma: en=%d, w=%d, h=%d, in_p=%d, in_l=%d, out_p=%d, out_l=%d \n",
			       DISP_REG_GET(DISP_REG_GAMMA_EN),
			       (DISP_REG_GET(DISP_REG_GAMMA_SIZE)>>16)&0x1fff,
			       DISP_REG_GET(DISP_REG_GAMMA_SIZE)&0x1fff,
			       DISP_REG_GET(DISP_REG_GAMMA_INPUT_COUNT)&0x1fff,
			       (DISP_REG_GET(DISP_REG_GAMMA_INPUT_COUNT)>>16)&0x1fff,
			       DISP_REG_GET(DISP_REG_GAMMA_OUTPUT_COUNT)&0x1fff,
			       (DISP_REG_GET(DISP_REG_GAMMA_OUTPUT_COUNT)>>16)&0x1fff );
			break;
		case DISP_MODULE_DPI0:
			DDPMSG("DPI0: \n");
			break;

		/* Dump CONFIG Reg */
		case DISP_MODULE_CONFIG:
			// print whole data path
			DDPMSG("clock:");
			{
				unsigned int i;
				unsigned int reg;

				reg = DISP_REG_GET(DISP_REG_CONFIG_MMSYS_CG_CON0);
				for (i=0; i<2; i++) {
					if ((reg&(1<<i))==0)
						dprintf(0, "%s, ", ddp_clock_0(i));
				}
				for (i=14; i<32; i++) {
					if ((reg&(1<<i))==0)
						dprintf(0, "%s, ", ddp_clock_0(i));
				}
				reg = DISP_REG_GET(DISP_REG_CONFIG_MMSYS_CG_CON1);
				for (i=0; i<9; i++) {
					if ((reg&(1<<i))==0)
						dprintf(0, "%s, ", ddp_clock_1(i));
				}
				dprintf(0, "\n");
			}
			break;

		case DISP_MODULE_MUTEX:
			for (i=0; i<5; i++) {
				if (DISP_REG_GET(DISP_REG_CONFIG_MUTEX0_EN+0x20*i)==1) {
					DDPMSG("mutex%d, en=1, mode=%s, module=(", i, ddp_get_fmt_name(DISP_MODULE_MUTEX, DISP_REG_GET(DISP_REG_CONFIG_MUTEX0_SOF)));
					for (j=11; j<25; j++) {
						if ((DISP_REG_GET(DISP_REG_CONFIG_MUTEX0_MOD)>>j)&0x1)
							dprintf(0, "%s,", ddp_get_mutex_module_name(j));
					}
					DDPMSG(")\n");
				}
			}
			break;

		/* Dump MERGE Reg */
		case DISP_MODULE_MERGE:
			DDPMSG("merge: en=%d, debug=0x%x \n",
			       DISP_REG_GET(DISP_REG_MERGE_ENABLE),
			       DISP_REG_GET(DISP_REG_MERGE_DEBUG));
			break;

		/* Dump SPLIT Reg */
		case DISP_MODULE_SPLIT0:
		case DISP_MODULE_SPLIT1:
			if (DISP_MODULE_SPLIT0==module) {
				index = 0;
			} else if (DISP_MODULE_SPLIT1==module) {
				index = 1;
			}
			DDPMSG("split%d, en=%d, debug=0x%x\n",
			       index,
			       DISP_REG_GET(DISP_REG_SPLIT_ENABLE+DISP_INDEX_OFFSET*index),
			       DISP_REG_GET(DISP_REG_SPLIT_DEBUG+DISP_INDEX_OFFSET*index));
			break;
		case DISP_MODULE_COLOR0:
		case DISP_MODULE_COLOR1:
			if (DISP_MODULE_COLOR0==module) {
				index = 0;
			} else if (DISP_MODULE_COLOR1==module) {
				index = 1;
			}
			DDPMSG("color%d: bypass=%d, w=%d, h=%d \n",
			       index,
			       (DISP_REG_GET(DISP_COLOR_CFG_MAIN+DISP_INDEX_OFFSET*index)>>7)&0x1,
			       DISP_REG_GET(DISP_COLOR_INTERNAL_IP_WIDTH+DISP_INDEX_OFFSET*index),
			       DISP_REG_GET(DISP_COLOR_INTERNAL_IP_HEIGHT+DISP_INDEX_OFFSET*index));
			break;
		case DISP_MODULE_AAL:
			DDPMSG("aal: bypass=%d, relay=%d, w=%d, h=%d \n",
			       DISP_REG_GET(DISP_AAL_EN)==0x0,
			       DISP_REG_GET(DISP_AAL_CFG)&0x01,
			       (DISP_REG_GET(DISP_AAL_SIZE)>>16)&0x1fff,
			       DISP_REG_GET(DISP_AAL_SIZE)&0x1fff);
			break;
		case DISP_MODULE_UFOE:
			DDPMSG("ufoe: bypass=%d \n", DISP_REG_GET(DISP_REG_UFO_START)==0x4);
			break;
		case DISP_MODULE_OD:
			DDPMSG("od: w=%d, h=%d, bypass=%d \n",
			       (DISP_REG_GET(DISP_REG_OD_SIZE)>>16)&0xffff,
			       DISP_REG_GET(DISP_REG_OD_SIZE)&0xffff,
			       DISP_REG_GET(DISP_REG_OD_CFG)&0x1 );
			break;
		case DISP_MODULE_CMDQ:
			DDPMSG("== DISP CMDQ  ==\n");
			break;
		case DISP_MODULE_DSI0:
		case DISP_MODULE_DSI1:
			if (DISP_MODULE_DSI0==module) {
				index = 0;
			} else if (DISP_MODULE_DSI1==module) {
				index = 1;
			}
			DDPMSG("DSI%d: \n", index);
			break;
		case DISP_MODULE_PWM0:
		case DISP_MODULE_PWM1:
			if (DISP_MODULE_PWM0==module) {
				index = 0;
			} else if (DISP_MODULE_PWM1==module) {
				index = 1;
			}
			DDPMSG("PWM%d: \n", index);
			break;
		default:
			DDPMSG("DDP error, dump_reg unknow module=%d \n", module);
	}
	return 0;
}

int disp_dump_reg(DISP_MODULE_ENUM module)
{
	/* unsigned int size; */
	/* unsigned int reg_base; */
	unsigned int index = 0;

	switch (module) {
		/* Dump WDMA Reg */
		case DISP_MODULE_WDMA0:
		case DISP_MODULE_WDMA1:
			if (DISP_MODULE_WDMA0==module) {
				index = 0;
			} else if (DISP_MODULE_WDMA1==module) {
				index = 1;
			}
			DDPMSG("==DISP WDMA%d: ==\n", index);
			DDPMSG("(0x000)W_INTEN      =0x%x\n", DISP_REG_GET(DISP_REG_WDMA_INTEN+DISP_INDEX_OFFSET*index));
			DDPMSG("(0x004)W_INTS       =0x%x\n", DISP_REG_GET(DISP_REG_WDMA_INTSTA+DISP_INDEX_OFFSET*index));
			DDPMSG("(0x008)W_EN         =0x%x\n", DISP_REG_GET(DISP_REG_WDMA_EN+DISP_INDEX_OFFSET*index));
			DDPMSG("(0x00c)W_RST        =0x%x\n", DISP_REG_GET(DISP_REG_WDMA_RST+DISP_INDEX_OFFSET*index));
			DDPMSG("(0x010)W_SMI_CON    =0x%x\n", DISP_REG_GET(DISP_REG_WDMA_SMI_CON+DISP_INDEX_OFFSET*index));
			DDPMSG("(0x014)W_CFG        =0x%x\n", DISP_REG_GET(DISP_REG_WDMA_CFG+DISP_INDEX_OFFSET*index));
			DDPMSG("(0x018)W_SRC_SIZE   =0x%x\n", DISP_REG_GET(DISP_REG_WDMA_SRC_SIZE+DISP_INDEX_OFFSET*index));
			DDPMSG("(0x01c)W_CLIP_SIZE  =0x%x\n", DISP_REG_GET(DISP_REG_WDMA_CLIP_SIZE+DISP_INDEX_OFFSET*index));
			DDPMSG("(0x020)W_CLIP_COORD =0x%x\n", DISP_REG_GET(DISP_REG_WDMA_CLIP_COORD+DISP_INDEX_OFFSET*index));
			DDPMSG("(0xf00)W_DST_ADDR0  =0x%x\n", DISP_REG_GET(DISP_REG_WDMA_DST_ADDR0+DISP_INDEX_OFFSET*index));
			DDPMSG("(0x028)W_DST_PITCH  =0x%x\n", DISP_REG_GET(DISP_REG_WDMA_DST_W_IN_BYTE+DISP_INDEX_OFFSET*index));
			DDPMSG("(0x02c)W_ALPHA      =0x%x\n", DISP_REG_GET(DISP_REG_WDMA_ALPHA+DISP_INDEX_OFFSET*index));
			DDPMSG("(0x038)W_BUF_CON1   =0x%x\n", DISP_REG_GET(DISP_REG_WDMA_BUF_CON1+DISP_INDEX_OFFSET*index));
			DDPMSG("(0x03c)W_BUF_CON2   =0x%x\n", DISP_REG_GET(DISP_REG_WDMA_BUF_CON2+DISP_INDEX_OFFSET*index));
			DDPMSG("(0x058)W_PRE_ADD0   =0x%x\n", DISP_REG_GET(DISP_REG_WDMA_PRE_ADD0+DISP_INDEX_OFFSET*index));
			DDPMSG("(0x05c)W_PRE_ADD2   =0x%x\n", DISP_REG_GET(DISP_REG_WDMA_PRE_ADD2+DISP_INDEX_OFFSET*index));
			DDPMSG("(0x060)W_POST_ADD0  =0x%x\n", DISP_REG_GET(DISP_REG_WDMA_POST_ADD0+DISP_INDEX_OFFSET*index));
			DDPMSG("(0x064)W_POST_ADD2  =0x%x\n", DISP_REG_GET(DISP_REG_WDMA_POST_ADD2+DISP_INDEX_OFFSET*index));
			DDPMSG("(0xf04)W_D_ADDR1    =0x%x\n", DISP_REG_GET(DISP_REG_WDMA_DST_ADDR1+DISP_INDEX_OFFSET*index));
			DDPMSG("(0xf08)W_D_ADDR2    =0x%x\n", DISP_REG_GET(DISP_REG_WDMA_DST_ADDR2+DISP_INDEX_OFFSET*index));
			DDPMSG("(0x078)W_D_UV_PICH  =0x%x\n", DISP_REG_GET(DISP_REG_WDMA_DST_UV_PITCH+DISP_INDEX_OFFSET*index));
			DDPMSG("(0x080)W_D_OFST0    =0x%x\n", DISP_REG_GET(DISP_REG_WDMA_DST_ADDR_OFFSET0+DISP_INDEX_OFFSET*index));
			DDPMSG("(0x084)W_D_OFST1    =0x%x\n", DISP_REG_GET(DISP_REG_WDMA_DST_ADDR_OFFSET1+DISP_INDEX_OFFSET*index));
			DDPMSG("(0x088)W_D_OFST2    =0x%x\n", DISP_REG_GET(DISP_REG_WDMA_DST_ADDR_OFFSET2+DISP_INDEX_OFFSET*index));
			DDPMSG("(0x0a0)W_FLOW_DBG   =0x%x\n", DISP_REG_GET(DISP_REG_WDMA_FLOW_CTRL_DBG+DISP_INDEX_OFFSET*index));
			DDPMSG("(0x0a4)W_EXEC_DBG   =0x%x\n", DISP_REG_GET(DISP_REG_WDMA_EXEC_DBG+DISP_INDEX_OFFSET*index));
			DDPMSG("(0x0a8)W_CT_DBG     =0x%x\n", DISP_REG_GET(DISP_REG_WDMA_CT_DBG+DISP_INDEX_OFFSET*index));
			DDPMSG("(0x0ac)W_DEBUG      =0x%x\n", DISP_REG_GET(DISP_REG_WDMA_DEBUG+DISP_INDEX_OFFSET*index));
			DDPMSG("(0x100)W_DUMMY      =0x%x\n", DISP_REG_GET(DISP_REG_WDMA_DUMMY+DISP_INDEX_OFFSET*index));
			break;

		/* Dump RDMA Reg */
		case DISP_MODULE_RDMA0:
		case DISP_MODULE_RDMA1:
		case DISP_MODULE_RDMA2:
			if (DISP_MODULE_RDMA0==module) {
				index = 0;
			} else if (DISP_MODULE_RDMA1==module) {
				index = 1;
			} else if (DISP_MODULE_RDMA2==module) {
				index = 2;
			}
			DDPMSG("== DISP RDMA%d  ==\n", index);
			DDPMSG("(0x000)R_INTEN       =0x%x\n", DISP_REG_GET(DISP_REG_RDMA_INT_ENABLE+DISP_INDEX_OFFSET*index));
			DDPMSG("(0x004)R_INTS        =0x%x\n", DISP_REG_GET(DISP_REG_RDMA_INT_STATUS+DISP_INDEX_OFFSET*index));
			DDPMSG("(0x010)R_CON         =0x%x\n", DISP_REG_GET(DISP_REG_RDMA_GLOBAL_CON+DISP_INDEX_OFFSET*index));
			DDPMSG("(0x014)R_SIZE0       =0x%x\n", DISP_REG_GET(DISP_REG_RDMA_SIZE_CON_0+DISP_INDEX_OFFSET*index));
			DDPMSG("(0x018)R_SIZE1       =0x%x\n", DISP_REG_GET(DISP_REG_RDMA_SIZE_CON_1+DISP_INDEX_OFFSET*index));
			DDPMSG("(0x01c)R_TAR_LINE    =0x%x\n", DISP_REG_GET(DISP_REG_RDMA_TARGET_LINE+DISP_INDEX_OFFSET*index));
			DDPMSG("(0x024)R_M_CON       =0x%x\n", DISP_REG_GET(DISP_REG_RDMA_MEM_CON+DISP_INDEX_OFFSET*index));
			DDPMSG("(0xf00)R_M_S_ADDR    =0x%x\n", DISP_REG_GET(DISP_REG_RDMA_MEM_START_ADDR+DISP_INDEX_OFFSET*index));
			DDPMSG("(0x02c)R_M_SRC_PITCH =0x%x\n", DISP_REG_GET(DISP_REG_RDMA_MEM_SRC_PITCH+DISP_INDEX_OFFSET*index));
			DDPMSG("(0x030)R_M_GMC_SET0  =0x%x\n", DISP_REG_GET(DISP_REG_RDMA_MEM_GMC_SETTING_0+DISP_INDEX_OFFSET*index));
			DDPMSG("(0x034)R_M_SLOW_CON  =0x%x\n", DISP_REG_GET(DISP_REG_RDMA_MEM_SLOW_CON+DISP_INDEX_OFFSET*index));
			DDPMSG("(0x038)R_M_GMC_SET1  =0x%x\n", DISP_REG_GET(DISP_REG_RDMA_MEM_GMC_SETTING_1+DISP_INDEX_OFFSET*index));
			DDPMSG("(0x040)R_FIFO_CON    =0x%x\n", DISP_REG_GET(DISP_REG_RDMA_FIFO_CON+DISP_INDEX_OFFSET*index));
			DDPMSG("(0x044)R_FIFO_LOG    =0x%x\n", DISP_REG_GET(DISP_REG_RDMA_FIFO_LOG+DISP_INDEX_OFFSET*index));
			DDPMSG("(0x078)R_PRE_ADD0    =0x%x\n", DISP_REG_GET(DISP_REG_RDMA_PRE_ADD_0+DISP_INDEX_OFFSET*index));
			DDPMSG("(0x07c)R_PRE_ADD1    =0x%x\n", DISP_REG_GET(DISP_REG_RDMA_PRE_ADD_1+DISP_INDEX_OFFSET*index));
			DDPMSG("(0x080)R_PRE_ADD2    =0x%x\n", DISP_REG_GET(DISP_REG_RDMA_PRE_ADD_2+DISP_INDEX_OFFSET*index));
			DDPMSG("(0x084)R_POST_ADD0   =0x%x\n", DISP_REG_GET(DISP_REG_RDMA_POST_ADD_0+DISP_INDEX_OFFSET*index));
			DDPMSG("(0x088)R_POST_ADD1   =0x%x\n", DISP_REG_GET(DISP_REG_RDMA_POST_ADD_1+DISP_INDEX_OFFSET*index));
			DDPMSG("(0x08c)R_POST_ADD2   =0x%x\n", DISP_REG_GET(DISP_REG_RDMA_POST_ADD_2+DISP_INDEX_OFFSET*index));
			DDPMSG("(0x090)R_DUMMY       =0x%x\n", DISP_REG_GET(DISP_REG_RDMA_DUMMY+DISP_INDEX_OFFSET*index));
			DDPMSG("(0x094)R_OUT_SEL     =0x%x\n", DISP_REG_GET(DISP_REG_RDMA_DEBUG_OUT_SEL+DISP_INDEX_OFFSET*index));

			break;

		/* Dump OVL Reg */
		case DISP_MODULE_OVL0:
		case DISP_MODULE_OVL1:
			if (DISP_MODULE_OVL0==module) {
				index = 0;
			} else if (DISP_MODULE_OVL1==module) {
				index = 1;
			}
			DDPMSG("== DISP OVL%d  ==\n", index);
			DDPMSG("(0x000)O_STA        =0x%x\n", DISP_REG_GET(DISP_REG_OVL_STA+DISP_INDEX_OFFSET*index));
			DDPMSG("(0x004)O_INTEN      =0x%x\n", DISP_REG_GET(DISP_REG_OVL_INTEN+DISP_INDEX_OFFSET*index));
			DDPMSG("(0x008)O_INTSTA     =0x%x\n", DISP_REG_GET(DISP_REG_OVL_INTSTA+DISP_INDEX_OFFSET*index));
			DDPMSG("(0x00c)O_EN         =0x%x\n", DISP_REG_GET(DISP_REG_OVL_EN+DISP_INDEX_OFFSET*index));
			DDPMSG("(0x010)O_TRIG       =0x%x\n", DISP_REG_GET(DISP_REG_OVL_TRIG+DISP_INDEX_OFFSET*index));
			DDPMSG("(0x014)O_RST        =0x%x\n", DISP_REG_GET(DISP_REG_OVL_RST+DISP_INDEX_OFFSET*index));
			DDPMSG("(0x020)O_ROI_SIZE   =0x%x\n", DISP_REG_GET(DISP_REG_OVL_ROI_SIZE+DISP_INDEX_OFFSET*index));
			DDPMSG("(0x024)O_PATH_CON   =0x%x\n", DISP_REG_GET(DISP_REG_OVL_DATAPATH_CON+DISP_INDEX_OFFSET*index));
			DDPMSG("(0x028)O_ROI_BGCLR  =0x%x\n", DISP_REG_GET(DISP_REG_OVL_ROI_BGCLR+DISP_INDEX_OFFSET*index));
			DDPMSG("(0x02c)O_SRC_CON    =0x%x\n", DISP_REG_GET(DISP_REG_OVL_SRC_CON+DISP_INDEX_OFFSET*index));
			if (DISP_REG_GET(DISP_REG_OVL_SRC_CON+DISP_INDEX_OFFSET*index)&0x1) {
				DDPMSG("(0x030)O0_CON      =0x%x\n",     DISP_REG_GET(DISP_REG_OVL_L0_CON+DISP_INDEX_OFFSET*index));
				DDPMSG("(0x034)O0_SRCKEY   =0x%x\n",     DISP_REG_GET(DISP_REG_OVL_L0_SRCKEY+DISP_INDEX_OFFSET*index));
				DDPMSG("(0x038)O0_SRC_SIZE =0x%x\n",     DISP_REG_GET(DISP_REG_OVL_L0_SRC_SIZE+DISP_INDEX_OFFSET*index));
				DDPMSG("(0x03c)O0_OFFSET   =0x%x\n",     DISP_REG_GET(DISP_REG_OVL_L0_OFFSET+DISP_INDEX_OFFSET*index));
				DDPMSG("(0xf40)O0_ADDR     =0x%x\n",     DISP_REG_GET(DISP_REG_OVL_L0_ADDR+DISP_INDEX_OFFSET*index));
				DDPMSG("(0x044)O0_PITCH    =0x%x\n",     DISP_REG_GET(DISP_REG_OVL_L0_PITCH+DISP_INDEX_OFFSET*index));
				DDPMSG("(0x048)O0_TILE     =0x%x\n",     DISP_REG_GET(DISP_REG_OVL_L0_TILE+DISP_INDEX_OFFSET*index));
				DDPMSG("(0x0c0)O0_R_CTRL   =0x%x\n",     DISP_REG_GET(DISP_REG_OVL_RDMA0_CTRL+DISP_INDEX_OFFSET*index));
				DDPMSG("(0x0c8)O0_R_M_GMC_SET1 =0x%x\n", DISP_REG_GET(DISP_REG_OVL_RDMA0_MEM_GMC_SETTING+DISP_INDEX_OFFSET*index));
				DDPMSG("(0x0cc)O0_R_M_SLOW_CON =0x%x\n", DISP_REG_GET(DISP_REG_OVL_RDMA0_MEM_SLOW_CON+DISP_INDEX_OFFSET*index));
				DDPMSG("(0x0d0)O0_R_FIFO_CTRL  =0x%x\n", DISP_REG_GET(DISP_REG_OVL_RDMA0_FIFO_CTRL+DISP_INDEX_OFFSET*index));
				DDPMSG("(0x1e0)O0_R_M_GMC_SET2 =0x%x\n", DISP_REG_GET(DISP_REG_OVL_RDMA0_MEM_GMC_SETTING2+DISP_INDEX_OFFSET*index));
				DDPMSG("(0x24c)O0_R_DBG   =0x%x\n",      DISP_REG_GET(DISP_REG_OVL_RDMA0_DBG+DISP_INDEX_OFFSET*index));
			}
			if (DISP_REG_GET(DISP_REG_OVL_SRC_CON+DISP_INDEX_OFFSET*index)&0x2) {
				DDPMSG("(0x050)O1_CON      =0x%x\n",     DISP_REG_GET(DISP_REG_OVL_L1_CON+DISP_INDEX_OFFSET*index));
				DDPMSG("(0x054)O1_SRCKEY   =0x%x\n",     DISP_REG_GET(DISP_REG_OVL_L1_SRCKEY+DISP_INDEX_OFFSET*index));
				DDPMSG("(0x058)O1_SRC_SIZE =0x%x\n",     DISP_REG_GET(DISP_REG_OVL_L1_SRC_SIZE+DISP_INDEX_OFFSET*index));
				DDPMSG("(0x05c)O1_OFFSET   =0x%x\n",     DISP_REG_GET(DISP_REG_OVL_L1_OFFSET+DISP_INDEX_OFFSET*index));
				DDPMSG("(0xf60)O1_ADDR     =0x%x\n",     DISP_REG_GET(DISP_REG_OVL_L1_ADDR+DISP_INDEX_OFFSET*index));
				DDPMSG("(0x064)O1_PITCH    =0x%x\n",     DISP_REG_GET(DISP_REG_OVL_L1_PITCH+DISP_INDEX_OFFSET*index));
				DDPMSG("(0x068)O1_TILE     =0x%x\n",     DISP_REG_GET(DISP_REG_OVL_L1_TILE+DISP_INDEX_OFFSET*index));
				DDPMSG("(0x0e0)O1_R_CTRL   =0x%x\n",     DISP_REG_GET(DISP_REG_OVL_RDMA1_CTRL+DISP_INDEX_OFFSET*index));
				DDPMSG("(0x0e8)O1_R_M_GMC_SET1 =0x%x\n", DISP_REG_GET(DISP_REG_OVL_RDMA1_MEM_GMC_SETTING+DISP_INDEX_OFFSET*index));
				DDPMSG("(0x0ec)O1_R_M_SLOW_CON =0x%x\n", DISP_REG_GET(DISP_REG_OVL_RDMA1_MEM_SLOW_CON+DISP_INDEX_OFFSET*index));
				DDPMSG("(0x0f0)O1_R_FIFO_CTRL  =0x%x\n", DISP_REG_GET(DISP_REG_OVL_RDMA1_FIFO_CTRL+DISP_INDEX_OFFSET*index));
				DDPMSG("(0x1e4)O1_R_M_GMC_SET2 =0x%x\n", DISP_REG_GET(DISP_REG_OVL_RDMA1_MEM_GMC_SETTING2+DISP_INDEX_OFFSET*index));
				DDPMSG("(0x250)O1_R_DBG =0x%x\n",        DISP_REG_GET(DISP_REG_OVL_RDMA1_DBG+DISP_INDEX_OFFSET*index));
			}
			if (DISP_REG_GET(DISP_REG_OVL_SRC_CON+DISP_INDEX_OFFSET*index)&0x4) {
				DDPMSG("(0x070)O2_CON      =0x%x\n",     DISP_REG_GET(DISP_REG_OVL_L2_CON+DISP_INDEX_OFFSET*index));
				DDPMSG("(0x074)O2_SRCKEY   =0x%x\n",     DISP_REG_GET(DISP_REG_OVL_L2_SRCKEY+DISP_INDEX_OFFSET*index));
				DDPMSG("(0x078)O2_SRC_SIZE =0x%x\n",     DISP_REG_GET(DISP_REG_OVL_L2_SRC_SIZE+DISP_INDEX_OFFSET*index));
				DDPMSG("(0x07c)O2_OFFSET   =0x%x\n",     DISP_REG_GET(DISP_REG_OVL_L2_OFFSET+DISP_INDEX_OFFSET*index));
				DDPMSG("(0xf80)O2_ADDR     =0x%x\n",     DISP_REG_GET(DISP_REG_OVL_L2_ADDR+DISP_INDEX_OFFSET*index));
				DDPMSG("(0x084)O2_PITCH    =0x%x\n",     DISP_REG_GET(DISP_REG_OVL_L2_PITCH+DISP_INDEX_OFFSET*index));
				DDPMSG("(0x088)O2_TILE     =0x%x\n",     DISP_REG_GET(DISP_REG_OVL_L2_TILE+DISP_INDEX_OFFSET*index));
				DDPMSG("(0x100)O2_R_CTRL   =0x%x\n",     DISP_REG_GET(DISP_REG_OVL_RDMA2_CTRL+DISP_INDEX_OFFSET*index));
				DDPMSG("(0x108)O2_R_M_GMC_SET1 =0x%x\n", DISP_REG_GET(DISP_REG_OVL_RDMA2_MEM_GMC_SETTING+DISP_INDEX_OFFSET*index));
				DDPMSG("(0x10c)O2_R_M_SLOW_CON =0x%x\n", DISP_REG_GET(DISP_REG_OVL_RDMA2_MEM_SLOW_CON+DISP_INDEX_OFFSET*index));
				DDPMSG("(0x110)O2_R_FIFO_CTRL  =0x%x\n", DISP_REG_GET(DISP_REG_OVL_RDMA2_FIFO_CTRL+DISP_INDEX_OFFSET*index));
				DDPMSG("(0x1e8)O2_R_M_GMC_SET2 =0x%x\n", DISP_REG_GET(DISP_REG_OVL_RDMA2_MEM_GMC_SETTING2+DISP_INDEX_OFFSET*index));
				DDPMSG("(0x254)O2_R_DBG =0x%x\n",        DISP_REG_GET(DISP_REG_OVL_RDMA2_DBG+DISP_INDEX_OFFSET*index));
			}
			if (DISP_REG_GET(DISP_REG_OVL_SRC_CON+DISP_INDEX_OFFSET*index)&0x8) {
				DDPMSG("(0x090)O3_CON      =0x%x\n",     DISP_REG_GET(DISP_REG_OVL_L3_CON+DISP_INDEX_OFFSET*index));
				DDPMSG("(0x094)O3_SRCKEY   =0x%x\n",     DISP_REG_GET(DISP_REG_OVL_L3_SRCKEY+DISP_INDEX_OFFSET*index));
				DDPMSG("(0x098)O3_SRC_SIZE =0x%x\n",     DISP_REG_GET(DISP_REG_OVL_L3_SRC_SIZE+DISP_INDEX_OFFSET*index));
				DDPMSG("(0x09c)O3_OFFSET   =0x%x\n",     DISP_REG_GET(DISP_REG_OVL_L3_OFFSET+DISP_INDEX_OFFSET*index));
				DDPMSG("(0xfa0)O3_ADDR     =0x%x\n",     DISP_REG_GET(DISP_REG_OVL_L3_ADDR+DISP_INDEX_OFFSET*index));
				DDPMSG("(0x0a4)O3_PITCH    =0x%x\n",     DISP_REG_GET(DISP_REG_OVL_L3_PITCH+DISP_INDEX_OFFSET*index));
				DDPMSG("(0x0a8)O3_TILE     =0x%x\n",     DISP_REG_GET(DISP_REG_OVL_L3_TILE+DISP_INDEX_OFFSET*index));
				DDPMSG("(0x120)O3_R_CTRL   =0x%x\n",     DISP_REG_GET(DISP_REG_OVL_RDMA3_CTRL+DISP_INDEX_OFFSET*index));
				DDPMSG("(0x128)O3_R_M_GMC_SET1 =0x%x\n", DISP_REG_GET(DISP_REG_OVL_RDMA3_MEM_GMC_SETTING+DISP_INDEX_OFFSET*index));
				DDPMSG("(0x12c)O3_R_M_SLOW_CON =0x%x\n", DISP_REG_GET(DISP_REG_OVL_RDMA3_MEM_SLOW_CON+DISP_INDEX_OFFSET*index));
				DDPMSG("(0x130)O3_R_FIFO_CTRL  =0x%x\n", DISP_REG_GET(DISP_REG_OVL_RDMA3_FIFO_CTRL+DISP_INDEX_OFFSET*index));
				DDPMSG("(0x1ec)O3_R_M_GMC_SET2 =0x%x\n", DISP_REG_GET(DISP_REG_OVL_RDMA3_MEM_GMC_SETTING2+DISP_INDEX_OFFSET*index));
				DDPMSG("(0x258)O3_R_DBG    =0x%x\n",     DISP_REG_GET(DISP_REG_OVL_RDMA3_DBG+DISP_INDEX_OFFSET*index));
			}
			DDPMSG("(0x1d4)O_DBG_MON_SEL =0x%x\n",       DISP_REG_GET(DISP_REG_OVL_DEBUG_MON_SEL+DISP_INDEX_OFFSET*index));
			DDPMSG("(0x200)O_DUMMY_REG   =0x%x\n",       DISP_REG_GET(DISP_REG_OVL_DUMMY_REG+DISP_INDEX_OFFSET*index));
			DDPMSG("(0x240)O_FLOW_CTRL   =0x%x\n",       DISP_REG_GET(DISP_REG_OVL_FLOW_CTRL_DBG+DISP_INDEX_OFFSET*index));
			DDPMSG("(0x244)O_ADDCON      =0x%x\n",       DISP_REG_GET(DISP_REG_OVL_ADDCON_DBG+DISP_INDEX_OFFSET*index));
			break;

		/* Dump GAMMA Reg */
		case DISP_MODULE_GAMMA:
			DDPMSG("== DISP GAMMA  ==\n");
			DDPMSG("(0x000)GA_EN        =0x%x\n", DISP_REG_GET(DISP_REG_GAMMA_EN));
			DDPMSG("(0x004)GA_RESET     =0x%x\n", DISP_REG_GET(DISP_REG_GAMMA_RESET));
			DDPMSG("(0x008)GA_INTEN     =0x%x\n", DISP_REG_GET(DISP_REG_GAMMA_INTEN));
			DDPMSG("(0x00c)GA_INTSTA    =0x%x\n", DISP_REG_GET(DISP_REG_GAMMA_INTSTA));
			DDPMSG("(0x010)GA_STATUS    =0x%x\n", DISP_REG_GET(DISP_REG_GAMMA_STATUS));
			DDPMSG("(0x020)GA_CFG       =0x%x\n", DISP_REG_GET(DISP_REG_GAMMA_CFG));
			DDPMSG("(0x024)GA_IN_COUNT  =0x%x\n", DISP_REG_GET(DISP_REG_GAMMA_INPUT_COUNT));
			DDPMSG("(0x028)GA_OUT_COUNT =0x%x\n", DISP_REG_GET(DISP_REG_GAMMA_OUTPUT_COUNT));
			DDPMSG("(0x02c)GA_CHKSUM    =0x%x\n", DISP_REG_GET(DISP_REG_GAMMA_CHKSUM));
			DDPMSG("(0x030)GA_SIZE      =0x%x\n", DISP_REG_GET(DISP_REG_GAMMA_SIZE));
			DDPMSG("(0x0c0)GA_DUMMY_REG =0x%x\n", DISP_REG_GET(DISP_REG_GAMMA_DUMMY_REG));
			DDPMSG("(0x800)GA_LUT       =0x%x\n", DISP_REG_GET(DISP_REG_GAMMA_LUT));
			break;

		/* Dump DPI0 Reg */
		case DISP_MODULE_DPI0:
			DDPMSG("== DISP DISP_MODULE_DPI  ==\n");
			break;

		/* Dump CONFIG Reg */
		case DISP_MODULE_CONFIG:
			DDPMSG("== DISP Config  ==\n");
			DDPMSG("(0x000)MM_INTEN     =0x%x\n", DISP_REG_GET(DISP_REG_CONFIG_MMSYS_INTEN));
			DDPMSG("(0x004)MM_INTSTA    =0x%x\n", DISP_REG_GET(DISP_REG_CONFIG_MMSYS_INTSTA));
			DDPMSG("(0x040)C_OVL0_MOUT  =0x%x\n", DISP_REG_GET(DISP_REG_CONFIG_DISP_OVL0_MOUT_EN));
			DDPMSG("(0x044)C_OVL1_MOUT  =0x%x\n", DISP_REG_GET(DISP_REG_CONFIG_DISP_OVL1_MOUT_EN));
			DDPMSG("(0x048)C_OD_MOUT    =0x%x\n", DISP_REG_GET(DISP_REG_CONFIG_DISP_OD_MOUT_EN));
			DDPMSG("(0x04C)C_GAMMA_MOUT =0x%x\n", DISP_REG_GET(DISP_REG_CONFIG_DISP_GAMMA_MOUT_EN));
			DDPMSG("(0x050)C_UFOE_MOUT  =0x%x\n", DISP_REG_GET(DISP_REG_CONFIG_DISP_UFOE_MOUT_EN));
			DDPMSG("(0x054)C_MOUT_RST   =0x%x\n", DISP_REG_GET(DISP_REG_CONFIG_MMSYS_MOUT_RST));
			DDPMSG("(0x084)C_COLOR0_SIN =0x%x\n", DISP_REG_GET(DISP_REG_CONFIG_DISP_COLOR0_SEL_IN));
			DDPMSG("(0x088)C_COLOR1_SIN =0x%x\n", DISP_REG_GET(DISP_REG_CONFIG_DISP_COLOR1_SEL_IN));
			DDPMSG("(0x08C)C_AAL_SIN    =0x%x\n", DISP_REG_GET(DISP_REG_CONFIG_DISP_AAL_SEL_IN));
			DDPMSG("(0x090)C_PATH0_SIN  =0x%x\n", DISP_REG_GET(DISP_REG_CONFIG_DISP_PATH0_SEL_IN));
			DDPMSG("(0x094)C_PATH1_SIN  =0x%x\n", DISP_REG_GET(DISP_REG_CONFIG_DISP_PATH1_SEL_IN));
			DDPMSG("(0x098)C_WDMA0_SIN  =0x%x\n", DISP_REG_GET(DISP_REG_CONFIG_DISP_MODULE_WDMA0_SEL_IN));
			DDPMSG("(0x09C)C_WDMA1_SIN  =0x%x\n", DISP_REG_GET(DISP_REG_CONFIG_DISP_WDMA1_SEL_IN));
			DDPMSG("(0x0A0)C_UFOE_SIN   =0x%x\n", DISP_REG_GET(DISP_REG_CONFIG_DISP_UFOE_SEL_IN));
			DDPMSG("(0x0A4)DSI0_SIN     =0x%x\n", DISP_REG_GET(DISP_REG_CONFIG_DSI0_SEL_IN));
			DDPMSG("(0x0A8)DSI1_SIN     =0x%x\n", DISP_REG_GET(DISP_REG_CONFIG_DSI1_SEL_IN));
			DDPMSG("(0x0AC)DPI_SIN      =0x%x\n", DISP_REG_GET(DISP_REG_CONFIG_DPI_SEL_IN));
			DDPMSG("(0x0B0)C_RDMA0_SOUT =0x%x\n", DISP_REG_GET(DISP_REG_CONFIG_DISP_RDMA0_SOUT_SEL_IN));
			DDPMSG("(0x0B4)C_RDMA1_SOUT =0x%x\n", DISP_REG_GET(DISP_REG_CONFIG_DISP_RDMA1_SOUT_SEL_IN));
			DDPMSG("(0x0B8)C_RDMA2_SOUT =0x%x\n", DISP_REG_GET(DISP_REG_CONFIG_DISP_RDMA2_SOUT_SEL_IN));
			DDPMSG("(0x0BC)C_COLOR0_SOUT=0x%x\n", DISP_REG_GET(DISP_REG_CONFIG_DISP_COLOR0_SOUT_SEL_IN));
			DDPMSG("(0x0C0)C_COLOR1_SOUT=0x%x\n", DISP_REG_GET(DISP_REG_CONFIG_DISP_COLOR1_SOUT_SEL_IN));
			DDPMSG("(0x0C4)C_PATH0_SOUT =0x%x\n", DISP_REG_GET(DISP_REG_CONFIG_DISP_PATH0_SOUT_SEL_IN));
			DDPMSG("(0x0C8)C_PATH1_SOUT =0x%x\n", DISP_REG_GET(DISP_REG_CONFIG_DISP_PATH1_SOUT_SEL_IN));
			DDPMSG("(0x0F0)MM_MISC      =0x%x\n", DISP_REG_GET(DISP_REG_CONFIG_MMSYS_MISC));
			DDPMSG("(0x100)MM_CG_CON0   =0x%x\n", DISP_REG_GET(DISP_REG_CONFIG_MMSYS_CG_CON0));
			DDPMSG("(0x110)MM_CG_CON1   =0x%x\n", DISP_REG_GET(DISP_REG_CONFIG_MMSYS_CG_CON1));
			DDPMSG("(0x8d0)LARB0_GREQ   =0x%x\n", DISP_REG_GET(DISP_REG_CONFIG_SMI_LARB0_GREQ));
			DDPMSG("(0x8b0)DISP_DL_VALID_0=0x%x \n", DISP_REG_GET(DISPSYS_CONFIG_BASE+0x8b0));
			DDPMSG("(0x8b4)DISP_DL_VALID_1=0x%x \n", DISP_REG_GET(DISPSYS_CONFIG_BASE+0x8b4));
			DDPMSG("(0x8b8)DISP_DL_READY_0=0x%x \n", DISP_REG_GET(DISPSYS_CONFIG_BASE+0x8b8));
			DDPMSG("(0x8bc)DISP_DL_READY_1=0x%x \n", DISP_REG_GET(DISPSYS_CONFIG_BASE+0x8bc));
			DDPMSG("(0x8c0)MDP_DL_VALID_0 =0x%x \n", DISP_REG_GET(DISPSYS_CONFIG_BASE+0x8c0));
			DDPMSG("(0x8c4)MDP_DL_VALID_1 =0x%x \n", DISP_REG_GET(DISPSYS_CONFIG_BASE+0x8c4));
			DDPMSG("(0x8c8)MDP_DL_READY_0 =0x%x \n", DISP_REG_GET(DISPSYS_CONFIG_BASE+0x8c8));
			DDPMSG("(0x8cc)MDP_DL_READY_1 =0x%x \n", DISP_REG_GET(DISPSYS_CONFIG_BASE+0x8cc));
			DDPMSG("(0x8d4)0x8d4        =0x%x\n",    DISP_REG_GET(DISPSYS_CONFIG_BASE+0x8d4));
			break;

		/* Dump MUTEX Reg */
		case DISP_MODULE_MUTEX:
			DDPMSG("== DISP Mutex  ==\n");
			DDPMSG("(0x000)M_INTEN   =0x%x\n", DISP_REG_GET(DISP_REG_CONFIG_MUTEX_INTEN));
			DDPMSG("(0x004)M_INTSTA  =0x%x\n", DISP_REG_GET(DISP_REG_CONFIG_MUTEX_INTSTA));
			DDPMSG("(0x020)M0_EN     =0x%x\n", DISP_REG_GET(DISP_REG_CONFIG_MUTEX0_EN));
			DDPMSG("(0x028)M0_RST    =0x%x\n", DISP_REG_GET(DISP_REG_CONFIG_MUTEX0_RST));
			DDPMSG("(0x02c)M0_MOD    =0x%x\n", DISP_REG_GET(DISP_REG_CONFIG_MUTEX0_MOD));
			DDPMSG("(0x030)M0_SOF    =0x%x\n", DISP_REG_GET(DISP_REG_CONFIG_MUTEX0_SOF));
			DDPMSG("(0x040)M1_EN     =0x%x\n", DISP_REG_GET(DISP_REG_CONFIG_MUTEX1_EN));
			DDPMSG("(0x048)M1_RST    =0x%x\n", DISP_REG_GET(DISP_REG_CONFIG_MUTEX1_RST));
			DDPMSG("(0x04c)M1_MOD    =0x%x\n", DISP_REG_GET(DISP_REG_CONFIG_MUTEX1_MOD));
			DDPMSG("(0x050)M1_SOF    =0x%x\n", DISP_REG_GET(DISP_REG_CONFIG_MUTEX1_SOF));
			DDPMSG("(0x060)M2_EN     =0x%x\n", DISP_REG_GET(DISP_REG_CONFIG_MUTEX2_EN));
			DDPMSG("(0x068)M2_RST    =0x%x\n", DISP_REG_GET(DISP_REG_CONFIG_MUTEX2_RST));
			DDPMSG("(0x06c)M2_MOD    =0x%x\n", DISP_REG_GET(DISP_REG_CONFIG_MUTEX2_MOD));
			DDPMSG("(0x070)M2_SOF    =0x%x\n", DISP_REG_GET(DISP_REG_CONFIG_MUTEX2_SOF));
			DDPMSG("(0x080)M3_EN     =0x%x\n", DISP_REG_GET(DISP_REG_CONFIG_MUTEX3_EN));
			DDPMSG("(0x088)M3_RST    =0x%x\n", DISP_REG_GET(DISP_REG_CONFIG_MUTEX3_RST));
			DDPMSG("(0x08c)M3_MOD    =0x%x\n", DISP_REG_GET(DISP_REG_CONFIG_MUTEX3_MOD));
			DDPMSG("(0x090)M3_SOF    =0x%x\n", DISP_REG_GET(DISP_REG_CONFIG_MUTEX3_SOF));
			DDPMSG("(0x0a0)M4_EN     =0x%x\n", DISP_REG_GET(DISP_REG_CONFIG_MUTEX4_EN));
			DDPMSG("(0x0a8)M4_RST    =0x%x\n", DISP_REG_GET(DISP_REG_CONFIG_MUTEX4_RST));
			DDPMSG("(0x0ac)M4_MOD    =0x%x\n", DISP_REG_GET(DISP_REG_CONFIG_MUTEX4_MOD));
			DDPMSG("(0x0b0)M4_SOF    =0x%x\n", DISP_REG_GET(DISP_REG_CONFIG_MUTEX4_SOF));
			DDPMSG("(0x0c0)M5_EN     =0x%x\n", DISP_REG_GET(DISP_REG_CONFIG_MUTEX5_EN));
			DDPMSG("(0x0c8)M5_RST    =0x%x\n", DISP_REG_GET(DISP_REG_CONFIG_MUTEX5_RST));
			DDPMSG("(0x0cc)M5_MOD    =0x%x\n", DISP_REG_GET(DISP_REG_CONFIG_MUTEX5_MOD));
			DDPMSG("(0x0d0)M5_SOF    =0x%x\n", DISP_REG_GET(DISP_REG_CONFIG_MUTEX5_SOF));
			DDPMSG("(0x200)DEBUG_OUT_SEL =0x%x\n", DISP_REG_GET(DISP_REG_CONFIG_DEBUG_OUT_SEL));
			break;

		/* Dump MERGE Reg */
		case DISP_MODULE_MERGE:
			DDPMSG("== DISP MERGE  ==%d\n", index);
			DDPMSG("(0x000)MERGE_EN       =0x%x\n", DISP_REG_GET(DISP_REG_MERGE_ENABLE));
			DDPMSG("(0x004)MERGE_SW_RESET =0x%x\n", DISP_REG_GET(DISP_REG_MERGE_SW_RESET));
			DDPMSG("(0x008)MERGE_DEBUG    =0x%x\n", DISP_REG_GET(DISP_REG_MERGE_DEBUG));
			break;

		/* Dump SPLIT Reg */
		case DISP_MODULE_SPLIT0:
		case DISP_MODULE_SPLIT1:
			if (DISP_MODULE_SPLIT0==module) {
				index = 0;
			} else if (DISP_MODULE_SPLIT1==module) {
				index = 1;
			}
			DDPMSG("== DISP SPLIT%d  ==\n", index);
			DDPMSG("(0x000)SPLIT_ENABLE =0x%x\n", DISP_REG_GET(DISP_REG_SPLIT_ENABLE+DISP_INDEX_OFFSET*index));
			DDPMSG("(0x004)SPLIT_RESET  =0x%x\n", DISP_REG_GET(DISP_REG_SPLIT_SW_RESET+DISP_INDEX_OFFSET*index));
			DDPMSG("(0x008)SPLIT_DEBUG  =0x%x\n", DISP_REG_GET(DISP_REG_SPLIT_DEBUG+DISP_INDEX_OFFSET*index));
			break;

		/* Dump COLOR Reg */
		case DISP_MODULE_COLOR0:
		case DISP_MODULE_COLOR1:
			if (DISP_MODULE_COLOR0==module) {
				index = 0;
			} else if (DISP_MODULE_COLOR1==module) {
				index = 1;
			}
			DDPMSG("== DISP COLOR%d  ==\n", index);
			DDPMSG("(0x400)COLOR_CFG_MAIN   =0x%x\n", DISP_REG_GET(DISP_COLOR_CFG_MAIN+DISP_INDEX_OFFSET*index));
			DDPMSG("(0xc00)COLOR_START      =0x%x\n", DISP_REG_GET(DISP_COLOR_START+DISP_INDEX_OFFSET*index));
			DDPMSG("(0xc50)COLOR_INTER_IP_W =0x%x\n", DISP_REG_GET(DISP_COLOR_INTERNAL_IP_WIDTH+DISP_INDEX_OFFSET*index));
			DDPMSG("(0xc54)COLOR_INTER_IP_H =0x%x\n", DISP_REG_GET(DISP_COLOR_INTERNAL_IP_HEIGHT+DISP_INDEX_OFFSET*index));
			break;

		/* Dump AAL Reg */
		case DISP_MODULE_AAL:
			DDPMSG("== DISP AAL%d ==\n", index);
			DDPMSG("(0x000)AAL_EN           =0x%x\n", DISP_REG_GET(DISP_AAL_EN));
			DDPMSG("(0x008)AAL_INTEN        =0x%x\n", DISP_REG_GET(DISP_AAL_INTEN));
			DDPMSG("(0x00c)AAL_INTSTA       =0x%x\n", DISP_REG_GET(DISP_AAL_INTSTA));
			DDPMSG("(0x020)AAL_CFG          =0x%x\n", DISP_REG_GET(DISP_AAL_CFG));
			DDPMSG("(0x030)AAL_SIZE         =0x%x\n", DISP_REG_GET(DISP_AAL_SIZE));
			DDPMSG("(0x20c)AAL_CABC_00      =0x%x\n", DISP_REG_GET(DISP_AAL_CABC_00));
			DDPMSG("(0x214)AAL_CABC_02      =0x%x\n", DISP_REG_GET(DISP_AAL_CABC_02));
			DDPMSG("(0x20c)AAL_STATUS_00    =0x%x\n", DISP_REG_GET(DISP_AAL_STATUS_00));
			DDPMSG("(0x210)AAL_STATUS_01    =0x%x\n", DISP_REG_GET(DISP_AAL_STATUS_00 + 0x4));
			DDPMSG("(0x2a0)AAL_STATUS_31    =0x%x\n", DISP_REG_GET(DISP_AAL_STATUS_32 - 0x4));
			DDPMSG("(0x2a4)AAL_STATUS_32    =0x%x\n", DISP_REG_GET(DISP_AAL_STATUS_32));
			DDPMSG("(0x354)AAL_DRE_GAIN_FILTER_00 =0x%x\n", DISP_REG_GET(DISP_AAL_DRE_GAIN_FILTER_00));
			DDPMSG("(0x3b0)AAL_DRE_MAPPING_00     =0x%x\n", DISP_REG_GET(DISP_AAL_DRE_MAPPING_00));
			break;

		/* Dump UFOE Reg */
		case DISP_MODULE_UFOE:
			DDPMSG("== DISP UFOE%d ==\n", index);
			DDPMSG("(0x000)UFOE_START =0x%x\n", DISP_REG_GET(DISP_REG_UFO_START));
			break;

		/* Dump OD Reg */
		case DISP_MODULE_OD:
			DDPMSG("== DISP OD%d ==\n", index);
			DDPMSG("(00)EN           =0x%x \n", DISP_REG_GET(DISP_REG_OD_EN           ));
			DDPMSG("(04)RESET        =0x%x \n", DISP_REG_GET(DISP_REG_OD_RESET        ));
			DDPMSG("(08)INTEN        =0x%x \n", DISP_REG_GET(DISP_REG_OD_INTEN        ));
			DDPMSG("(0C)INTSTA       =0x%x \n", DISP_REG_GET(DISP_REG_OD_INTSTA       ));
			DDPMSG("(10)STATUS       =0x%x \n", DISP_REG_GET(DISP_REG_OD_STATUS       ));
			DDPMSG("(20)CFG          =0x%x \n", DISP_REG_GET(DISP_REG_OD_CFG          ));
			DDPMSG("(24)INPUT_COUNT	=0x%x \n", DISP_REG_GET(DISP_REG_OD_INPUT_COUNT  ));
			DDPMSG("(28)OUTPUT_COUNT =0x%x \n", DISP_REG_GET(DISP_REG_OD_OUTPUT_COUNT ));
			DDPMSG("(2C)CHKSUM       =0x%x \n", DISP_REG_GET(DISP_REG_OD_CHKSUM       ));
			DDPMSG("(30)SIZE	        =0x%x \n", DISP_REG_GET(DISP_REG_OD_SIZE        ));
			DDPMSG("(40)HSYNC_WIDTH  =0x%x \n", DISP_REG_GET(DISP_REG_OD_HSYNC_WIDTH  ));
			DDPMSG("(44)VSYNC_WIDTH	=0x%x \n", DISP_REG_GET(DISP_REG_OD_VSYNC_WIDTH  ));
			DDPMSG("(48)MISC         =0x%x \n", DISP_REG_GET(DISP_REG_OD_MISC         ));
			DDPMSG("(C0)DUMMY_REG    =0x%x \n", DISP_REG_GET(DISP_REG_OD_DUMMY_REG    ));
			break;


		/* Dump CMDQ Reg */
		case DISP_MODULE_CMDQ:
			DDPMSG("== DISP CMDQ  ==\n");
			break;

		/* Dump DSI Reg */
		case DISP_MODULE_DSI0:
		case DISP_MODULE_DSI1:
			if (DISP_MODULE_DSI0==module) {
				index = 0;
			} else if (DISP_MODULE_DSI1==module) {
				index = 1;
			}
			DDPMSG("== DISP DSI%d  ==\n", index);
			break;

		/* Dump PWM Reg */
		case DISP_MODULE_PWM0:
		case DISP_MODULE_PWM1:
			if (DISP_MODULE_PWM0==module) {
				index = 0;
			} else if (DISP_MODULE_PWM1==module) {
				index = 1;
			}
			DDPMSG("== DISP WMM%d  ==\n", index);
			break;
		default:
			DDPMSG("DDP error, dump_reg unknow module=%d \n", module);
	}
	return 0;
}

