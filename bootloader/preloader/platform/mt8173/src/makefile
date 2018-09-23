ifeq ($(CREATE_SEC_LIB),TRUE)
    ifeq ($(CREATE_HW_CRYPTO_LIB),TRUE)
        SUBDIRS += hw_crypto_lib
    else
SUBDIRS += secure_lib
    endif
endif
SUBDIRS += security core drivers init

include $(COMMON_DIR_MK)

