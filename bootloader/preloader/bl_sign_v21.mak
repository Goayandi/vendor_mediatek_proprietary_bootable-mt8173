$(D_BIN)/$(PL_IMAGE_NAME).bin: $(D_BIN)/$(PL_IMAGE_NAME)_NO_GFH.bin $(PBP_TOOL) $(CHIP_KEY) $(GFH_PATH)/pl_gfh_config_cert_chain.ini
	@echo "[ Enable Secure Chip Support ]"
	@echo "============================================"
	@echo "PBP_PY_SUPPORT=YES"
	cp -f $< $@
	@echo "PBP_PATH = " $(PBP_TOOL)
	python $(PBP_TOOL) -i $(CHIP_KEY) -k ${PRELOADER_OUT}/key_cert.bin -g $(GFH_PATH)/pl_gfh_config_cert_chain.ini -c ${KEY_PATH}/pl_content.ini -func sign -o $@  $(D_BIN)/$(PL_IMAGE_NAME)_NO_GFH.bin
	@echo "$(PBP_TOOL) pass !!!!"
