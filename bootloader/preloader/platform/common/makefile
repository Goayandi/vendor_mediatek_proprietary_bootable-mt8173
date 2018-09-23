###################################################################
# flag
###################################################################

include $(MTK_PATH_PLATFORM)/default.mak
include $(MTK_PATH_PLATFORM)/feature.mak
include ${MTK_ROOT_CUSTOM}/${TARGET}/cust_bldr.mak

MOD_SRC  = 
MOD_OBJ  = $(PRELOADER_OUT)/obj

###################################################################
# source files
# - add your file here !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
###################################################################

MOD_SRC  := \
	addr_trans.c \
	div0.c \
	stdlib.c \
	string.c

ifeq ("$(CFG_BOOT_DEV)", "BOOTDEV_UFS")
	MOD_SRC += ufs_aio_core.c
	MOD_SRC += ufs_aio_error.c
	MOD_SRC += ufs_aio_hcd.c
	MOD_SRC += ufs_aio_interface.c
	MOD_SRC += ufs_aio_quirks.c
	MOD_SRC += ufs_aio_utils.c
endif

ifeq ("$(CFG_MBLOCK_LIB)", "1")
	MOD_SRC  += mblock.c
endif
###################################################################
# target
###################################################################

include $(COMMON_FILE_MK)
