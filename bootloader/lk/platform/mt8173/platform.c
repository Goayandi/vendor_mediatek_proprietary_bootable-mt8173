/* Copyright Statement:
*
* This software/firmware and related documentation ("MediaTek Software") are
* protected under relevant copyright laws. The information contained herein
* is confidential and proprietary to MediaTek Inc. and/or its licensors.
* Without the prior written permission of MediaTek inc. and/or its licensors,
* any reproduction, modification, use or disclosure of MediaTek Software,
* and information contained herein, in whole or in part, shall be strictly prohibited.
*/
/* MediaTek Inc. (C) 2015. All rights reserved.
*
* BY OPENING THIS FILE, RECEIVER HEREBY UNEQUIVOCALLY ACKNOWLEDGES AND AGREES
* THAT THE SOFTWARE/FIRMWARE AND ITS DOCUMENTATIONS ("MEDIATEK SOFTWARE")
* RECEIVED FROM MEDIATEK AND/OR ITS REPRESENTATIVES ARE PROVIDED TO RECEIVER ON
* AN "AS-IS" BASIS ONLY. MEDIATEK EXPRESSLY DISCLAIMS ANY AND ALL WARRANTIES,
* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF
* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE OR NONINFRINGEMENT.
* NEITHER DOES MEDIATEK PROVIDE ANY WARRANTY WHATSOEVER WITH RESPECT TO THE
* SOFTWARE OF ANY THIRD PARTY WHICH MAY BE USED BY, INCORPORATED IN, OR
* SUPPLIED WITH THE MEDIATEK SOFTWARE, AND RECEIVER AGREES TO LOOK ONLY TO SUCH
* THIRD PARTY FOR ANY WARRANTY CLAIM RELATING THERETO. RECEIVER EXPRESSLY ACKNOWLEDGES
* THAT IT IS RECEIVER'S SOLE RESPONSIBILITY TO OBTAIN FROM ANY THIRD PARTY ALL PROPER LICENSES
* CONTAINED IN MEDIATEK SOFTWARE. MEDIATEK SHALL ALSO NOT BE RESPONSIBLE FOR ANY MEDIATEK
* SOFTWARE RELEASES MADE TO RECEIVER'S SPECIFICATION OR TO CONFORM TO A PARTICULAR
* STANDARD OR OPEN FORUM. RECEIVER'S SOLE AND EXCLUSIVE REMEDY AND MEDIATEK'S ENTIRE AND
* CUMULATIVE LIABILITY WITH RESPECT TO THE MEDIATEK SOFTWARE RELEASED HEREUNDER WILL BE,
* AT MEDIATEK'S OPTION, TO REVISE OR REPLACE THE MEDIATEK SOFTWARE AT ISSUE,
* OR REFUND ANY SOFTWARE LICENSE FEES OR SERVICE CHARGE PAID BY RECEIVER TO
* MEDIATEK FOR SUCH MEDIATEK SOFTWARE AT ISSUE.
*/

#include <mt_partition.h>
#include <video.h>
#include <dev/uart.h>
#include <arch/arm/mmu.h>
#include <arch/ops.h>
#include <target/board.h>
#include <string.h>
#include <stdlib.h>
#include <platform/mt_reg_base.h>
#include <platform/mt_disp_drv.h>
#include <platform/boot_mode.h>
#include <platform/mt_logo.h>
#include <platform/env.h>
#include <platform/mt_gpio.h>
#include <platform/mt_pmic.h>
#include <platform/mt_pmic_wrap_init.h>
#include <platform/mt_i2c.h>
#include <platform/mtk_key.h>
#include <platform/mt_rtc.h>
#include <platform/mt_leds.h>
#include <platform/upmu_common.h>
#include <platform/mt_irq.h>
#include <platform/mmc_core.h>
#include <platform/mtk_wdt.h>
#ifdef MTK_BOOT_SOUND_SUPPORT
#include <platform/mt_boot_sound.h>
#endif

#if defined(MTK_SECURITY_SW_SUPPORT)
#include "oemkey.h"
#endif

#if defined(MTK_SECURITY_SW_SUPPORT)
extern u8 g_oemkey[OEM_PUBK_SZ];
#endif

#ifdef LK_DL_CHECK
/*block if check dl fail*/
#undef LK_DL_CHECK_BLOCK_LEVEL
#endif

#ifdef MTK_LK_IRRX_SUPPORT
#include <platform/mtk_ir_lk_core.h>
#endif

#ifdef MTK_ALPS_BOX_SUPPORT
#include <platform/hdmi_drv.h>
#include <platform/disp_drv_platform.h>
#endif

#ifdef MTK_ALPS_BOX_SUPPORT

#define LCM_WIDTH CFG_DISPLAY_WIDTH
#define LCM_HEIGHT CFG_DISPLAY_HEIGHT

#else

/* Take LCM_WIDTH and LCM_HEIGHT from ProjectConfig.mk */
#ifndef LCM_WIDTH
#define LCM_WIDTH CFG_DISPLAY_WIDTH
#endif

#ifndef LCM_HEIGHT
#define LCM_HEIGHT CFG_DISPLAY_HEIGHT
#endif

#endif

extern U32 pmic_init(void);
extern kal_bool is_low_battery(void);
extern void platform_early_init_timer();
extern void jump_da(u32 addr, u32 arg1, u32 arg2);
#ifdef MT_SRAM_REPAIR_SUPPORT
extern int repair_sram(void);
#endif

BOOT_ARGUMENT *g_boot_arg;
BOOT_ARGUMENT boot_addr;
int g_nr_bank;
BI_DRAM bi_dram[MAX_NR_BANK];
unsigned int g_fb_base;
unsigned int g_fb_size;
unsigned int fp_t;
#define ALIGN_TO(x, n)  \
    (((x) + ((n) - 1)) & ~((n) - 1))

#ifdef MTK_ALPS_BOX_SUPPORT

#define BOOT_LOGO_INDEX1   0
#define BOOT_LOGO_INDEX2   1
#define BOOT_LOGO_INDEX3   2
#define BOOT_LOGO_INDEX4   3
#define BOOT_LOGO_INDEX5   4
#define BOOT_LOGO_INDEX6   5

#if HDMI_MAIN_PATH_LK
int BOOT_LOGO_INDEX = BOOT_LOGO_INDEX4;
#elif HDMI_SUB_PATH_LK
int BOOT_LOGO_INDEX = BOOT_LOGO_INDEX1;
#else
int BOOT_LOGO_INDEX = BOOT_LOGO_INDEX1;
#endif


#define HDMI_RES_MASK_LK 0x80
#define HDMI_RES_VALID 0x7f

#endif

int dram_init(void)
{
	int i;
	unsigned int dram_rank_num;
	/***************************************************************************
	 * Get parameters from pre-loader. Get as early as possible
	 * The address of BOOT_ARGUMENT_LOCATION will be used by Linux later
	 * So copy the parameters from BOOT_ARGUMENT_LOCATION to LK's memory region
	 ***************************************************************************/
	g_boot_arg = &boot_addr;
	memcpy(g_boot_arg, (void *)BOOT_ARGUMENT_LOCATION, sizeof(BOOT_ARGUMENT));

#ifdef MACH_FPGA
	g_nr_bank = 2;
	bi_dram[0].start = DRAM_PHY_ADDR + RIL_SIZE;
	bi_dram[0].size = (256 * 1024 * 1024) - RIL_SIZE;
	bi_dram[1].start = bi_dram[0].start + bi_dram[0].size;
	bi_dram[1].size = (256 * 1024 * 1024);
#else
	g_nr_bank = g_boot_arg->dram_rank_num;

	if (g_nr_bank == 0 || g_nr_bank > MAX_NR_BANK) {
		/* dprintf(CRITICAL, "[LK ERROR] DRAM bank number is not correct!!!"); */
		while (1) ;
	}

	/* return the actual DRAM info */
	bi_dram[0].start = DRAM_PHY_ADDR + RIL_SIZE;
	bi_dram[0].size = g_boot_arg->dram_rank_size[0] - RIL_SIZE;
	for (i = 1; i < g_nr_bank; i++) {
		bi_dram[i].start = bi_dram[i-1].start + bi_dram[i-1].size;
		bi_dram[i].size = g_boot_arg->dram_rank_size[i];
	}
#endif

	return 0;
}

/*******************************************************
 * Routine: memory_size
 * Description: return DRAM size to LCM driver
 ******************************************************/
u64 physical_memory_size(void)
{
	int i;
	unsigned long long size = 0;

	for (i = 0; i < g_nr_bank; i++)
		size += bi_dram[i].size;
	size += RIL_SIZE;

	return size;
}

u32 memory_size(void)
{
	unsigned long long size = physical_memory_size();

	if (((unsigned long long)DRAM_PHY_ADDR + size) > 0x100000000ULL) {
		size -= ((unsigned long long)DRAM_PHY_ADDR + size - 0x100000000ULL);
	}

	return (unsigned int)size;
}

void sw_env()
{
#ifdef LK_DL_CHECK
#ifdef MTK_EMMC_SUPPORT
	int dl_status = mmc_get_dl_info();
	dprintf(INFO, "mt65xx_sw_env--dl_status: %d\n", dl_status);
	if (dl_status != 0) {
		video_printf("=> TOOL DL image Fail!\n");
		dprintf(CRITICAL, "TOOL DL image Fail\n");
#ifdef LK_DL_CHECK_BLOCK_LEVEL
		dprintf(CRITICAL, "uboot is blocking by dl info\n");
		while (1);
#endif
	}
#endif
#endif

#ifndef USER_BUILD
	switch (g_boot_mode) {
		case META_BOOT:
			video_printf(" => META MODE\n");
			break;
		case FACTORY_BOOT:
			video_printf(" => FACTORY MODE\n");
			break;
		case RECOVERY_BOOT:
			video_printf(" => RECOVERY MODE\n");
			break;
		case SW_REBOOT:
			/* video_printf(" => SW RESET\n"); */
			break;
		case NORMAL_BOOT:
			/* if(g_boot_arg->boot_reason != BR_RTC && get_env("hibboot") != NULL && atoi(get_env("hibboot")) == 1) */
			if (get_env("hibboot") != NULL && atoi(get_env("hibboot")) == 1)
				video_printf(" => HIBERNATION BOOT\n");
			else
				video_printf(" => NORMAL BOOT\n");
			break;
		case ADVMETA_BOOT:
			video_printf(" => ADVANCED META MODE\n");
			break;
		case ATE_FACTORY_BOOT:
			video_printf(" => ATE FACTORY MODE\n");
			break;
#ifdef MTK_KERNEL_POWER_OFF_CHARGING
		case KERNEL_POWER_OFF_CHARGING_BOOT:
			video_printf(" => POWER OFF CHARGING MODE\n");
			break;
		case LOW_POWER_OFF_CHARGING_BOOT:
			video_printf(" => LOW POWER OFF CHARGING MODE\n");
			break;
#endif
		case ALARM_BOOT:
			video_printf(" => ALARM BOOT\n");
			break;
		case FASTBOOT:
			video_printf(" => FASTBOOT mode...\n");
			break;
		default:
			video_printf(" => UNKNOWN BOOT\n");
	}
	return;
#endif

#ifdef USER_BUILD
	if (g_boot_mode == FASTBOOT)
		video_printf(" => FASTBOOT mode...\n");
	return;
#endif
}

void platform_init_mmu_mappings(void)
{
	/* configure available RAM banks */
	dram_init();
	arm_mmu_init();
	/* Enable D-cache  */
#if 1
	unsigned int addr;
	unsigned int dram_size = 0;

	dram_size = memory_size();

	for (addr = 0; addr < dram_size; addr += (1024 * 1024)) {
		/*virtual to physical 1-1 mapping */
		arm_mmu_map_section(bi_dram[0].start + addr, bi_dram[0].start + addr, MMU_MEMORY_TYPE_NORMAL_WRITE_BACK_ALLOCATE | MMU_MEMORY_AP_READ_WRITE);
	}
	/* add eXecuteNever for tee memory */
	for (addr = (dram_size & ~(MB-1)); bi_dram[0].start + addr < 4096 * MB; addr += MB) {
		arm_mmu_map_section(bi_dram[0].start + addr, bi_dram[0].start + addr, MMU_MEMORY_TYPE_STRONGLY_ORDERED | MMU_MEMORY_XN);
	}
#endif
	arch_enable_mmu();
}

void platform_init_mmu(void)
{
	unsigned long long addr;
	unsigned int vaddr;
	unsigned long long dram_size;

	/* configure available RAM banks */
	dram_init();

	dram_size = physical_memory_size();

	if (((unsigned long long)DRAM_PHY_ADDR + dram_size) <= 0x100000000ULL) {
		arm_mmu_init();
		for (addr = 0; addr < dram_size; addr += (1024*1024)) {
			/*virtual to physical 1-1 mapping*/
			arm_mmu_map_section(bi_dram[0].start+addr, bi_dram[0].start+addr, MMU_MEMORY_TYPE_NORMAL_WRITE_BACK_ALLOCATE | MMU_MEMORY_AP_READ_WRITE);
		}
		/* add eXecuteNever for tee memory */
		for (addr = (dram_size & ~(MB-1)); bi_dram[0].start+addr < 4096 * MB; addr += MB) {
			arm_mmu_map_section(bi_dram[0].start + addr, bi_dram[0].start + addr, MMU_MEMORY_TYPE_STRONGLY_ORDERED | MMU_MEMORY_XN);
		}
	} else {
		arm_mmu_lpae_init();
		for (addr = 0; addr < dram_size-((unsigned long long)DRAM_PHY_ADDR+dram_size-0x100000000ULL); addr += (unsigned long long)(1024*1024*1024)) {
			arm_mmu_map_block(bi_dram[0].start+addr, (unsigned int)(bi_dram[0].start+addr), LPAE_MMU_MEMORY_TYPE_NORMAL_WRITE_BACK);
		}
		/* add eXecuteNever for tee memory */
		for(addr = (dram_size & ~((1024*MB)-1)); bi_dram[0].start+addr < 4096 * MB; addr += 1024*MB) {
			arm_mmu_map_block(bi_dram[0].start + addr, bi_dram[0].start + addr, LPAE_MMU_MEMORY_TYPE_STRONGLY_ORDERED | LPAE_MMU_MEMORY_XN);
		}
	}
	arch_enable_mmu();
}

void platform_k64_check(void)
{
	dprintf(CRITICAL, "kernel_boot_opt=%d\n", g_boot_arg->kernel_boot_opt);

	switch (g_boot_arg->kernel_boot_opt) {
		case BOOT_OPT_64S3:
		case BOOT_OPT_64S1:
		case BOOT_OPT_64N2:
		case BOOT_OPT_64N1:
			g_is_64bit_kernel = 1;
			dprintf(CRITICAL, "64Bit Kernel\n");
			break;

		case BOOT_OPT_32S3:
		case BOOT_OPT_32S1:
		case BOOT_OPT_32N2:
		case BOOT_OPT_32N1:
		/* maybe need to do something in the feature */
		default:
			g_is_64bit_kernel = 0;
			dprintf(CRITICAL, "32Bit Kernel\n");
			break;
	}
}

void platform_early_init(void)
{
#ifdef MT_SRAM_REPAIR_SUPPORT
	int repair_ret;
#endif
#ifdef LK_PROFILING
#ifdef MT_SRAM_REPAIR_SUPPORT
	unsigned int time_repair_sram;
#endif
	unsigned int time_wdt_early_init;
	unsigned int time_led_init;
	unsigned int time_pmic_init;
	unsigned int time_platform_early_init;

	time_platform_early_init = get_timer(0);
#endif

	platform_init_interrupts();
	g_is_64bit_kernel = 1;
	platform_early_init_timer();

#ifndef MACH_FPGA
	mt_gpio_set_default();
#endif

	/* initialize the uart */
	uart_init_early();

#ifdef MT_SRAM_REPAIR_SUPPORT
#ifdef LK_PROFILING
	time_repair_sram = get_timer(0);
#endif
	repair_ret = repair_sram();
	if (repair_ret != 0) {
		dprintf(CRITICAL, "Sram repair failed %d\n", repair_ret);
		while (1);
	}
#ifdef LK_PROFILING
	dprintf(INFO, "[PROFILE] ------- Repair SRAM takes %d ms --------\n", (int)get_timer(time_repair_sram));
#endif
#endif

	/* i2c_v1_init(); */

#ifdef LK_PROFILING
	time_wdt_early_init = get_timer(0);
#endif
	mtk_wdt_init();

	/* i2c init */
	i2c_hw_init();

#ifdef LK_PROFILING
	dprintf(INFO, "[PROFILE] ------- WDT Init  takes %d ms --------\n", (int)get_timer(time_wdt_early_init));
#endif

#ifdef MACH_FPGA
	mtk_timer_init();   /* GPT4 will be initialized at PL after */
	mtk_wdt_disable();  /* WDT will be triggered when uncompressing linux image on FPGA */
#endif

#ifdef MTK_ALPS_BOX_SUPPORT
	/*box no need led set pwm for panel backlight*/
#else

#ifndef MACH_FPGA
#ifdef LK_PROFILING
	time_led_init = get_timer(0);
#endif
	leds_init();
#ifdef LK_PROFILING
	dprintf(INFO, "[PROFILE] ------- led init takes %d ms --------\n", (int)get_timer(time_led_init));
#endif
#endif
#endif

	/* Workaround by Peng */
	/* pwrap_init_lk(); */
	/* pwrap_init_for_early_porting(); */

#ifdef LK_PROFILING
	time_pmic_init = get_timer(0);
#endif
	pmic_init();
	/* Workaround by Weiqi */
	/* mt6331_upmu_set_rg_vgp1_en(1); */
	/* mt6331_upmu_set_rg_vcam_io_en(1); */
#ifdef LK_PROFILING
	dprintf(INFO, "[PROFILE] ------- pmic_init takes %d ms --------\n", (int)get_timer(time_pmic_init));
#endif

#ifdef LK_PROFILING
	dprintf(INFO, "[PROFILE] ------- platform_early_init takes %d ms --------\n", (int)get_timer(time_platform_early_init));
#endif
}

extern void mt65xx_bat_init(void);
#if defined(MTK_KERNEL_POWER_OFF_CHARGING)

int kernel_charging_boot(void)
{
	if ((g_boot_mode == KERNEL_POWER_OFF_CHARGING_BOOT || g_boot_mode == LOW_POWER_OFF_CHARGING_BOOT) && upmu_is_chr_det() == KAL_TRUE) {
		dprintf(INFO, "[%s] Kernel Power Off Charging with Charger/Usb\n", __func__);
		return 1;
	} else if ((g_boot_mode == KERNEL_POWER_OFF_CHARGING_BOOT || g_boot_mode == LOW_POWER_OFF_CHARGING_BOOT) && upmu_is_chr_det() == KAL_FALSE) {
		dprintf(INFO, "[%s] Kernel Power Off Charging without Charger/Usb\n", __func__);
		return -1;
	} else
		return 0;
}
#endif

#ifdef ENABLE_L2_SHARING
#define L2C_SIZE_CFG_OFF 8
#define L2C_SWITCH 12
/* config L2 cache and sram to its size */
void config_L2_size(void)
{
	volatile unsigned int cache_cfg;
	/* set L2C size to 512KB */
	cache_cfg = DRV_Reg(MCUCFG_BASE);
	cache_cfg &= ~(0xF << L2C_SIZE_CFG_OFF);
	cache_cfg |= 0x3 << L2C_SIZE_CFG_OFF;
	cache_cfg &= ~(1 << L2C_SWITCH);
	DRV_WriteReg(MCUCFG_BASE, cache_cfg);
}

/* config SRAM back from L2 cache for DA relocation */
static void config_shared_SRAM_size(void)
{
	volatile unsigned int cache_cfg;
	/* set L2C size to 256KB */
	cache_cfg = DRV_Reg(MCUCFG_BASE);
	cache_cfg &= (~0xF) << L2C_SIZE_CFG_OFF;
	cache_cfg |= 0x1 << L2C_SIZE_CFG_OFF;
	cache_cfg |= (1 << L2C_SWITCH);
	DRV_WriteReg(MCUCFG_BASE, cache_cfg);
}
#endif

void platform_uninit(void)
{
#ifdef MTK_BOOT_SOUND_SUPPORT
	mt_boot_sound_deinit();
#endif
#ifndef MACH_FPGA
	leds_deinit();
#endif
	platform_deinit_interrupts();
}

#ifdef MTK_ALPS_BOX_SUPPORT

void set_boot_logo_index(unsigned hdmi_res)
{
	switch (hdmi_res) {
		/* case HDMI_VIDEO_720x480i_60Hz: */
		/* case HDMI_VIDEO_720x576i_50Hz: */
		case HDMI_VIDEO_720x480p_60Hz:
			BOOT_LOGO_INDEX = BOOT_LOGO_INDEX1;
			break;

		case HDMI_VIDEO_720x576p_50Hz:
			BOOT_LOGO_INDEX = BOOT_LOGO_INDEX2;
			break;

		case HDMI_VIDEO_1280x720p_60Hz:
		case HDMI_VIDEO_1280x720p_50Hz:
			BOOT_LOGO_INDEX = BOOT_LOGO_INDEX3;
			break;

		case HDMI_VIDEO_1920x1080i_60Hz:
		case HDMI_VIDEO_1920x1080p_60Hz:
		case HDMI_VIDEO_1920x1080i_50Hz:
		case HDMI_VIDEO_1920x1080p_50Hz:
			BOOT_LOGO_INDEX = BOOT_LOGO_INDEX4;
			break;

		case HDMI_VIDEO_2160P_23_976HZ: /* 13 */
		case HDMI_VIDEO_2160P_24HZ: /* 14 */
		case HDMI_VIDEO_2160P_25HZ: /* 15 */
		case HDMI_VIDEO_2160P_29_97HZ:  /* 16 */
		case HDMI_VIDEO_2160P_30HZ: /* 17 */
			BOOT_LOGO_INDEX = BOOT_LOGO_INDEX5;
			break;

		case HDMI_VIDEO_2161P_24HZ: /* 17 */
			BOOT_LOGO_INDEX = BOOT_LOGO_INDEX6;
			break;

		default:
			BOOT_LOGO_INDEX = BOOT_LOGO_INDEX4;

			dprintf(0, "error in %s set cvbs boot logo index, hdmi_res %d\n", hdmi_res);
	}

	dprintf(0, "set hdmi_res %d boot logo index %d\n", hdmi_res, BOOT_LOGO_INDEX);
}
#endif

extern void flush_invalidate_cache_v7(void);
static void flush_all(void)
{
	__asm("stmfd   sp!, {r0-r6, r9-r11}");
	/*fixme flush_invalidate_cache_v7(); */
	__asm("ldmfd   sp!, {r0-r6, r9-r11}");
}

void platform_init(void)
{
#ifdef LK_PROFILING
	unsigned int time_nand_emmc;
	unsigned int time_env;
	unsigned int time_disp_init;
	unsigned int time_load_logo;
	unsigned int time_backlight;
	unsigned int time_boot_mode;
#ifdef MTK_SECURITY_SW_SUPPORT
	unsigned int time_security_init;
#endif
	unsigned int time_bat_init;
	unsigned int time_RTC_boot_Check;
	unsigned int time_show_logo;
	unsigned int time_sw_env;
	unsigned int time_platform_init;

	time_platform_init = get_timer(0);
#endif
	u64 pl_start_addr = 0;
	plinfo_get_brom_header_block_size(&pl_start_addr);

	dprintf(CRITICAL, "platform_init()\n");

#ifdef DUMMY_AP
	dummy_ap_entry();
#endif

#ifdef LK_PROFILING
	time_nand_emmc = get_timer(0);
#endif
#ifdef MTK_EMMC_SUPPORT
	mmc_legacy_init(1);
#else
#ifndef MACH_FPGA
	nand_init();
	nand_driver_test();
#endif
#endif
#ifdef LK_PROFILING
	dprintf(INFO, "[PROFILE] ------- NAND/EMMC init takes %d ms --------\n", (int)get_timer(time_nand_emmc));
#endif

#ifdef MTK_KERNEL_POWER_OFF_CHARGING
	if ((g_boot_arg->boot_reason == BR_USB) && (upmu_is_chr_det() == KAL_FALSE)) {
		dprintf(INFO, "[%s] Unplugged Charger/Usb between Pre-loader and Uboot in Kernel Charging Mode, Power Off\n", __func__);
		mt6575_power_off();
	}
#endif

#ifdef LK_PROFILING
	time_env = get_timer(0);
#endif
	env_init();
	print_env();
#ifdef LK_PROFILING
	dprintf(INFO, "[PROFILE] ------- ENV init takes %d ms --------\n", (int)get_timer(time_env));
#endif

#ifdef LK_PROFILING
	time_disp_init = get_timer(0);
#endif

#ifdef MTK_ALPS_BOX_SUPPORT
#if HDMI_SUB_PATH_LK
	{
		unsigned hdmi_res = boot_addr.hdmi_res;

		/* unsigned hdmi_res = HDMI_VIDEO_1920x1080p_60Hz; */

		hdmi_checkedid(0);

		dprintf(0, "[LK] ------- get hdmi_res 0x%x from boot --------\n", hdmi_res);

		dprintf(0, "[LK] ------- LCM_WIDTH 0x%x CFG_DISPLAY_WIDTH 0x%x --------\n", LCM_WIDTH, CFG_DISPLAY_WIDTH);

		if (hdmi_res & HDMI_RES_MASK_LK) {
			hdmi_res &= HDMI_RES_VALID;

		} else {
			hdmi_res = HDMI_VIDEO_1920x1080p_60Hz;
		}

		/* hdmi_res = HDMI_VIDEO_720x480p_60Hz; */
		/* hdmi_res = HDMI_VIDEO_1920x1080p_60Hz; */

		hdmi_res = vOutputResolution();

		dprintf(0, "[LK] ------- set hdmi_res 0x%x  --------\n", hdmi_res);

		set_boot_logo_index(hdmi_res);

		disp_set_hdmi_resolution(hdmi_res);

	}
#endif

#endif

#ifndef LK_NO_DISP
	/* initialize the frame buffet information */
	g_fb_size = mt_disp_get_vram_size();
	g_fb_base = (unsigned int)g_boot_arg->tee_reserved_mem.start - g_fb_size;
	/* dprintf(INFO, "FB base = 0x%x, FB size = %d\n", g_fb_base, g_fb_size); */
	dprintf(0, "FB base = 0x%x, FB size = %d\n", g_fb_base, g_fb_size);

	mt_disp_init((void *)g_fb_base);
#ifdef LK_PROFILING
	dprintf(0, "[PROFILE] ------- disp init takes %d ms --------\n", (int)get_timer(time_disp_init));
#endif

#ifdef LK_PROFILING
	time_load_logo = get_timer(0);
#endif
	drv_video_init();

	mboot_common_load_logo((unsigned long)mt_get_logo_db_addr(), "logo");
	/* dprintf(INFO, "Show BLACK_PICTURE\n"); */
	/* mt_disp_fill_rect(0, 0, CFG_DISPLAY_WIDTH, CFG_DISPLAY_HEIGHT, 0x0); */
	/* mt_disp_power(TRUE); */
	/* mt_disp_update(0, 0, CFG_DISPLAY_WIDTH, CFG_DISPLAY_HEIGHT); */
	/* mt_disp_power(1);           //power on display related modules */
#ifdef LK_PROFILING
	dprintf(0, "[PROFILE] ------- load_logo takes %d ms --------\n", (int)get_timer(time_load_logo));
#endif
#endif /*#ifndef LK_NO_DISP */
	/*for kpd pmic mode setting */
	set_kpd_pmic_mode();

#ifndef MACH_FPGA
	if (is_low_battery() == KAL_FALSE) {

#ifdef LK_PROFILING
		time_backlight = get_timer(0);
#endif
		fp_t = get_timer(0);
#ifdef LK_PROFILING
		dprintf(0, "[PROFILE] ------- backlight takes %d ms --------\n", (int)get_timer(time_backlight));
#endif

	}
#endif

#ifndef MACH_FPGA
#ifdef LK_PROFILING
	time_boot_mode = get_timer(0);
#endif
	boot_mode_select();

#ifdef MTK_LK_IRRX_SUPPORT
	mtk_ir_wait_event();
#endif

#ifdef LK_PROFILING
	dprintf(0, "[PROFILE] ------- boot mode select takes %d ms --------\n", (int)get_timer(time_boot_mode));
#endif
#endif

#ifdef MTK_BOOT_SOUND_SUPPORT
	if (g_boot_mode == NORMAL_BOOT || g_boot_mode == FASTBOOT || g_boot_mode == ALARM_BOOT) {
		mt_boot_sound_init();
	}
#endif

#ifdef MTK_SECURITY_SW_SUPPORT
#ifdef LK_PROFILING
	time_security_init = get_timer(0);
#endif

	/* initialize security library */
	sec_func_init(pl_start_addr);

#ifdef LK_PROFILING
	dprintf(0, "[PROFILE] ------- Security init takes %d ms --------\n", (int)get_timer(time_security_init));
#endif

	seclib_set_oemkey(g_oemkey, OEM_PUBK_SZ);

	/*Verify logo before use it*/
	if ( 0 != sec_logo_check() ) {
		dprintf(CRITICAL,"<ASSERT> %s:line %d\n",__FILE__,__LINE__);
		while (1);
	}
#endif

	/*Show download logo & message on screen */
	if (g_boot_arg->boot_mode == DOWNLOAD_BOOT) {
		dprintf(CRITICAL, "[LK] boot mode is DOWNLOAD_BOOT\n");

#ifdef MTK_SECURITY_SW_SUPPORT
		/* verify da before jumping to da */
		if (sec_usbdl_enabled()) {
			u8 *da_addr = (u8 *) g_boot_arg->da_info.addr;
			u32 da_len = g_boot_arg->da_info.len;
			u32 sig_len = g_boot_arg->da_info.sig_len;
			u8 *sig_addr = (unsigned char *)da_addr + (da_len - sig_len);

			if (da_len == 0 || sig_len == 0) {
				dprintf(INFO, "[LK] da argument is invalid\n");
				dprintf(INFO, "da_addr = 0x%x\n", (int)da_addr);
				dprintf(INFO, "da_len  = 0x%x\n", da_len);
				dprintf(INFO, "sig_len = 0x%x\n", sig_len);
			}

			if (sec_usbdl_verify_da(da_addr, (da_len - sig_len), sig_addr, sig_len)) {
				/* da verify fail */
				video_printf(" => Not authenticated tool, download stop...\n");
				while (1);  /* fix me, should not be infinite loop in lk */
			}
		} else
#endif
		{
			dprintf(INFO, " DA verification disabled...\n");
		}
#ifndef LK_NO_DISP
#if HDMI_SUB_PATH_LK
		dprintf(0, "mt_disp_show_boot_logo_by_index #%d\n", __LINE__);
		mt_disp_show_boot_logo_by_index(BOOT_LOGO_INDEX);
#else
		dprintf(0, "mt_disp_show_boot_logo #%d\n", __LINE__);
		mt_disp_show_boot_logo();
#endif
		video_printf(" => Downloading...\n");
		mt65xx_backlight_on();
#endif /*#ifndef LK_NO_DISP */

		mtk_wdt_disable();  /* Disable wdt before jump to DA */
		platform_uninit();
#ifdef HAVE_CACHE_PL310
		l2_disable();
#endif
		arch_disable_cache(UCACHE);
		arch_disable_mmu();
#ifdef ENABLE_L2_SHARING
		config_shared_SRAM_size();
#endif

		jump_da(g_boot_arg->da_info.addr, g_boot_arg->da_info.arg1, g_boot_arg->da_info.arg2);
	}
#ifdef LK_PROFILING
	time_bat_init = get_timer(0);
#endif
	mt65xx_bat_init();
#ifdef LK_PROFILING
	dprintf(0, "[PROFILE] ------- battery init takes %d ms --------\n", (int)get_timer(time_bat_init));
#endif

#ifndef CFG_POWER_CHARGING
#ifdef LK_PROFILING
	time_RTC_boot_Check = get_timer(0);
#endif
	/* NOTE: if define CFG_POWER_CHARGING, will rtc_boot_check() in mt65xx_bat_init() */
	rtc_boot_check(false);
#ifdef LK_PROFILING
	dprintf(0, "[PROFILE] ------- RTC boot check Init  takes %d ms --------\n", (int)get_timer(time_RTC_boot_Check));
#endif
#endif

#ifndef MACH_FPGA
#ifdef LK_PROFILING
	time_sw_env = get_timer(0);
#endif
	sw_env();
#ifdef LK_PROFILING
	dprintf(0, "[PROFILE] ------- sw_env takes %d ms --------\n", (int)get_timer(time_sw_env));
#endif
#endif

#ifdef LK_PROFILING
	time_show_logo = get_timer(0);
#endif

#ifndef LK_NO_DISP
#ifdef MTK_KERNEL_POWER_OFF_CHARGING
	if (kernel_charging_boot() == 1) {

		mt_disp_power(TRUE);
		mt_disp_show_low_battery();
		mt65xx_leds_brightness_set(6, 110);
	} else if (g_boot_mode != KERNEL_POWER_OFF_CHARGING_BOOT && g_boot_mode != LOW_POWER_OFF_CHARGING_BOOT) {
		if (g_boot_mode != ALARM_BOOT && (g_boot_mode != FASTBOOT)) {
#if HDMI_SUB_PATH_LK || HDMI_MAIN_PATH_LK
			dprintf(0, "mt_disp_show_boot_logo_by_index #%d\n", __LINE__);
			mt_disp_show_boot_logo_by_index(BOOT_LOGO_INDEX);
#else
			dprintf(0, "mt_disp_show_boot_logo #%d\n", __LINE__);
			mt_disp_show_boot_logo();
#endif
		}
	}
#else
	if (g_boot_mode != ALARM_BOOT && (g_boot_mode != FASTBOOT)) {
#if HDMI_SUB_PATH_LK || HDMI_MAIN_PATH_LK
		dprintf(0, "mt_disp_show_boot_logo_by_index #%d\n", __LINE__);
		mt_disp_show_boot_logo_by_index(BOOT_LOGO_INDEX);
#else
		dprintf(0, "mt_disp_show_boot_logo #%d\n", __LINE__);
		mt_disp_show_boot_logo();
#endif
	}
#endif
	flush_all();

	/*after logo and title update, open backlight */
#ifdef MTK_ALPS_BOX_SUPPORT
#else
	mt65xx_backlight_on();
#endif

#endif /*#ifndef LK_NO_DISP */

#ifdef LK_PROFILING
	dprintf(0, "[PROFILE] ------- show logo takes %d ms --------\n", (int)get_timer(time_show_logo));
#endif

#ifdef LK_PROFILING
	dprintf(0, "[PROFILE] ------- platform_init takes %d ms --------\n", (int)get_timer(time_platform_init));
#endif
}
