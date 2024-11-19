VIVADO := $(XILINX_VIVADO)/bin/vivado
# mode = appro / precise
MODE ?= precise

TRIGGER_FILE := mode_trigger.txt
.PHONY: update_trigger
update_trigger:
	@echo "MODE is set to $(MODE)" > $(TRIGGER_FILE)

$(TEMP_DIR)/rtl_cam.xo: scripts/package_kernel.tcl scripts/gen_rtl_cam_xo.tcl src/hdl/*.sv src/hdl/*.v $(TRIGGER_FILE)
	mkdir -p $(TEMP_DIR)
	$(VIVADO) -mode batch -source scripts/gen_rtl_cam_xo.tcl -tclargs $(TEMP_DIR)/rtl_cam.xo cam $(TARGET) $(PLATFORM) $(XSA) $(MODE)

$(TEMP_DIR)/krnl_output.xo: ./src/krnl_output.cpp
	mkdir -p $(TEMP_DIR)
	v++ $(VPP_FLAGS) -t $(TARGET) --platform $(PLATFORM) -c -k krnl_output -I'$(<D)' -o'$@' '$<' 

$(TEMP_DIR)/krnl_input.xo: ./src/krnl_input.cpp
	mkdir -p $(TEMP_DIR)
	v++ $(VPP_FLAGS) -t $(TARGET) --platform $(PLATFORM) -c -k krnl_input -I'$(<D)' -o'$@' '$<' 

$(TEMP_DIR)/krnl_input_SLR1.xo: ./src/krnl_input_SLR1.cpp
	mkdir -p $(TEMP_DIR)
	v++ $(VPP_FLAGS) -t $(TARGET) --platform $(PLATFORM) -c -k krnl_input_SLR1 -I'$(<D)' -o'$@' '$<'

$(TEMP_DIR)/krnl_output_SLR1.xo: ./src/krnl_output_SLR1.cpp
	mkdir -p $(TEMP_DIR)
	v++ $(VPP_FLAGS) -t $(TARGET) --platform $(PLATFORM) -c -k krnl_output_SLR1 -I'$(<D)' -o'$@' '$<'