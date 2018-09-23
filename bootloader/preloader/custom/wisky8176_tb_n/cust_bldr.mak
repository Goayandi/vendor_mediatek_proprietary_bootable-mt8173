###################################################################
# Include Project Feautre  (cust_bldr.h)
###################################################################

#ifeq ("$(MTK_EMMC_SUPPORT)","yes")
ifdef MTK_EMMC_SUPPORT
CFG_BOOT_DEV :=BOOTDEV_SDMMC
else
CFG_BOOT_DEV :=BOOTDEV_NAND
endif

CFG_UART_LOG :=UART1
CFG_UART_META :=UART1

CFG_USB_UART_SWITCH := 0
CFG_DDR_HIGH_VCORE :=1
CFG_DISP_PTPOD_SUPPORT :=1
CFG_VDEC_PTPOD_SUPPORT :=1
ifeq ($(MTK_IN_HOUSE_TEE_SUPPORT),yes)
CFG_TEE_SUPPORT :=1
CFG_MTK_IN_HOUSE_TEE_SUPPORT :=1
CFG_ATF_ROM_MEMADDR :=0x00101000
endif
