/* Copyright Statement:
 *
 * This software/firmware and related documentation ("MediaTek Software") are
 * protected under relevant copyright laws. The information contained herein is
 * confidential and proprietary to MediaTek Inc. and/or its licensors. Without
 * the prior written permission of MediaTek inc. and/or its licensors, any
 * reproduction, modification, use or disclosure of MediaTek Software, and
 * information contained herein, in whole or in part, shall be strictly
 * prohibited.
 *
 * MediaTek Inc. (C) 2010. All rights reserved.
 *
 * BY OPENING THIS FILE, RECEIVER HEREBY UNEQUIVOCALLY ACKNOWLEDGES AND AGREES
 * THAT THE SOFTWARE/FIRMWARE AND ITS DOCUMENTATIONS ("MEDIATEK SOFTWARE")
 * RECEIVED FROM MEDIATEK AND/OR ITS REPRESENTATIVES ARE PROVIDED TO RECEIVER
 * ON AN "AS-IS" BASIS ONLY. MEDIATEK EXPRESSLY DISCLAIMS ANY AND ALL
 * WARRANTIES, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE OR
 * NONINFRINGEMENT. NEITHER DOES MEDIATEK PROVIDE ANY WARRANTY WHATSOEVER WITH
 * RESPECT TO THE SOFTWARE OF ANY THIRD PARTY WHICH MAY BE USED BY,
 * INCORPORATED IN, OR SUPPLIED WITH THE MEDIATEK SOFTWARE, AND RECEIVER AGREES
 * TO LOOK ONLY TO SUCH THIRD PARTY FOR ANY WARRANTY CLAIM RELATING THERETO.
 * RECEIVER EXPRESSLY ACKNOWLEDGES THAT IT IS RECEIVER'S SOLE RESPONSIBILITY TO
 * OBTAIN FROM ANY THIRD PARTY ALL PROPER LICENSES CONTAINED IN MEDIATEK
 * SOFTWARE. MEDIATEK SHALL ALSO NOT BE RESPONSIBLE FOR ANY MEDIATEK SOFTWARE
 * RELEASES MADE TO RECEIVER'S SPECIFICATION OR TO CONFORM TO A PARTICULAR
 * STANDARD OR OPEN FORUM. RECEIVER'S SOLE AND EXCLUSIVE REMEDY AND MEDIATEK'S
 * ENTIRE AND CUMULATIVE LIABILITY WITH RESPECT TO THE MEDIATEK SOFTWARE
 * RELEASED HEREUNDER WILL BE, AT MEDIATEK'S OPTION, TO REVISE OR REPLACE THE
 * MEDIATEK SOFTWARE AT ISSUE, OR REFUND ANY SOFTWARE LICENSE FEES OR SERVICE
 * CHARGE PAID BY RECEIVER TO MEDIATEK FOR SUCH MEDIATEK SOFTWARE AT ISSUE.
 *
 * The following software/firmware and/or related documentation ("MediaTek
 * Software") have been modified by MediaTek Inc. All revisions are subject to
 * any receiver's applicable license agreements with MediaTek Inc.
 */

#include "typedefs.h"
#include "platform.h"


#include "pll.h"
#include "timer.h"
#include "spm.h"

#include "dramc_pi_api.h"
#include "dramc_common.h"
#include "dramc_register.h"

int A_Reg3e0=0, A_Reg3e4=0;
int B_Reg3e0=0, B_Reg3e4=0;

extern DRAMC_CTX_T DramCtx_LPDDR3;
extern DRAMC_CTX_T DramCtx_PCDDR3;

void mt_mempll_init(DRAMC_CTX_T *p)
{
    /*p->channel = CHANNEL_A;
    MemPllInit((DRAMC_CTX_T *) p);

    p->channel = CHANNEL_B;*/
    MemPllPreInit((DRAMC_CTX_T *) p);
    MemPllInit((DRAMC_CTX_T *) p);
    return;
}

void mt_mempll_cali(DRAMC_CTX_T *p)
{
    // called after chA and chB init done
    // MEMPLL05 registers, some are located @ chA and others are @ chB
    p->channel = CHANNEL_A;
    DramcPllPhaseCal(p);
    p->channel = CHANNEL_B;
    DramcPllPhaseCal(p);

    //Should only be called after channel A/B MEMPLL phase calibration had been done.
   //  DramCPllGroupsCal(p);   //no need for MT8173 , open would let channel B fail
    return;
}

void mt_mempll_pre(void)
{
    DRAMC_CTX_T *psDramCtx;

#ifdef DDRTYPE_LPDDR3
    psDramCtx = &DramCtx_LPDDR3;
#endif

#ifdef DDRTYPE_DDR3
    psDramCtx = &DramCtx_PCDDR3;
#endif

    print("[PLL] mempll_init\n");
    mt_mempll_init(psDramCtx);
    return;
}

void mt_mempll_post(void)
{
    DRAMC_CTX_T *psDramCtx;

#ifdef DDRTYPE_LPDDR3
    psDramCtx = &DramCtx_LPDDR3;
#endif

#ifdef DDRTYPE_DDR3
    psDramCtx = &DramCtx_PCDDR3;
#endif

    print("[PLL] mempll_cali\n");
    mt_mempll_cali(psDramCtx);
    return;
}


unsigned int mt_get_smallcpu_freq(void)
{
	int output = 0;
	unsigned int temp, clk26cali_0, clk_cfg_8, clk_misc_cfg_1, clk26cali_1;
	unsigned int top_ckmuxsel, top_ckdiv1, ir_rosc_ctl;

	clk26cali_0 = DRV_Reg32(CLK26CALI_0);
	DRV_WriteReg32(CLK26CALI_0, clk26cali_0 | 0x80);	/* enable fmeter_en */

	clk_misc_cfg_1 = DRV_Reg32(CLK_MISC_CFG_1);
	DRV_WriteReg32(CLK_MISC_CFG_1, 0xFFFFFF00);	/* select divider */

	clk_cfg_8 = DRV_Reg32(CLK_CFG_8);
	DRV_WriteReg32(CLK_CFG_8, (46 << 8));	/* select armpll_occ_mon */

	top_ckmuxsel = DRV_Reg32(TOP_CKMUXSEL);
	DRV_WriteReg32(TOP_CKMUXSEL, (top_ckmuxsel & 0xFFFFFFFC) | 0x1);

	top_ckdiv1 = DRV_Reg32(TOP_CKDIV1);
	DRV_WriteReg32(TOP_CKDIV1, (top_ckdiv1 & 0xFFFFFFE0) | 0xb);

	ir_rosc_ctl = DRV_Reg32(IR_ROSC_CTL);
	DRV_WriteReg32(IR_ROSC_CTL, ir_rosc_ctl | 0x08100000);

	temp = DRV_Reg32(CLK26CALI_0);
	DRV_WriteReg32(CLK26CALI_0, temp | 0x1);	/* start fmeter */

	/* wait frequency meter finish */
	while (DRV_Reg32(CLK26CALI_0) & 0x1) {
		print("wait for frequency meter finish, CLK26CALI = 0x%x\n", DRV_Reg32(CLK26CALI_0));
		/* mdelay(10); */
	}

	temp = DRV_Reg32(CLK26CALI_1) & 0xFFFF;

	output = ((temp * 26000) / 1024) * 4;	/* Khz */

	DRV_WriteReg32(CLK_CFG_8, clk_cfg_8);
	DRV_WriteReg32(CLK_MISC_CFG_1, clk_misc_cfg_1);
	DRV_WriteReg32(CLK26CALI_0, clk26cali_0);
	DRV_WriteReg32(TOP_CKMUXSEL, top_ckmuxsel);
	DRV_WriteReg32(TOP_CKDIV1, top_ckdiv1);
	DRV_WriteReg32(IR_ROSC_CTL, ir_rosc_ctl);

	/* print("CLK26CALI = 0x%x, cpu frequency = %d Khz\n", temp, output); */

	return output;
}

unsigned int mt_get_bigcpu_freq(void)
{
	int output = 0;
	unsigned int temp, clk26cali_0, clk_cfg_8, clk_misc_cfg_1, clk26cali_1;
	unsigned int top_ckmuxsel, top_ckdiv1, ir_rosc_ctl, ca15l_mon_sel;

	clk26cali_0 = DRV_Reg32(CLK26CALI_0);
	DRV_WriteReg32(CLK26CALI_0, clk26cali_0 | 0x80);	/* enable fmeter_en */

	clk_misc_cfg_1 = DRV_Reg32(CLK_MISC_CFG_1);
	DRV_WriteReg32(CLK_MISC_CFG_1, 0xFFFFFF00);	/* select divider */

	clk_cfg_8 = DRV_Reg32(CLK_CFG_8);
	DRV_WriteReg32(CLK_CFG_8, (46 << 8));	/* select abist_cksw */

	top_ckmuxsel = DRV_Reg32(TOP_CKMUXSEL);
	DRV_WriteReg32(TOP_CKMUXSEL, (top_ckmuxsel & 0xFFFFFFF3) | (0x1 << 2));

	top_ckdiv1 = DRV_Reg32(TOP_CKDIV1);
	DRV_WriteReg32(TOP_CKDIV1, (top_ckdiv1 & 0xFFFFFC1F) | (0xb << 5));

	ca15l_mon_sel = DRV_Reg32(CA15L_MON_SEL);
	DRV_WriteReg32(CA15L_MON_SEL, ca15l_mon_sel | 0x00000500);

	ir_rosc_ctl = DRV_Reg32(IR_ROSC_CTL);
	DRV_WriteReg32(IR_ROSC_CTL, ir_rosc_ctl | 0x10000000);

	temp = DRV_Reg32(CLK26CALI_0);
	DRV_WriteReg32(CLK26CALI_0, temp | 0x1);	/* start fmeter */

	/* wait frequency meter finish */
	while (DRV_Reg32(CLK26CALI_0) & 0x1) {
		print("wait for frequency meter finish, CLK26CALI = 0x%x\n", DRV_Reg32(CLK26CALI_0));
		/* mdelay(10); */
	}

	temp = DRV_Reg32(CLK26CALI_1) & 0xFFFF;

	output = ((temp * 26000) / 1024) * 4;	/* Khz */

	DRV_WriteReg32(CLK_CFG_8, clk_cfg_8);
	DRV_WriteReg32(CLK_MISC_CFG_1, clk_misc_cfg_1);
	DRV_WriteReg32(CLK26CALI_0, clk26cali_0);
	DRV_WriteReg32(TOP_CKMUXSEL, top_ckmuxsel);
	DRV_WriteReg32(TOP_CKDIV1, top_ckdiv1);
	DRV_WriteReg32(CA15L_MON_SEL, ca15l_mon_sel);
	DRV_WriteReg32(IR_ROSC_CTL, ir_rosc_ctl);

	/* print("CLK26CALI = 0x%x, cpu frequency = %d Khz\n", temp, output); */

	return output;
}


unsigned int mt_get_mem_freq(void)
{
	int output = 0;
	unsigned int temp, clk26cali_0, clk_cfg_8, clk_misc_cfg_1, clk26cali_1;

	clk26cali_0 = DRV_Reg32(CLK26CALI_0);
	DRV_WriteReg32(CLK26CALI_0, clk26cali_0 | 0x80);	/* enable fmeter_en */

	clk_misc_cfg_1 = DRV_Reg32(CLK_MISC_CFG_1);
	DRV_WriteReg32(CLK_MISC_CFG_1, 0xFFFFFF00);	/* select divider */

	clk_cfg_8 = DRV_Reg32(CLK_CFG_8);
	DRV_WriteReg32(CLK_CFG_8, (24 << 8));	/* select abist_cksw */

	temp = DRV_Reg32(CLK26CALI_0);
	DRV_WriteReg32(CLK26CALI_0, temp | 0x1);	/* start fmeter */

	/* wait frequency meter finish */
	while (DRV_Reg32(CLK26CALI_0) & 0x1) {
		print("wait for frequency meter finish, CLK26CALI = 0x%x\n", DRV_Reg32(CLK26CALI_0));
		/* mdelay(10); */
	}

	temp = DRV_Reg32(CLK26CALI_1) & 0xFFFF;

	output = (temp * 26000) / 1024;	/* Khz */

	DRV_WriteReg32(CLK_CFG_8, clk_cfg_8);
	DRV_WriteReg32(CLK_MISC_CFG_1, clk_misc_cfg_1);
	DRV_WriteReg32(CLK26CALI_0, clk26cali_0);

	/* print("CLK26CALI = 0x%x, mem frequency = %d Khz\n", temp, output); */

	return output;
}

unsigned int mt_get_bus_freq(void)
{
	int output = 0;
	unsigned int temp, clk26cali_0, clk_cfg_9, clk_misc_cfg_1, clk26cali_2;

	clk26cali_0 = DRV_Reg32(CLK26CALI_0);
	DRV_WriteReg32(CLK26CALI_0, clk26cali_0 | 0x80);	/* enable fmeter_en */

	clk_misc_cfg_1 = DRV_Reg32(CLK_MISC_CFG_1);
	DRV_WriteReg32(CLK_MISC_CFG_1, 0x00FFFFFF);	/* select divider */

	clk_cfg_9 = DRV_Reg32(CLK_CFG_9);
	DRV_WriteReg32(CLK_CFG_9, (1 << 16));	/* select ckgen_cksw */

	temp = DRV_Reg32(CLK26CALI_0);
	DRV_WriteReg32(CLK26CALI_0, temp | 0x10);	/* start fmeter */

	/* wait frequency meter finish */
	while (DRV_Reg32(CLK26CALI_0) & 0x10) {
		print("wait for frequency meter finish, CLK26CALI = 0x%x\n", DRV_Reg32(CLK26CALI_0));
		/* mdelay(10); */
	}

	temp = DRV_Reg32(CLK26CALI_2) & 0xFFFF;

	output = (temp * 26000) / 1024;	/* Khz */

	DRV_WriteReg32(CLK_CFG_9, clk_cfg_9);
	DRV_WriteReg32(CLK_MISC_CFG_1, clk_misc_cfg_1);
	DRV_WriteReg32(CLK26CALI_0, clk26cali_0);

	/* print("CLK26CALI = 0x%x, bus frequency = %d Khz\n", temp, output); */

	return output;
}

void set_freq_od(enum fod_mode mode, unsigned long clks)
{
	CHIP_SW_VER ver;
	int wait_pll = 0;

	ver = mt_get_chip_sw_ver();

	if (clks & FCLK_VENCPLL) {
		if (mode == FREQ_OD) {
			if (ver == CHIP_SW_VER_01)
				DRV_WriteReg32(VENCPLL_CON1, 0x800CB13B);	/* 660 MHz */
			else
				DRV_WriteReg32(VENCPLL_CON1, 0x800D0000);	/* 676 MHz */
			wait_pll = 1;
		} else if (mode == FREQ_PTPOD) {
			DRV_WriteReg32(VENCPLL_CON1, 0x800F6276);	/* 800 MHz */
			wait_pll = 1;
		}
	}

	if (clks & FCLK_VCODECPLL) {
		if (mode == FREQ_OD) {
			DRV_WriteReg32(VCODECPLL_CON1, 0x800B13B1);	/* 384 MHz (1152 / 3) */
			wait_pll = 1;
		} else if (mode == FREQ_PTPOD) {
			if (ver == CHIP_SW_VER_01)
				DRV_WriteReg32(VCODECPLL_CON1, 0x800E4000);	/* 494 MHz (1482 / 3) */
			else
				DRV_WriteReg32(VCODECPLL_CON1, 0x800EA000);	/* 507 MHz (1521 / 3) */
			wait_pll = 1;
		}
	}

	if (clks & FCLK_MMPLL) {
		if (mode == FREQ_OD) {
			DRV_WriteReg32(MMPLL_CON1, 0x82118000);	/* 455 MHz */
			wait_pll = 1;
		} else if (mode == FREQ_PTPOD) {
			DRV_WriteReg32(MMPLL_CON1, 0x821713B1);	/* 600 MHz */
			wait_pll = 1;
		}
	}

	if (clks & FCLK_AXI_CK) {
		if (mode == FREQ_OD) {
			DRV_WriteReg32(CLK_CFG_0,
				DRV_Reg32(CLK_CFG_0) & 0xFFFFFF00 | 0x00000005);
			/* axi_ck = 208 MHz (UNIVPLL2_D2) */
		} else if (mode == FREQ_PTPOD) {
			DRV_WriteReg32(CLK_CFG_0,
				DRV_Reg32(CLK_CFG_0) & 0xFFFFFF00 | 0x00000001);
			/* axi_ck = 273 MHz (SYSPLL1_D2) */
		}
	}

	if (clks & FCLK_VENCLT_CK) {
		if (mode == FREQ_OD) {
			DRV_WriteReg32(CLK_CFG_5,
				DRV_Reg32(CLK_CFG_5) & 0x00FFFFFF | 0x06000000);
			/* venclite_ck = 312 MHz (UNIVPLL1_D2) */
		} else if (mode == FREQ_PTPOD) {
			DRV_WriteReg32(CLK_CFG_5,
				DRV_Reg32(CLK_CFG_5) & 0x00FFFFFF | 0x0A000000);
			/* venclite_ck = 370.5 MHz (VCODECPLL_370P5_CK) */
		}
	}

	if (clks & FCLK_CCI400_CK) {
		if (mode == FREQ_OD) {
			if (ver == CHIP_SW_VER_01) {
				DRV_WriteReg32(CLK_CFG_6,
					DRV_Reg32(CLK_CFG_6) & 0xFF00FFFF | 0x00050100);
				/* cci400_ck = 546 MHz (SYSPLL_D2) */
			} else {
				DRV_WriteReg32(CLK_CFG_6,
					DRV_Reg32(CLK_CFG_6) & 0xFF00FFFF | 0x00040100);
				/* cci400_ck = 624 MHz (UNIVPLL_D2) */
			}
		} else if (mode == FREQ_PTPOD) {
			if (ver == CHIP_SW_VER_01) {
				DRV_WriteReg32(CLK_CFG_6,
					DRV_Reg32(CLK_CFG_6) & 0xFF00FFFF | 0x00040100);
				/* cci400_ck = 624 MHz (UNIVPLL_D2) */
			} else {
				DRV_WriteReg32(CLK_CFG_6,
					DRV_Reg32(CLK_CFG_6) & 0xFF00FFFF | 0x00040100);
				/* cci400_ck = 624 MHz (UNIVPLL_D2) */
			}
		}
	}

	if (wait_pll)
		gpt_busy_wait_us(5);

}

/* after pmic_init */
void mt_pll_post_init(void)
{
	unsigned int temp;

	mt_mempll_pre();
	mt_mempll_post();

	/* set mem_clk */
	DRV_WriteReg32(CLK_CFG_0, 0x01000105);	/* ddrphycfg_ck = 26MHz */

	temp = DRV_Reg32(AP_PLL_CON3);
	DRV_WriteReg32(AP_PLL_CON3, temp & 0xFFF00000);	/* set all PLLs HW control */

	temp = DRV_Reg32(AP_PLL_CON4);
	DRV_WriteReg32(AP_PLL_CON4, temp & 0xFFFFFFF0);	/* set all PLLs HW control */

#if 0
	print("mt_pll_post_init: mt_get_smallcpu_freq = %dKhz\n", mt_get_smallcpu_freq());
	print("mt_pll_post_init: mt_get_bigcpu_freq = %dKhz\n", mt_get_bigcpu_freq());
	print("mt_pll_post_init: mt_get_bus_freq = %dKhz\n", mt_get_bus_freq());
	print("mt_pll_post_init: mt_get_mem_freq = %dKhz\n", mt_get_mem_freq());
#endif
}

/* after pmic_init */
void mt_arm_pll_sel(void)
{
	unsigned int temp;

	temp = DRV_Reg32(TOP_CKDIV1);
	DRV_WriteReg32(TOP_CKDIV1, temp & 0xFFFFFC00);	/* CPU clock divide by 1 */

	DRV_WriteReg32(TOP_CKMUXSEL, 0x0245);	/* select ARMPLL */

	print("[PLL] mt_arm_pll_sel done\n");
}

#define	INFRA_GLOBALCON_DCMCTL		(INFRACFG_AO_BASE + 0x050)
#define INFRA_GLOBALCON_DCMDBC		(INFRACFG_AO_BASE + 0x054)
#define INFRA_GLOBALCON_DCMFSEL		(INFRACFG_AO_BASE + 0x058)

#define INFRA_GLOBALCON_DCMCTL_MASK	0x00000303
#define INFRA_GLOBALCON_DCMCTL_ON	0x00000303

#define INFRA_GLOBALCON_DCMDBC_MASK	((0x7f << 0) | (1 << 8) |	\
					 (0x7f << 16) | (1 << 24))
#define INFRA_GLOBALCON_DCMDBC_ON	((0 << 0) | (1 << 8) |		\
					 (0 << 16) | (1 << 24))

#define INFRA_GLOBALCON_DCMFSEL_MASK	((0x7 << 0) | (0xf << 8) |	\
					 (0x1f << 16) | (0x1f << 24))
#define INFRA_GLOBALCON_DCMFSEL_ON	((0 << 0) | (0 << 8) |		\
					 (0x10 << 16) | (0x10 << 24))

#define setl_mask(addr, mask, val) \
	DRV_WriteReg32(addr, (DRV_Reg32(addr) & ~(mask)) | (val))

void mt_pll_init(void)
{
	int ret = 0;
	unsigned int temp;
	CHIP_SW_VER ver;

	ver = mt_get_chip_sw_ver();

	DRV_WriteReg32(CLKSQ_STB_CON0, 0x05010501);	/* reduce CLKSQ disable time */

	DRV_WriteReg32(PLL_ISO_CON0, 0x00080008);	/* extend PWR/ISO control timing to 1us */

	DRV_WriteReg32(AP_PLL_CON6, 0x00000000);

	/*************
	 * xPLL PWR ON
	 **************/

	temp = DRV_Reg32(ARMCA15PLL_PWR_CON0);
	DRV_WriteReg32(ARMCA15PLL_PWR_CON0, temp | 0x1);

	temp = DRV_Reg32(ARMCA7PLL_PWR_CON0);
	DRV_WriteReg32(ARMCA7PLL_PWR_CON0, temp | 0x1);

	temp = DRV_Reg32(MAINPLL_PWR_CON0);
	DRV_WriteReg32(MAINPLL_PWR_CON0, temp | 0x1);

	temp = DRV_Reg32(UNIVPLL_PWR_CON0);
	DRV_WriteReg32(UNIVPLL_PWR_CON0, temp | 0x1);

	temp = DRV_Reg32(MMPLL_PWR_CON0);
	DRV_WriteReg32(MMPLL_PWR_CON0, temp | 0x1);

	temp = DRV_Reg32(MSDCPLL_PWR_CON0);
	DRV_WriteReg32(MSDCPLL_PWR_CON0, temp | 0x1);

	temp = DRV_Reg32(VENCPLL_PWR_CON0);
	DRV_WriteReg32(VENCPLL_PWR_CON0, temp | 0x1);

	temp = DRV_Reg32(TVDPLL_PWR_CON0);
	DRV_WriteReg32(TVDPLL_PWR_CON0, temp | 0x1);

	temp = DRV_Reg32(MPLL_PWR_CON0);
	DRV_WriteReg32(MPLL_PWR_CON0, temp | 0x1);

	temp = DRV_Reg32(VCODECPLL_PWR_CON0);
	DRV_WriteReg32(VCODECPLL_PWR_CON0, temp | 0x1);

	temp = DRV_Reg32(APLL1_PWR_CON0);
	DRV_WriteReg32(APLL1_PWR_CON0, temp | 0x1);

	temp = DRV_Reg32(APLL2_PWR_CON0);
	DRV_WriteReg32(APLL2_PWR_CON0, temp | 0x1);

	temp = DRV_Reg32(MSDCPLL2_PWR_CON0);
	DRV_WriteReg32(MSDCPLL2_PWR_CON0, temp | 0x1);

	temp = DRV_Reg32(LVDSPLL_PWR_CON0);
	DRV_WriteReg32(LVDSPLL_PWR_CON0, temp | 0x1);

	gpt_busy_wait_us(5);	/* wait for xPLL_PWR_ON ready (min delay is 1us) */

	/******************
	 * xPLL ISO Disable
	 *******************/

	temp = DRV_Reg32(ARMCA15PLL_PWR_CON0);
	DRV_WriteReg32(ARMCA15PLL_PWR_CON0, temp & 0xFFFFFFFD);

	temp = DRV_Reg32(ARMCA7PLL_PWR_CON0);
	DRV_WriteReg32(ARMCA7PLL_PWR_CON0, temp & 0xFFFFFFFD);

	temp = DRV_Reg32(MAINPLL_PWR_CON0);
	DRV_WriteReg32(MAINPLL_PWR_CON0, temp & 0xFFFFFFFD);

	temp = DRV_Reg32(UNIVPLL_PWR_CON0);
	DRV_WriteReg32(UNIVPLL_PWR_CON0, temp & 0xFFFFFFFD);

	temp = DRV_Reg32(MMPLL_PWR_CON0);
	DRV_WriteReg32(MMPLL_PWR_CON0, temp & 0xFFFFFFFD);

	temp = DRV_Reg32(MSDCPLL_PWR_CON0);
	DRV_WriteReg32(MSDCPLL_PWR_CON0, temp & 0xFFFFFFFD);

	temp = DRV_Reg32(VENCPLL_PWR_CON0);
	DRV_WriteReg32(VENCPLL_PWR_CON0, temp & 0xFFFFFFFD);

	temp = DRV_Reg32(TVDPLL_PWR_CON0);
	DRV_WriteReg32(TVDPLL_PWR_CON0, temp & 0xFFFFFFFD);

	temp = DRV_Reg32(MPLL_PWR_CON0);
	DRV_WriteReg32(MPLL_PWR_CON0, temp & 0xFFFFFFFD);

	temp = DRV_Reg32(VCODECPLL_PWR_CON0);
	DRV_WriteReg32(VCODECPLL_PWR_CON0, temp & 0xFFFFFFFD);

	temp = DRV_Reg32(APLL1_PWR_CON0);
	DRV_WriteReg32(APLL1_PWR_CON0, temp & 0xFFFFFFFD);

	temp = DRV_Reg32(APLL2_PWR_CON0);
	DRV_WriteReg32(APLL2_PWR_CON0, temp & 0xFFFFFFFD);

	temp = DRV_Reg32(MSDCPLL2_PWR_CON0);
	DRV_WriteReg32(MSDCPLL2_PWR_CON0, temp & 0xFFFFFFFD);

	temp = DRV_Reg32(LVDSPLL_PWR_CON0);
	DRV_WriteReg32(LVDSPLL_PWR_CON0, temp & 0xFFFFFFFD);

	/********************
	 * xPLL Frequency Set
	 *********************/

	DRV_WriteReg32(ARMCA15PLL_CON1, 0x81106000);	/* 851.5MHz */

	DRV_WriteReg32(ARMCA7PLL_CON1, 0x800AA000);	/* 1105MHz */

	DRV_WriteReg32(MAINPLL_CON1, 0x800A8000);	/* 1092MHz */

	DRV_WriteReg32(MMPLL_CON1, 0x82118000);	/* 455MHz */

	DRV_WriteReg32(MSDCPLL_CON1, 0x800F6276);	/* 800MHz */

	if (ver == CHIP_SW_VER_01)
		DRV_WriteReg32(VENCPLL_CON1, 0x800CB13B);	/* 660 MHz */
	else
		DRV_WriteReg32(VENCPLL_CON1, 0x800D0000);	/* 676 MHz */

	DRV_WriteReg32(TVDPLL_CON1, 0x80112276);	/* 445.5MHz */

	DRV_WriteReg32(MPLL_CON1, 0x801C0000);
	DRV_WriteReg32(MPLL_CON0, 0x00010110);	/* 52MHz */

	DRV_WriteReg32(VCODECPLL_CON1, 0x800B13B1);	/* 384 MHz (1152 / 3) */

	/* APLL1 and APLL2 use the default setting */
	/* MSDCPLL2 use the default setting: 800MHz (0x800F6276) */
	/* LVDSPLL use the default setting: 150MHz (0x800B89D9) */

	/***********************
	 * xPLL Frequency Enable
	 ************************/

	temp = DRV_Reg32(ARMCA15PLL_CON0);
	DRV_WriteReg32(ARMCA15PLL_CON0, temp | 0x1);

	temp = DRV_Reg32(ARMCA7PLL_CON0);
	DRV_WriteReg32(ARMCA7PLL_CON0, temp | 0x1);

	temp = DRV_Reg32(MAINPLL_CON0) & (~0x70);
	DRV_WriteReg32(MAINPLL_CON0, temp | 0x1);

	temp = DRV_Reg32(UNIVPLL_CON0);
	DRV_WriteReg32(UNIVPLL_CON0, temp | 0x1);

	temp = DRV_Reg32(MMPLL_CON0);
	DRV_WriteReg32(MMPLL_CON0, temp | 0x1);

	temp = DRV_Reg32(MSDCPLL_CON0);
	DRV_WriteReg32(MSDCPLL_CON0, temp | 0x1);

	temp = DRV_Reg32(VENCPLL_CON0);
	DRV_WriteReg32(VENCPLL_CON0, temp | 0x1);

	temp = DRV_Reg32(TVDPLL_CON0);
	DRV_WriteReg32(TVDPLL_CON0, temp | 0x1);

	temp = DRV_Reg32(MPLL_CON0);
	DRV_WriteReg32(MPLL_CON0, temp | 0x1);

	temp = DRV_Reg32(VCODECPLL_CON0);
	DRV_WriteReg32(VCODECPLL_CON0, temp | 0x1);

	temp = DRV_Reg32(APLL1_CON0);
	DRV_WriteReg32(APLL1_CON0, temp | 0x1);

	temp = DRV_Reg32(APLL2_CON0);
	DRV_WriteReg32(APLL2_CON0, temp | 0x1);

	temp = DRV_Reg32(MSDCPLL2_CON0);
	DRV_WriteReg32(MSDCPLL2_CON0, temp | 0x1);

	temp = DRV_Reg32(LVDSPLL_CON0);
	DRV_WriteReg32(LVDSPLL_CON0, temp | 0x1);

	gpt_busy_wait_us(40);	/* wait for PLL stable (min delay is 20us) */

	/***************
	 * xPLL DIV RSTB
	 ****************/

	temp = DRV_Reg32(ARMCA7PLL_CON0);
	DRV_WriteReg32(ARMCA7PLL_CON0, temp | 0x01000000);

	temp = DRV_Reg32(MAINPLL_CON0);
	DRV_WriteReg32(MAINPLL_CON0, temp | 0x01000000);

	temp = DRV_Reg32(UNIVPLL_CON0);
	DRV_WriteReg32(UNIVPLL_CON0, temp | 0x01000000);

	/**************
	 * INFRA CLKMUX
	 ***************/

	temp = DRV_Reg32(TOP_DCMCTL);
	DRV_WriteReg32(TOP_DCMCTL, temp | 0x1);	/* enable infrasys DCM */

	/**************
	 * Enable Infra DCM
	***************/

	setl_mask(INFRA_GLOBALCON_DCMDBC,
		INFRA_GLOBALCON_DCMDBC_MASK, INFRA_GLOBALCON_DCMDBC_ON);
	setl_mask(INFRA_GLOBALCON_DCMFSEL,
		INFRA_GLOBALCON_DCMFSEL_MASK, INFRA_GLOBALCON_DCMFSEL_ON);
	setl_mask(INFRA_GLOBALCON_DCMCTL,
		INFRA_GLOBALCON_DCMCTL_MASK, INFRA_GLOBALCON_DCMCTL_ON);

	DRV_WriteReg32(CLK_MODE, 0x1);
	DRV_WriteReg32(CLK_MODE, 0x0);	/* enable TOPCKGEN */

	/************
	 * TOP CLKMUX
	 *************/

	DRV_WriteReg32(CLK_CFG_0, 0x01000005);	/* ddrphycfg_ck = 26MHz, not set mem_clk */

	DRV_WriteReg32(CLK_CFG_1, 0x01010100);	/* pwm = 26Mhz */

	DRV_WriteReg32(CLK_CFG_2, 0x01010000);	/* uart = camtg = 26Mhz */

	if (ver != CHIP_SW_VER_01) {
		/* MSDC_1 use MSDCPLL_D4, MSDC_50_0 use MSDCPLL_D4, */
		DRV_WriteReg32(CLK_CFG_3, 0x02060201);
	}

	DRV_WriteReg32(CLK_CFG_4, 0x01000502);	/* audio = 26M */

	DRV_WriteReg32(CLK_CFG_5, 0x06000100);	/* bit16~23 is reserved, pmicspi use 26MHz */

	if (ver == CHIP_SW_VER_01)
		DRV_WriteReg32(CLK_CFG_6, 0x01050101);	/* cci400 use syspll_d2, 546MHz */
	else
		DRV_WriteReg32(CLK_CFG_6, 0x01040101);	/* cci400 use univpll_d2, 624MHz */

	DRV_WriteReg32(CLK_CFG_7, 0x01010201);

	DRV_WriteReg32(CLK_CFG_12, 0x01010100);	/* spinfi_bclk = 26M */
	DRV_WriteReg32(CLK_CFG_13, 0x01020202);

	if (ver == CHIP_SW_VER_01)
		DRV_WriteReg32(CLK_CFG_3, 0x02060201);

	DRV_WriteReg32(CLK_SCP_CFG_0, 0x7FF);	/* enable scpsys clock off control */
	DRV_WriteReg32(CLK_SCP_CFG_1, 0x15);	/* enable scpsys clock off control */

	spm_mtcmos_ctrl_disp(STA_POWER_ON);
}

int spm_mtcmos_ctrl_disp(int state)
{
	int err = 0;

	volatile unsigned int val;
	unsigned long flags;

	spm_write(SPM_POWERON_CONFIG_SET, (SPM_PROJECT_CODE << 16) | (1U << 0));

	if (state == STA_POWER_DOWN) {
		spm_write(SPM_DIS_PWR_CON, spm_read(SPM_DIS_PWR_CON) | SRAM_PDN);

		while ((spm_read(SPM_DIS_PWR_CON) & DIS_SRAM_ACK) != DIS_SRAM_ACK) ;

		spm_write(SPM_DIS_PWR_CON, spm_read(SPM_DIS_PWR_CON) | PWR_ISO);

		val = spm_read(SPM_DIS_PWR_CON);
		val = (val & ~PWR_RST_B) | PWR_CLK_DIS;
		spm_write(SPM_DIS_PWR_CON, val);

		spm_write(SPM_DIS_PWR_CON, spm_read(SPM_DIS_PWR_CON) & ~(PWR_ON | PWR_ON_S));

		while ((spm_read(SPM_PWR_STATUS) & DIS_PWR_STA_MASK)
			|| (spm_read(SPM_PWR_STATUS_2ND) & DIS_PWR_STA_MASK)) {
		}
	} else {	/* STA_POWER_ON */
		spm_write(SPM_DIS_PWR_CON, spm_read(SPM_DIS_PWR_CON) | PWR_ON);
		spm_write(SPM_DIS_PWR_CON, spm_read(SPM_DIS_PWR_CON) | PWR_ON_S);

		while (!(spm_read(SPM_PWR_STATUS) & DIS_PWR_STA_MASK)
			|| !(spm_read(SPM_PWR_STATUS_2ND) & DIS_PWR_STA_MASK)) {
		}

		spm_write(SPM_DIS_PWR_CON, spm_read(SPM_DIS_PWR_CON) & ~PWR_CLK_DIS);
		spm_write(SPM_DIS_PWR_CON, spm_read(SPM_DIS_PWR_CON) & ~PWR_ISO);
		spm_write(SPM_DIS_PWR_CON, spm_read(SPM_DIS_PWR_CON) | PWR_RST_B);

		spm_write(SPM_DIS_PWR_CON, spm_read(SPM_DIS_PWR_CON) & ~SRAM_PDN);

		while ((spm_read(SPM_DIS_PWR_CON) & DIS_SRAM_ACK)) ;
	}

	return err;
}
