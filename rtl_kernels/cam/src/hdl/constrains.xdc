# set_property USE_DSP48 FORCE [get_cells -hierarchical -filter {get_cells -hierarchical -filter {REFNAME =~ *krnl_cam_rtl*}}]

set_property LOC SLR1 [get_cells */cam_kernel1]