VIVADO := $(XILINX_VIVADO)/bin/vivado
$(TEMP_DIR)/rtl_cam.xo: scripts/package_kernel.tcl scripts/gen_rtl_cam_xo.tcl src/hdl/*.sv src/hdl/*.v src/hdl/*.xdc
	mkdir -p $(TEMP_DIR)
	$(VIVADO) -mode batch -source scripts/gen_rtl_cam_xo.tcl -tclargs $(TEMP_DIR)/rtl_cam.xo cam $(TARGET) $(PLATFORM) $(XSA)
$(TEMP_DIR)/krnl_cam.xo: ./src/krnl_cam.cpp
	mkdir -p $(TEMP_DIR)
	v++ $(VPP_FLAGS) -t $(TARGET) --platform $(PLATFORM) -c -k krnl_cam -I'$(<D)' -o'$@' '$<' 
