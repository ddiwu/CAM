VIVADO := $(XILINX_VIVADO)/bin/vivado
# mode = appro / precise
MODE ?= precise

TRIGGER_FILE := mode_trigger.txt
.PHONY: update_trigger
update_trigger:
	@echo "MODE is set to $(MODE)" > $(TRIGGER_FILE)

$(TEMP_DIR)/rtl_cam.xo: scripts/package_kernel.tcl scripts/gen_rtl_cam_xo.tcl src/hdl/*.sv src/hdl/*.v src/hdl/*.xdc $(TRIGGER_FILE)
	mkdir -p $(TEMP_DIR)
	$(VIVADO) -mode batch -source scripts/gen_rtl_cam_xo.tcl -tclargs $(TEMP_DIR)/rtl_cam.xo cam $(TARGET) $(PLATFORM) $(XSA) $(MODE)

$(TEMP_DIR)/krnl_output.xo: ./src/krnl_output.cpp
	mkdir -p $(TEMP_DIR)
	v++ $(VPP_FLAGS) -t $(TARGET) --platform $(PLATFORM) -c -k krnl_output -I'$(<D)' -o'$@' '$<' 

$(TEMP_DIR)/krnl_input.xo: ./src/krnl_input.cpp
	mkdir -p $(TEMP_DIR)
	v++ $(VPP_FLAGS) -t $(TARGET) --platform $(PLATFORM) -c -k krnl_input -I'$(<D)' -o'$@' '$<' 

$(TEMP_DIR)/post_router.xo: ./src/post_router.cpp
	mkdir -p $(TEMP_DIR)
	$(VPP) $(VPP_FLAGS) -c -k post_router --temp_dir $(TEMP_DIR)  -I'$(<D)' -o'$@' '$<'

$(TEMP_DIR)/mem_read.xo: ./src/mem_read.cpp
	mkdir -p $(TEMP_DIR)
	$(VPP) $(VPP_FLAGS) -c -k mem_read --temp_dir $(TEMP_DIR)  -I'$(<D)' -o'$@' '$<'

$(TEMP_DIR)/router.xo: ./src/router.cpp
	mkdir -p $(TEMP_DIR)
	$(VPP) $(VPP_FLAGS) -c -k router --temp_dir $(TEMP_DIR)  -I'$(<D)' -o'$@' '$<'

$(TEMP_DIR)/mem_write.xo: ./src/mem_write.cpp
	mkdir -p $(TEMP_DIR)
	$(VPP) $(VPP_FLAGS) -c -k mem_write --temp_dir $(TEMP_DIR)  -I'$(<D)' -o'$@' '$<'