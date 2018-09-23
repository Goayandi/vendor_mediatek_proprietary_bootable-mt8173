//#include <string.h>
//#include "CTP_type.h"
//#include "CTP_shell.h"
//#include <intrCtrl.h>
//#include <config.h>
//#include "common.h"
//#include "version.h"
//#include "BusMonitor.h"
//#include "api.h"
//#include "driver_api.h"

#include <target/board.h>
#include <platform/env.h>
#include <platform/mt_gpt.h>

#include <string.h>
#include <stdlib.h>


#include <platform/hdmi_drv.h>
#include <platform/hdmi_reg.h>
#include <platform/hdmiedid.h>


#define HDMIDRV_BASE     (0x14025000)
#define HDMISYS_BASE     (0x14000000)
#define HDMIPLL_BASE     (0x10209000)  //pll
#define HDMICKGEN_BASE   (0x10000000)
#define HDMI_ANALOG_BASE (0x10209000)
#define HDMI_EFUSE_BASE (0x10200000)
#define HDMI_GPIO_BASE (0x10005000)

#ifndef FALSE
#define FALSE   0
#endif

#ifndef TRUE
#define TRUE    1
#endif


void internal_hdmi_read(unsigned int u4Reg, unsigned int *p4Data)
{
	*p4Data = (*(volatile unsigned int*)(u4Reg));
}

void internal_hdmi_write(unsigned int u4Reg, unsigned int u4data)
{
	*(volatile unsigned int*)(u4Reg) = (u4data);
}

//////////////////////////////////////////////
unsigned int hdmi_drv_read(unsigned short u2Reg)
{
	unsigned int u4Data;
	internal_hdmi_read(HDMIDRV_BASE+u2Reg, &u4Data);
	return u4Data;
}

void hdmi_drv_write(unsigned short u2Reg, unsigned int u4Data)
{
	internal_hdmi_write(HDMIDRV_BASE+u2Reg, u4Data);
}
/////////////////////////////////////////////////
unsigned int hdmi_sys_read(unsigned short u2Reg)
{
	unsigned int u4Data;
	internal_hdmi_read(HDMISYS_BASE+u2Reg, &u4Data);
	return u4Data;
}

void hdmi_sys_write(unsigned short u2Reg, unsigned int u4Data)
{
	internal_hdmi_write(HDMISYS_BASE+u2Reg, u4Data);
}
/////////////////////////////////////////////////////
unsigned int hdmi_hdmitopck_read(unsigned short u2Reg)
{
	unsigned int u4Data;
	internal_hdmi_read(HDMICKGEN_BASE+u2Reg, &u4Data);
	return u4Data;
}

void hdmi_hdmitopck_write(unsigned short u2Reg, unsigned int u4Data)
{
	internal_hdmi_write(HDMICKGEN_BASE+u2Reg, u4Data);

}
///////////////////////////////////////////////////////
unsigned int hdmi_pll_read(unsigned short u2Reg)
{
	unsigned int u4Data;
	internal_hdmi_read(HDMIPLL_BASE+u2Reg, &u4Data);
	return u4Data;
}

void hdmi_pll_write(unsigned short u2Reg, unsigned int u4Data)
{
	internal_hdmi_write(HDMIPLL_BASE+u2Reg, u4Data);
}

unsigned int hdmi_pad_read(unsigned short u2Reg)
{
	unsigned int u4Data;
	internal_hdmi_read(HDMI_GPIO_BASE + u2Reg, &u4Data);
	return u4Data;
}

void hdmi_pad_write(unsigned short u2Reg, unsigned int u4Data)
{
	internal_hdmi_write(HDMI_GPIO_BASE + u2Reg, u4Data);
}

////////////////////////////////////////////////////////////
#define vWriteHdmiANA(dAddr, dVal)  (*((volatile unsigned int *)(HDMI_ANALOG_BASE + dAddr)) = (dVal))
#define dReadHdmiANA(dAddr)         (*((volatile unsigned int *)(HDMI_ANALOG_BASE + dAddr)))
#define vWriteHdmiANAMsk(dAddr, dVal, dMsk) (vWriteHdmiANA((dAddr), (dReadHdmiANA(dAddr) & (~(dMsk))) | ((dVal) & (dMsk))))
/////////////////////////////////////////////////////////////////////////////////////////

#define vWriteByteHdmiGRL(dAddr, dVal)  (hdmi_drv_write(dAddr, dVal))
#define bReadByteHdmiGRL(bAddr)         (hdmi_drv_read(bAddr))
#define vWriteHdmiGRLMsk(dAddr, dVal, dMsk) (vWriteByteHdmiGRL((dAddr), (bReadByteHdmiGRL(dAddr) & (~(dMsk))) | ((dVal) & (dMsk))))

#define vWriteHdmiSYS(dAddr, dVal)  (hdmi_sys_write(dAddr, dVal))
#define dReadHdmiSYS(dAddr)         (hdmi_sys_read(dAddr))
#define vWriteHdmiSYSMsk(dAddr, dVal, dMsk) (vWriteHdmiSYS((dAddr), (dReadHdmiSYS(dAddr) & (~(dMsk))) | ((dVal) & (dMsk))))

#define vWriteHdmiTOPCK(dAddr, dVal)  (hdmi_hdmitopck_write(dAddr, dVal))
#define dReadHdmiTOPCK(dAddr)         (hdmi_hdmitopck_read(dAddr))
#define vWriteHdmiTOPCKMsk(dAddr, dVal, dMsk) (vWriteHdmiTOPCK((dAddr), (dReadHdmiTOPCK(dAddr) & (~(dMsk))) | ((dVal) & (dMsk))))

#define vWriteIoPll(dAddr, dVal)  (hdmi_pll_write(dAddr, dVal))
#define dReadIoPll(dAddr)         (hdmi_pll_read(dAddr))
#define vWriteIoPllMsk(dAddr, dVal, dMsk) vWriteIoPll((dAddr), (dReadIoPll(dAddr) & (~(dMsk))) | ((dVal) & (dMsk)))


#define vWriteIoPad(dAddr, dVal)  (hdmi_pad_write(dAddr, dVal))
#define dReadIoPad(dAddr)         (hdmi_pad_read(dAddr))
#define vWriteIoPadMsk(dAddr, dVal, dMsk) vWriteIoPad((dAddr), (dReadIoPad(dAddr) & (~(dMsk))) | ((dVal) & (dMsk)))

unsigned char bResolution_4K2K(unsigned char bResIndex)
{
	if ((bResIndex == HDMI_VIDEO_2160P_23_976HZ) || (bResIndex == HDMI_VIDEO_2160P_24HZ)
	        || (bResIndex == HDMI_VIDEO_2160P_25HZ) || (bResIndex == HDMI_VIDEO_2160P_29_97HZ)
	        || (bResIndex == HDMI_VIDEO_2160P_30HZ) || (bResIndex == HDMI_VIDEO_2161P_24HZ))
		return TRUE;
	else
		return FALSE;

}

void vSetHDMITxPLL(unsigned char bResIndex, unsigned char bdeepmode)
{
	unsigned char u4Feq = 0;
	unsigned int v4valueclk = 0;
	unsigned int v4valued2 = 0;
	unsigned int v4valued1 = 0;
	unsigned int v4valued0 = 0;


	if ((bResIndex == HDMI_VIDEO_720x480p_60Hz) || (bResIndex == HDMI_VIDEO_720x576p_50Hz))
		u4Feq = 0;  /* 27M */
	else if ((bResIndex == HDMI_VIDEO_1920x1080p_60Hz)
	         || (bResIndex == HDMI_VIDEO_1920x1080p_50Hz)
	         || (bResIndex == HDMI_VIDEO_1280x720p3d_60Hz)
	         || (bResIndex == HDMI_VIDEO_1280x720p3d_50Hz)
	         || (bResIndex == HDMI_VIDEO_1920x1080i3d_60Hz)
	         || (bResIndex == HDMI_VIDEO_1920x1080i3d_50Hz)
	         || (bResIndex == HDMI_VIDEO_1920x1080p3d_24Hz)
	         || (bResIndex == HDMI_VIDEO_1920x1080p3d_23Hz))
		u4Feq = 2;  /* 148M */
	else
		u4Feq = 1;  /* 74M */

	if (bResolution_4K2K(bResIndex)) {
		u4Feq = 2;  /* 148M */
		bdeepmode = HDMI_DEEP_COLOR_16_BIT;
	}

	vWriteIoPllMsk(HDMI_CON0, ((PREDIV[u4Feq][bdeepmode - 1]) << PREDIV_SHIFT),
	               RG_HDMITX_PLL_PREDIV);
	vWriteIoPllMsk(HDMI_CON0, RG_HDMITX_PLL_POSDIV, RG_HDMITX_PLL_POSDIV);
	vWriteIoPllMsk(HDMI_CON0, (0x1 << PLL_IC_SHIFT), RG_HDMITX_PLL_IC);
	vWriteIoPllMsk(HDMI_CON0, (0x1 << PLL_IR_SHIFT), RG_HDMITX_PLL_IR);
	vWriteIoPllMsk(HDMI_CON1, ((TXDIV[u4Feq][bdeepmode - 1]) << PLL_TXDIV_SHIFT),
	               RG_HDMITX_PLL_TXDIV);
	vWriteIoPllMsk(HDMI_CON0, ((FBKSEL[u4Feq][bdeepmode - 1]) << PLL_FBKSEL_SHIFT),
	               RG_HDMITX_PLL_FBKSEL);
	vWriteIoPllMsk(HDMI_CON0, ((FBKDIV[u4Feq][bdeepmode - 1]) << PLL_FBKDIV_SHIFT),
	               RG_HDMITX_PLL_FBKDIV);
	vWriteIoPllMsk(HDMI_CON1, ((DIVEN[u4Feq][bdeepmode - 1]) << PLL_DIVEN_SHIFT),
	               RG_HDMITX_PLL_DIVEN);
	vWriteIoPllMsk(HDMI_CON0, ((HTPLLBP[u4Feq][bdeepmode - 1]) << PLL_BP_SHIFT),
	               RG_HDMITX_PLL_BP);
	vWriteIoPllMsk(HDMI_CON0, ((HTPLLBC[u4Feq][bdeepmode - 1]) << PLL_BC_SHIFT),
	               RG_HDMITX_PLL_BC);
	vWriteIoPllMsk(HDMI_CON0, ((HTPLLBR[u4Feq][bdeepmode - 1]) << PLL_BR_SHIFT),
	               RG_HDMITX_PLL_BR);

	v4valueclk = *((volatile unsigned int *)(HDMI_EFUSE_BASE + 0x4c8));
	v4valueclk = v4valueclk >> 24;
	v4valued2 = *((volatile unsigned int *)(HDMI_EFUSE_BASE + 0x4c8));
	v4valued2 = (v4valued2 & 0x00ff0000) >> 16;

	v4valued1 = *((volatile unsigned int *)(HDMI_EFUSE_BASE + 0x530));
	v4valued1 = (v4valued1 & 0x00000fc0) >> 6;
	v4valued0 = *((volatile unsigned int *)(HDMI_EFUSE_BASE + 0x530));
	v4valued0 = v4valued0 & 0x3f;

	if ((v4valueclk == 0) || (v4valued2 == 0) || (v4valued1 == 0) || (v4valued0 == 0)) {
		v4valueclk = 0x30;
		v4valued2 = 0x30;
		v4valued1 = 0x30;
		v4valued0 = 0x30;
	}


	if ((u4Feq == 2) && (bdeepmode != HDMI_NO_DEEP_COLOR)) {
		vWriteIoPllMsk(HDMI_CON3, RG_HDMITX_PRD_IMP_EN, RG_HDMITX_PRD_IMP_EN);
		vWriteIoPllMsk(HDMI_CON4, (0x6 << PRD_IBIAS_CLK_SHIFT), RG_HDMITX_PRD_IBIAS_CLK);
		vWriteIoPllMsk(HDMI_CON4, (0x6 << PRD_IBIAS_D2_SHIFT), RG_HDMITX_PRD_IBIAS_D2);
		vWriteIoPllMsk(HDMI_CON4, (0x6 << PRD_IBIAS_D1_SHIFT), RG_HDMITX_PRD_IBIAS_D1);
		vWriteIoPllMsk(HDMI_CON4, (0x6 << PRD_IBIAS_D0_SHIFT), RG_HDMITX_PRD_IBIAS_D0);

		vWriteIoPllMsk(HDMI_CON3, (0xf << DRV_IMP_EN_SHIFT), RG_HDMITX_DRV_IMP_EN);
		vWriteIoPllMsk(HDMI_CON6, (v4valueclk << DRV_IMP_CLK_SHIFT), RG_HDMITX_DRV_IMP_CLK);
		vWriteIoPllMsk(HDMI_CON6, (v4valued2 << DRV_IMP_D2_SHIFT), RG_HDMITX_DRV_IMP_D2);
		vWriteIoPllMsk(HDMI_CON6, (v4valued1 << DRV_IMP_D1_SHIFT), RG_HDMITX_DRV_IMP_D1);
		vWriteIoPllMsk(HDMI_CON6, (v4valued0 << DRV_IMP_D0_SHIFT), RG_HDMITX_DRV_IMP_D0);

		vWriteIoPllMsk(HDMI_CON5, (0x1c << DRV_IBIAS_CLK_SHIFT), RG_HDMITX_DRV_IBIAS_CLK);
		vWriteIoPllMsk(HDMI_CON5, (0x1c << DRV_IBIAS_D2_SHIFT), RG_HDMITX_DRV_IBIAS_D2);
		vWriteIoPllMsk(HDMI_CON5, (0x1c << DRV_IBIAS_D1_SHIFT), RG_HDMITX_DRV_IBIAS_D1);
		vWriteIoPllMsk(HDMI_CON5, (0x1c << DRV_IBIAS_D0_SHIFT), RG_HDMITX_DRV_IBIAS_D0);
	} else {
		vWriteIoPllMsk(HDMI_CON3, 0, RG_HDMITX_PRD_IMP_EN);
		vWriteIoPllMsk(HDMI_CON4, (0x3 << PRD_IBIAS_CLK_SHIFT), RG_HDMITX_PRD_IBIAS_CLK);
		vWriteIoPllMsk(HDMI_CON4, (0x3 << PRD_IBIAS_D2_SHIFT), RG_HDMITX_PRD_IBIAS_D2);
		vWriteIoPllMsk(HDMI_CON4, (0x3 << PRD_IBIAS_D1_SHIFT), RG_HDMITX_PRD_IBIAS_D1);
		vWriteIoPllMsk(HDMI_CON4, (0x3 << PRD_IBIAS_D0_SHIFT), RG_HDMITX_PRD_IBIAS_D0);

		vWriteIoPllMsk(HDMI_CON3, (0x0 << DRV_IMP_EN_SHIFT), RG_HDMITX_DRV_IMP_EN);
		vWriteIoPllMsk(HDMI_CON6, (v4valueclk << DRV_IMP_CLK_SHIFT), RG_HDMITX_DRV_IMP_CLK);
		vWriteIoPllMsk(HDMI_CON6, (v4valued2 << DRV_IMP_D2_SHIFT), RG_HDMITX_DRV_IMP_D2);
		vWriteIoPllMsk(HDMI_CON6, (v4valued1 << DRV_IMP_D1_SHIFT), RG_HDMITX_DRV_IMP_D1);
		vWriteIoPllMsk(HDMI_CON6, (v4valued0 << DRV_IMP_D0_SHIFT), RG_HDMITX_DRV_IMP_D0);

		vWriteIoPllMsk(HDMI_CON5, (0xa << DRV_IBIAS_CLK_SHIFT), RG_HDMITX_DRV_IBIAS_CLK);
		vWriteIoPllMsk(HDMI_CON5, (0xa << DRV_IBIAS_D2_SHIFT), RG_HDMITX_DRV_IBIAS_D2);
		vWriteIoPllMsk(HDMI_CON5, (0xa << DRV_IBIAS_D1_SHIFT), RG_HDMITX_DRV_IBIAS_D1);
		vWriteIoPllMsk(HDMI_CON5, (0xa << DRV_IBIAS_D0_SHIFT), RG_HDMITX_DRV_IBIAS_D0);


	}

	/* power on sequence of hdmi */

}

/* power on sequence of hdmi */
void vTxSignalOnOff(unsigned char bOn)
{

	if (bOn) {
		vWriteIoPllMsk(HDMI_CON1, RG_HDMITX_PLL_AUTOK_EN, RG_HDMITX_PLL_AUTOK_EN);
		vWriteIoPllMsk(HDMI_CON0, RG_HDMITX_PLL_POSDIV, RG_HDMITX_PLL_POSDIV);
		vWriteIoPllMsk(HDMI_CON3, 0, RG_HDMITX_MHLCK_EN);
		vWriteIoPllMsk(HDMI_CON1, RG_HDMITX_PLL_BIAS_EN, RG_HDMITX_PLL_BIAS_EN);
		udelay(100);
		vWriteIoPllMsk(HDMI_CON0, RG_HDMITX_PLL_EN, RG_HDMITX_PLL_EN);
		udelay(100);
		vWriteIoPllMsk(HDMI_CON1, RG_HDMITX_PLL_BIAS_LPF_EN, RG_HDMITX_PLL_BIAS_LPF_EN);
		vWriteIoPllMsk(HDMI_CON1, RG_HDMITX_PLL_TXDIV_EN, RG_HDMITX_PLL_TXDIV_EN);
		vWriteIoPllMsk(HDMI_CON3, RG_HDMITX_SER_EN, RG_HDMITX_SER_EN);
		vWriteIoPllMsk(HDMI_CON3, RG_HDMITX_PRD_EN, RG_HDMITX_PRD_EN);
		vWriteIoPllMsk(HDMI_CON3, RG_HDMITX_DRV_EN, RG_HDMITX_DRV_EN);
		udelay(100);
	} else {
		vWriteIoPllMsk(HDMI_CON3, 0, RG_HDMITX_DRV_EN);
		vWriteIoPllMsk(HDMI_CON3, 0, RG_HDMITX_PRD_EN);
		vWriteIoPllMsk(HDMI_CON3, 0, RG_HDMITX_SER_EN);
		vWriteIoPllMsk(HDMI_CON1, 0, RG_HDMITX_PLL_TXDIV_EN);
		vWriteIoPllMsk(HDMI_CON1, 0, RG_HDMITX_PLL_BIAS_LPF_EN);
		udelay(100);
		vWriteIoPllMsk(HDMI_CON0, 0, RG_HDMITX_PLL_EN);
		udelay(100);
		vWriteIoPllMsk(HDMI_CON1, 0, RG_HDMITX_PLL_BIAS_EN);
		vWriteIoPllMsk(HDMI_CON0, 0, RG_HDMITX_PLL_POSDIV);
		vWriteIoPllMsk(HDMI_CON1, 0, RG_HDMITX_PLL_AUTOK_EN);
		udelay(100);
	}
}

void vConfigHdmiSYS(unsigned char bResIndex)
{
	unsigned char u4Feq=0;

	if ((bResIndex==HDMI_VIDEO_720x480p_60Hz)||(bResIndex==HDMI_VIDEO_720x576p_50Hz))
		u4Feq = 0; //27M
	else if ((bResIndex==HDMI_VIDEO_1920x1080p_60Hz)||(bResIndex==HDMI_VIDEO_1920x1080p_50Hz)
	         ||(bResIndex==HDMI_VIDEO_1280x720p3d_60Hz)||(bResIndex==HDMI_VIDEO_1280x720p3d_50Hz)
	         ||(bResIndex==HDMI_VIDEO_1920x1080i3d_60Hz)||(bResIndex==HDMI_VIDEO_1920x1080i3d_50Hz)
	         ||(bResIndex==HDMI_VIDEO_1920x1080p3d_24Hz)||(bResIndex==HDMI_VIDEO_1920x1080p3d_23Hz))
		u4Feq = 2; //148M
	else
		u4Feq = 1; //74M

	if (bResolution_4K2K(bResIndex)) {
		u4Feq = 3;  /* 297M no deepcolor */
	}

	/*Clear The TVDPLL Div argument,0:26M */
	vWriteHdmiTOPCKMsk(CLK_CFG_6, 0, CLK_DPI0_SEL);
	/*  */
	vWriteHdmiTOPCKMsk(CLK_CFG_7, 0, (0x3 << 8));
	/* This is for HDMI2.0,HDMI1.4 no need*/
	//vWriteHdmiTOPCKMsk(CLK_CFG_8, 0, (0x3 << 16));
	/* */
	vWriteHdmiSYSMsk(HDMI_SYS_CFG20, 0, HDMI2P0_EN);

	vWriteHdmiSYSMsk(MMSYS_CG_CON1, DPI_PIXEL_CLK_EN|HDMI_PIXEL_CLK_EN|HDMI_PLL_CLK_EN, HDMI_PLL_CLK|HDMI_PIXEL_CLK|DPI_PIXEL_CLK);
	//vWriteHdmiSYSMsk(MMSYS_CG_CON1, HDMI_HDCP_CLK_EN|HDMI_HDCP24_CLK_EN, HDMI_HDCP24_CLK|HDMI_HDCP_CLK);

	if (_stAvdAVInfo.e_hdmi_aud_in == SV_SPDIF)
		vWriteHdmiSYSMsk(MMSYS_CG_CON1, HDMI_SPDIF_CLK_EN, HDMI_SPDIF_CLK|HDMI_AUDIO_BCLK);
	else
		vWriteHdmiSYSMsk(MMSYS_CG_CON1, HDMI_SPDIF_CLK_EN|HDMI_AUDIO_BCLK_EN, HDMI_SPDIF_CLK|HDMI_AUDIO_BCLK);

	vWriteHdmiSYSMsk(HDMI_SYS_CFG20, 0, HDMI_OUT_FIFO_EN| MHL_MODE_ON);
	udelay(100);
	vWriteHdmiSYSMsk(HDMI_SYS_CFG20, HDMI_OUT_FIFO_EN, HDMI_OUT_FIFO_EN| MHL_MODE_ON);

	if (u4Feq == 3)
		vWriteHdmiTOPCKMsk(CLK_CFG_6, CLK_DPI0_SEL_D2, CLK_DPI0_SEL);
	else if (u4Feq==2)
		vWriteHdmiTOPCKMsk(CLK_CFG_6, CLK_DPI0_SEL_D4, CLK_DPI0_SEL);
	else if (u4Feq==1)
		vWriteHdmiTOPCKMsk(CLK_CFG_6, CLK_DPI0_SEL_D8, CLK_DPI0_SEL);
	else if (u4Feq==0)
		vWriteHdmiTOPCKMsk(CLK_CFG_6, CLK_DPI0_SEL_D8, CLK_DPI0_SEL);

	vWriteHdmiTOPCKMsk(CLK_CFG_7, (0x1 << 8), (0x3 << 8));

	vWriteHdmiANAMsk(PLL_TEST_CON0, (0x3 << 16),TVDPLL_POSDIV_2);

	vWriteHdmiTOPCKMsk(CLK_AUDDIV_4, CLK_MUX_POWERUP, CLK_MUX_POWERDOWN);
	if (_stAvdAVInfo.e_hdmi_aud_in == SV_SPDIF) {
		vWriteHdmiTOPCKMsk(CLK_AUDDIV_4, CLK_SEL_INPUT_SPDIF, CLK_SEL_INPUT_SPDIF);
		vWriteHdmiTOPCKMsk(CLK_AUDDIV_4, CLK_SPDIF_HDMI1, CLK_SPDIF_HDMI2);
		vWriteHdmiTOPCKMsk(CLK_AUDDIV_4, CLK_SPDIF_1, CLK_SPDIF_2);
	} else
		vWriteHdmiTOPCKMsk(CLK_AUDDIV_4, CLK_SEL_INPUT_IIS, CLK_SEL_INPUT_SPDIF);

	//vWriteHdmiTOPCKMsk(HDMICLK_CFG_9, AD_APLL_CK, CLK_APLL_SEL);
}

unsigned char vAudioPllSelect(void)
{
	if ((_stAvdAVInfo.e_iec_frame == IEC_44K)
	        || (_stAvdAVInfo.e_iec_frame == IEC_88K)
	        || (_stAvdAVInfo.e_iec_frame == IEC_176K)
	        || (_stAvdAVInfo.e_iec_frame == IEC_22K)) {
		return 1; /*apll1*/
	} else
		return 2; /*apll2*/
}

void vConfigHdmi2SYS(unsigned char bResIndex)
{
	unsigned char u4Feq=0;

	if ((bResIndex==HDMI_VIDEO_720x480p_60Hz)||(bResIndex==HDMI_VIDEO_720x576p_50Hz))
		u4Feq = 0; //27M
	else if ((bResIndex==HDMI_VIDEO_1920x1080p_60Hz)||(bResIndex==HDMI_VIDEO_1920x1080p_50Hz)
	         ||(bResIndex==HDMI_VIDEO_1280x720p3d_60Hz)||(bResIndex==HDMI_VIDEO_1280x720p3d_50Hz)
	         ||(bResIndex==HDMI_VIDEO_1920x1080i3d_60Hz)||(bResIndex==HDMI_VIDEO_1920x1080i3d_50Hz)
	         ||(bResIndex==HDMI_VIDEO_1920x1080p3d_24Hz)||(bResIndex==HDMI_VIDEO_1920x1080p3d_23Hz))
		u4Feq = 2; //148M
	else
		u4Feq = 1; //74M

	if ((bResIndex==HDMI_VIDEO_2160P_30HZ)||(bResIndex==HDMI_VIDEO_2161P_24HZ)) {
		u4Feq = 3;
	}

	vWriteHdmiTOPCKMsk(CLK_CFG_6, 0, CLK_DPI0_SEL);
	vWriteHdmiTOPCKMsk(CLK_CFG_7, 0, (0x3 << 8));
	vWriteHdmiTOPCKMsk(CLK_CFG_8, 0, (0x3 << 16));

	vWriteHdmiSYSMsk(MMSYS_CG_CON1, DPI_PIXEL_CLK_EN|HDMI_PIXEL_CLK_EN|HDMI_PLL_CLK_EN, HDMI_PLL_CLK|HDMI_PIXEL_CLK|DPI_PIXEL_CLK);
	vWriteHdmiSYSMsk(MMSYS_CG_CON1, HDMI_HDCP_CLK_EN|HDMI_HDCP24_CLK_EN, HDMI_HDCP24_CLK|HDMI_HDCP_CLK);

	if (_stAvdAVInfo.e_hdmi_aud_in == SV_SPDIF)
		vWriteHdmiSYSMsk(MMSYS_CG_CON1, HDMI_SPDIF_CLK_EN, HDMI_SPDIF_CLK|HDMI_AUDIO_BCLK);
	else
		vWriteHdmiSYSMsk(MMSYS_CG_CON1, HDMI_SPDIF_CLK_EN|HDMI_AUDIO_BCLK_EN, HDMI_SPDIF_CLK|HDMI_AUDIO_BCLK);

	vWriteHdmiSYSMsk(HDMI_SYS_CFG20, 0, HDMI_OUT_FIFO_EN| MHL_MODE_ON);
	udelay(100);
	vWriteHdmiSYSMsk(HDMI_SYS_CFG20, HDMI_OUT_FIFO_EN, HDMI_OUT_FIFO_EN| MHL_MODE_ON);

	if (u4Feq==3)
		vWriteHdmiTOPCKMsk(CLK_CFG_6, CLK_DPI0_SEL_D2, CLK_DPI0_SEL);
	else if (u4Feq==2)
		vWriteHdmiTOPCKMsk(CLK_CFG_6, CLK_DPI0_SEL_D4, CLK_DPI0_SEL);
	else if (u4Feq==1)
		vWriteHdmiTOPCKMsk(CLK_CFG_6, CLK_DPI0_SEL_D8, CLK_DPI0_SEL);
	else if (u4Feq==0)
		vWriteHdmiTOPCKMsk(CLK_CFG_6, CLK_DPI0_SEL_D8, CLK_DPI0_SEL);

	vWriteHdmiTOPCKMsk(CLK_CFG_7, (0x1 << 8), (0x3 << 8));
	vWriteHdmiTOPCKMsk(CLK_CFG_8, (0x2 << 16) + (0x2 << 8), (0x3 << 16) + (0x3 << 8));

	vWriteHdmiANAMsk(PLL_TEST_CON0, (0x3 << 16),TVDPLL_POSDIV_2);

	vWriteHdmiTOPCKMsk(CLK_AUDDIV_4, CLK_MUX_POWERUP, CLK_MUX_POWERDOWN);
	if (_stAvdAVInfo.e_hdmi_aud_in == SV_SPDIF) {
		vWriteHdmiTOPCKMsk(CLK_AUDDIV_4, CLK_SEL_INPUT_SPDIF, CLK_SEL_INPUT_SPDIF);
		vWriteHdmiTOPCKMsk(CLK_AUDDIV_4, CLK_SPDIF_HDMI2, CLK_SPDIF_HDMI2);
		if (vAudioPllSelect() == 1)
			vWriteHdmiTOPCKMsk(CLK_AUDDIV_4, CLK_SEL_APLL1, CLK_SEL_APLL2);
		else
			vWriteHdmiTOPCKMsk(CLK_AUDDIV_4, CLK_SEL_APLL2, CLK_SEL_APLL2);
	} else
		vWriteHdmiTOPCKMsk(CLK_AUDDIV_4, CLK_SEL_INPUT_IIS, CLK_SEL_INPUT_SPDIF);

	//vWriteHdmiTOPCKMsk(HDMICLK_CFG_9, AD_APLL_CK, CLK_APLL_SEL);
}


void vEnableDeepColor(unsigned char ui1Mode)
{
	unsigned int u4Data;

	if (ui1Mode == HDMI_DEEP_COLOR_10_BIT) {
		u4Data = COLOR_10BIT_MODE;
	} else if (ui1Mode == HDMI_DEEP_COLOR_12_BIT) {
		u4Data = COLOR_12BIT_MODE;
	} else if (ui1Mode == HDMI_DEEP_COLOR_16_BIT) {
		u4Data = COLOR_16BIT_MODE;
	} else {
		u4Data = COLOR_8BIT_MODE;
	}

	if (u4Data == COLOR_8BIT_MODE) {
		/*DEEP_COLOR_MODE_MASK(bit 3) is not included in mt8173, need to be test */
		vWriteHdmiSYSMsk(HDMI_SYS_CFG20, u4Data, DEEP_COLOR_MODE_MASK| DEEP_COLOR_EN);
	} else {
		vWriteHdmiSYSMsk(HDMI_SYS_CFG20, u4Data|DEEP_COLOR_EN, DEEP_COLOR_MODE_MASK| DEEP_COLOR_EN);
	}

}

void vEnableDeepColor2(unsigned char ui1Mode)
{
	unsigned int u4Data;

	if (ui1Mode == HDMI_DEEP_COLOR_10_BIT) {
		u4Data = DEEPCOLOR_MODE_10BIT;
	} else if (ui1Mode == HDMI_DEEP_COLOR_12_BIT) {
		u4Data = DEEPCOLOR_MODE_12BIT;
	} else if (ui1Mode == HDMI_DEEP_COLOR_16_BIT) {
		u4Data = DEEPCOLOR_MODE_16BIT;
	} else {
		u4Data = DEEPCOLOR_MODE_8BIT;
	}

	if (u4Data == DEEPCOLOR_MODE_8BIT) {
		vWriteHdmiGRLMsk(TOP_CFG00, u4Data, DEEPCOLOR_MODE_MASKBIT| DEEPCOLOR_PAT_EN);
	} else {
		vWriteHdmiGRLMsk(TOP_CFG00, u4Data|DEEPCOLOR_PAT_EN, DEEPCOLOR_MODE_MASKBIT| DEEPCOLOR_PAT_EN);
	}

}

void vEnableHdmiMode(unsigned char bOn)
{
	unsigned char bData;
	if (bOn==1) {
		bData=bReadByteHdmiGRL(GRL_CFG1);
		bData &= ~CFG1_DVI;//enable HDMI mode
		vWriteByteHdmiGRL(GRL_CFG1,bData);
	} else {
		bData=bReadByteHdmiGRL(GRL_CFG1);
		bData |= CFG1_DVI;//disable HDMI mode
		vWriteByteHdmiGRL(GRL_CFG1,bData);
	}

}

void vEnableHdmi2Mode(unsigned char bOn)
{
	if (bOn==1) {
		vWriteHdmiGRLMsk(TOP_CFG00, HDMI_MODE_HDMI, HDMI_MODE_HDMI);
	} else {
		vWriteHdmiGRLMsk(TOP_CFG00, HDMI_MODE_DVI, HDMI_MODE_HDMI);
	}

}

void vResetAudioHDMI2(unsigned char bRst)
{
	if (bRst) {
		vWriteHdmiGRLMsk(AIP_TXCTRL,RST4AUDIO|RST4AUDIO_FIFO|RST4AUDIO_ACR,RST4AUDIO|RST4AUDIO_FIFO|RST4AUDIO_ACR);
	} else {
		vWriteHdmiGRLMsk(AIP_TXCTRL,0,RST4AUDIO|RST4AUDIO_FIFO|RST4AUDIO_ACR);
	}
}

void vEnableNCTSAutoWrite(void)
{
	unsigned char bData;
	bData=bReadByteHdmiGRL(GRL_DIVN);
	bData |= NCTS_WRI_ANYTIME;//enabel N-CTS can be written in any time
	vWriteByteHdmiGRL(GRL_DIVN,bData);

}

void vResetHDMI(unsigned char bRst)
{
	if (bRst) {
		vWriteHdmiSYSMsk(HDMI_SYS_CFG1C,HDMI_RST,HDMI_RST);
	} else {
		vWriteHdmiSYSMsk(HDMI_SYS_CFG1C,0,HDMI_RST);
		vWriteHdmiGRLMsk(GRL_CFG3,0,CFG3_CONTROL_PACKET_DELAY);//Designer suggest adjust Control packet deliver time
		vWriteHdmiSYSMsk(HDMI_SYS_CFG1C,ANLG_ON,ANLG_ON);
	}
}

void vResetHDMI2(unsigned char bRst)
{
	if (bRst) {
		vWriteHdmiSYSMsk(HDMI_SYS_CFG1C,HDMI_RST,HDMI_RST);
	} else {
		vWriteHdmiSYSMsk(HDMI_SYS_CFG1C,0,HDMI_RST);
	}
}

void vEnableNotice(unsigned char bOn)
{
	unsigned char bData;
	if (bOn == 1) {
		bData=bReadByteHdmiGRL(GRL_CFG2);
		/*Using inverse audio clock to latch data for audio down sample mode */
		bData |= 0x40; //temp. solve 720p issue. to avoid audio packet jitter problem
		vWriteByteHdmiGRL(GRL_CFG2, bData);
	} else {
		bData=bReadByteHdmiGRL(GRL_CFG2);
		bData &= ~0x40;
		vWriteByteHdmiGRL(GRL_CFG2, bData);
	}
}

void vHwNCTSOnOff(unsigned char bHwNctsOn)
{
	unsigned char bData;
	bData=bReadByteHdmiGRL(GRL_CTS_CTRL);

	if (bHwNctsOn == 1)
		bData &= ~CTS_CTRL_SOFT;
	else
		bData |= CTS_CTRL_SOFT;

	vWriteByteHdmiGRL(GRL_CTS_CTRL, bData);

}

void vSetChannelSwap(unsigned char u1SwapBit)
{
	vWriteHdmiGRLMsk(GRL_CH_SWAP, u1SwapBit, 0xff);
}

void vEnableIecTxRaw(void)
{
	unsigned char bData;
	bData=bReadByteHdmiGRL(GRL_MIX_CTRL);
	bData |= MIX_CTRL_FLAT;
	vWriteByteHdmiGRL(GRL_MIX_CTRL, bData);
}

void vSetHdmiI2SDataFmt(unsigned char bFmt)
{
	unsigned char bData;
	bData=bReadByteHdmiGRL(GRL_CFG0);
	bData &=~0x33;
	switch (bFmt) {
		case RJT_24BIT:
			bData |= (CFG0_I2S_MODE_RTJ|CFG0_I2S_MODE_24Bit);
			break;

		case RJT_16BIT:
			bData |= (CFG0_I2S_MODE_RTJ|CFG0_I2S_MODE_16Bit);
			break;

		case LJT_24BIT:
			bData |= (CFG0_I2S_MODE_LTJ|CFG0_I2S_MODE_24Bit);
			break;

		case LJT_16BIT:
			bData |= (CFG0_I2S_MODE_LTJ|CFG0_I2S_MODE_16Bit);
			break;

		case I2S_24BIT:
			bData |= (CFG0_I2S_MODE_I2S|CFG0_I2S_MODE_24Bit);
			break;

		case I2S_16BIT:
			bData |= (CFG0_I2S_MODE_I2S|CFG0_I2S_MODE_16Bit);
			break;

	}


	vWriteByteHdmiGRL(GRL_CFG0, bData);
}

void vAOUT_BNUM_SEL(unsigned char  bBitNum)
{
	vWriteByteHdmiGRL(GRL_AOUT_BNUM_SEL, bBitNum);

}

void vSetHdmiHighBitrate(unsigned char fgHighBitRate)
{
	unsigned char bData;
	if (fgHighBitRate ==1) {
		bData=bReadByteHdmiGRL(GRL_AOUT_BNUM_SEL);
		bData |= HIGH_BIT_RATE_PACKET_ALIGN;
		vWriteByteHdmiGRL(GRL_AOUT_BNUM_SEL, bData);
		udelay(100);//1ms
		bData=bReadByteHdmiGRL(GRL_AUDIO_CFG);
		bData |= HIGH_BIT_RATE;
		vWriteByteHdmiGRL(GRL_AUDIO_CFG, bData);
	} else {
		bData=bReadByteHdmiGRL(GRL_AOUT_BNUM_SEL);
		bData &= ~HIGH_BIT_RATE_PACKET_ALIGN;
		vWriteByteHdmiGRL(GRL_AOUT_BNUM_SEL, bData);

		bData=bReadByteHdmiGRL(GRL_AUDIO_CFG);
		bData &= ~HIGH_BIT_RATE;
		vWriteByteHdmiGRL(GRL_AUDIO_CFG, bData);
	}


}

void vDSTNormalDouble(unsigned char fgEnable)
{
	unsigned char bData;
	if (fgEnable) {
		bData=bReadByteHdmiGRL(GRL_AUDIO_CFG);
		bData |=DST_NORMAL_DOUBLE;
		vWriteByteHdmiGRL(GRL_AUDIO_CFG, bData);
	} else {
		bData=bReadByteHdmiGRL(GRL_AUDIO_CFG);
		bData&=~DST_NORMAL_DOUBLE;
		vWriteByteHdmiGRL(GRL_AUDIO_CFG, bData);
	}

}

void vEnableDSTConfig(unsigned char fgEnable)
{
	unsigned char bData;
	if (fgEnable) {
		bData=bReadByteHdmiGRL(GRL_AUDIO_CFG);
		bData |=SACD_DST;
		vWriteByteHdmiGRL(GRL_AUDIO_CFG, bData);
	} else {
		bData=bReadByteHdmiGRL(GRL_AUDIO_CFG);
		bData&=~SACD_DST;
		vWriteByteHdmiGRL(GRL_AUDIO_CFG, bData);
	}

}

void vDisableDsdConfig(void)
{
	unsigned char bData;

	bData=bReadByteHdmiGRL(GRL_AUDIO_CFG);
	bData&=~SACD_SEL;
	vWriteByteHdmiGRL(GRL_AUDIO_CFG, bData);

}

void vSetHdmiI2SChNum(unsigned char bChNum, unsigned char bChMapping)
{
	unsigned char bData, bData1, bData2, bData3;

	if (bChNum==2) { //I2S 2ch
		bData = 0x04;//2ch data
		bData1 = 0x50;//data0


	} else if ((bChNum==3)||(bChNum==4)) { //I2S 2ch
		if ((bChNum==4)&&(bChMapping == 0x08)) {
			bData = 0x14;//4ch data

		} else {
			bData = 0x0c;//4ch data
		}
		bData1 = 0x50;//data0


	} else if ((bChNum == 6)||(bChNum == 5)) { //I2S 5.1ch
		if ((bChNum==6)&&(bChMapping == 0x0E)) {
			bData = 0x3C;//6.0 ch data
			bData1 = 0x50;//data0
		} else {
			bData = 0x1C;//5.1ch data, 5/0ch
			bData1 = 0x50;//data0
		}


	} else if (bChNum == 8) { //I2S 5.1ch
		bData = 0x3C;//7.1ch data
		bData1 = 0x50;//data0
	} else if (bChNum == 7) { //I2S 6.1ch
		bData = 0x3C;//6.1ch data
		bData1 = 0x50;//data0
	} else {
		bData = 0x04;//2ch data
		bData1 = 0x50;//data0
	}

	bData2=0xc6;
	bData3=0xfa;

	vWriteByteHdmiGRL(GRL_CH_SW0, bData1);
	vWriteByteHdmiGRL(GRL_CH_SW1, bData2);
	vWriteByteHdmiGRL(GRL_CH_SW2, bData3);
	vWriteByteHdmiGRL(GRL_I2S_UV, bData);

	//vDisableDsdConfig();

}

void vSetHdmiIecI2s(unsigned char bIn)
{
	unsigned char bData;

	if (bIn == SV_SPDIF) {

		bData=bReadByteHdmiGRL(GRL_CFG1);

		if ((bData&CFG1_SPDIF)==0) {
			bData |= CFG1_SPDIF;
			vWriteByteHdmiGRL(GRL_CFG1, bData);
		}
	} else {
		bData=bReadByteHdmiGRL(GRL_CFG1);
		if (bData&CFG1_SPDIF) {
			bData &= ~CFG1_SPDIF;
			vWriteByteHdmiGRL(GRL_CFG1, bData);
		}
		bData=bReadByteHdmiGRL(GRL_CFG1);
	}
}

void vSetHDMISRCOff(void)
{
	unsigned char bData;

	bData=bReadByteHdmiGRL(GRL_MIX_CTRL);
	bData &= ~MIX_CTRL_SRC_EN;
	vWriteByteHdmiGRL(GRL_MIX_CTRL, bData);
	bData = 0x00;
	vWriteByteHdmiGRL(GRL_SHIFT_L1, bData);
}

void vSetHDMIFS(unsigned char bFs, unsigned char fgAclInv)
{

	unsigned char bData;

	bData=bReadByteHdmiGRL(GRL_CFG5);
	bData &= CFG5_CD_RATIO_MASK;
	bData |= bFs;
	vWriteByteHdmiGRL(GRL_CFG5, bData);

	if (fgAclInv ==1) {
		bData=bReadByteHdmiGRL(GRL_CFG2);
		bData |= 0x80;
		vWriteByteHdmiGRL(GRL_CFG2, bData);
	} else {
		bData=bReadByteHdmiGRL(GRL_CFG2);
		bData &= ~0x80;
		vWriteByteHdmiGRL(GRL_CFG2, bData);
	}

}

void vHalHDMI_NCTS(unsigned char bAudioFreq, unsigned char bPix, unsigned char bDeepMode)
{
	unsigned char bTemp, bData, bData1[NCTS_BYTES];
	unsigned int u4Temp;


	bData=0;
	vWriteByteHdmiGRL(GRL_NCTS, bData);//YT suggest 3 dummy N-CTS
	vWriteByteHdmiGRL(GRL_NCTS, bData);
	vWriteByteHdmiGRL(GRL_NCTS, bData);

	for (bTemp=0; bTemp<NCTS_BYTES; bTemp++) {
		bData1[bTemp] = 0;
	}

	if (bDeepMode == HDMI_NO_DEEP_COLOR) {
		for (bTemp=0; bTemp<NCTS_BYTES; bTemp++) {

			if ((bAudioFreq < 7) && (bPix < 9))

				bData1[bTemp]= HDMI_NCTS[bAudioFreq][bPix][bTemp];
		}

		u4Temp = (bData1[0]<<24)|(bData1[1]<<16)|(bData1[2]<<8)|(bData1[3]);//CTS

	} else {
		for (bTemp=0; bTemp<NCTS_BYTES; bTemp++) {
			if ((bAudioFreq < 7) && (bPix < 9))

				bData1[bTemp] = HDMI_NCTS[bAudioFreq][bPix][bTemp];
		}

		u4Temp = (bData1[0]<<24)|(bData1[1]<<16)|(bData1[2]<<8)|(bData1[3]);

		if (bDeepMode == HDMI_DEEP_COLOR_10_BIT) {
			u4Temp = (u4Temp >> 2)*5;// (*5/4)
		} else if (bDeepMode == HDMI_DEEP_COLOR_12_BIT) {
			u4Temp = (u4Temp >> 1)*3;// (*3/2)
		} else if (bDeepMode == HDMI_DEEP_COLOR_16_BIT) {
			u4Temp = (u4Temp <<1);// (*2)
		}

		bData1[0]= (u4Temp >> 24)& 0xff;
		bData1[1]= (u4Temp >> 16)& 0xff;
		bData1[2]= (u4Temp >> 8)& 0xff;
		bData1[3]= (u4Temp)& 0xff;

	}
	for (bTemp=0; bTemp<NCTS_BYTES; bTemp++) {
		bData = bData1[bTemp];
		vWriteByteHdmiGRL( GRL_NCTS, bData);
	}

}

void vHDMI_NCTS(unsigned char bHDMIFsFreq, unsigned char bResolution, unsigned char bdeepmode)
{
	unsigned char  bPix;


	vWriteHdmiGRLMsk(DUMMY_304, AUDIO_I2S_NCTS_SEL_64, AUDIO_I2S_NCTS_SEL);

	switch (bResolution) {
		case HDMI_VIDEO_720x480p_60Hz:
		case HDMI_VIDEO_720x576p_50Hz:
		default:
			bPix = 0;
			break;

		case HDMI_VIDEO_1280x720p_60Hz: //74.175M pixel clock
		case HDMI_VIDEO_1920x1080i_60Hz:
		case HDMI_VIDEO_1920x1080p_23Hz:
			bPix = 2;
			break;

		case HDMI_VIDEO_1280x720p_50Hz: //74.25M pixel clock
		case HDMI_VIDEO_1920x1080i_50Hz:
		case HDMI_VIDEO_1920x1080p_24Hz:
			bPix = 3;
			break;
		case HDMI_VIDEO_1920x1080p_60Hz: //148.35M pixel clock
		case HDMI_VIDEO_1280x720p3d_60Hz:
		case HDMI_VIDEO_1920x1080i3d_60Hz:
		case HDMI_VIDEO_1920x1080p3d_23Hz:
			bPix = 4;
			break;
		case HDMI_VIDEO_1920x1080p_50Hz: //148.50M pixel clock
		case HDMI_VIDEO_1280x720p3d_50Hz:
		case HDMI_VIDEO_1920x1080i3d_50Hz:
		case HDMI_VIDEO_1920x1080p3d_24Hz:
			bPix = 5;
			break;
		case HDMI_VIDEO_2160P_23_976HZ:
		case HDMI_VIDEO_2160P_29_97HZ:// 296.976m pixel clock
			bPix = 7;
			break;
		case HDMI_VIDEO_2160P_24HZ:   //297m pixel clock
		case HDMI_VIDEO_2160P_25HZ:   //297m pixel clock
		case HDMI_VIDEO_2160P_30HZ:   //297m pixel clock
		case HDMI_VIDEO_2161P_24HZ:   //297m pixel clock
			bPix = 8;
			break;

	}

	vHalHDMI_NCTS(bHDMIFsFreq, bPix, bdeepmode);

}

void vReEnableSRC(void)
{
	unsigned char bData;

	bData=bReadByteHdmiGRL( GRL_MIX_CTRL);
	if (bData & MIX_CTRL_SRC_EN) {
		bData &= ~MIX_CTRL_SRC_EN;
		vWriteByteHdmiGRL( GRL_MIX_CTRL, bData);
		udelay(255);
		bData |= MIX_CTRL_SRC_EN;
		vWriteByteHdmiGRL(GRL_MIX_CTRL, bData);
	}

}

void vHwSet_Hdmi_I2S_C_Status (unsigned char *prLChData, unsigned char *prRChData)
{
	unsigned char bData;

	bData = prLChData[0];

	vWriteByteHdmiGRL(GRL_I2S_C_STA0, bData);
	vWriteByteHdmiGRL(GRL_L_STATUS_0, bData);

	bData = prRChData[0];

	vWriteByteHdmiGRL(GRL_R_STATUS_0, bData);

	bData= prLChData[1];
	vWriteByteHdmiGRL( GRL_I2S_C_STA1, bData);
	vWriteByteHdmiGRL(GRL_L_STATUS_1, bData);
	bData= prRChData[1];
	vWriteByteHdmiGRL(GRL_R_STATUS_1, bData);

	bData= prLChData[2];
	vWriteByteHdmiGRL(GRL_I2S_C_STA2, bData);
	vWriteByteHdmiGRL(GRL_L_STATUS_2, bData);
	bData= prRChData[2];
	vWriteByteHdmiGRL(GRL_R_STATUS_2, bData);

	bData= prLChData[3];
	vWriteByteHdmiGRL(GRL_I2S_C_STA3, bData);
	vWriteByteHdmiGRL(GRL_L_STATUS_3, bData);
	bData= prRChData[3];
	vWriteByteHdmiGRL(GRL_R_STATUS_3, bData);

	bData=prLChData[4];
	vWriteByteHdmiGRL( GRL_I2S_C_STA4, bData);
	vWriteByteHdmiGRL(GRL_L_STATUS_4, bData);
	bData=prRChData[4];
	vWriteByteHdmiGRL(GRL_R_STATUS_4, bData);

	for (bData =0; bData < 19; bData++) {
		vWriteByteHdmiGRL(GRL_L_STATUS_5+bData*4, 0);
		vWriteByteHdmiGRL(GRL_R_STATUS_5+bData*4, 0);

	}
}

void vHDMI_I2S_C_Status(void)
{
	unsigned char bData = 0;
	unsigned char bhdmi_RCh_status[5];
	unsigned char bhdmi_LCh_status[5];


	bhdmi_LCh_status[0]= _stAvdAVInfo.bhdmiLChstatus[0];
	bhdmi_LCh_status[1]= _stAvdAVInfo.bhdmiLChstatus[1];
	bhdmi_LCh_status[2]= _stAvdAVInfo.bhdmiLChstatus[2];
	bhdmi_RCh_status[0]= _stAvdAVInfo.bhdmiRChstatus[0];
	bhdmi_RCh_status[1]= _stAvdAVInfo.bhdmiRChstatus[1];
	bhdmi_RCh_status[2]= _stAvdAVInfo.bhdmiRChstatus[2];


	bhdmi_LCh_status[0]&= ~0x02;
	bhdmi_RCh_status[0]&= ~0x02;

	bData = _stAvdAVInfo.bhdmiLChstatus[3]&0xf0;

	switch (_stAvdAVInfo.e_hdmi_fs) {
		case HDMI_FS_32K:
			bData |= 0x03 ;
			break ;
		case HDMI_FS_44K:
			break ;
		case HDMI_FS_88K:
			bData |= 0x08 ;
			break ;
		case HDMI_FS_96K:
			bData |= 0x0A ;
			break ;
		case HDMI_FS_176K:
			bData |= 0x0C;
			break ;
		case HDMI_FS_192K:
			bData |= 0x0E ;
			break ;
		case HDMI_FS_48K:
		default:
			bData |= 0x02 ;
			break ;

	}


	bhdmi_LCh_status[3] = bData;
	bhdmi_RCh_status[3] = bData;

	bData= _stAvdAVInfo.bhdmiLChstatus[4];

	bData |= ((~( bhdmi_LCh_status[3] & 0x0f))<<4);

	bhdmi_LCh_status[4]= bData;
	bhdmi_RCh_status[4]= bData;

	vHwSet_Hdmi_I2S_C_Status (&bhdmi_LCh_status[0], &bhdmi_RCh_status[0]);


}

void vHalSendAudioInfoFrame(unsigned char bData1,unsigned char bData2,unsigned char bData4,unsigned char bData5)
{
	unsigned char bAUDIO_CHSUM;
	unsigned char bData=0;

	vWriteHdmiGRLMsk(GRL_CTRL, 0, CTRL_AUDIO_EN);
	vWriteByteHdmiGRL(GRL_INFOFRM_VER, AUDIO_VERS);
	vWriteByteHdmiGRL(GRL_INFOFRM_TYPE, AUDIO_TYPE);
	vWriteByteHdmiGRL(GRL_INFOFRM_LNG, AUDIO_LEN);

	bAUDIO_CHSUM = AUDIO_TYPE + AUDIO_VERS + AUDIO_LEN;

	bAUDIO_CHSUM += bData1;
	bAUDIO_CHSUM += bData2;
	bAUDIO_CHSUM += bData4;
	bAUDIO_CHSUM += bData5;

	bAUDIO_CHSUM = 0x100 - bAUDIO_CHSUM;
	vWriteByteHdmiGRL(GRL_IFM_PORT, bAUDIO_CHSUM);
	vWriteByteHdmiGRL(GRL_IFM_PORT, bData1);
	vWriteByteHdmiGRL(GRL_IFM_PORT, bData2);//bData2
	vWriteByteHdmiGRL(GRL_IFM_PORT, 0);//bData3
	vWriteByteHdmiGRL(GRL_IFM_PORT, bData4);
	vWriteByteHdmiGRL(GRL_IFM_PORT, bData5);

	for (bData=0; bData<5; bData++) {
		vWriteByteHdmiGRL(GRL_IFM_PORT, 0);
	}
	bData=bReadByteHdmiGRL( GRL_CTRL);
	bData |= CTRL_AUDIO_EN;
	vWriteByteHdmiGRL(GRL_CTRL, bData);

}

void vSendAudioInfoFrame(void)
{

	unsigned char _bAudInfoFm[5];

	_bAudInfoFm[0] = 0;
	_bAudInfoFm[1] = 0;
	_bAudInfoFm[2] = 0;
	_bAudInfoFm[3] = 0;
	_bAudInfoFm[4] = 0;

	if (_stAvdAVInfo.e_hdmi_aud_in == SV_SPDIF) {
		_bAudInfoFm[0] = 0x00; //CC as 0,
		_bAudInfoFm[3] = 0x00;//CA 2ch
	} else { //pcm
		switch (_stAvdAVInfo.ui1_aud_out_ch_number) {
			case 2: // FL/FR
				_bAudInfoFm[0] = 0x01;
				_bAudInfoFm[3] = 0x00;
				break;
			case 6: // FL/FR/FC/RL/RR/RC
				_bAudInfoFm[0] = 0x05;
				_bAudInfoFm[3] = 0x0B;
				break;
			case 8: // FL/FR/FC/RL/RR/RC
				_bAudInfoFm[0] = 0x07;
				_bAudInfoFm[3] = 0x1F;
				break;

			default:
				_bAudInfoFm[0] = 0x01;
				_bAudInfoFm[3] = 0x00;
				break;
		}

	}

	vHalSendAudioInfoFrame(_bAudInfoFm[0],_bAudInfoFm[1],_bAudInfoFm[3],_bAudInfoFm[4]);


}

void vHalSendAVIInfoFrame(unsigned char *pr_bData)
{
	unsigned char bAVI_CHSUM;
	unsigned char bData1=0, bData2=0, bData3=0, bData4=0, bData5=0;
	unsigned char bData;

	bData1= *pr_bData;
	bData2= *(pr_bData+1);
	bData3= *(pr_bData+2);
	bData4= *(pr_bData+3);
	bData5= *(pr_bData+4);

	vWriteHdmiGRLMsk(GRL_CTRL, 0, CTRL_AVI_EN);
	vWriteByteHdmiGRL(GRL_INFOFRM_VER, AVI_VERS);
	vWriteByteHdmiGRL(GRL_INFOFRM_TYPE, AVI_TYPE);
	vWriteByteHdmiGRL(GRL_INFOFRM_LNG, AVI_LEN);

	bAVI_CHSUM = AVI_TYPE + AVI_VERS + AVI_LEN;

	bAVI_CHSUM += bData1;
	bAVI_CHSUM += bData2;
	bAVI_CHSUM += bData3;
	bAVI_CHSUM += bData4;
	bAVI_CHSUM += bData5;
	bAVI_CHSUM = 0x100 - bAVI_CHSUM;
	vWriteByteHdmiGRL(GRL_IFM_PORT, bAVI_CHSUM);
	vWriteByteHdmiGRL(GRL_IFM_PORT, bData1);
	vWriteByteHdmiGRL(GRL_IFM_PORT, bData2);
	vWriteByteHdmiGRL(GRL_IFM_PORT, bData3);
	vWriteByteHdmiGRL(GRL_IFM_PORT, bData4);
	vWriteByteHdmiGRL(GRL_IFM_PORT, bData5);

	for (bData2=0; bData2<8; bData2++) {
		vWriteByteHdmiGRL(GRL_IFM_PORT, 0);
	}
	bData=bReadByteHdmiGRL(GRL_CTRL);
	bData |= CTRL_AVI_EN;
	vWriteByteHdmiGRL(GRL_CTRL, bData);
}

void vSendAVIInfoFrame(unsigned char ui1resindex, unsigned char ui1colorspace)
{
	unsigned char AviInfoFm[5];
	if (ui1colorspace == HDMI_YCBCR_444) {
		AviInfoFm[0]=0x40;
	} else if (ui1colorspace == HDMI_YCBCR_422) {
		AviInfoFm[0]=0x20;
	} else {
		AviInfoFm[0]=0x00;
	}

	if (!bResolution_4K2K(ui1resindex))
		AviInfoFm[0] |= 0x10; /* A0=1, Active format (R0~R3) inf valid */


	AviInfoFm[1]=0x0;//bData2


	if ((ui1resindex==HDMI_VIDEO_720x480p_60Hz)||(ui1resindex==HDMI_VIDEO_720x576p_50Hz)) {
		AviInfoFm[1] |= AV_INFO_SD_ITU601;
	} else {
		AviInfoFm[1] |= AV_INFO_HD_ITU709;
	}

	AviInfoFm[1] |= 0x20;
	AviInfoFm[1] |= 0x08;
	AviInfoFm[2] =0; //bData3
	AviInfoFm[2] |= 0x04; //limit Range
	if (bResolution_4K2K(ui1resindex))
		AviInfoFm[3] = 0; /* bData4 */
	else
		AviInfoFm[3] = HDMI_VIDEO_ID_CODE[ui1resindex];   /* bData4 */


	if ((AviInfoFm[1] & AV_INFO_16_9_OUTPUT) && ((ui1resindex==HDMI_VIDEO_720x480p_60Hz)||(ui1resindex==HDMI_VIDEO_720x576p_50Hz))) {
		AviInfoFm[3] = AviInfoFm[3]+1;
	}

	AviInfoFm[4]  = 0x00;

	vHalSendAVIInfoFrame(&AviInfoFm[0]);

}


void vSend_AVUNMUTE(void)
{
	unsigned char bData;

	bData=bReadByteHdmiGRL(GRL_CFG4);
	bData |= CFG4_AV_UNMUTE_EN;//disable original mute
	bData &= ~CFG4_AV_UNMUTE_SET; //disable

	vWriteByteHdmiGRL(GRL_CFG4, bData);
	udelay(30);

	bData &= ~CFG4_AV_UNMUTE_EN;//disable original mute
	bData |= CFG4_AV_UNMUTE_SET; //disable

	vWriteByteHdmiGRL(GRL_CFG4, bData);

}


void vSetHDMIAudioIn(void)
{
	unsigned char bData2;


	bData2 = HDMI_PCM_24BIT; //vCheckPcmBitSize(0);

	vSetChannelSwap(LFE_CC_SWAP);
	vEnableIecTxRaw();

	vSetHdmiI2SDataFmt(HDMI_I2S_16BIT);

	if (bData2 == HDMI_PCM_24BIT)
		vAOUT_BNUM_SEL(AOUT_24BIT);
	else
		vAOUT_BNUM_SEL(AOUT_16BIT);

	vSetHdmiHighBitrate(0);
	vDSTNormalDouble(0);
	vEnableDSTConfig(0);

	vDisableDsdConfig();
	vSetHdmiI2SChNum(_stAvdAVInfo.ui1_aud_out_ch_number, 0);
	vSetHdmiIecI2s(_stAvdAVInfo.e_hdmi_aud_in);

}

void vHDMIAudioSRC(unsigned char ui1hdmifs, unsigned char ui1resindex,unsigned char bdeepmode)
{
	vHwNCTSOnOff(0);

	switch (ui1hdmifs) {
		case HDMI_FS_44K:
			vSetHDMISRCOff();
			vSetHDMIFS(CFG5_FS128, 0);
			break;

		case HDMI_FS_48K:
			vSetHDMISRCOff();
			vSetHDMIFS(CFG5_FS128, 0);
			break;

		default:
			vSetHDMISRCOff();
			vSetHDMIFS(CFG5_FS128, 0);
			break;
	}

	vHDMI_NCTS(ui1hdmifs, ui1resindex,  bdeepmode);
	vReEnableSRC();

}

void TVD_config_pll(unsigned int resolutionmode)
{
	*(volatile unsigned int*)(0x10209040) = (0x0);
	vWriteHdmiANAMsk(MHL_TVDPLL_PWR,0,RG_TVDPLL_PWR_ON);
	udelay(5);

	vWriteHdmiANAMsk(0x0,0x171,0x171);
	vWriteHdmiANAMsk(MHL_TVDPLL_PWR,RG_TVDPLL_PWR_ON,RG_TVDPLL_PWR_ON);
	udelay(1);
	vWriteHdmiANAMsk(0x0,0x173,0x173);
	udelay(5);
	//vWriteHdmiANAMsk(MHL_TVDPLL_CON1,TVDPLL_SDM_PCW_CHG,TVDPLL_SDM_PCW_CHG);
	switch (resolutionmode) {
		case HDMI_VIDEO_720x480p_60Hz:
		case HDMI_VIDEO_720x576p_50Hz:

			vWriteHdmiANAMsk(MHL_TVDPLL_CON1,(0xc7627 << RG_TVDPLL_SDM_PCW),RG_TVDPLL_SDM_PCW_MASK);
			vWriteHdmiANAMsk(MHL_TVDPLL_CON0,0,RG_TVDPLL_EN);
			vWriteHdmiANAMsk(MHL_TVDPLL_CON0,(0x1 << RG_TVDPLL_POSDIV),RG_TVDPLL_POSDIV_MASK);
			*(volatile unsigned int*)(0x10209040) = (0xff030000);
			vWriteHdmiANAMsk(MHL_TVDPLL_CON0,RG_TVDPLL_EN,RG_TVDPLL_EN);

			break;

		case HDMI_VIDEO_1920x1080p_30Hz:
		case HDMI_VIDEO_1280x720p_50Hz:
		case HDMI_VIDEO_1920x1080i_50Hz:
		case HDMI_VIDEO_1920x1080p_25Hz:
		case HDMI_VIDEO_1920x1080p_24Hz:
		case HDMI_VIDEO_1920x1080p_50Hz:
		case HDMI_VIDEO_1280x720p3d_50Hz:
		case HDMI_VIDEO_1920x1080i3d_50Hz:
		case HDMI_VIDEO_1920x1080p3d_24Hz:
		case HDMI_VIDEO_2160P_30HZ:
			vWriteHdmiANAMsk(MHL_TVDPLL_CON1,(0x112276 << RG_TVDPLL_SDM_PCW),RG_TVDPLL_SDM_PCW_MASK);
			vWriteHdmiANAMsk(MHL_TVDPLL_CON0,0,RG_TVDPLL_EN);
			vWriteHdmiANAMsk(MHL_TVDPLL_CON0,(0x0 << RG_TVDPLL_POSDIV),RG_TVDPLL_POSDIV_MASK);
			*(volatile unsigned int*)(0x10209040) = (0xff030000);
			vWriteHdmiANAMsk(MHL_TVDPLL_CON0,RG_TVDPLL_EN,RG_TVDPLL_EN);

			break;

		case HDMI_VIDEO_1280x720p_60Hz:
		case HDMI_VIDEO_1920x1080i_60Hz:
		case HDMI_VIDEO_1920x1080p_23Hz:
		case HDMI_VIDEO_1920x1080p_29Hz:
		case HDMI_VIDEO_1920x1080p_60Hz:
		case HDMI_VIDEO_1280x720p3d_60Hz:
		case HDMI_VIDEO_1920x1080i3d_60Hz:
		case HDMI_VIDEO_1920x1080p3d_23Hz:

			vWriteHdmiANAMsk(MHL_TVDPLL_CON1,(0x111e08 << RG_TVDPLL_SDM_PCW),RG_TVDPLL_SDM_PCW_MASK);
			vWriteHdmiANAMsk(MHL_TVDPLL_CON0,0,RG_TVDPLL_EN);
			vWriteHdmiANAMsk(MHL_TVDPLL_CON0,(0x0 << RG_TVDPLL_POSDIV),RG_TVDPLL_POSDIV_MASK);
			*(volatile unsigned int*)(0x10209040) = (0xff030000);
			vWriteHdmiANAMsk(MHL_TVDPLL_CON0,RG_TVDPLL_EN,RG_TVDPLL_EN);

			break;
		default: {
			break;
		}
	}

	udelay(20);
	vWriteHdmiANAMsk(MHL_TVDPLL_CON1,0,TVDPLL_SDM_PCW_CHG);
	udelay(20);
	vWriteHdmiANAMsk(MHL_TVDPLL_CON1,TVDPLL_SDM_PCW_CHG,TVDPLL_SDM_PCW_CHG);
	udelay(20);
}


void dpi_setting_res(unsigned char arg)
{
	switch (arg) {
		case HDMI_VIDEO_720x480p_60Hz: {

			*(volatile unsigned int *)0x1401d000 = 0x00000001;
			*(volatile unsigned int *)0x1401d004 = 0x00000000;
			*(volatile unsigned int *)0x1401d008 = 0x00000000;
			*(volatile unsigned int *)0x1401d00c = 0x00000007;
			*(volatile unsigned int *)0x1401d010 = 0x00410010;

			*(volatile unsigned int *)0x1401d0e0 = 0x02000000;
			*(volatile unsigned int *)0x1401d014 = 0x00000000;
			*(volatile unsigned int *)0x1401d018 = 0x01e002d0;
			*(volatile unsigned int *)0x1401d020 = 0x0000003e;
			*(volatile unsigned int *)0x1401d024 = 0x0010003c;
			*(volatile unsigned int *)0x1401d028 = 0x00000006;
			*(volatile unsigned int *)0x1401d02c = 0x0009001e;
			*(volatile unsigned int *)0x1401d068 = 0x00000000;
			*(volatile unsigned int *)0x1401d06c = 0x00000000;
			*(volatile unsigned int *)0x1401d070 = 0x00000000;
			*(volatile unsigned int *)0x1401d074 = 0x00000000;
			*(volatile unsigned int *)0x1401d078 = 0x00000000;
			*(volatile unsigned int *)0x1401d07c = 0x00000000;

			*(volatile unsigned int *)0x1401df00 = 0x00000040;


			*(volatile unsigned int *)0x1401d0a0 = 0x00000000;


			break;
		}
		case HDMI_VIDEO_720x576p_50Hz: {

			*(volatile unsigned int *)0x1401d000 = 0x00000001;
			*(volatile unsigned int *)0x1401d004 = 0x00000000;
			*(volatile unsigned int *)0x1401d008 = 0x00000000;
			*(volatile unsigned int *)0x1401d00c = 0x00000007;
			*(volatile unsigned int *)0x1401d010 = 0x00410010;

			*(volatile unsigned int *)0x1401d0e0 = 0x02000000;
			*(volatile unsigned int *)0x1401d014 = 0x00000000;
			*(volatile unsigned int *)0x1401d018 = 0x024002d0;
			*(volatile unsigned int *)0x1401d020 = 0x00000040;
			*(volatile unsigned int *)0x1401d024 = 0x000c0044;
			*(volatile unsigned int *)0x1401d028 = 0x00000005;
			*(volatile unsigned int *)0x1401d02c = 0x00050027;
			*(volatile unsigned int *)0x1401d030 = 0x00010001;
			*(volatile unsigned int *)0x1401d068 = 0x00000000;
			*(volatile unsigned int *)0x1401d06c = 0x00000000;
			*(volatile unsigned int *)0x1401d070 = 0x00000000;
			*(volatile unsigned int *)0x1401d074 = 0x00000000;
			*(volatile unsigned int *)0x1401d078 = 0x00000000;
			*(volatile unsigned int *)0x1401d07c = 0x00000000;

			*(volatile unsigned int *)0x1401df00 = 0x00000040;


			*(volatile unsigned int *)0x1401d0a0 = 0x00000000;

			break;
		}
		case HDMI_VIDEO_1280x720p_60Hz: {

			*(volatile unsigned int *)0x1401d000 = 0x00000001;
			*(volatile unsigned int *)0x1401d004 = 0x00000000;
			*(volatile unsigned int *)0x1401d008 = 0x00000001;
			*(volatile unsigned int *)0x1401d00c = 0x00000000;
			*(volatile unsigned int *)0x1401d010 = 0x00410000;

			*(volatile unsigned int *)0x1401d0e0 = 0x82000200;
			*(volatile unsigned int *)0x1401d014 = 0x00006000;
			*(volatile unsigned int *)0x1401d018 = 0x02d00500;
			*(volatile unsigned int *)0x1401d020 = 0x00000028;
			*(volatile unsigned int *)0x1401d024 = 0x006e00dc;
			*(volatile unsigned int *)0x1401d028 = 0x00000005;
			*(volatile unsigned int *)0x1401d02c = 0x00050014;
			*(volatile unsigned int *)0x1401d068 = 0x00000000;
			*(volatile unsigned int *)0x1401d06c = 0x00000000;
			*(volatile unsigned int *)0x1401d070 = 0x00000000;
			*(volatile unsigned int *)0x1401d074 = 0x00000000;
			*(volatile unsigned int *)0x1401d078 = 0x00000000;
			*(volatile unsigned int *)0x1401d07c = 0x00000000;
			*(volatile unsigned int *)0x1401df00 = 0x00000040;


			*(volatile unsigned int *)0x1401d0a0 = 0x00000000;


			break;
		}
		case HDMI_VIDEO_1280x720p_50Hz: {

			*(volatile unsigned int *)0x1401d000 = 0x00000001;
			*(volatile unsigned int *)0x1401d004 = 0x00000000;
			*(volatile unsigned int *)0x1401d008 = 0x00000001;
			*(volatile unsigned int *)0x1401d00c = 0x00000000;
			*(volatile unsigned int *)0x1401d010 = 0x00410000;

			*(volatile unsigned int *)0x1401d0e0 = 0x82000200;
			*(volatile unsigned int *)0x1401d014 = 0x00006000;
			*(volatile unsigned int *)0x1401d018 = 0x02d00500;
			*(volatile unsigned int *)0x1401d020 = 0x00000028;
			*(volatile unsigned int *)0x1401d024 = 0x01b800dc;
			*(volatile unsigned int *)0x1401d028 = 0x00000005;
			*(volatile unsigned int *)0x1401d02c = 0x00050014;
			*(volatile unsigned int *)0x1401d068 = 0x00000000;
			*(volatile unsigned int *)0x1401d06c = 0x00000000;
			*(volatile unsigned int *)0x1401d070 = 0x00000000;
			*(volatile unsigned int *)0x1401d074 = 0x00000000;
			*(volatile unsigned int *)0x1401d078 = 0x00000000;
			*(volatile unsigned int *)0x1401d07c = 0x00000000;
			*(volatile unsigned int *)0x1401df00 = 0x00000040;


			*(volatile unsigned int *)0x1401d0a0 = 0x00000000;


			break;
		}
		case HDMI_VIDEO_1920x1080p_24Hz: {
			*(volatile unsigned int *)0x1401d000 = 0x00000001;
			*(volatile unsigned int *)0x1401d004 = 0x00000000;
			*(volatile unsigned int *)0x1401d008 = 0x00000001;
			*(volatile unsigned int *)0x1401d00c = 0x00000000;
			*(volatile unsigned int *)0x1401d010 = 0x00410000;

			*(volatile unsigned int *)0x1401d0e0 = 0x82000200;
			*(volatile unsigned int *)0x1401d014 = 0x00006000;
			*(volatile unsigned int *)0x1401d018 = 0x04380780;
			*(volatile unsigned int *)0x1401d020 = 0x0000002c;
			*(volatile unsigned int *)0x1401d024 = 0x027e0094;
			*(volatile unsigned int *)0x1401d028 = 0x00000005;
			*(volatile unsigned int *)0x1401d02c = 0x00040024;
			*(volatile unsigned int *)0x1401d068 = 0x00000000;
			*(volatile unsigned int *)0x1401d06c = 0x00000000;
			*(volatile unsigned int *)0x1401d070 = 0x00000000;
			*(volatile unsigned int *)0x1401d074 = 0x00000000;
			*(volatile unsigned int *)0x1401d078 = 0x00000000;
			*(volatile unsigned int *)0x1401d07c = 0x00000000;
			*(volatile unsigned int *)0x1401df00 = 0x00000040;


			*(volatile unsigned int *)0x1401d0a0 = 0x00000000;

			break;
		}


		case HDMI_VIDEO_1920x1080p_60Hz: {

			*(volatile unsigned int *)0x1401d000 = 0x00000001;
			*(volatile unsigned int *)0x1401d004 = 0x00000000;
			*(volatile unsigned int *)0x1401d008 = 0x00000001;
			*(volatile unsigned int *)0x1401d00c = 0x00000000;
			*(volatile unsigned int *)0x1401d010 = 0x00410000;

			*(volatile unsigned int *)0x1401d0e0 = 0x82000200;
			*(volatile unsigned int *)0x1401d014 = 0x00006000;
			*(volatile unsigned int *)0x1401d018 = 0x04380780;
			*(volatile unsigned int *)0x1401d020 = 0x0000002c;
			*(volatile unsigned int *)0x1401d024 = 0x00580094;
			*(volatile unsigned int *)0x1401d028 = 0x00000005;
			*(volatile unsigned int *)0x1401d02c = 0x00040024;
			*(volatile unsigned int *)0x1401d068 = 0x00000000;
			*(volatile unsigned int *)0x1401d06c = 0x00000000;
			*(volatile unsigned int *)0x1401d070 = 0x00000000;
			*(volatile unsigned int *)0x1401d074 = 0x00000000;
			*(volatile unsigned int *)0x1401d078 = 0x00000000;
			*(volatile unsigned int *)0x1401d07c = 0x00000000;


			*(volatile unsigned int *)0x1401d0a0 = 0x00000001;

			*(volatile unsigned int *)0x1401df00 = 0x00000040;



			break;
		}
		case HDMI_VIDEO_1920x1080p_50Hz: {

			*(volatile unsigned int *)0x1401d000 = 0x00000001;
			*(volatile unsigned int *)0x1401d004 = 0x00000000;
			*(volatile unsigned int *)0x1401d008 = 0x00000001;
			*(volatile unsigned int *)0x1401d00c = 0x00000000;
			*(volatile unsigned int *)0x1401d010 = 0x00410000;

			*(volatile unsigned int *)0x1401d0e0 = 0x82000200;
			*(volatile unsigned int *)0x1401d014 = 0x00006000;
			*(volatile unsigned int *)0x1401d018 = 0x04380780;
			*(volatile unsigned int *)0x1401d020 = 0x0000002c;
			*(volatile unsigned int *)0x1401d024 = 0x02100094;
			*(volatile unsigned int *)0x1401d028 = 0x00000005;
			*(volatile unsigned int *)0x1401d02c = 0x00040024;
			*(volatile unsigned int *)0x1401d068 = 0x00000000;
			*(volatile unsigned int *)0x1401d06c = 0x00000000;
			*(volatile unsigned int *)0x1401d070 = 0x00000000;
			*(volatile unsigned int *)0x1401d074 = 0x00000000;
			*(volatile unsigned int *)0x1401d078 = 0x00000000;
			*(volatile unsigned int *)0x1401d07c = 0x00000000;



			*(volatile unsigned int *)0x1401d0a0 = 0x00000001;

			*(volatile unsigned int *)0x1401df00 = 0x00000040;

			break;
		}
		case HDMI_VIDEO_1920x1080i_60Hz: {

			*(volatile unsigned int *)0x1401d000 = 0x00000001;
			*(volatile unsigned int *)0x1401d004 = 0x00000000;
			*(volatile unsigned int *)0x1401d008 = 0x00000001;
			*(volatile unsigned int *)0x1401d00c = 0x00000000;
			*(volatile unsigned int *)0x1401d010 = 0x00430004;

			*(volatile unsigned int *)0x1401d0e0 = 0x82000200;
			*(volatile unsigned int *)0x1401d014 = 0x00006000;
			*(volatile unsigned int *)0x1401d018 = 0x021c0780;
			*(volatile unsigned int *)0x1401d020 = 0x0000002c;
			*(volatile unsigned int *)0x1401d024 = 0x00580094;
			*(volatile unsigned int *)0x1401d028 = 0x00000005;
			*(volatile unsigned int *)0x1401d02c = 0x0002000f;
			*(volatile unsigned int *)0x1401d068 = 0x00010005;
			*(volatile unsigned int *)0x1401d06c = 0x00020010;
			*(volatile unsigned int *)0x1401d070 = 0x00000000;
			*(volatile unsigned int *)0x1401d074 = 0x00000000;
			*(volatile unsigned int *)0x1401d078 = 0x00000000;
			*(volatile unsigned int *)0x1401d07c = 0x00000000;
			*(volatile unsigned int *)0x1401df00 = 0x00000040;


			*(volatile unsigned int *)0x1401d0a0 = 0x00000000;


			break;
		}
		case HDMI_VIDEO_1920x1080i_50Hz: {

			*(volatile unsigned int *)0x1401d000 = 0x00000001;
			*(volatile unsigned int *)0x1401d004 = 0x00000000;
			*(volatile unsigned int *)0x1401d008 = 0x00000001;
			*(volatile unsigned int *)0x1401d00c = 0x00000000;
			*(volatile unsigned int *)0x1401d010 = 0x00430004;

			*(volatile unsigned int *)0x1401d0e0 = 0x82000200;
			*(volatile unsigned int *)0x1401d014 = 0x00006000;
			*(volatile unsigned int *)0x1401d018 = 0x021c0780;
			*(volatile unsigned int *)0x1401d020 = 0x0000002c;
			*(volatile unsigned int *)0x1401d024 = 0x02100094;
			*(volatile unsigned int *)0x1401d028 = 0x00000005;
			*(volatile unsigned int *)0x1401d02c = 0x0002000f;
			*(volatile unsigned int *)0x1401d068 = 0x00010005;
			*(volatile unsigned int *)0x1401d06c = 0x00020010;
			*(volatile unsigned int *)0x1401d070 = 0x00000000;
			*(volatile unsigned int *)0x1401d074 = 0x00000000;
			*(volatile unsigned int *)0x1401d078 = 0x00000000;
			*(volatile unsigned int *)0x1401d07c = 0x00000000;
			*(volatile unsigned int *)0x1401df00 = 0x00000040;


			*(volatile unsigned int *)0x1401d0a0 = 0x00000000;



			break;
		}

		case HDMI_VIDEO_1280x720p3d_60Hz: {

			*(volatile unsigned int *)0x1401d000 = 0x00000001;
			*(volatile unsigned int *)0x1401d004 = 0x00000000;
			*(volatile unsigned int *)0x1401d008 = 0x00000001;
			*(volatile unsigned int *)0x1401d00c = 0x00000000;
			*(volatile unsigned int *)0x1401d010 = 0x004100a8;

			*(volatile unsigned int *)0x1401d0e0 = 0x02000000;
			*(volatile unsigned int *)0x1401d014 = 0x00006002;
			*(volatile unsigned int *)0x1401d018 = 0x02d00500;
			*(volatile unsigned int *)0x1401d020 = 0x00000028;
			*(volatile unsigned int *)0x1401d024 = 0x006e00dc;
			*(volatile unsigned int *)0x1401d028 = 0x00000005;
			*(volatile unsigned int *)0x1401d02c = 0x00050014;
			*(volatile unsigned int *)0x1401d068 = 0x00000000;
			*(volatile unsigned int *)0x1401d06c = 0x00000000;
			*(volatile unsigned int *)0x1401d070 = 0x00000005;
			*(volatile unsigned int *)0x1401d074 = 0x00050014;
			*(volatile unsigned int *)0x1401d078 = 0x00000000;
			*(volatile unsigned int *)0x1401d07c = 0x00000000;
			*(volatile unsigned int *)0x1401df00 = 0x00000040;



			*(volatile unsigned int *)0x1401d0a0 = 0x00000001;
			break;
		}

		case HDMI_VIDEO_1920x1080p3d_24Hz: {
			*(volatile unsigned int *)0x1401d000 = 0x00000001;
			*(volatile unsigned int *)0x1401d004 = 0x00000000;
			*(volatile unsigned int *)0x1401d008 = 0x00000000;
			*(volatile unsigned int *)0x1401d00c = 0x00000007;
			*(volatile unsigned int *)0x1401d010 = 0x00410078;

			*(volatile unsigned int *)0x1401d0e0 = 0x02000000;
			*(volatile unsigned int *)0x1401d014 = 0x00006000;
			*(volatile unsigned int *)0x1401d018 = 0x04380780;
			*(volatile unsigned int *)0x1401d020 = 0x0000002c;
			*(volatile unsigned int *)0x1401d024 = 0x027e0094;
			*(volatile unsigned int *)0x1401d028 = 0x00000005;
			*(volatile unsigned int *)0x1401d02c = 0x00040024;
			*(volatile unsigned int *)0x1401d068 = 0x00000000;
			*(volatile unsigned int *)0x1401d06c = 0x00000000;
			*(volatile unsigned int *)0x1401d070 = 0x00000005;
			*(volatile unsigned int *)0x1401d074 = 0x00040024;
			*(volatile unsigned int *)0x1401d078 = 0x00000000;
			*(volatile unsigned int *)0x1401d07c = 0x00000000;
			*(volatile unsigned int *)0x1401df00 = 0x00000041;



			*(volatile unsigned int *)0x1401d0a0 = 0x00000000;

			break;
		}

		case HDMI_VIDEO_2160P_30HZ: {
			*(volatile unsigned int *)(0x1401d000 + 0x0) = 0x00000001;
			*(volatile unsigned int *)(0x1401d000 + 0x4) = 0x00000000;
			*(volatile unsigned int *)(0x1401d000 + 0x8) = 0x00000001;
			*(volatile unsigned int *)(0x1401d000 + 0xc) = 0x00000000;
			*(volatile unsigned int *)(0x1401d000 + 0x10) = 0x00410000;

			*(volatile unsigned int *)(0x1401d000 + 0xe0) = 0x82000200;
			*(volatile unsigned int *)(0x1401d000 + 0x14) = 0x00006000;
			*(volatile unsigned int *)(0x1401d000 + 0x18) = 0x08700f00;
			*(volatile unsigned int *)(0x1401d000 + 0x20) = 0x00000058;
			*(volatile unsigned int *)(0x1401d000 + 0x24) = 0x00b00128;
			*(volatile unsigned int *)(0x1401d000 + 0x28) = 0x0000000a;
			*(volatile unsigned int *)(0x1401d000 + 0x2c) = 0x00080048;
			*(volatile unsigned int *)(0x1401d000 + 0x68) = 0x00000000;
			*(volatile unsigned int *)(0x1401d000 + 0x6c) = 0x00000000;
			*(volatile unsigned int *)(0x1401d000 + 0x70) = 0x00000000;
			*(volatile unsigned int *)(0x1401d000 + 0x74) = 0x00000000;
			*(volatile unsigned int *)(0x1401d000 + 0x78) = 0x00000000;
			*(volatile unsigned int *)(0x1401d000 + 0x7c) = 0x00000000;



			*(volatile unsigned int *)(0x1401d000 + 0xa0) = 0x00000001;

			//*(volatile unsigned int *)(0x1401d000 + 0xf00) = 0x00000041;


			break;
		}

		default:
			break;
	}

	udelay(10);
	*((volatile unsigned int *)(0x1401d004)) = (1);
	udelay(40);
	*((volatile unsigned int *)(0x1401d004)) = (0);

}


void UnMuteHDMIAudio(void)
{
	unsigned char bData;

	bData = bReadByteHdmiGRL(GRL_AUDIO_CFG);
	bData &=~ AUDIO_ZERO;
	vWriteByteHdmiGRL(GRL_AUDIO_CFG, bData);
}

void vHalSendVendorSpecificInfoFrame(unsigned char fg3DRes, unsigned char bVIC,
                                     unsigned char b3dstruct)
{
	unsigned char bVS_CHSUM;
	unsigned char bPB1,bPB2,bPB3,bPB4,bPB5;
	//bPB5
	vWriteHdmiGRLMsk(GRL_ACP_ISRC_CTRL, 0, VS_EN );
	vWriteByteHdmiGRL(GRL_INFOFRM_VER, VS_VERS);//HB1
	vWriteByteHdmiGRL(GRL_INFOFRM_TYPE, VS_TYPE);//HB0
	vWriteByteHdmiGRL(GRL_INFOFRM_LNG, VS_LEN);//HB2
	bPB1 = 0x03;
	bPB2 = 0x0C;
	bPB3 = 0x00;

	if (fg3DRes == TRUE)
		bPB4 = 0x40;
	else
		bPB4 = 0x20;    /* for 4k2k */

	bPB5 = 0x00;
	if (b3dstruct != 0xff)
		bPB5 |= b3dstruct << 4;
	else            /* for4k2k */
		bPB5 = bVIC;


	bVS_CHSUM=VS_VERS+VS_TYPE+VS_LEN+bPB1+bPB2+bPB3+bPB4;

	bVS_CHSUM=0x100-bVS_CHSUM;

	vWriteByteHdmiGRL(GRL_IFM_PORT, bVS_CHSUM);//check sum
	vWriteByteHdmiGRL(GRL_IFM_PORT, bPB1);//24bit IEEE Registration Identifier
	vWriteByteHdmiGRL(GRL_IFM_PORT, bPB2);//24bit IEEE Registration Identifier
	vWriteByteHdmiGRL(GRL_IFM_PORT, bPB3);//24bit IEEE Registration Identifier
	vWriteByteHdmiGRL(GRL_IFM_PORT, bPB4);//HDMI_Video_Format

	vWriteHdmiGRLMsk(GRL_ACP_ISRC_CTRL, VS_EN , VS_EN );
}

void vSendVendorSpecificInfoFrame(unsigned char ui1resindex)
{
	unsigned char bResTableIndex, b3DStruct, bVic;
	unsigned char fg3DRes;

	fg3DRes = TRUE;

	if (ui1resindex == HDMI_VIDEO_1920x1080p3d_23Hz)
		ui1resindex = HDMI_VIDEO_1920x1080p_23Hz;
	else if (ui1resindex == HDMI_VIDEO_1920x1080p3d_24Hz)
		ui1resindex = HDMI_VIDEO_1920x1080p_24Hz;
	else if (ui1resindex == HDMI_VIDEO_1920x1080i3d_60Hz)
		ui1resindex = HDMI_VIDEO_1920x1080i_60Hz;
	else if (ui1resindex == HDMI_VIDEO_1920x1080i3d_50Hz)
		ui1resindex = HDMI_VIDEO_1920x1080i_50Hz;
	else if (ui1resindex == HDMI_VIDEO_1280x720p3d_60Hz)
		ui1resindex = HDMI_VIDEO_1280x720p_60Hz;
	else if (ui1resindex == HDMI_VIDEO_1280x720p3d_50Hz)
		ui1resindex = HDMI_VIDEO_1280x720p_50Hz;
	else
		fg3DRes = FALSE;

	b3DStruct = 0;
	bVic = 0;

	bResTableIndex = HDMI_VIDEO_ID_CODE[ui1resindex];   /* bData4 */

	if (fg3DRes == TRUE)
		vHalSendVendorSpecificInfoFrame(fg3DRes, bResTableIndex, b3DStruct);
	else if (bResolution_4K2K(ui1resindex)) {
		if (ui1resindex == HDMI_VIDEO_2160P_29_97HZ)
			bVic = 1;
		else if (ui1resindex == HDMI_VIDEO_2160P_30HZ)
			bVic = 1;
		else if (ui1resindex == HDMI_VIDEO_2160P_25HZ)
			bVic = 2;
		else if (ui1resindex == HDMI_VIDEO_2160P_23_976HZ)
			bVic = 3;
		else if (ui1resindex == HDMI_VIDEO_2160P_24HZ)
			bVic = 3;
		else if (ui1resindex == HDMI_VIDEO_2161P_24HZ)
			bVic = 4;
		vHalSendVendorSpecificInfoFrame(0, bVic, 0xff);
	}

}

void dpi_enable_colorbar(unsigned dpi_id,unsigned enable)
{
	dprintf(0,"%s,dpi_id %d,enable\n",__func__,dpi_id,enable);

	if (dpi_id == 0) {
		if (enable) {
			*(volatile unsigned int *)0x1401df00 = (0x00000041);
		} else {
			*(volatile unsigned int *)0x1401df00 = (0x00000040);
		}
	}

}

unsigned int hdmi_widht = 0;
unsigned int hdmi_height = 0;
unsigned int hdmi_res = 0;

unsigned int hdmi_get_width(void)
{
	return hdmi_widht;
}

unsigned int hdmi_get_height(void)
{
	return hdmi_height;
}
void hdmi1_setting(unsigned int resolutionmode, unsigned int deepcolor, unsigned int colorspace,unsigned int audiofs, unsigned int chnumber, unsigned int audiotype)
{
	vConfigHdmiSYS(resolutionmode);
	vEnableDeepColor(deepcolor);
	vResetHDMI(1);
	vResetHDMI(0);
	vWriteHdmiGRLMsk(VIDEO_CFG_4, NORMAL_PATH, VIDEO_SOURCE_SEL);
	vEnableNotice(1);

	if (edid_vsdb_exist == TRUE)
		vEnableHdmiMode(1);
	else
		vEnableHdmiMode(0);

	vEnableNCTSAutoWrite();
	vWriteHdmiGRLMsk(GRL_CFG4,0,CFG_MHL_MODE);
	if ((resolutionmode==HDMI_VIDEO_1920x1080i_50Hz)||(resolutionmode==HDMI_VIDEO_1920x1080i_60Hz))
		vWriteHdmiGRLMsk(GRL_CFG2,0,MHL_DE_SEL);
	else
		vWriteHdmiGRLMsk(GRL_CFG2,MHL_DE_SEL,MHL_DE_SEL);

	vHwNCTSOnOff(0);
	vSetHDMIAudioIn();
	vHDMIAudioSRC(audiofs, resolutionmode,deepcolor);
	vHDMI_I2S_C_Status();
	vSendAudioInfoFrame();
	vHwNCTSOnOff(1);

	vSendAVIInfoFrame(resolutionmode,  colorspace);
	vSendVendorSpecificInfoFrame(resolutionmode);

	vSend_AVUNMUTE();
	UnMuteHDMIAudio();

}

void vAipCtrlInit(void)
{
	vWriteHdmiGRLMsk(AIP_CTRL, AUD_SEL_OWRT|NO_MCLK_CTSGEN_SEL|CTS_REQ_EN, AUD_SEL_OWRT|NO_MCLK_CTSGEN_SEL|MCLK_EN|CTS_REQ_EN);
	vWriteHdmiGRLMsk(AIP_TPI_CTRL, TPI_AUDIO_LOOKUP_DIS, TPI_AUDIO_LOOKUP_EN);
}

void vSetHdmi2I2SDataFmt(unsigned int bLength)
{
	vWriteHdmiGRLMsk(AIP_I2S_CTRL, bLength<<I2S_IN_LENGTH_SHIFT, I2S_IN_LENGTH);
}

void vSetHdmi2I2SSckEdge(unsigned int bEdge)
{
	vWriteHdmiGRLMsk(AIP_I2S_CTRL, bEdge, SCK_EDGE_RISE);
}

void vSetHdmi2I2SCbitOrder(unsigned int bCbit)
{
	vWriteHdmiGRLMsk(AIP_I2S_CTRL, bCbit, CBIT_ORDER_SAME);
}

void vSetHdmi2I2SVbit(unsigned int bVbit)
{
	vWriteHdmiGRLMsk(AIP_I2S_CTRL, bVbit, VBIT_COM);
}

void vSetHdmi2I2SWS(unsigned int bWS)
{
	vWriteHdmiGRLMsk(AIP_I2S_CTRL, bWS, WS_HIGH);
}

void vSetHdmi2I2SJustify(unsigned int bJustify)
{
	vWriteHdmiGRLMsk(AIP_I2S_CTRL, bJustify, JUSTIFY_RIGHT);
}

void vSetHdmi2I2SDataDir(unsigned int bDataDir)
{
	vWriteHdmiGRLMsk(AIP_I2S_CTRL, bDataDir, DATA_DIR_LSB);
}

void vSetHdmi2I2S1stbit(unsigned int b1stbit)
{
	vWriteHdmiGRLMsk(AIP_I2S_CTRL, b1stbit, I2S_1ST_BIT_NOSHIFT);
}

void vSetHdmi2I2SfifoMap(unsigned int bFifoMap)
{
	vWriteHdmiGRLMsk(AIP_I2S_CTRL, bFifoMap, FIFO3_MAP|FIFO2_MAP|FIFO1_MAP|FIFO0_MAP);
}

void vSetHdmi2I2SCH(unsigned int bCH)
{
	vWriteHdmiGRLMsk(AIP_CTRL, bCH << I2S_EN_SHIFT, I2S_EN);
}

void vSetHdmi2SpdifConfig(void)
{
	vWriteHdmiGRLMsk(AIP_SPDIF_CTRL, WR_1UI_UNLOCK, WR_1UI_LOCK);
	vWriteHdmiGRLMsk(AIP_SPDIF_CTRL, FS_UNOVERRIDE, FS_OVERRIDE_WRITE);
	vWriteHdmiGRLMsk(AIP_SPDIF_CTRL, WR_2UI_UNLOCK, WR_2UI_LOCK);
	vWriteHdmiGRLMsk(AIP_SPDIF_CTRL, 0x4 << MAX_1UI_WRITE_SHIFT, MAX_1UI_WRITE);
	vWriteHdmiGRLMsk(AIP_SPDIF_CTRL, 0x9 << MAX_2UI_WRITE_SHIFT, MAX_2UI_WRITE);
	vWriteHdmiGRLMsk(AIP_SPDIF_CTRL, 0x4 << AUD_ERR_THRESH_SHIFT, AUD_ERR_THRESH);
	vWriteHdmiGRLMsk(AIP_SPDIF_CTRL, I2S2DSD_EN, I2S2DSD_EN);
}

void vEnableInputAudioType(unsigned char bspdifi2s)
{
	vWriteHdmiGRLMsk(AIP_CTRL, bspdifi2s << SPDIF_EN_SHIFT, SPDIF_EN);
}

void vSetHdmi2I2SChNum(unsigned char bChNum, unsigned char bChMapping)
{
	unsigned int bData, bData1, bData2, bData3;
	if (bChNum==2) { //I2S 2ch
		bData = 0x1;//2ch data
		bData1 = 0x50;//data0


	} else if ((bChNum==3)||(bChNum==4)) { //I2S 2ch
		if ((bChNum==4)&&(bChMapping == 0x08)) {
			bData = 0x3;//4ch data

		} else {
			bData = 0x03;//4ch data
		}
		bData1 = 0x50;//data0


	} else if ((bChNum == 6)||(bChNum == 5)) { //I2S 5.1ch
		if ((bChNum==6)&&(bChMapping == 0x0E)) {
			bData = 0xf;//6.0 ch data
			bData1 = 0x50;//data0
		} else {
			bData = 0x7;//5.1ch data, 5/0ch
			bData1 = 0x50;//data0
		}


	} else if (bChNum == 8) { //I2S 5.1ch
		bData = 0xf;//7.1ch data
		bData1 = 0x50;//data0
	} else if (bChNum == 7) { //I2S 6.1ch
		bData = 0xf;//6.1ch data
		bData1 = 0x50;//data0
	} else {
		bData = 0x01;//2ch data
		bData1 = 0x50;//data0
	}

	bData2=0xc6;
	bData3=0xfa;

	vSetHdmi2I2SfifoMap((MAP_SD3 << 6) | (MAP_SD2 << 4) | (MAP_SD1 << 2) | (MAP_SD0 << 0));
	vSetHdmi2I2SCH(bData);

	if (bChNum == 2) {
		vWriteHdmiGRLMsk(AIP_TXCTRL,LAYOUT0,LAYOUT1);
	} else {
		vWriteHdmiGRLMsk(AIP_TXCTRL,LAYOUT1,LAYOUT1);
	}

}

void vSetHDMI2AudioIn(void)
{
	if (_stAvdAVInfo.e_hdmi_aud_in == SV_I2S) {
		vSetHdmi2I2SDataFmt(I2S_LENGTH_24BITS);
		vSetHdmi2I2SSckEdge(SCK_EDGE_RISE);
		vSetHdmi2I2SCbitOrder(CBIT_ORDER_SAME);
		vSetHdmi2I2SVbit(VBIT_PCM);
		vSetHdmi2I2SWS(WS_LOW);
		vSetHdmi2I2SJustify(JUSTIFY_RIGHT);
		vSetHdmi2I2SDataDir(DATA_DIR_MSB);
		vSetHdmi2I2S1stbit(I2S_1ST_BIT_NOSHIFT);
		vEnableInputAudioType(SV_I2S);
		vSetHdmi2I2SChNum(_stAvdAVInfo.ui1_aud_out_ch_number, 0);

	} else {
		vSetHdmi2SpdifConfig();
		vEnableInputAudioType(SV_SPDIF);
		vSetHdmi2I2SChNum(2, 0);
	}

	vWriteByteHdmiGRL(TOP_AUD_MAP, C_SD7 + C_SD6 + C_SD5 + C_SD4 + C_SD3 + C_SD2 + C_SD1 + C_SD0);

}

void vHwSet_Hdmi2_I2S_C_Status (unsigned char *prLChData, unsigned char *prRChData)
{
	vWriteByteHdmiGRL(AIP_I2S_CHST0, (prLChData[3] << 24) +(prLChData[2] << 16) +(prLChData[1] << 8) + prLChData[0]);
	vWriteByteHdmiGRL(AIP_I2S_CHST1, prLChData[4]);
}

void vHDMI2_I2S_C_Status(void)
{
	unsigned char bData = 0;
	unsigned char bhdmi_RCh_status[5];
	unsigned char bhdmi_LCh_status[5];


	bhdmi_LCh_status[0]= _stAvdAVInfo.bhdmiLChstatus[0];
	bhdmi_LCh_status[1]= _stAvdAVInfo.bhdmiLChstatus[1];
	bhdmi_LCh_status[2]= _stAvdAVInfo.bhdmiLChstatus[2];
	bhdmi_RCh_status[0]= _stAvdAVInfo.bhdmiRChstatus[0];
	bhdmi_RCh_status[1]= _stAvdAVInfo.bhdmiRChstatus[1];
	bhdmi_RCh_status[2]= _stAvdAVInfo.bhdmiRChstatus[2];


	bhdmi_LCh_status[0]&= ~0x02;
	bhdmi_RCh_status[0]&= ~0x02;

	bData = _stAvdAVInfo.bhdmiLChstatus[3]&0xf0;

	switch (_stAvdAVInfo.e_hdmi_fs) {
		case HDMI_FS_32K:
			bData |= 0x03 ;
			break ;
		case HDMI_FS_44K:
			break ;
		case HDMI_FS_88K:
			bData |= 0x08 ;
			break ;
		case HDMI_FS_96K:
			bData |= 0x0A ;
			break ;
		case HDMI_FS_176K:
			bData |= 0x0C;
			break ;
		case HDMI_FS_192K:
			bData |= 0x0E ;
			break ;
		case HDMI_FS_48K:
		default:
			bData |= 0x02 ;
			break ;

	}


	bhdmi_LCh_status[3] = bData;
	bhdmi_RCh_status[3] = bData;

	bData= _stAvdAVInfo.bhdmiLChstatus[4];

	bData |= ((~( bhdmi_LCh_status[3] & 0x0f))<<4);

	bhdmi_LCh_status[4]= bData;
	bhdmi_RCh_status[4]= bData;

	vHwSet_Hdmi2_I2S_C_Status (&bhdmi_LCh_status[0], &bhdmi_RCh_status[0]);


}

void vHalSendAudio2InfoFrame(unsigned char bData1,unsigned char bData2,unsigned char bData4,unsigned char bData5)
{
	unsigned char bAUDIO_CHSUM;
	unsigned int bData=0,bData3=0;

	bAUDIO_CHSUM = AUDIO_TYPE + AUDIO_VERS + AUDIO_LEN;

	bAUDIO_CHSUM += bData1;
	bAUDIO_CHSUM += bData2;
	bAUDIO_CHSUM += bData4;
	bAUDIO_CHSUM += bData5;

	bAUDIO_CHSUM = 0x100 - bAUDIO_CHSUM;

	vWriteHdmiGRLMsk(TOP_INFO_EN, AUD_DIS, AUD_EN);

	vWriteByteHdmiGRL(TOP_AIF_HEADER, (AUDIO_LEN << 16)+(AUDIO_VERS << 8)+AUDIO_TYPE);
	vWriteByteHdmiGRL(TOP_AIF_PKT00,  (bData3 << 24)+(bData2 << 16)+(bData1 << 8)+bAUDIO_CHSUM);
	vWriteByteHdmiGRL(TOP_AIF_PKT01, (bData5 << 8)+bData4);
	vWriteByteHdmiGRL(TOP_AIF_PKT02, 0x00000000);
	vWriteByteHdmiGRL(TOP_AIF_PKT03, 0x00000000);

	bData=bReadByteHdmiGRL(TOP_INFO_EN);
	bData |= AUD_EN;
	vWriteByteHdmiGRL(TOP_INFO_EN, bData);

	bData=bReadByteHdmiGRL(TOP_INFO_RPT);
	bData |= AUD_RPT_EN;
	vWriteByteHdmiGRL(TOP_INFO_RPT, bData);

	bData=bReadByteHdmiGRL(TOP_INFO_EN);
	bData |= AUD_EN_WR;
	vWriteByteHdmiGRL(TOP_INFO_EN, bData);

}

void vSendAudio2InfoFrame(void)
{

	unsigned char _bAudInfoFm[5];

	_bAudInfoFm[0] = 0;
	_bAudInfoFm[1] = 0;
	_bAudInfoFm[2] = 0;
	_bAudInfoFm[3] = 0;
	_bAudInfoFm[4] = 0;

	if (_stAvdAVInfo.e_hdmi_aud_in == SV_SPDIF) {
		_bAudInfoFm[0] = 0x00; //CC as 0,
		_bAudInfoFm[3] = 0x00;//CA 2ch
	} else { //pcm
		switch (_stAvdAVInfo.ui1_aud_out_ch_number) {
			case 2: // FL/FR
				_bAudInfoFm[0] = 0x01;
				_bAudInfoFm[3] = 0x00;
				break;
			case 6: // FL/FR/FC/RL/RR/RC
				_bAudInfoFm[0] = 0x05;
				_bAudInfoFm[3] = 0x0B;
				break;
			case 8: // FL/FR/FC/RL/RR/RC
				_bAudInfoFm[0] = 0x07;
				_bAudInfoFm[3] = 0x13;
				break;

			default:
				_bAudInfoFm[0] = 0x01;
				_bAudInfoFm[3] = 0x00;
				break;
		}

	}

	vHalSendAudio2InfoFrame(_bAudInfoFm[0],_bAudInfoFm[1],_bAudInfoFm[3],_bAudInfoFm[4]);


}

void vEnableAudio2(unsigned int bOn)
{
	vWriteHdmiGRLMsk(AIP_CTRL, bOn << AUD_IN_EN_SHIFT, AUD_IN_EN);
}

void vHwNCTSOnOff2(unsigned char bHwNctsOn)
{
	unsigned int bData;
	bData = bReadByteHdmiGRL(AIP_CTRL);

	if (bHwNctsOn == 0)
		bData |= CTS_SW_SEL;
	else
		bData &= ~CTS_SW_SEL;

	vWriteByteHdmiGRL(AIP_CTRL, bData);

}

void vHalHDMI2_NCTS(unsigned char bAudioFreq, unsigned char bPix, unsigned char bDeepMode)
{
	unsigned char bTemp, bData, bData1[NCTS_BYTES];
	unsigned int u4Temp, u4NTemp = 0;

	bData = 0;

	for (bTemp = 0; bTemp < NCTS_BYTES; bTemp++) {
		bData1[bTemp] = 0;
	}

	if (bDeepMode == HDMI_NO_DEEP_COLOR) {
		for (bTemp = 0; bTemp < NCTS_BYTES; bTemp++) {

			if ((bAudioFreq < 7) && (bPix < 9))

				bData1[bTemp] = HDMI_NCTS[bAudioFreq][bPix][bTemp];
		}

		u4NTemp = (bData1[4] << 16) | (bData1[5] << 8) | (bData1[6]);   /* N */
		u4Temp = (bData1[0] << 24) | (bData1[1] << 16) | (bData1[2] << 8) | (bData1[3]);    /* CTS */

	} else {
		for (bTemp = 0; bTemp < NCTS_BYTES; bTemp++) {
			if ((bAudioFreq < 7) && (bPix < 9))

				bData1[bTemp] = HDMI_NCTS[bAudioFreq][bPix][bTemp];
		}

		u4NTemp = (bData1[4] << 16) | (bData1[5] << 8) | (bData1[6]);   /* N */
		u4Temp = (bData1[0] << 24) | (bData1[1] << 16) | (bData1[2] << 8) | (bData1[3]);

		if (bDeepMode == HDMI_DEEP_COLOR_10_BIT) {
			u4Temp = (u4Temp >> 2) * 5; /* (*5/4) */
		} else if (bDeepMode == HDMI_DEEP_COLOR_12_BIT) {
			u4Temp = (u4Temp >> 1) * 3; /* (*3/2) */
		} else if (bDeepMode == HDMI_DEEP_COLOR_16_BIT) {
			u4Temp = (u4Temp << 1); /* (*2) */
		}

		bData1[0] = (u4Temp >> 24) & 0xff;
		bData1[1] = (u4Temp >> 16) & 0xff;
		bData1[2] = (u4Temp >> 8) & 0xff;
		bData1[3] = (u4Temp) & 0xff;

	}

	vWriteByteHdmiGRL(AIP_N_VAL, (bData1[4] << 16) + (bData1[5] << 8) + (bData1[6] << 0));
	vWriteByteHdmiGRL(AIP_CTS_SVAL,
	                  (bData1[0] << 24) + (bData1[1] << 16) + (bData1[2] << 8) +
	                  (bData1[3] << 0));

}

void vHDMI2_NCTS(unsigned char bHDMIFsFreq, unsigned char bResolution, unsigned char bdeepmode)
{
	unsigned char bPix;

	switch (bResolution) {
		case HDMI_VIDEO_720x480p_60Hz:
		case HDMI_VIDEO_720x576p_50Hz:
		default:
			bPix = 0;
			break;

		case HDMI_VIDEO_1280x720p_60Hz: /* 74.175M pixel clock */
		case HDMI_VIDEO_1920x1080i_60Hz:
		case HDMI_VIDEO_1920x1080p_23Hz:
			bPix = 2;
			break;

		case HDMI_VIDEO_1280x720p_50Hz: /* 74.25M pixel clock */
		case HDMI_VIDEO_1920x1080i_50Hz:
		case HDMI_VIDEO_1920x1080p_24Hz:
			bPix = 3;
			break;
		case HDMI_VIDEO_1920x1080p_60Hz:    /* 148.35M pixel clock */
		case HDMI_VIDEO_1280x720p3d_60Hz:
		case HDMI_VIDEO_1920x1080i3d_60Hz:
		case HDMI_VIDEO_1920x1080p3d_23Hz:
			bPix = 4;
			break;
		case HDMI_VIDEO_1920x1080p_50Hz:    /* 148.50M pixel clock */
		case HDMI_VIDEO_1280x720p3d_50Hz:
		case HDMI_VIDEO_1920x1080i3d_50Hz:
		case HDMI_VIDEO_1920x1080p3d_24Hz:
			bPix = 5;
			break;

		case HDMI_VIDEO_2160P_23_976HZ:
		case HDMI_VIDEO_2160P_29_97HZ:  /* 296.976m pixel clock */
			bPix = 7;
			break;

		case HDMI_VIDEO_2160P_24HZ: /* 297m pixel clock */
		case HDMI_VIDEO_2160P_25HZ: /* 297m pixel clock */
		case HDMI_VIDEO_2160P_30HZ: /* 297m pixel clock */
		case HDMI_VIDEO_2161P_24HZ: /* 297m pixel clock */
			bPix = 8;
			break;
	}

	vHalHDMI2_NCTS(bHDMIFsFreq, bPix, bdeepmode);

}

void vHDMI2AudioSRC(unsigned char ui1hdmifs, unsigned char ui1resindex, unsigned char bdeepmode)
{
	vHwNCTSOnOff2(FALSE);

	switch (ui1hdmifs) {
		case HDMI_FS_44K:

			break;

		case HDMI_FS_48K:

			break;

		default:

			break;
	}

	vHDMI2_NCTS(ui1hdmifs, ui1resindex, bdeepmode);
}

void vHalSendAVI2InfoFrame(unsigned char *pr_bData)
{
	unsigned char bAVI_CHSUM;
	unsigned char bData1=0, bData2=0, bData3=0, bData4=0, bData5=0;
	unsigned int bData;

	bData1= *pr_bData;
	bData2= *(pr_bData+1);
	bData3= *(pr_bData+2);
	bData4= *(pr_bData+3);
	bData5= *(pr_bData+4);

	vWriteHdmiGRLMsk(TOP_INFO_EN, AVI_DIS, AVI_EN);

	bAVI_CHSUM = AVI_TYPE + AVI_VERS + AVI_LEN;

	bAVI_CHSUM += bData1;
	bAVI_CHSUM += bData2;
	bAVI_CHSUM += bData3;
	bAVI_CHSUM += bData4;
	bAVI_CHSUM += bData5;
	bAVI_CHSUM = 0x100 - bAVI_CHSUM;


	vWriteByteHdmiGRL(TOP_AVI_HEADER, (AVI_LEN << 16) + (AVI_VERS << 8) + (AVI_TYPE << 0));
	vWriteByteHdmiGRL(TOP_AVI_PKT00, (bData3 << 24) + (bData2 << 16) + (bData1 << 8) + (bAVI_CHSUM << 0));
	vWriteByteHdmiGRL(TOP_AVI_PKT01, (bData5 << 8) + (bData4 << 0));
	vWriteByteHdmiGRL(TOP_AVI_PKT02, 0);
	vWriteByteHdmiGRL(TOP_AVI_PKT03, 0);
	vWriteByteHdmiGRL(TOP_AVI_PKT04, 0);

	bData=bReadByteHdmiGRL(TOP_INFO_EN);
	bData |= AVI_EN;
	vWriteByteHdmiGRL(TOP_INFO_EN, bData);

	bData=bReadByteHdmiGRL(TOP_INFO_RPT);
	bData |= AVI_RPT_EN;
	vWriteByteHdmiGRL(TOP_INFO_RPT, bData);

	bData=bReadByteHdmiGRL(TOP_INFO_EN);
	bData |= AVI_EN_WR;
	vWriteByteHdmiGRL(TOP_INFO_EN, bData);

}

void vSendAVI2InfoFrame(unsigned char ui1resindex, unsigned char ui1colorspace)
{
	unsigned char AviInfoFm[5];
	if (ui1colorspace == HDMI_YCBCR_444) {
		AviInfoFm[0]=0x40;
	} else if (ui1colorspace == HDMI_YCBCR_422) {
		AviInfoFm[0]=0x20;
	} else {
		AviInfoFm[0]=0x00;
	}

	if (!((ui1resindex==HDMI_VIDEO_2160P_30HZ)||(ui1resindex==HDMI_VIDEO_2161P_24HZ)))
		AviInfoFm[0] |= 0x10; //A0=1, Active format (R0~R3) inf valid

	AviInfoFm[1]=0x0;//bData2


	if ((ui1resindex==HDMI_VIDEO_720x480p_60Hz)||(ui1resindex==HDMI_VIDEO_720x576p_50Hz)) {
		AviInfoFm[1] |= AV_INFO_SD_ITU601;
	} else {
		AviInfoFm[1] |= AV_INFO_HD_ITU709;
	}

	AviInfoFm[1] |= 0x20;
	AviInfoFm[1] |= 0x08;
	AviInfoFm[2] =0; //bData3
	AviInfoFm[2] |= 0x04; //limit Range
	if (!((ui1resindex==HDMI_VIDEO_2160P_30HZ)||(ui1resindex==HDMI_VIDEO_2161P_24HZ)))
		AviInfoFm[3] = HDMI_VIDEO_ID_CODE[ui1resindex];//bData4
	else
		AviInfoFm[3] = 0;

	if ((AviInfoFm[1] & AV_INFO_16_9_OUTPUT) && ((ui1resindex==HDMI_VIDEO_720x480p_60Hz)||(ui1resindex==HDMI_VIDEO_720x576p_50Hz))) {
		AviInfoFm[3] = AviInfoFm[3]+1;
	}

	AviInfoFm[4]  = 0x00;

	vHalSendAVI2InfoFrame(&AviInfoFm[0]);

}

void vHalSendVendor2SpecificInfoFrame(unsigned char fg3DRes, unsigned char bVIC,
                                      unsigned char b3dstruct)
{
	unsigned char bVS_CHSUM;
	unsigned char bPB1, bPB2, bPB3, bPB4, bPB5;
	unsigned int bData;

	bPB1 = 0x03;
	bPB2 = 0x0C;
	bPB3 = 0x00;

	if (fg3DRes == TRUE)
		bPB4 = 0x40;
	else
		bPB4 = 0x20;    /* for 4k2k */

	bPB5 = 0x00;
	if (b3dstruct != 0xff)
		bPB5 |= b3dstruct << 4;
	else            /* for4k2k */
		bPB5 = bVIC;


	bVS_CHSUM = VS_VERS + VS_TYPE + VS_LEN + bPB1 + bPB2 + bPB3 + bPB4 + bPB5;

	bVS_CHSUM = 0x100 - bVS_CHSUM;

	vWriteHdmiGRLMsk(TOP_INFO_EN, VSIF_DIS, VSIF_EN);

	vWriteByteHdmiGRL(TOP_VSIF_HEADER, (VS_LEN << 16) + (VS_VERS << 8) + VS_TYPE);
	vWriteByteHdmiGRL(TOP_VSIF_PKT00, (bPB3 << 24) + (bPB2 << 16) + (bPB1 << 8) + bVS_CHSUM);
	vWriteByteHdmiGRL(TOP_VSIF_PKT01, (bPB5 << 8) + bPB4);
	vWriteByteHdmiGRL(TOP_VSIF_PKT02, 0x00000000);
	vWriteByteHdmiGRL(TOP_VSIF_PKT03, 0x00000000);
	vWriteByteHdmiGRL(TOP_VSIF_PKT04, 0x00000000);
	vWriteByteHdmiGRL(TOP_VSIF_PKT05, 0x00000000);
	vWriteByteHdmiGRL(TOP_VSIF_PKT06, 0x00000000);
	vWriteByteHdmiGRL(TOP_VSIF_PKT07, 0x00000000);

	bData = bReadByteHdmiGRL(TOP_INFO_EN);
	bData |= VSIF_EN;
	vWriteByteHdmiGRL(TOP_INFO_EN, bData);

	bData = bReadByteHdmiGRL(TOP_INFO_RPT);
	bData |= VSIF_RPT_EN;
	vWriteByteHdmiGRL(TOP_INFO_RPT, bData);

	bData = bReadByteHdmiGRL(TOP_INFO_EN);
	bData |= VSIF_EN_WR;
	vWriteByteHdmiGRL(TOP_INFO_EN, bData);
}


void vSendVendor2SpecificInfoFrame(unsigned char ui1resindex)
{
	unsigned char bResTableIndex, b3DStruct, bVic;
	unsigned char fg3DRes;

	fg3DRes = TRUE;

	if (ui1resindex == HDMI_VIDEO_1920x1080p3d_23Hz)
		ui1resindex = HDMI_VIDEO_1920x1080p_23Hz;
	else if (ui1resindex == HDMI_VIDEO_1920x1080p3d_24Hz)
		ui1resindex = HDMI_VIDEO_1920x1080p_24Hz;
	else if (ui1resindex == HDMI_VIDEO_1920x1080i3d_60Hz)
		ui1resindex = HDMI_VIDEO_1920x1080i_60Hz;
	else if (ui1resindex == HDMI_VIDEO_1920x1080i3d_50Hz)
		ui1resindex = HDMI_VIDEO_1920x1080i_50Hz;
	else if (ui1resindex == HDMI_VIDEO_1280x720p3d_60Hz)
		ui1resindex = HDMI_VIDEO_1280x720p_60Hz;
	else if (ui1resindex == HDMI_VIDEO_1280x720p3d_50Hz)
		ui1resindex = HDMI_VIDEO_1280x720p_50Hz;
	else
		fg3DRes = FALSE;

	b3DStruct = 0;
	bVic = 0;

	bResTableIndex = HDMI_VIDEO_ID_CODE[ui1resindex];   /* bData4 */

	if (fg3DRes == TRUE)
		vHalSendVendor2SpecificInfoFrame(fg3DRes, bResTableIndex, b3DStruct);
	else if ((ui1resindex == HDMI_VIDEO_2160P_30HZ) ||(ui1resindex == HDMI_VIDEO_2161P_24HZ)) {
		if (ui1resindex == HDMI_VIDEO_2160P_30HZ)
			bVic = 1;
		else if (ui1resindex == HDMI_VIDEO_2161P_24HZ)
			bVic = 4;
		vHalSendVendor2SpecificInfoFrame(0, bVic, 0xff);
	}

}

void vSend_AVUNMUTE2(void)
{
	unsigned int bData;
	/*GCP packet*/
	vWriteHdmiGRLMsk(TOP_CFG01, CP_CLR_MUTE_EN, CP_CLR_MUTE_EN);

	bData=bReadByteHdmiGRL(TOP_INFO_EN);
	bData |= CP_EN;
	vWriteByteHdmiGRL(TOP_INFO_EN, bData);

	bData=bReadByteHdmiGRL(TOP_INFO_RPT);
	bData |= CP_RPT_EN;
	vWriteByteHdmiGRL(TOP_INFO_RPT, bData);

	bData=bReadByteHdmiGRL(TOP_INFO_EN);
	bData |= CP_EN_WR;
	vWriteByteHdmiGRL(TOP_INFO_EN, bData);


}

void vMutevidoeAudio2(unsigned char bOnOff)
{
	if (bOnOff == 1)
		vWriteByteHdmiGRL(HDCP_TOP_CTRL, 0xffffffff);
	else
		vWriteByteHdmiGRL(HDCP_TOP_CTRL, 0x0);
}

void UnMuteHDMI2Audio(void)
{
	unsigned int bData;

	bData = bReadByteHdmiGRL(AIP_TXCTRL);
	bData &=~ AUD_MUTE_FIFO_EN;
	vWriteByteHdmiGRL(AIP_TXCTRL, bData);
}

void hdmi2_setting(unsigned int resolutionmode, unsigned int deepcolor, unsigned int colorspace,unsigned int audiofs, unsigned int chnumber, unsigned int audiotype)
{
	vWriteHdmiSYSMsk(HDMI_SYS_CFG20, HDMI2P0_EN, HDMI2P0_EN);

	vConfigHdmi2SYS(resolutionmode);

	vResetHDMI2(1);
	vResetHDMI2(0);

	vEnableDeepColor2(deepcolor);

	if ((resolutionmode==HDMI_VIDEO_720x480p_60Hz)||(resolutionmode==HDMI_VIDEO_720x576p_50Hz))
		vWriteHdmiGRLMsk(TOP_MISC_CTLR, HSYNC_POL_POS|VSYNC_POL_POS, HSYNC_POL_POS|VSYNC_POL_POS);
	else
		vWriteHdmiGRLMsk(TOP_MISC_CTLR, HSYNC_POL_NEG|VSYNC_POL_NEG, HSYNC_POL_POS|VSYNC_POL_POS);

	if (edid_vsdb_exist == TRUE)
		vEnableHdmi2Mode(1);
	else
		vEnableHdmi2Mode(0);

	vResetAudioHDMI2(1);
	vAipCtrlInit();
	vSetHDMI2AudioIn();
	vHDMI2_I2S_C_Status();
	vSendAudio2InfoFrame();
	vEnableAudio2(1);
	vResetAudioHDMI2(0);

	vHDMI2AudioSRC(audiofs, resolutionmode,deepcolor);
	vHwNCTSOnOff2(1);

	vSendAVI2InfoFrame(resolutionmode,  colorspace);
	vSendVendor2SpecificInfoFrame(resolutionmode);

	vSend_AVUNMUTE2();
	vMutevidoeAudio2(0);

	UnMuteHDMI2Audio();
}

void set_hdmi_tmds_driver(unsigned int resolutionmode, unsigned int deepcolor, unsigned int colorspace,unsigned int audiofs, unsigned int chnumber, unsigned int audiotype)
{
	unsigned char bTemp,bTemp1;
	dprintf(0,"-->>>>>>>>>-set_hdmi_tmds_driver start->>>>>>>>>>\n");
	dprintf(0,"Resolution=%d,deepcolor=%d,colorspace=%d,audiofs=%d,chnumer=%d,audiotype=%d\n",resolutionmode,deepcolor,colorspace,audiofs,chnumber,audiotype);

	hdmi_res = resolutionmode;
	switch (hdmi_res) {
		case HDMI_VIDEO_720x480p_60Hz:
			hdmi_widht = 720;
			hdmi_height = 480;
			break;

		case HDMI_VIDEO_720x576p_50Hz:
			hdmi_widht = 720;
			hdmi_height = 576;
			break;

		case HDMI_VIDEO_1280x720p_60Hz:
		case HDMI_VIDEO_1280x720p_50Hz:
			hdmi_widht = 1280;
			hdmi_height = 720;
			break;

		case HDMI_VIDEO_1920x1080i_60Hz:  // 4
		case HDMI_VIDEO_1920x1080i_50Hz:  // 5
		case HDMI_VIDEO_1920x1080p_30Hz:  // 6
		case HDMI_VIDEO_1920x1080p_25Hz:  // 7
		case HDMI_VIDEO_1920x1080p_24Hz:  // 8
		case HDMI_VIDEO_1920x1080p_23Hz:  // 9
		case HDMI_VIDEO_1920x1080p_29Hz:  // a
		case HDMI_VIDEO_1920x1080p_60Hz:  // b
		case HDMI_VIDEO_1920x1080p_50Hz:  // c
			hdmi_widht = 1920;
			hdmi_height = 1080;
			break;

		case HDMI_VIDEO_2160P_23_976HZ://13
		case HDMI_VIDEO_2160P_24HZ://14
		case HDMI_VIDEO_2160P_25HZ://15
		case HDMI_VIDEO_2160P_29_97HZ://16
		case HDMI_VIDEO_2160P_30HZ://17
			hdmi_widht = 3840;
			hdmi_height = 2160;
			break;

		case HDMI_VIDEO_2161P_24HZ:
			hdmi_widht = 4096;
			hdmi_height = 2160;
			break;

		default:
			hdmi_widht = 1920;
			hdmi_height = 1080;
			break;
	}

	TVD_config_pll(resolutionmode);
	dpi_setting_res(resolutionmode);
	vTxSignalOnOff(SV_OFF);

	_stAvdAVInfo.ui1_aud_out_ch_number = chnumber;
	_stAvdAVInfo.e_hdmi_fs = audiofs;//HDMI_FS_192K;
	_stAvdAVInfo.e_hdmi_aud_in = audiotype;//SV_I2S;
	_stAvdAVInfo.bhdmiLChstatus[0] = 0;
	_stAvdAVInfo.bhdmiLChstatus[1] = 0;
	_stAvdAVInfo.bhdmiLChstatus[2] = chnumber;
	_stAvdAVInfo.bhdmiLChstatus[3] = 0;
	_stAvdAVInfo.bhdmiLChstatus[4] = 0;
	_stAvdAVInfo.bhdmiLChstatus[5] = 0;
	_stAvdAVInfo.bhdmiRChstatus[0] = 0;
	_stAvdAVInfo.bhdmiRChstatus[1] = 0;
	_stAvdAVInfo.bhdmiRChstatus[2] = chnumber;
	_stAvdAVInfo.bhdmiRChstatus[3] = 0;
	_stAvdAVInfo.bhdmiRChstatus[4] = 0;
	_stAvdAVInfo.bhdmiRChstatus[5] = 0;

	vWriteIoPadMsk(IO_PAD_PD, 0, IO_PAD_HOT_PLUG_PD);
	vWriteIoPadMsk(IO_PAD_EN, IO_PAD_HOT_PLUG_EN, IO_PAD_HOT_PLUG_EN);

	vWriteHdmiSYSMsk(HDMI_SYS_CFG20, HDMI_PCLK_FREE_RUN, HDMI_PCLK_FREE_RUN);
	vWriteHdmiSYSMsk(HDMI_SYS_CFG20, HDMI_OUT_FIFO_EN, HDMI_OUT_FIFO_EN);
	vWriteHdmiSYSMsk(HDMI_SYS_CFG1C, ANLG_ON|HDMI_ON, ANLG_ON|HDMI_ON);

	vSetHDMITxPLL(resolutionmode, deepcolor);
	vTxSignalOnOff(SV_ON);

	if (!fgDDCDataRead(0x3a, 0x50, 1, &bTemp)) {
		bTemp1 = 0;
		dprintf(0,"cant read edid\n");
	} else if (bTemp & 0x4) {
		bTemp1 = 1;
		dprintf(0,"output by hdmi2\n");
	} else {
		bTemp1 = 0;
		dprintf(0,"output by hdmi1\n");
	}

	if (bTemp1 == 1)
		hdmi2_setting(resolutionmode, deepcolor, colorspace,audiofs, chnumber, audiotype);
	else
		hdmi1_setting(resolutionmode, deepcolor, colorspace,audiofs, chnumber, audiotype);
	dprintf(0,"<<<<<<<<<<<<<<--set_hdmi_tmds_driver end--<<<<<<<<<<<<\n");
}


