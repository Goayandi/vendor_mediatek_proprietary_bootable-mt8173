#s_chip_ini

$(D_BIN)/$(PL_IMAGE_NAME).bin: $(D_BIN)/$(PL_IMAGE_NAME)_NO_GFH.bin $(PBP_TOOL) $(CHIP_CONFIG) $(CHIP_KEY) $(GFH_PATH)/GFH_CONFIG.ini
	@echo "[ Enable Secure Chip Support ]"
	@echo "============================================"
	@echo "INI_GFH_GEN=YES"
	cp -f $< $@
	$(PBP_TOOL) -m $(CHIP_CONFIG) -i $(CHIP_KEY) -g $(GFH_PATH)/GFH_CONFIG.ini $@
	@echo "$(PBP_TOOL) pass !!!!"
