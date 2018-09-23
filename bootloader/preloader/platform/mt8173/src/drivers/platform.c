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
#include "boot_device.h"
#include "nand.h"
#include "mmc_common_inter.h"

#include "uart.h"
#include "nand_core.h"
#include "pll.h"
#include "i2c.h"
#include "rtc.h"
#include "emi.h"
#include "pmic.h"
#include "wdt.h"
#include "ram_console.h"
#include "cust_sec_ctrl.h"
#include "gpio.h"
#include "pmic_wrap_init.h"
#include "keypad.h"
#include "usbphy.h"
#include "timer.h"
#include "dram_buffer.h"
#include "ssusb_sifslv_ippc_c_header.h"
#include "mtk-phy-d60802.h"
#include "usbd.h"
#include "spm_mtcmos.h"
#ifdef MTK_TPS61280_SUPPORT
#include "tps61280.h"
#endif
#ifdef MTK_BQ24297_SUPPORT
#include <bq24297.h>
#endif

/*============================================================================*/
/* CONSTAND DEFINITIONS                                                       */
/*============================================================================*/
#define MOD "[PLFM]"

/*============================================================================*/
/* GLOBAL VARIABLES                                                           */
/*============================================================================*/
unsigned int sys_stack[CFG_SYS_STACK_SZ >> 2];
const unsigned int sys_stack_sz = CFG_SYS_STACK_SZ;
boot_mode_t g_boot_mode;
boot_dev_t g_boot_dev;
meta_com_t g_meta_com_type = META_UNKNOWN_COM;
u32 g_meta_com_id = 0;
boot_reason_t g_boot_reason;
ulong g_boot_time;
u32 g_ddr_reserve_enable = 0;
u32 g_ddr_reserve_success = 0;
u32 g_smc_boot_opt;
u32 g_lk_boot_opt;
u32 g_kernel_boot_opt;

extern unsigned int part_num;
extern part_hdr_t   part_info[PART_MAX_NUM];

#if defined (MTK_KERNEL_POWER_OFF_CHARGING)
kal_bool kpoc_flag = false;
#endif

#if CFG_RTC_REG_EMERGENCY_DL_SUPPORT
/*
    flag to indicate if entering emergency download mode
    it is set by rtc_boot_check() function
*/
kal_bool emergency_dl_flag = false;
uint32   emerg_dl_pldlmd_usb_enum_to = 0;  /* usb enumeration time-out (ms) in pl dl mode, 0: never timeout */
uint32   emerg_dl_pldlmd_usb_hk_to   = 0;  /* usb handshake   time-out (ms) in pl dl mode, 0: never timeout */
extern void rtc_exit_emergency_dl_mode(void);
#endif /* CFG_RTC_REG_EMERGENCY_DL_SUPPORT */

#if CFG_BOOT_ARGUMENT
#define bootarg g_dram_buf->bootarg
#endif

#if CFG_USB_AUTO_DETECT
bool g_usbdl_flag;
#endif


/*============================================================================*/
/* EXTERNAL FUNCTIONS                                                         */
/*============================================================================*/
static u32 boot_device_init(void)
{
    #if (CFG_BOOT_DEV == BOOTDEV_SDMMC)
    return (u32)mmc_init_device();
    #else
    return (u32)nand_init_device();
    #endif
}

int usb_accessory_in(void)
{
#if !CFG_FPGA_PLATFORM
    int exist = 0;

    if (PMIC_CHRDET_EXIST == pmic_IsUsbCableIn()) {
        exist = 1;
        #if !CFG_USBIF_COMPLIANCE
        /* enable charging current as early as possible to avoid can't enter
         * following battery charging flow when low battery
         */
        platform_set_chrg_cur(450);
        #endif
    }
    return exist;
#else
    return 1;
#endif
}

extern bool is_uart_cable_inserted(void);

int usb_cable_in(void)
{
#if !CFG_FPGA_PLATFORM
    int exist = 0;
    CHARGER_TYPE ret;

    if ((g_boot_reason == BR_USB) || usb_accessory_in()) {
        ret = mt_charger_type_detection();
        if (ret == STANDARD_HOST || ret == CHARGING_HOST) {
            print("\n%s USB cable in\n", MOD);
            mt_usb_phy_poweron();
            mt_usb_phy_savecurrent();

            exist = 1;
        } else if (ret == NONSTANDARD_CHARGER || ret == STANDARD_CHARGER) {
            #if CFG_USBIF_COMPLIANCE
            platform_set_chrg_cur(450);
            #endif
        }
    }

    return exist;
#else
    print("\n%s USB cable in\n", MOD);
    mt_usb_phy_poweron();
    mt_usb_phy_savecurrent();

    return 1;
#endif
}

#if CFG_FPGA_PLATFORM
void show_tx(void)
{
	UINT8 var;
	USBPHY_I2C_READ8(0x6E, &var);
	UINT8 var2 = (var >> 3) & ~0xFE;
	print("[USB]addr: 0x6E (TX), value: %x - %x\n", var, var2);
}

void store_tx(UINT8 value)
{
	UINT8 var;
	UINT8 var2;
	USBPHY_I2C_READ8(0x6E, &var);

	if (value == 0) {
		var2 = var & ~(1 << 3);
	} else {
		var2 = var | (1 << 3);
	}

	USBPHY_I2C_WRITE8(0x6E, var2);
	USBPHY_I2C_READ8(0x6E, &var);
	var2 = (var >> 3) & ~0xFE;

	print("[USB]addr: 0x6E TX [AFTER WRITE], value after: %x - %x\n", var, var2);
}

void show_rx(void)
{
	UINT8 var;
	USBPHY_I2C_READ8(0x77, &var);
	UINT8 var2 = (var >> 7) & ~0xFE;
	print("[USB]addr: 0x77 (RX) [AFTER WRITE], value after: %x - %x\n", var, var2);
}

void test_uart(void)
{
	int i=0;
	UINT8 val = 0;
	for (i=0; i<1000; i++)
	{
			show_tx();
            mdelay(300);
			if (val) {
				val = 0;
			}
			else {
				val = 1;
			}
			store_tx(val);
			show_rx();
            mdelay(1000);
	}
}
#endif

void set_to_usb_mode(void)
{
		UINT8 var;
#if !CFG_FPGA_PLATFORM
		/* Turn on USB MCU Bus Clock */
		var = READ_REG(PERI_GLOBALCON_PDN0_SET);
		print("\n[USB]USB bus clock: 0x008, value: %x\n", var);
		USB_CLR_BIT(USB0_PDN, PERI_GLOBALCON_PDN0_SET);
		var = READ_REG(PERI_GLOBALCON_PDN0_SET);
		print("\n[USB]USB bus clock: 0x008, value after: %x\n", var);

		/* Switch from BC1.1 mode to USB mode */
		var = USBPHY_READ8(0x1A);
		print("\n[USB]addr: 0x1A, value: %x\n", var);
		USBPHY_WRITE8(0x1A, var & 0x7f);
		print("\n[USB]addr: 0x1A, value after: %x\n", USBPHY_READ8(0x1A));

		/* Set RG_UART_EN to 0 */
		var = USBPHY_READ8(0x6E);
		print("\n[USB]addr: 0x6E, value: %x\n", var);
		USBPHY_WRITE8(0x6E, var & ~0x01);
		print("\n[USB]addr: 0x6E, value after: %x\n", USBPHY_READ8(0x6E));

		/* Set RG_USB20_DM_100K_EN to 0 */
		var = USBPHY_READ8(0x22);
		print("\n[USB]addr: 0x22, value: %x\n", var);
		USBPHY_WRITE8(0x22, var & ~0x02);
		print("\n[USB]addr: 0x22, value after: %x\n", USBPHY_READ8(0x22));
#else
		/* Set RG_UART_EN to 0 */
		USBPHY_I2C_READ8(0x6E, &var);
		print("\n[USB]addr: 0x6E, value: %x\n", var);
		USBPHY_I2C_WRITE8(0x6E, var & ~0x01);
		USBPHY_I2C_READ8(0x6E, &var);
		print("\n[USB]addr: 0x6E, value after: %x\n", var);

		/* Set RG_USB20_DM_100K_EN to 0 */
		USBPHY_I2C_READ8(0x22, &var);
		print("\n[USB]addr: 0x22, value: %x\n", var);
		USBPHY_I2C_WRITE8(0x22, var & ~0x02);
		USBPHY_I2C_READ8(0x22, &var);
		print("\n[USB]addr: 0x22, value after: %x\n", var);
#endif
		var = READ_REG(UART1_BASE + 0x90);
		print("\n[USB]addr: 0x11002090 (UART1), value: %x\n", var);
		WRITE_REG(var & ~0x01, UART1_BASE + 0x90);
		print("\n[USB]addr: 0x11002090 (UART1), value after: %x\n", READ_REG(UART1_BASE + 0x90));
}

void set_to_uart_mode(void)
{
		UINT8 var;
#if !CFG_FPGA_PLATFORM
		/* Turn on USB MCU Bus Clock */
		var = READ_REG(PERI_GLOBALCON_PDN0_SET);
		print("\n[USB]USB bus clock: 0x008, value: %x\n", var);
		USB_CLR_BIT(USB0_PDN, PERI_GLOBALCON_PDN0_SET);
		var = READ_REG(PERI_GLOBALCON_PDN0_SET);
		print("\n[USB]USB bus clock: 0x008, value after: %x\n", var);

		/* Switch from BC1.1 mode to USB mode */
		var = USBPHY_READ8(0x1A);
		print("\n[USB]addr: 0x1A, value: %x\n", var);
		USBPHY_WRITE8(0x1A, var & 0x7f);
		print("\n[USB]addr: 0x1A, value after: %x\n", USBPHY_READ8(0x1A));

		/* Set ru_uart_mode to 2'b01 */
		var = USBPHY_READ8(0x6B);
		print("\n[USB]addr: 0x6B, value: %x\n", var);
		USBPHY_WRITE8(0x6B, var | 0x5C);
		print("\n[USB]addr: 0x6B, value after: %x\n", USBPHY_READ8(0x6B));

		/* Set RG_UART_EN to 1 */
		var = USBPHY_READ8(0x6E);
		print("\n[USB]addr: 0x6E, value: %x\n", var);
		USBPHY_WRITE8(0x6E, var | 0x07);
		print("\n[USB]addr: 0x6E, value after: %x\n", USBPHY_READ8(0x6E));

		/* Set RG_USB20_DM_100K_EN to 1 */
		var = USBPHY_READ8(0x22);
		print("\n[USB]addr: 0x22, value: %x\n", var);
		USBPHY_WRITE8(0x22, var | 0x02);
		print("\n[USB]addr: 0x22, value after: %x\n", USBPHY_READ8(0x22));

		/* Set RG_SUSPENDM to 1 */
		var = USBPHY_READ8(0x68);
		print("\n[USB]addr: 0x68, value: %x\n", var);
		USBPHY_WRITE8(0x68, var | 0x08);
		print("\n[USB]addr: 0x68, value after: %x\n", USBPHY_READ8(0x68));

		/* force suspendm = 1 */
		var = USBPHY_READ8(0x6A);
		print("\n[USB]addr: 0x6A, value: %x\n", var);
		USBPHY_WRITE8(0x6A, var | 0x04);
		print("\n[USB]addr: 0x6A, value after: %x\n", USBPHY_READ8(0x6A));
#else
		/* Set ru_uart_mode to 2'b01 */
		USBPHY_I2C_READ8(0x6B, &var);
		print("\n[USB]addr: 0x6B, value: %x\n", var);
		USBPHY_I2C_WRITE8(0x6B, var | 0x7C);
		USBPHY_I2C_READ8(0x6B, &var);
		print("\n[USB]addr: 0x6B, value after: %x\n", var);

		/* Set RG_UART_EN to 1 */
		USBPHY_I2C_READ8(0x6E, &var);
		print("\n[USB]addr: 0x6E, value: %x\n", var);
		USBPHY_I2C_WRITE8(0x6E, var | 0x07);
		USBPHY_I2C_READ8(0x6E, &var);
		print("\n[USB]addr: 0x6E, value after: %x\n", var);

		/* Set RG_USB20_DM_100K_EN to 1 */
		USBPHY_I2C_READ8(0x22, &var);
		print("\n[USB]addr: 0x22, value: %x\n", var);
		USBPHY_I2C_WRITE8(0x22, var | 0x02);
		USBPHY_I2C_READ8(0x22, &var);
		print("\n[USB]addr: 0x22, value after: %x\n", var);
#endif
		mdelay(100);

        var = DRV_Reg32(UART0_BASE+0x90);
		print("\n[USB]addr: 0x11002090 (UART1), value: %x\n", var);
		DRV_WriteReg32(UART0_BASE+0x90, 0x0001);
		print("\n[USB]addr: 0x11002090 (UART1), value after: %x\n", DRV_Reg32(UART0_BASE+0x90));
}

void platform_vusb_on(void)
{
#if !CFG_FPGA_PLATFORM
    U32 ret=0;

    ret=pmic_config_interface( (U32)(DIGLDO_CON1),
                               (U32)(1),
                               (U32)(PMIC_RG_VUSB_EN_MASK),
                               (U32)(PMIC_RG_VUSB_EN_SHIFT)
	                         );
    if(ret!=0)
    {
        print("[platform_vusb_on] Fail\n");
    }
    else
    {
        print("[platform_vusb_on] PASS\n");
    }
#endif
    return;

}

void platform_parse_bootopt(void)
{
#if 0
    u8 *bootimg_opt;
    u8 bootimg_opt_buf[10];

    u8 bootimg_opt_str_array[10][5];
    u32 i;

    i = 0;
    /* offset of boot option in boot image header 0x40 */
    bootimg_opt = CFG_BOOTIMG_HEADER_MEMADDR + 0x40;
    memset(bootimg_opt_str_array,'\0',50);
    memcpy(bootimg_opt_str_array[BOOT_OPT_64S3],STR_BOOT_OPT_64S3,4);
    memcpy(bootimg_opt_str_array[BOOT_OPT_64S1],STR_BOOT_OPT_64S1,4);
    memcpy(bootimg_opt_str_array[BOOT_OPT_32S3],STR_BOOT_OPT_32S3,4);
    memcpy(bootimg_opt_str_array[BOOT_OPT_32S1],STR_BOOT_OPT_32S1,4);
    memcpy(bootimg_opt_str_array[BOOT_OPT_64N2],STR_BOOT_OPT_64N2,4);
    memcpy(bootimg_opt_str_array[BOOT_OPT_64N1],STR_BOOT_OPT_64N1,4);
    memcpy(bootimg_opt_str_array[BOOT_OPT_32N2],STR_BOOT_OPT_32N2,4);
    memcpy(bootimg_opt_str_array[BOOT_OPT_32N1],STR_BOOT_OPT_32N1,4);

    memset(bootimg_opt_buf,'\0',10);
    memcpy(bootimg_opt_buf, bootimg_opt, 8);
    bootimg_opt = bootimg_opt + 8;
    if ( 0 == strcmp(bootimg_opt_buf, "bootopt="))
    {
        //"SMC option"
        memset(bootimg_opt_buf,'\0',10);
        memcpy(bootimg_opt_buf, bootimg_opt, 4);

        for(i = 0; i < BOOT_OPT_UNKNOWN; i++)
        {
            if ( 0 == strcmp(bootimg_opt_str_array[i], bootimg_opt_buf))
            {
                g_smc_boot_opt = i;
                print("%s,%s,boot_opt=0x%x\n", MOD, bootimg_opt_str_array[g_smc_boot_opt], g_smc_boot_opt);
                break;
            }
        }

        //"LK option"
        bootimg_opt = bootimg_opt + 5;
        memset(bootimg_opt_buf,'\0',10);
        memcpy(bootimg_opt_buf, bootimg_opt, 4);

        for(i = 0; i < BOOT_OPT_UNKNOWN; i++)
        {
            if ( 0 == strcmp(bootimg_opt_str_array[i], bootimg_opt_buf))
            {
                g_lk_boot_opt = i;
                print("%s,%s,boot_opt=0x%x\n", MOD, bootimg_opt_str_array[g_lk_boot_opt], g_lk_boot_opt);
                break;
            }
        }

        //"Kernel option"
        bootimg_opt = bootimg_opt + 5;
        memset(bootimg_opt_buf,'\0',10);
        memcpy(bootimg_opt_buf, bootimg_opt, 4);

        for(i = 0; i < BOOT_OPT_UNKNOWN; i++)
        {
            if ( 0 == strcmp(bootimg_opt_str_array[i], bootimg_opt_buf))
            {
                g_kernel_boot_opt = i;
                print("%s,%s,boot_opt=0x%x\n", MOD, bootimg_opt_str_array[g_kernel_boot_opt], g_kernel_boot_opt);
                break;
            }
        }
    }
    else    // if no specific bootopt
#endif
    {
        //bootopt=64S3,32S1,64S1
        g_smc_boot_opt = BOOT_OPT_64S3;
#if CFG_ATF_SUPPORT
        g_lk_boot_opt = BOOT_OPT_32N1;
        g_kernel_boot_opt = BOOT_OPT_64N1;
#else
        g_lk_boot_opt = BOOT_OPT_32S1;
        g_kernel_boot_opt = BOOT_OPT_64S1;
#endif
    }
}


void platform_set_boot_args()
{
#if CFG_BOOT_ARGUMENT
    bootarg.magic = BOOT_ARGUMENT_MAGIC;
    bootarg.mode  = g_boot_mode;
    bootarg.e_flag = sp_check_platform();
    bootarg.log_port = CFG_UART_LOG;
    bootarg.log_baudrate = CFG_LOG_BAUDRATE;
    bootarg.log_enable = (u8)log_status();
#if !CFG_FPGA_PLATFORM
/*In FPGA phase, dram related function should by pass*/
    bootarg.dram_rank_num = get_dram_rank_nr();
    get_dram_rank_size(bootarg.dram_rank_size);
#endif
    bootarg.boot_reason = g_boot_reason;
    bootarg.meta_com_type = (u32)g_meta_com_type;
    bootarg.meta_com_id = g_meta_com_id;
    bootarg.boot_time = get_timer(g_boot_time);

    bootarg.part_num =  g_dram_buf->part_num;
    bootarg.part_info = g_dram_buf->part_info;

    bootarg.ddr_reserve_enable = g_ddr_reserve_enable;
    bootarg.ddr_reserve_success= g_ddr_reserve_success;
    bootarg.dram_buf_size =  sizeof(dram_buf_t);

    bootarg.smc_boot_opt = g_smc_boot_opt;
    bootarg.lk_boot_opt = g_lk_boot_opt;
    bootarg.kernel_boot_opt = g_kernel_boot_opt;

#if CFG_ATF_SUPPORT
    {
        u32 sec_addr, sec_size = 0, atf_log_size = 0;
        u32 i;
        u64 total_size = 0;

        tee_get_secmem_start(&sec_addr);
        tee_get_secmem_size(&sec_size, &atf_log_size);
        bootarg.tee_reserved_mem.start = sec_addr;
        bootarg.tee_reserved_mem.size = atf_log_size;

        for(i=0;i<bootarg.dram_rank_num;i++)
            total_size += bootarg.dram_rank_size[i];

        if(CFG_DRAM_ADDR + total_size < 0x100000000)
        {
            sec_addr = bootarg.dram_rank_num;
            while(sec_addr > 0 && sec_size > 0)
            {
                sec_addr--;
                if(bootarg.dram_rank_size[sec_addr] > sec_size)
                {
                    bootarg.dram_rank_size[sec_addr] -= sec_size;
                    sec_size = 0;
                }
                else
                {
                    sec_size -= bootarg.dram_rank_size[sec_addr];
                    bootarg.dram_rank_size[sec_addr] = 0;
                }
            }
        }
    }
#else
    bootarg.tee_reserved_mem.start = 0;
    bootarg.tee_reserved_mem.size = 0;
#endif

    bootarg.non_secure_sram_addr = CFG_NON_SECURE_SRAM_ADDR;
    bootarg.non_secure_sram_size = CFG_NON_SECURE_SRAM_SIZE;

    print("%s NON SECURE SRAM ADDR: 0x%x\n", MOD, bootarg.non_secure_sram_addr);
    print("%s NON SECURE SRAM SIZE: 0x%x\n", MOD, bootarg.non_secure_sram_size);


#if CFG_WORLD_PHONE_SUPPORT
    print("%s md_type[0] = %d \n", MOD, bootarg.md_type[0]);
    print("%s md_type[1] = %d \n", MOD, bootarg.md_type[1]);
#endif

    print("\n%s boot reason: %d\n", MOD, g_boot_reason);
    print("%s boot mode: %d\n", MOD, g_boot_mode);
    print("%s META COM%d: %d\n", MOD, bootarg.meta_com_id, bootarg.meta_com_type);
    print("%s <0x%x>: 0x%x\n", MOD, &bootarg.e_flag, bootarg.e_flag);
    print("%s boot time: %dms\n", MOD, bootarg.boot_time);
    print("%s DDR reserve mode: enable = %d, success = %d\n", MOD, bootarg.ddr_reserve_enable, bootarg.ddr_reserve_success);
    print("%s dram_buf_size: 0x%x\n", MOD, bootarg.dram_buf_size);
    print("%s smc_boot_opt: 0x%x\n", MOD, bootarg.smc_boot_opt);
    print("%s lk_boot_opt: 0x%x\n", MOD, bootarg.lk_boot_opt);
    print("%s kernel_boot_opt: 0x%x\n", MOD, bootarg.kernel_boot_opt);
    print("%s tee_reserved_mem: 0x%llx, 0x%llx\n", MOD, bootarg.tee_reserved_mem.start, bootarg.tee_reserved_mem.size);

#endif

}

void platform_set_dl_boot_args(da_info_t *da_info)
{
#if CFG_BOOT_ARGUMENT
    if (da_info->addr != BA_FIELD_BYPASS_MAGIC)
	bootarg.da_info.addr = da_info->addr;

    if (da_info->arg1 != BA_FIELD_BYPASS_MAGIC)
	bootarg.da_info.arg1 = da_info->arg1;

    if (da_info->arg2 != BA_FIELD_BYPASS_MAGIC)
	bootarg.da_info.arg2 = da_info->arg2;

    if (da_info->len != BA_FIELD_BYPASS_MAGIC)
	bootarg.da_info.len = da_info->len;

    if (da_info->sig_len != BA_FIELD_BYPASS_MAGIC)
	bootarg.da_info.sig_len = da_info->sig_len;
#endif

    return;
}

void platform_wdt_all_kick(void)
{
    /* kick watchdog to avoid cpu reset */
    mtk_wdt_restart();

#if !CFG_FPGA_PLATFORM
    /* kick PMIC watchdog to keep charging */
    pl_kick_chr_wdt();
#endif
}

void platform_wdt_kick(void)
{
    /* kick hardware watchdog */
    mtk_wdt_restart();
}

#if CFG_DT_MD_DOWNLOAD
void platform_modem_download(void)
{
    print("[%s] modem download...\n", MOD);

    /* Switch to MT6261 USB:
     * GPIO_USB_SW1(USB_SW1)=1		//GPIO115, GPIO48
     * GPIO_USB_SW2(USB_SW2)=0		//GPIO116, GPIO196
     * phone
    #define GPIO_UART_URTS0_PIN         (GPIO115 | 0x80000000)
    #define GPIO_UART_UCTS0_PIN         (GPIO116 | 0x80000000)
     * EVB
    #define GPIO_EXT_USB_SW1         (GPIO48 | 0x80000000)
    #define GPIO_EXT_USB_SW2         (GPIO196 | 0x80000000)
     */
    mt_set_gpio_mode(GPIO_EXT_USB_SW1, GPIO_EXT_USB_SW1_M_GPIO);
    mt_set_gpio_mode(GPIO_EXT_USB_SW2, GPIO_EXT_USB_SW2_M_GPIO);
    mt_set_gpio_dir(GPIO_EXT_USB_SW1, GPIO_DIR_OUT);
    mt_set_gpio_dir(GPIO_EXT_USB_SW2, GPIO_DIR_OUT);
    mt_set_gpio_out(GPIO_EXT_USB_SW1, GPIO_OUT_ONE);
    mt_set_gpio_out(GPIO_EXT_USB_SW2, GPIO_OUT_ZERO);

    /* Press MT6261 DL key to enter download mode
     * GPIO_KPD_KROW1_PIN(GPIO_KCOL0)=0 //GPIO120, GPIO105
     * phone
    #define GPIO_KPD_KROW1_PIN         (GPIO120 | 0x80000000)
     * evb
    #define GPIO_EXT_MD_DL_KEY         (GPIO105 | 0x80000000)
     *
     */

    mt_set_gpio_mode(GPIO_EXT_MD_DL_KEY, GPIO_EXT_MD_DL_KEY_M_GPIO);
    mt_set_gpio_dir(GPIO_EXT_MD_DL_KEY, GPIO_DIR_OUT);
    mt_set_gpio_out(GPIO_EXT_MD_DL_KEY, GPIO_OUT_ZERO);

    /* Bring-up MT6261:
     * GPIO_OTG_DRVVBUS_PIN(GPIO_USB_DRVVBUS)=0,
     * GPIO_52_RST(GPIO_RESETB)=INPUT/NOPULL, GPIO118, GPIO166
     #define GPIO_UART_UTXD3_PIN         (GPIO118 | 0x80000000)
     #define GPIO_EXT_MD_RST         (GPIO166 | 0x80000000)
     * GPIO_RST_KEY(GPIO_PWRKEY)=0->1->0
     #define GPIO_UART_URXD3_PIN         (GPIO117 | 0x80000000)
     * GPIO_PWR_KEY(GPIO_PWRKEY)=1
     #define GPIO_EXT_MD_PWR_KEY         (GPIO167 | 0x80000000)
     */
//    mt_set_gpio_mode(GPIO_OTG_DRVVBUS_PIN, GPIO_OTG_DRVVBUS_PIN_M_GPIO);
    mt_set_gpio_mode(GPIO_EXT_MD_RST, GPIO_EXT_MD_RST_M_GPIO);

    /* MD DRVVBUS to low */
//    mt_set_gpio_dir(GPIO_OTG_DRVVBUS_PIN, GPIO_DIR_OUT);
//    mt_set_gpio_out(GPIO_OTG_DRVVBUS_PIN, GPIO_OUT_ZERO);
    mt_set_gpio_mode(GPIO_EXT_MD_PWR_KEY, GPIO_EXT_MD_PWR_KEY_M_GPIO);
    mt_set_gpio_dir(GPIO_EXT_MD_PWR_KEY, GPIO_DIR_OUT);
    mt_set_gpio_out(GPIO_EXT_MD_PWR_KEY, GPIO_OUT_ONE); /* default @ reset state */

    /* MD reset pin: hold to zero */
    mt_set_gpio_pull_enable(GPIO_EXT_MD_RST, GPIO_PULL_DISABLE);
    mt_set_gpio_dir(GPIO_EXT_MD_RST, GPIO_DIR_OUT);
    mt_set_gpio_out(GPIO_EXT_MD_RST, GPIO_OUT_ZERO); /* default @ reset state */
    mdelay(200);

    /* MD reset pin: released */
    mt_set_gpio_out(GPIO_EXT_MD_RST, GPIO_OUT_ONE);
    mdelay(200);
    mt_set_gpio_dir(GPIO_EXT_MD_RST, GPIO_DIR_IN);

    print("[%s] AP modem download done\n", MOD);
	//keep kick WDT to avoid HW WDT reset
    while (1) {
        platform_wdt_all_kick();
		mdelay(1000);
    }
}
#endif

#if CFG_USB_AUTO_DETECT
void platform_usbdl_flag_check()
{
    U32 usbdlreg = 0;
    usbdlreg = DRV_Reg32(SRAMROM_USBDL);
     /*Set global variable to record the usbdl flag*/
    if(usbdlreg & USBDL_BIT_EN)
        g_usbdl_flag = 1;
    else
        g_usbdl_flag = 0;
}

void platform_usb_auto_detect_flow()
{

    print("USB DL Flag is %d when enter preloader  \n",g_usbdl_flag);

    /*usb download flag haven't set */
	if(g_usbdl_flag == 0){
        /*set up usbdl flag*/
        platform_safe_mode(1,CFG_USB_AUTO_DETECT_TIMEOUT_MS);
        print("Preloader going reset and trigger BROM usb auto detectiton!!\n");

        /*WDT by pass powerkey reboot*/
        mtk_arch_reset(1);

	}else{
    /*usb download flag have been set*/
    }

}
#endif


void platform_safe_mode(int en, u32 timeout)
{
#if !CFG_FPGA_PLATFORM

    U32 usbdlreg = 0;

    /* if anything is wrong and caused wdt reset, enter bootrom download mode */
    timeout = !timeout ? USBDL_TIMEOUT_MAX : timeout / 1000;
    timeout <<= 2;
    timeout &= USBDL_TIMEOUT_MASK; /* usbdl timeout cannot exceed max value */

    usbdlreg |= timeout;
    if (en)
	    usbdlreg |= USBDL_BIT_EN;
    else
	    usbdlreg &= ~USBDL_BIT_EN;

    usbdlreg &= ~USBDL_BROM ;
    /*Add magic number for MT6582*/
    usbdlreg |= USBDL_MAGIC;

    DRV_WriteReg32(SRAMROM_USBDL, usbdlreg);

    return;
        return;
#endif

}

#if CFG_EMERGENCY_DL_SUPPORT
void platform_emergency_download(u32 timeout)
{
    /* enter download mode */
    print("%s emergency download mode(timeout: %ds).\n", MOD, timeout / 1000);
    platform_safe_mode(1, timeout);

#if CFG_RTC_REG_EMERGENCY_DL_SUPPORT
    if (true == emergency_dl_flag)
        rtc_exit_emergency_dl_mode();
#endif

#if !CFG_FPGA_PLATFORM
    mtk_arch_reset(0); /* don't bypass power key */
#endif

    while(1);
}
#endif



int platform_get_mcp_id(u8 *id, u32 len, u32 *fw_id_len)
{
    int ret = -1;

    memset(id, 0, len);

#if (CFG_BOOT_DEV == BOOTDEV_SDMMC)
    ret = mmc_get_device_id(id, len,fw_id_len);
#else
    ret = nand_get_device_id(id, len);
#endif

    return ret;
}

void platform_set_chrg_cur(int ma)
{
    hw_set_cc(ma);
}

static boot_reason_t platform_boot_status(void)
{
#if !CFG_FPGA_PLATFORM

#if defined (MTK_KERNEL_POWER_OFF_CHARGING)
	ulong begin = get_timer(0);
	do  {
		if (rtc_boot_check()) {
			print("%s RTC boot!\n", MOD);
			return BR_RTC;
		}
		if(!kpoc_flag)
			break;
	} while (get_timer(begin) < 1000 && kpoc_flag);
#else
    if (rtc_boot_check()) {
        print("%s RTC boot!\n", MOD);
        return BR_RTC;
    }

#endif
#endif

    if (mtk_wdt_boot_check() == WDT_NORMAL_REBOOT) {
        print("%s WDT normal boot!\n", MOD);
        return BR_WDT;
    } else if (mtk_wdt_boot_check() == WDT_BY_PASS_PWK_REBOOT){
        print("%s WDT reboot bypass power key!\n", MOD);
        return BR_WDT_BY_PASS_PWK;
    }
#if !CFG_FPGA_PLATFORM
    /* check power key */
    if (mtk_detect_key(PL_PMIC_PWR_KEY)) {
        print("%s Power key boot!\n", MOD);
        rtc_mark_bypass_pwrkey();
        return BR_POWER_KEY;
    }
#endif

#if !CFG_EVB_PLATFORM
    if (usb_accessory_in()) {
        print("%s USB/charger boot!\n", MOD);
        return BR_USB;
    } else {
    	if (rtc_2sec_reboot_check()) {
				print("%s 2sec reboot!\n", MOD);
				return BR_2SEC_REBOOT;
			}
		}

    print("%s Unknown boot!\n", MOD);
    pl_power_off();
    /* should nerver be reached */
#endif

    print("%s Power key boot!\n", MOD);

    return BR_POWER_KEY;
}

#if CFG_LOAD_DSP_ROM || CFG_LOAD_MD_ROM
int platform_is_three_g(void)
{
    u32 tmp = sp_check_platform();

    return (tmp & 0x1) ? 0 : 1;
}
#endif

CHIP_SW_VER mt_get_chip_sw_ver(void)
{
	CHIP_SW_VER sw_ver;
	unsigned int ver;
	//    unsigned int hw_subcode = DRV_Reg32(APHW_SUBCODE);

	ver = DRV_Reg32(APSW_VER);
	if ( 0x0 == ver )
		sw_ver = CHIP_SW_VER_01;
	else
		sw_ver = CHIP_SW_VER_02;

	return sw_ver;
}

u32 mt_get_chip_info(CHIP_INFO id)
{
    if (CHIP_INFO_FUNCTION_CODE == id) 
    {       
        unsigned int val = seclib_get_devinfo_with_index(24) & 0xFF000000; //[31:24]
        return (val >> 24);
    }
    else if (CHIP_INFO_PROJECT_CODE == id)
    {
        unsigned int val = seclib_get_devinfo_with_index(24) & 0x00003FFF; //[13:0]        
        return (val);
    }
    else if (CHIP_INFO_DATE_CODE == id)
    {
        unsigned int val = seclib_get_devinfo_with_index(24) & 0x00FFC000; //[23:14]
        return (val >> 14);
    }    
    else if (CHIP_INFO_FAB_CODE == id)
    {
        unsigned int val = seclib_get_devinfo_with_index(25) & 0x70000000; //[30:28]    
        return (val >> 28);
    }

    print("%s No info in PL\n", MOD);
    return 0x0000ffff;
}

// ------------------------------------------------
// detect download mode
// ------------------------------------------------

bool platform_com_wait_forever_check(void)
{
#ifdef USBDL_DETECT_VIA_KEY
    /* check download key */
    if (TRUE == mtk_detect_key(COM_WAIT_KEY)) {
        print("%s COM handshake timeout force disable: Key\n", MOD);
        return TRUE;
    }
#endif

#ifdef USBDL_DETECT_VIA_AT_COMMAND
    print("platform_com_wait_forever_check\n");
    /* check SRAMROM_USBDL_TO_DIS */
    if (USBDL_TO_DIS == (INREG32(SRAMROM_USBDL_TO_DIS) & USBDL_TO_DIS)) {
	print("%s COM handshake timeout force disable: AT Cmd\n", MOD);
	CLRREG32(SRAMROM_USBDL_TO_DIS, USBDL_TO_DIS);
	return TRUE;
    }
#endif

    return FALSE;
}

/* NOTICE: need to revise if platform supports >4G memory size*/
u64 platform_memory_size(void)
{
    static u64 mem_size = 0;
    int nr_bank;
    int i;
    int rank_size[4], *rksize = &rank_size[0];
    u32 size = 0;

    if (!mem_size) {
        nr_bank = get_dram_rank_nr();

        get_dram_rank_size(rank_size);

        for (i = 0; i < nr_bank; i++)
            size += (u32)*rksize++;
        mem_size = size;
    }

    /* 4GB dram can't be accessed by 32bit preloader */
    if (mem_size + CFG_DRAM_ADDR > 0xffffffff)
    {
        mem_size = 0x100000000 - CFG_DRAM_ADDR;
    }

    return mem_size;
}

void platform_pre_init(void)
{
    u32 i2c_ret, pmic_ret;
    u32 pwrap_ret=0,i=0;


    /* init timer */
    mtk_timer_init();

    /* init boot time */
    g_boot_time = get_timer(0);


#if !CFG_FPGA_PLATFORM
    /* init pll */
    mt_pll_init();
#endif

    /* init uart baudrate when pll on */
    mtk_uart_init(UART_SRC_CLK_FRQ, CFG_LOG_BAUDRATE);

    /*GPIO init*/
    mt_gpio_init();

#if !CFG_FPGA_PLATFORM

    #if (CFG_USB_UART_SWITCH)
	if (is_uart_cable_inserted()) {
		print("\n%s Switch to UART Mode\n", MOD);
		set_to_uart_mode();
	} else {
		print("\n%s Keep stay in USB Mode\n", MOD);
	}
    #endif
#endif
      mt_pll_post_init();//temply enable for DDR channel B 

    //retry 3 times for pmic wrapper init
    pwrap_init_preloader();
	ext_buck_en(1);
	//i2c hw init
	i2c_hw_init();

#if !CFG_FPGA_PLATFORM
    pmic_ret = pmic_init();
//	BOOTING_TIME_PROFILING_LOG("PMIC");
    pmic_vcore_init();

    mt_pll_post_init();

    mt_arm_pll_sel();
//    BOOTING_TIME_PROFILING_LOG("GPIO");
#endif

//enable long press reboot function***************

#ifndef CFG_EVB_PLATFORM
#ifdef KPD_PMIC_LPRST_TD
	//make sure ues old original PMIC_RG_PWRKEY_RST_EN control long press reset
	{
		u32 reg_val = 0;
		pmic_read_interface(EFUSE_DOUT_288_303, &reg_val, 0xFFFF, 0);
		if ((reg_val & 0x02) == 0x02)
			pmic_config_interface(0x50E, 0x01, 0x01, 15);
	}

	#ifdef ONEKEY_REBOOT_NORMAL_MODE_PL
		pmic_config_interface(TOP_RST_MISC, 0x01, PMIC_RG_PWRKEY_RST_EN_MASK, PMIC_RG_PWRKEY_RST_EN_SHIFT);
		pmic_config_interface(TOP_RST_MISC, 0x00, PMIC_RG_HOMEKEY_RST_EN_MASK, PMIC_RG_HOMEKEY_RST_EN_SHIFT);
		pmic_config_interface(GPIO_SMT_CON3,0x01, PMIC_RG_HOMEKEY_PUEN_MASK, PMIC_RG_HOMEKEY_PUEN_SHIFT);
		pmic_config_interface(TOP_RST_MISC, (U32)KPD_PMIC_LPRST_TD, PMIC_RG_PWRKEY_RST_TD_MASK, PMIC_RG_PWRKEY_RST_TD_SHIFT);
	#endif
	#ifdef TWOKEY_REBOOT_NORMAL_MODE_PL
		pmic_config_interface(TOP_RST_MISC, 0x01, PMIC_RG_PWRKEY_RST_EN_MASK, PMIC_RG_PWRKEY_RST_EN_SHIFT);
		pmic_config_interface(TOP_RST_MISC, 0x01, PMIC_RG_HOMEKEY_RST_EN_MASK, PMIC_RG_HOMEKEY_RST_EN_SHIFT);
		pmic_config_interface(GPIO_SMT_CON3,0x01, PMIC_RG_HOMEKEY_PUEN_MASK, PMIC_RG_HOMEKEY_PUEN_SHIFT);
		pmic_config_interface(TOP_RST_MISC, (U32)KPD_PMIC_LPRST_TD, PMIC_RG_PWRKEY_RST_TD_MASK, PMIC_RG_PWRKEY_RST_TD_SHIFT);
	#endif
#else
	{
		u32 reg_val = 0;
		pmic_read_interface(EFUSE_DOUT_288_303, &reg_val, 0xFFFF, 0);
		if ((reg_val & 0x02) == 0x02)
			pmic_config_interface(0x50E, 0x01, 0x01, 15);
	}
	pmic_config_interface(TOP_RST_MISC, 0x00, PMIC_RG_PWRKEY_RST_EN_MASK, PMIC_RG_PWRKEY_RST_EN_SHIFT);
	pmic_config_interface(TOP_RST_MISC, 0x00, PMIC_RG_HOMEKEY_RST_EN_MASK, PMIC_RG_HOMEKEY_RST_EN_SHIFT);
#endif
#else
#if !CFG_FPGA_PLATFORM
	{
		u32 reg_val = 0;
		pmic_read_interface(EFUSE_DOUT_288_303, &reg_val, 0xFFFF, 0);
		if ((reg_val & 0x02) == 0x02)
			pmic_config_interface(0x50E, 0x01, 0x01, 15);
	}
	pmic_config_interface(TOP_RST_MISC, 0x00, PMIC_RG_PWRKEY_RST_EN_MASK, PMIC_RG_PWRKEY_RST_EN_SHIFT);
	pmic_config_interface(TOP_RST_MISC, 0x00, PMIC_RG_HOMEKEY_RST_EN_MASK, PMIC_RG_HOMEKEY_RST_EN_SHIFT);
#endif
#endif
//************************************************
	pmic_config_interface(TOP_RST_MISC, 0x01, PMIC_RG_PWRKEY_RST_EN_MASK, PMIC_RG_PWRKEY_RST_EN_SHIFT);
    print("%s Init I2C: %s(%d)\n", MOD, i2c_ret ? "FAIL" : "OK", i2c_ret);
    print("%s Init PWRAP: %s(%d)\n", MOD, pwrap_ret ? "FAIL" : "OK", pwrap_ret);
    print("%s Init PMIC: %s(%d)\n", MOD, pmic_ret ? "FAIL" : "OK", pmic_ret);


    print("%s chip_ver[%x]\n", MOD, mt_get_chip_sw_ver());

#ifdef MTK_BQ24297_SUPPORT
    check_usb_input_limit();
#endif
}


#ifdef MTK_MT8193_SUPPORT
extern int mt8193_init(void);
#endif


void mt_vcore_pll_ptpod_adj(void)
{
   
   pmic_vcore_init();
   //add PLL init here

   #if (CFG_DISP_PTPOD_SUPPORT || CFG_VDEC_PTPOD_SUPPORT || CFG_VENC_PTPOD_SUPPORT)
   set_freq_od(FREQ_PTPOD, FCLK_MMPLL);
   set_freq_od(FREQ_PTPOD, FCLK_AXI_CK);
   set_freq_od(FREQ_PTPOD, FCLK_CCI400_CK); 
   #endif
   
   #if CFG_DISP_PTPOD_SUPPORT
   set_freq_od(FREQ_PTPOD, FCLK_VENCPLL);
   #endif
   
   #if CFG_VDEC_PTPOD_SUPPORT
   set_freq_od(FREQ_PTPOD, FCLK_VCODECPLL);
   #endif
   
   #if CFG_VENC_PTPOD_SUPPORT
   set_freq_od(FREQ_PTPOD, FCLK_VENCLT_CK);
   #endif
   
}

void platform_init(void)
{
    u32 ret, tmp;
    boot_reason_t reason;

    /* check DDR-reserve mode */
    check_ddr_reserve_status();

    /* init watch dog, will enable AP watch dog */
    mtk_wdt_init();

#if !CFG_FPGA_PLATFORM
    /*init kpd PMIC mode support*/
    set_kpd_pmic_mode();
#endif

#if CFG_MDWDT_DISABLE
    /* no need to disable MD WDT, the code here is for backup reason */

    /* disable MD0 watch dog. */
    DRV_WriteReg32(0x20050000, 0x2200);

    /* disable MD1 watch dog. */
    DRV_WriteReg32(0x30050020, 0x2200);
#endif
//add mtk
    DRV_WriteReg32(0x1021581c,0x000200);    /* set lvds analog iso as soon as earilier */
    DRV_WriteReg32(0x1021681c,0x000200);
 //add


#if !CFG_FPGA_PLATFORM/* FIXME */
    g_boot_reason = reason = platform_boot_status();

    if (reason == BR_RTC || reason == BR_POWER_KEY || reason == BR_USB || reason == BR_WDT || reason == BR_WDT_BY_PASS_PWK || reason == BR_2SEC_REBOOT)
        rtc_bbpu_power_on();
#else

    g_boot_reason = BR_POWER_KEY;

#endif

    enable_PMIC_kpd_clock();

#ifndef DDR_LOG_SAVEIN_EMMC
    /* init memory */
    mt_mem_init();
    /*init dram buffer*/
    init_dram_buffer();
    /* Init pll which releate to Vcore */
   mt_vcore_pll_ptpod_adj();
   /* switch log buffer to dram */
    log_buf_ctrl(1);
#endif

#if 0 /* FIXME */
    /* enable CA9 share bits for USB(30)/NFI(29)/MSDC(28) modules to access ISRAM */
    tmp = DRV_Reg32(0xC1000200);
    tmp |= ((1<<30)|(1<<29)|(1<<28));
    DRV_WriteReg32 (0xC1000200, tmp);
#endif

#ifdef MTK_MT8193_SUPPORT
	mt8193_init();
#endif
	ram_console_init();
	ram_console_reboot_reason_save(g_rgu_status);

#ifdef MTK_TPS61280_SUPPORT
    tps61280_init();
#endif

    /* init device storeage */
    ret = boot_device_init();
    print("%s Init Boot Device: %s(%d)\n", MOD, ret ? "FAIL" : "OK", ret);

/* Save DDR calibration data to emmc
 * need initialize eMMC first
 */
#ifdef DDR_LOG_SAVEIN_EMMC
    /* init memory */
    mt_mem_init();
    /*init dram buffer*/
    init_dram_buffer();
    /* Init pll which releate to Vcore */
    mt_vcore_pll_ptpod_adj();
    /* switch log buffer to dram */
    log_buf_ctrl(1);
	mmc_dma_mode(0);
#endif
    ptp_init();

#if CFG_EMERGENCY_DL_SUPPORT && !CFG_FPGA_PLATFORM
    /* check if to enter emergency download mode */
    /* Move after dram_inital and boot_device_init.
      Use excetution time to remove delay time in mtk_kpd_gpio_set()*/
#ifndef USER_BUILD
    if (mtk_detect_dl_keys()) {
#if CFG_RTC_REG_EMERGENCY_DL_SUPPORT
        if ( true == emergency_dl_flag ) {
#ifdef MTK_BQ24297_SUPPORT
            bq24297_set_watchdog(0x0);
#endif
        }
#endif
        platform_emergency_download(CFG_EMERGENCY_DL_TIMEOUT_MS);
    }
#endif
#endif

#if CFG_RTC_REG_EMERGENCY_DL_SUPPORT
    /* check if to enter emergency download mode by using information in rtc reg
ister */
    if ( true == emergency_dl_flag ) {
#ifdef MTK_BQ24297_SUPPORT
        bq24297_set_watchdog(0);
#endif

#if CFG_RTC_REG_EMERGENCY_DL_USE_PL_DL_MD_SUPPORT
        print("[RTC] use pl dl mode for emergency dl mode\n");
        emerg_dl_pldlmd_usb_enum_to = CFG_EMERGENCY_DL_TIMEOUT_MS;
        emerg_dl_pldlmd_usb_hk_to   = CFG_EMERGENCY_DL_TIMEOUT_MS;
#else
        print("[RTC] use brom dl mode for emergency dl mode (%d ms timeout)\n", CFG_EMERGENCY_DL_TIMEOUT_MS );
        platform_emergency_download(CFG_EMERGENCY_DL_TIMEOUT_MS);
#endif
    }
#endif /* CFG_RTC_REG_EMERGENCY_DL_SUPPORT */

#if CFG_REBOOT_TEST
    mtk_wdt_sw_reset();
    while(1);
#endif
}

void platform_post_init(void)
{
#if CFG_BATTERY_DETECT
    /* normal boot to check battery exists or not */
    if (g_boot_mode == NORMAL_BOOT && !hw_check_battery() && usb_accessory_in()) {
        print("%s Wait for battery inserted...\n", MOD);
        /* disable pmic pre-charging led */
        pl_close_pre_chr_led();
        /* enable force charging mode */
        pl_charging(1);
        do {
            mdelay(300);
            /* check battery exists or not */
            if (hw_check_battery())
                break;
            /* kick all watchdogs */
            platform_wdt_all_kick();
        } while(1);
        /* disable force charging mode */
        pl_charging(0);
    }
#endif


#if CFG_MDJTAG_SWITCH
    unsigned int md_pwr_con;

    /* md0 default power on and clock on */
    /* md1 default power on and clock off */

    /* ungate md1 */
    /* rst_b = 0 */
    md_pwr_con = DRV_Reg32(0x10006280);
    md_pwr_con &= ~0x1;
    DRV_WriteReg32(0x10006280, md_pwr_con);

    /* enable clksq2 for md1 */
    DRV_WriteReg32(0x10209000, 0x00001137);
    udelay(200);
    DRV_WriteReg32(0x10209000, 0x0000113f);

    /* rst_b = 1 */
    md_pwr_con = DRV_Reg32(0x10006280);
    md_pwr_con |= 0x1;
    DRV_WriteReg32(0x10006280, md_pwr_con);

    /* switch to MD legacy JTAG */
    /* this step is not essentially required */
#endif

#if CFG_MDMETA_DETECT
    if (g_boot_mode == META_BOOT || g_boot_mode == ADVMETA_BOOT) {
	/* trigger md0 to enter meta mode */
        DRV_WriteReg32(0x20000010, 0x1);
	/* trigger md1 to enter meta mode */
        DRV_WriteReg32(0x30000010, 0x1);
    } else {
	/* md0 does not enter meta mode */
        DRV_WriteReg32(0x20000010, 0x0);
	/* md1 does not enter meta mode */
        DRV_WriteReg32(0x30000010, 0x0);
    }
#endif

    DRV_WriteReg32(0x14000904,0);
    platform_parse_bootopt();
    platform_set_boot_args();
}

void platform_error_handler(void)
{
    int i = 0;
    /* if log is disabled, re-init log port and enable it */
    if (log_status() == 0) {
        mtk_uart_init(UART_SRC_CLK_FRQ, CFG_LOG_BAUDRATE);
        log_ctrl(1);
    }
    print("PL fatal error...\n");

    #if !CFG_FPGA_PLATFORM
    sec_util_brom_download_recovery_check();
    #endif

#if defined(ONEKEY_REBOOT_NORMAL_MODE_PL) || defined(TWOKEY_REBOOT_NORMAL_MODE_PL)
    /* add delay for Long Preessed Reboot count down
       only pressed power key will have this delay */
    print("PL delay for Long Press Reboot\n");
    for ( i=3; i > 0;i-- ) {
        if (mtk_detect_key(PL_PMIC_PWR_KEY)) {
            platform_wdt_kick();
            mdelay(5000);   //delay 5s/per kick,
        } else {
            break; //Power Key Release,
        }
    }
#endif

    /* enter emergency download mode */
    #if CFG_EMERGENCY_DL_SUPPORT
    platform_emergency_download(CFG_EMERGENCY_DL_TIMEOUT_MS);
    #endif

    while(1);
}

void platform_assert(char *file, int line, char *expr)
{
    print("<ASSERT> %s:line %d %s\n", file, line, expr);
    platform_error_handler();
}

/*=======================================================================*/
/* platform core handler                                                 */	
/*=======================================================================*/
//macro
#define BOOTROM_PWR_CTRL        (INFRACFG_AO_BASE + 0x804)
#define BOOTROM_BOOT_ADDR       (INFRACFG_AO_BASE + 0x800)
#define CA7_BUS_CONFIG          (MCUCFG_BASE + 0x1C)
#define CA7_MISC_CONFIG3        (MCUCFG_BASE + 0x3c)
#define CA15L_CPUCFG            (CA15L_CONFIG_BASE + 0x08)
#define CA15L_MISCDBG           (CA15L_CONFIG_BASE + 0x0C)
#define CA15L_CLKENM_DIV        (CA15L_CONFIG_BASE + 0x48)
#define CA15L_RST_CTL           (CA15L_CONFIG_BASE + 0x44)
#define CA15L_CONFIG_RES        (CA15L_CONFIG_BASE + 0x68)

#define AARCH64_SECONDARY_CPUS

//extern
extern void ext_buck_en(int val);
extern void ext_buck_vproc_vsel(int val);

void platform_core_handler(void)
{
    int i;

    //before bldr_pre_process(), ext buck is default off

    //FIXME: check with Henry -> need to ext_buck_vproc_vsel(1)?! -> Default on for bring up
    //pre-init bigcore vproc
    //ext_buck_vproc_vsel(1);
    //ext_buck_vproc_en(1);

    /* Set CONFIG_RES[31:0]=32h000F_FFFF to disable rguX reset wait for cpuX L1 pdn ack */
    DRV_WriteReg32(CA15L_CONFIG_RES, 0x000FFFFF);
    /* Set CA15L_MISCDBG[4] to set AINACTS */
    DRV_WriteReg32(CA15L_MISCDBG, DRV_Reg32(CA15L_MISCDBG) | 0x00000010);
    /* Set CA15L_CLKENM_DIV[12] = 1 to force delay IP shift */
    DRV_WriteReg32(CA15L_CLKENM_DIV, DRV_Reg32(CA15L_CLKENM_DIV) | 0x00001000);
    /* Set CA15L_RST_CTL[14] = 0 L2RSTDISABLE. Deault value is 0*/
    DRV_WriteReg32(CA15L_RST_CTL, DRV_Reg32(CA15L_RST_CTL) & ~0x00004000);

    /* Set bL core secondary CPUs as AA64 if AARCH64_SECONDARY_CPUS is defined */
    #ifdef AARCH64_SECONDARY_CPUS
    DRV_WriteReg32(CA15L_CPUCFG, DRV_Reg32(CA15L_CPUCFG) | 0xF0000000);
    DRV_WriteReg32(CA7_MISC_CONFIG3, DRV_Reg32(CA7_MISC_CONFIG3) | 0x0000E000);
    #else
    DRV_WriteReg32(CA15L_CPUCFG, DRV_Reg32(CA15L_CPUCFG) & ~0xF0000000);
    DRV_WriteReg32(CA7_MISC_CONFIG3, DRV_Reg32(CA7_MISC_CONFIG3) & ~0x0000E000);
    #endif

    
    #if CFG_LOAD_SLT_AARCH64_KERNEL
        if (0 == aarch64_slt_done())
        {
            print("\n%s in platform_core_handler\n", MOD);
            return ;
        }
    #endif

    for (i = 3; i > 0; --i) /* initial power state */
        spm_mtcmos_ctrl_cpu(i, STA_POWER_DOWN, 0);
}


//functin definition
void platform_core_handler_1st(void)
{
    CHIP_SW_VER ver = mt_get_chip_sw_ver();

    if (ver >= CHIP_SW_VER_02)
    {
        //before bldr_pre_process(), ext buck is default off
        
        //FIXME: check with cory, james -> need to ext_buck_vproc_vsel(1)?!
        //pre-init bigcore vproc
        //ext_buck_vproc_vsel(1);

        /* Set CONFIG_RES[31:0]=32h000F_FFFF to disable rguX reset wait for cpuX L1 pdn ack */
        DRV_WriteReg32(CA15L_CONFIG_RES, 0x000FFFFF);
        /* Set CA15L_CLKENM_DIV bit 12 = 1 to force delay IP shift */
        DRV_WriteReg32(CA15L_CLKENM_DIV, DRV_Reg32(CA15L_CLKENM_DIV) | 0x00001000);

        spm_mtcmos_ctrl_cpusys_init(E2_STAGE_1);
    }
    else if (ver >= CHIP_SW_VER_01)
    {
        //pre-init bigcore vproc
        ext_buck_vproc_vsel(1);

    #ifdef MTK_FORCE_CLUSTER1
        {
            extern U32 romBase;
            U32 bldr_start = (U32) &romBase;
            U32 arch, arch_tmp;
            //debug purpose
            U32 cpu_id, id_data;

            //get cluster id, cpu id
            asm
            (
                "DSB\n"
                "MRC     p15, 0, %0, c0, c0, 5\n"
                "DSB\n"
                "ISB\n"
                "AND     %1, %0, #3\n"
                "AND     %0, %0, #0xf00\n"
                "LSR     %0, %0, #6\n"
                "ORR     %0, %1, %0\n"
                : "=r" (cpu_id), "=r" (id_data)
                :
            );

            //get arch
            asm
            (
                "DSB\n"
                "MRC     p15, 0, %0, c0, c0, 0\n"
                "DSB\n"
                "ISB\n"
                "LDR     %1, =#0xfff0\n"
                "AND     %0, %0, %1\n"
                "LSR     %0, %0, #4\n"
                : "=r" (arch), "=r" (arch_tmp)
                :
            );

            if (arch == 0xC07)
            {
                //on little
                /* init timer */
                mtk_timer_init();

                mtk_uart_init(UART_SRC_CLK_FRQ, CFG_LOG_BAUDRATE);
                print("@L_0x%x_0x%x\n", cpu_id, arch);

                /* Set CONFIG_RES[31:0]=32h000F_FFFF to disable rguX reset wait for cpuX L1 pdn ack */
                DRV_WriteReg32(CA15L_CONFIG_RES, 0x000FFFFF);
                /* set big cluster id to 0 */
                DRV_WriteReg32(CA15L_CPUCFG, DRV_Reg32(CA15L_CPUCFG) & 0xFFF0FFFF); //set bit[19:16] = 0

                spm_mtcmos_ctrl_cpusys_init(E1_BIG_STAGE_1);

                DRV_WriteReg32(BOOTROM_BOOT_ADDR, bldr_start);
                DRV_WriteReg32(BOOTROM_PWR_CTRL, DRV_Reg32(BOOTROM_PWR_CTRL) | 0x80000000); //set bit[31] = 1

                spm_mtcmos_ctrl_cpusys_init(E1_BIG_STAGE_2);

                while(1)
                {
                    asm ( "ISB\n" );
                    asm ( "DSB\n" );
                    asm ( "WFI\n" );
                }
            }
            else
            {
                //on big, arch is 0xC0D
                mtk_uart_init(UART_SRC_CLK_FRQ, CFG_LOG_BAUDRATE);
                print("@B_0x%x_0x%x\n", cpu_id, arch);

                print("CA7_BUS_CONFIG: 0x%x, CA15L_MISCDBG: 0x%x\n", DRV_Reg32(CA7_BUS_CONFIG), DRV_Reg32(CA15L_MISCDBG));
                spm_mtcmos_ctrl_cpusys_init(E1_BIG_STAGE_3);
            }
        }
    #endif  //MTK_FORCE_CLUSTER1
    }
}

void platform_core_handler_2nd(void)
{
    //after bldr_pre_process(), ext buck is on

    CHIP_SW_VER ver = mt_get_chip_sw_ver();

    if (ver >= CHIP_SW_VER_02)
    {
        spm_mtcmos_ctrl_cpusys_init(E2_STAGE_2);

        DRV_WriteReg32(BOOTROM_PWR_CTRL, DRV_Reg32(BOOTROM_PWR_CTRL) | 0x80000000); //set bit[31] = 1

        print("@@@### mt_get_chip_sw_ver: %d ###@@@\n", ver);
    }
    else if (ver >= CHIP_SW_VER_01)
    {
    #ifndef MTK_FORCE_CLUSTER1
        /* Set CONFIG_RES[31:0]=32h000F_FFFF to disable rguX reset wait for cpuX L1 pdn ack */
        DRV_WriteReg32(CA15L_CONFIG_RES, 0x000FFFFF);

        spm_mtcmos_ctrl_cpusys_init(E1_LITTLE_STAGE_1);

        DRV_WriteReg32(BOOTROM_PWR_CTRL, DRV_Reg32(BOOTROM_PWR_CTRL) | 0x80000000); //set bit[31] = 1

        print("@@@### mt_get_chip_sw_ver: %d, little ###@@@\n", ver);
    #else
        print("@@@### mt_get_chip_sw_ver: %d, big ###@@@\n", ver);
    #endif //#ifndef MTK_FORCE_CLUSTER1
    }
}



