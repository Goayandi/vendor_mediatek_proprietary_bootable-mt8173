###################################################################
# flag
###################################################################

MOD_SRC  =
MOD_OBJ  = $(PRELOADER_OUT)/obj

###################################################################
# source files
# - add your file here !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
###################################################################

MOD_SRC  := \
	circbuf.c \
	mmc_common_inter.c \
	mmc_core.c \
	mmc_test.c \
	msdc.c \
	msdc_utils.c \
	i2c.c \
	keypad.c \
	usbd.c \
	usbphy.c \
	usbtty.c \
	emi.c \
	dramc_pi_basic_api.c \
	dramc_pi_calibration_api.c \
	timer.c \
	wdt.c \
	efi.c \
	platform.c \
	pll.c \
	device_apc.c \
	gpio.c \
	auxadc.c \
	uart.c \
 	pmic_wrap_init.c \
	pmic.c \
	pmic_chr_type_det.c \
	rtc.c \
	da9212.c \
	memory.c \
	memory_test.s \
	sync_write.c \
	spm_mtcmos.c \
	mt_ptp.c \
	mt_ptp2.c \
	hotplug.s \
#	pmic_auxadc.c

ifeq ("$(MTK_EMMC_RPMB_SUPPORT)","yes")
	MOD_SRC += mmc_rpmb.c
endif

ifneq ("$(MTK_EMMC_SUPPORT)","yes")
	MOD_SRC  += nand.c
	MOD_SRC  +=	bmt.c
endif

ifeq ("$(PL_MODE)","DUMMY_AP")
	MOD_SRC  += gpio_init.c
endif

ifeq ("$(PL_MODE)","TINY")
	MOD_SRC  += gpio_init.c
endif

ifeq ("$(MTK_BQ24160_SUPPORT)","yes")
	MOD_SRC += bq24160.c	
endif

ifeq ("$(MTK_BQ24297_SUPPORT)","yes")
	MOD_SRC += bq24297.c
endif

ifeq ("$(MTK_BQ25890_SUPPORT)","yes")
        MOD_SRC += bq25890.c
endif

ifeq ("$(MTK_TPS61280_SUPPORT)","yes")
	MOD_SRC += tps61280.c
endif

ifeq ("$(CFG_RAM_CONSOLE)", "1")
	MOD_SRC	+= ram_console.c
endif

###################################################################
# target
###################################################################

include $(COMMON_FILE_MK)
