LOCAL_DIR := $(GET_LOCAL_DIR)
ifneq ($(wildcard ../../../device/mediatek/build),)
MTK_ROOT := ../../..
else
MTK_ROOT := ../../../..
endif

ARCH    := arm
ARM_CPU := cortex-a7
CPU     := generic

MMC_SLOT         := 1

# choose one of following value -> 1: disabled/ 2: permissive /3: enforcing
SELINUX_STATUS := 2

# overwrite SELINUX_STATUS value with PRJ_SELINUX_STATUS, if defined. it's by project variable.
ifdef PRJ_SELINUX_STATUS
	SELINUX_STATUS := $(PRJ_SELINUX_STATUS)
endif

ifeq (yes,$(strip $(MTK_BUILD_ROOT)))
SELINUX_STATUS := 2
endif

DEFINES += SELINUX_STATUS=$(SELINUX_STATUS)

DEFINES += PERIPH_BLK_BLSP=1
DEFINES += WITH_CPU_EARLY_INIT=0 WITH_CPU_WARM_BOOT=0 \
	   MMC_SLOT=$(MMC_SLOT)

ifneq ($(CUSTOM_DEVICE_TREE),)
ASMFLAGS += -DCUSTOM_DEVICE_TREE=\"$(CUSTOM_DEVICE_TREE)\"
endif

ifeq ($(MTK_SECURITY_SW_SUPPORT), yes)
	DEFINES += MTK_SECURITY_SW_SUPPORT
endif
DEFINES += PROJECT=\"$(PROJECT)\"
#LCM_WIDTH/LCM_HEIGHT defined in ProductConfig.mk
ifneq ($(LCM_WIDTH),)
      LCM_WIDTH := $(LCM_WIDTH)
      DEFINES += DRAM_LCM_WIDTH=$(LCM_WIDTH)
endif

ifneq ($(LCM_HEIGHT),)
      LCM_HEIGHT := $(LCM_HEIGHT)
      DEFINES += DRAM_LCM_HEIGHT=$(LCM_HEIGHT)
endif

ifeq ($(MTK_SEC_FASTBOOT_UNLOCK_SUPPORT), yes)
	DEFINES += MTK_SEC_FASTBOOT_UNLOCK_SUPPORT
ifeq ($(MTK_SEC_FASTBOOT_UNLOCK_KEY_SUPPORT), yes)
	DEFINES += MTK_SEC_FASTBOOT_UNLOCK_KEY_SUPPORT
endif
endif

ifeq ($(MTK_KERNEL_POWER_OFF_CHARGING),yes)
#Fastboot support off-mode-charge 0/1
#1: charging mode, 0:skip charging mode
DEFINES += MTK_OFF_MODE_CHARGE_SUPPORT
endif

KEDUMP_MINI := yes
ARCH_HAVE_MT_RAMDUMP := no

DEFINES += $(shell echo $(BOOT_LOGO) | tr a-z A-Z)

MTK_EMMC_POWER_ON_WP := no
ifeq ($(MTK_EMMC_SUPPORT),yes)
ifeq ($(MTK_EMMC_POWER_ON_WP),yes)
	DEFINES += MTK_EMMC_POWER_ON_WP
endif
endif

$(info libshowlogo new path ------- $(LOCAL_DIR)/../../lib/libshowlogo)
INCLUDES += -I$(LOCAL_DIR)/include \
            -I$(LOCAL_DIR)/include/platform \
            -I$(LOCAL_DIR)/../../lib/libshowlogo \
            -Icustom/$(FULL_PROJECT)/lk/include/target \
            -Icustom/$(FULL_PROJECT)/lk/lcm/inc \
            -Icustom/$(FULL_PROJECT)/lk/inc \
            -Icustom/$(FULL_PROJECT)/common \
            -Icustom/$(FULL_PROJECT)/kernel/dct/ \
            -I$(BUILDDIR)/include/dfo \
            -I$(LOCAL_DIR)/../../dev/lcm/inc

INCLUDES += -I$(DRVGEN_OUT)/inc
#for ptgen
INCLUDES += -I$(MTK_ROOT)/$(MTK_ROOT_OUT)/PTGEN/lk/inc
INCLUDES += -I$(MTK_ROOT)/$(MTK_ROOT_OUT)/PTGEN/common
#for nandgen

OBJS += \
	$(LOCAL_DIR)/bitops.o \
	$(LOCAL_DIR)/mt_gpio.o \
	$(LOCAL_DIR)/mt_disp_drv.o \
	$(LOCAL_DIR)/mt_gpio_init.o \
	$(LOCAL_DIR)/mt_i2c.o \
	$(LOCAL_DIR)/mtk_auxadc.o \
	$(LOCAL_DIR)/platform.o \
	$(LOCAL_DIR)/uart.o \
	$(LOCAL_DIR)/interrupts.o \
	$(LOCAL_DIR)/timer.o \
	$(LOCAL_DIR)/debug.o \
	$(LOCAL_DIR)/boot_mode.o \
	$(LOCAL_DIR)/load_image.o \
	$(LOCAL_DIR)/atags.o \
	$(LOCAL_DIR)/mt_partition.o \
	$(LOCAL_DIR)/partition_parser.o \
	$(LOCAL_DIR)/addr_trans.o \
	$(LOCAL_DIR)/factory.o \
	$(LOCAL_DIR)/mt_gpt.o\
	$(LOCAL_DIR)/mtk_key.o \
	$(LOCAL_DIR)/recovery.o\
	$(LOCAL_DIR)/meta.o\
	$(LOCAL_DIR)/mt_logo.o\
	$(LOCAL_DIR)/boot_mode_menu.o\
	$(LOCAL_DIR)/env.o\
	$(LOCAL_DIR)/mt_pmic_wrap_init.o\
        $(LOCAL_DIR)/mmc.o\
	$(LOCAL_DIR)/mmc_core.o\
        $(LOCAL_DIR)/msdc.o\
        $(LOCAL_DIR)/msdc_utils.o\
        $(LOCAL_DIR)/upmu_common.o\
	$(LOCAL_DIR)/mt_pmic.o\
	$(LOCAL_DIR)/mt_pmic_chr_type_det.o\
	$(LOCAL_DIR)/mtk_wdt.o\
        $(LOCAL_DIR)/mt_rtc.o\
        $(LOCAL_DIR)/mt_usbphy.o\
        $(LOCAL_DIR)/mt_usb.o\
        $(LOCAL_DIR)/mt_ssusb_qmu.o\
        $(LOCAL_DIR)/mt_mu3d_hal_qmu_drv.o\
        $(LOCAL_DIR)/mt_leds.o\
	$(LOCAL_DIR)/ddp_manager.o\
	$(LOCAL_DIR)/ddp_path.o\
	$(LOCAL_DIR)/ddp_ovl.o\
	$(LOCAL_DIR)/ddp_rdma.o\
	$(LOCAL_DIR)/ddp_misc.o\
	$(LOCAL_DIR)/ddp_split.o\
	$(LOCAL_DIR)/ddp_info.o\
	$(LOCAL_DIR)/ddp_dump.o\
	$(LOCAL_DIR)/ddp_dsi.o\
	$(LOCAL_DIR)/ddp_dpi.o\
	$(LOCAL_DIR)/ddp_ufoe.o\
	$(LOCAL_DIR)/primary_display.o\
	$(LOCAL_DIR)/disp_lcm.o\
	$(LOCAL_DIR)/ddp_pwm.o\
	$(LOCAL_DIR)/pwm.o \
	$(LOCAL_DIR)/boot_a64.o \
	$(LOCAL_DIR)/mt_efuse.o

#OBJS += $(LOCAL_DIR)/../../platform/mt8173/fastboot_oem_commands.o

ifeq ($(MTK_SECURITY_SW_SUPPORT), yes)
OBJS +=	$(LOCAL_DIR)/sec_logo_auth.o
endif

ifeq ($(DEVICE_TREE_SUPPORT),yes)
OBJS += \
	$(LOCAL_DIR)/device_tree.o
endif

# SETTING of USBPHY type
OBJS += $(LOCAL_DIR)/mt_usbphy_d60802.o
#OBJS += $(LOCAL_DIR)/mt_usbphy_e60802.o

ifeq ($(MTK_BQ24297_SUPPORT),yes)
OBJS += $(LOCAL_DIR)/bq24297.o
DEFINES += MTK_BQ24297_SUPPORT
endif

ifeq ($(MTK_BQ24296_SUPPORT),yes)
OBJS += $(LOCAL_DIR)/bq24297.o
DEFINES += MTK_BQ24296_SUPPORT
endif

ifeq ($(MTK_BQ24196_SUPPORT),yes)
OBJS += $(LOCAL_DIR)/bq24196.o
DEFINES += MTK_BQ24196_SUPPORT
endif

ifeq ($(MTK_BQ25890_SUPPORT),yes)
OBJS += $(LOCAL_DIR)/bq25890.o
DEFINES += MTK_BQ25890_SUPPORT
endif

ifeq ($(MTK_BQ25892_SUPPORT),yes)
OBJS += $(LOCAL_DIR)/bq25892.o
DEFINES += MTK_BQ25892_SUPPORT
endif
ifeq ($(MTK_BQ25896_SUPPORT),yes)
OBJS += $(LOCAL_DIR)/bq25890.o
DEFINES += MTK_BQ25896_SUPPORT
endif

ifeq ($(MTK_BQ24261_SUPPORT),yes)
OBJS += $(LOCAL_DIR)/bq24261.o
DEFINES += MTK_BQ24261_SUPPORT
endif

ifeq ($(MTK_RT9466_SUPPORT),yes)
OBJS += $(LOCAL_DIR)/rt9466.o
DEFINES += MTK_RT9466_SUPPORT
endif

OBJS += $(LOCAL_DIR)/mt_battery.o

ifeq ($(MTK_DISABLE_POWER_ON_OFF_VOLTAGE_LIMITATION),yes)
DEFINES += MTK_DISABLE_POWER_ON_OFF_VOLTAGE_LIMITATION
endif

ifeq ($(MTK_JEITA_STANDARD_SUPPORT), yes)
DEFINES += MTK_JEITA_STANDARD_SUPPORT
endif

ifeq ($(MTK_AUTO_POWER_ON_WITH_CHARGER), yes)
DEFINES += MTK_AUTO_POWER_ON_WITH_CHARGER
endif

ifneq ($(MTK_EMMC_SUPPORT),yes)
#	OBJS +=$(LOCAL_DIR)/mtk_nand.o
#	OBJS +=$(LOCAL_DIR)/bmt.o
endif


ifeq ($(MTK_MT8193_SUPPORT),yes)
#OBJS +=$(LOCAL_DIR)/mt8193_init.o
#OBJS +=$(LOCAL_DIR)/mt8193_ckgen.o
#OBJS +=$(LOCAL_DIR)/mt8193_i2c.o
endif

ifeq ($(MTK_KERNEL_POWER_OFF_CHARGING),yes)
OBJS +=$(LOCAL_DIR)/mt_kernel_power_off_charging.o
endif


ifeq ($(DUMMY_AP),yes)
OBJS +=$(LOCAL_DIR)/dummy_ap.o
#OBJS +=$(LOCAL_DIR)/spm_md_mtcmos.o
endif

ifeq ($(MTK_BOOT_SOUND_SUPPORT),yes)
OBJS += $(LOCAL_DIR)/mt_boot_sound.o
DEFINES += MTK_BOOT_SOUND_SUPPORT
endif

ifeq ($(MTK_ALPS_BOX_SUPPORT),yes)
OBJS += $(LOCAL_DIR)/hdmi_tmds.o \
	$(LOCAL_DIR)/hdmiedid.o \
	$(LOCAL_DIR)/hdmiddc.o \
	$(LOCAL_DIR)/extd_ddp.o
endif

ifeq ($(MTK_ALPS_BOX_SUPPORT), y)
ccflags-y += -DMTK_ALPS_BOX_SUPPORT
endif

ifeq ($(MTK_EMMC_POWER_ON_WP),yes)
    OBJS +=$(LOCAL_DIR)/partition_wp.o
endif

ifeq ($(MTK_EFUSE_WRITER_SUPPORT), yes)
    DEFINES += MTK_EFUSE_WRITER_SUPPORT
endif

ifeq ($(MTK_GOOGLE_TRUSTY_SUPPORT),yes)
DEFINES += MTK_GOOGLE_TRUSTY_SUPPORT
endif

# MTK IN-HOUSE TEE Secure chunk memory with CMA support
ifeq ($(filter-out yes, \
       $(if $(MTK_IN_HOUSE_TEE_SUPPORT),$(MTK_IN_HOUSE_TEE_SUPPORT),no) \
       $(if $(MTK_SEC_VIDEO_PATH_SUPPORT),$(MTK_SEC_VIDEO_PATH_SUPPORT),no) \
       $(if $(MTK_WVDRM_L1_SUPPORT),$(MTK_WVDRM_L1_SUPPORT),no)),)
DEFINES += MTK_SHARED_SECURE_POOL_SUPPORT
DEFINES += MTK_SHARED_SECURE_POOL_SIZE=$(if $(PRJ_SHARED_SECURE_POOL_SIZE),$(PRJ_SHARED_SECURE_POOL_SIZE),0xc000000)
endif

ifeq ($(wildcard $(LOCAL_DIR)/lib/libhw_crypto.a),)
LIBHW_CRYPTO :=
else
LIBHW_CRYPTO := -lhw_crypto
endif

ifeq ($(CUSTOM_SEC_AUTH_SUPPORT), yes)
LIBSEC := -L$(LOCAL_DIR)/lib -lsec
else
LIBSEC := -L$(LOCAL_DIR)/lib -lsec -lauth
endif
LIBSEC_PLAT := -lsplat -ldevinfo

LINKER_SCRIPT += $(BUILDDIR)/system-onesegment.ld

include platform/common/rules.mk
