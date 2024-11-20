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

$(TEMP_DIR)/mem_read.xo: ./src/mem_read.cpp
	mkdir -p $(TEMP_DIR)
	v++ $(VPP_FLAGS) -t $(TARGET) --platform $(PLATFORM) -c -k mem_read -I'$(<D)' -o'$@' '$<'

$(TEMP_DIR)/mem_write.xo: ./src/mem_write.cpp
	mkdir -p $(TEMP_DIR)
	v++ $(VPP_FLAGS) -t $(TARGET) --platform $(PLATFORM) -c -k mem_write -I'$(<D)' -o'$@' '$<'

$(TEMP_DIR)/increment.xo: ./src/increment.cpp
	mkdir -p $(TEMP_DIR)
	v++ $(VPP_FLAGS) -t $(TARGET) --platform $(PLATFORM) -c -k increment -I'$(<D)' -o'$@' '$<'

$(TEMP_DIR)/register_slice.xo: ./src/register_slice.cpp
	mkdir -p $(TEMP_DIR)
	v++ $(VPP_FLAGS) -t $(TARGET) --platform $(PLATFORM) -c -k register_slice -I'$(<D)' -o'$@' '$<'
