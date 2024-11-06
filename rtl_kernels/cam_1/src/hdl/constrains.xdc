# set_property USE_DSP48 FORCE [get_cells -hierarchical -filter {get_cells -hierarchical -filter {REFNAME =~ *krnl_cam_rtl*}}]

# create_clock -period 3.333 -name aclk [get_ports aclk]