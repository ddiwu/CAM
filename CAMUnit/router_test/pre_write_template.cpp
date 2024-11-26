#include <ap_axi_sdata.h>
#include <ap_int.h>
#include <hls_stream.h>
#include <iostream>
#include <iomanip>

#define CUSTOMIZED_BLOCK_NUM 16 

extern "C" {
void pre_write(hls::stream<ap_axiu<512, 0, 0, CUSTOMIZED_BLOCK_NUM>>& in, 
               {{OUTPUT_STREAM_DECLARATIONS}}) {
#pragma HLS interface axis port=in
{{PRAGMA_DECLARATIONS}}
#pragma HLS interface ap_ctrl_none port=return

    ap_axiu<512, 0, 0, 0> out_packet;
    while (true) {
#pragma HLS pipeline II=1
        ap_axiu<512, 0, 0, CUSTOMIZED_BLOCK_NUM> v = in.read();
        out_packet.data = v.data;

        if (v.last) {
            break;
        }

        ap_int<CUSTOMIZED_BLOCK_NUM> dest = v.dest;

        // Check each bit of v.dest using range(i, i) and write to corresponding output stream
        for (int i = 0; i < CUSTOMIZED_BLOCK_NUM; i++) {
#pragma HLS unroll
            if (dest.range(i, i)) {
                {{OUTPUT_WRITE_LOGIC}}
            }
        }
    }

    // Send end-of-stream packet to all output streams
    out_packet.data = 0;
    out_packet.last = 1;
    {{END_STREAM_LOGIC}}
}
}
