#include "tc_header.h"


extern "C" {
void mem_write(ap_uint<512>* mem, hls::stream<ap_axiu<STREAM_LENGTH, 0, 0, CUSTOMIZED_BLOCK_NUM>>& router_out) {
    int index = 0;
    mem_write_loop: while (true) {
    #pragma HLS pipeline II=1
        ap_axiu<STREAM_LENGTH, 0, 0, CUSTOMIZED_BLOCK_NUM> v = router_out.read();
        // DEBUG("[INFO] : valid", get_valid_flag(v.data), " instruction", decode_instruction(v.data), " parallelism", get_parallelism(v.data), " data", get_data(v.data), " last", v.last);
        if (v.last) break;
        index++;
    }
    mem[0] = 0x0;
}
}
