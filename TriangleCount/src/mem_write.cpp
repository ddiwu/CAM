#include "tc_header.h"

// In this function, we accumulate the triangle count, output it into mem[0]
extern "C" {
void mem_write(ap_uint<32>* mem, hls::stream<ap_uint<520>>& router_out) {
    ap_uint<32> triangle_count = 0;
    mem_write_loop: while (true) {
    #pragma HLS pipeline II=1
        ap_uint<520> v = router_out.read();
        // DEBUG("[INFO] : valid", get_valid_flag(v.data), " instruction", decode_instruction(v.data), " parallelism", get_parallelism(v.data), " data", get_data(v.data), " last", v.last);
        if (v.range(519, 516) == END_OF_STREAM) break;

        if ((v.range(519, 516) == SEARCH_MQ) && (v.range(0,0) == 1)) {
            triangle_count ++;
        }
    }
    mem[0] = triangle_count;
}
}
