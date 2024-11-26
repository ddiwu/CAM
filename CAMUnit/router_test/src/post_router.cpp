#include <ap_axi_sdata.h>
#include <ap_int.h>
#include <hls_stream.h>
#include <iostream>
#include <iomanip>

#define CUSTOMIZED_BLOCK_NUM 16 

extern "C" {
void post_router(hls::stream<ap_axiu<512, 0, 0, CUSTOMIZED_BLOCK_NUM>>& in, 
               hls::stream<ap_axiu<512, 0, 0, 0>>& out_1,
               hls::stream<ap_axiu<512, 0, 0, 0>>& out_2,
               hls::stream<ap_axiu<512, 0, 0, 0>>& out_3,
               hls::stream<ap_axiu<512, 0, 0, 0>>& out_4,
               hls::stream<ap_axiu<512, 0, 0, 0>>& out_5,
               hls::stream<ap_axiu<512, 0, 0, 0>>& out_6,
               hls::stream<ap_axiu<512, 0, 0, 0>>& out_7,
               hls::stream<ap_axiu<512, 0, 0, 0>>& out_8,
               hls::stream<ap_axiu<512, 0, 0, 0>>& out_9,
               hls::stream<ap_axiu<512, 0, 0, 0>>& out_10,
               hls::stream<ap_axiu<512, 0, 0, 0>>& out_11,
               hls::stream<ap_axiu<512, 0, 0, 0>>& out_12,
               hls::stream<ap_axiu<512, 0, 0, 0>>& out_13,
               hls::stream<ap_axiu<512, 0, 0, 0>>& out_14,
               hls::stream<ap_axiu<512, 0, 0, 0>>& out_15,
               hls::stream<ap_axiu<512, 0, 0, 0>>& out_16) {
#pragma HLS interface axis port=in
#pragma HLS interface axis port=out_1
#pragma HLS interface axis port=out_2
#pragma HLS interface axis port=out_3
#pragma HLS interface axis port=out_4
#pragma HLS interface axis port=out_5
#pragma HLS interface axis port=out_6
#pragma HLS interface axis port=out_7
#pragma HLS interface axis port=out_8
#pragma HLS interface axis port=out_9
#pragma HLS interface axis port=out_10
#pragma HLS interface axis port=out_11
#pragma HLS interface axis port=out_12
#pragma HLS interface axis port=out_13
#pragma HLS interface axis port=out_14
#pragma HLS interface axis port=out_15
#pragma HLS interface axis port=out_16
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
                                if (i == 0) out_1.write(out_packet);
                if (i == 1) out_2.write(out_packet);
                if (i == 2) out_3.write(out_packet);
                if (i == 3) out_4.write(out_packet);
                if (i == 4) out_5.write(out_packet);
                if (i == 5) out_6.write(out_packet);
                if (i == 6) out_7.write(out_packet);
                if (i == 7) out_8.write(out_packet);
                if (i == 8) out_9.write(out_packet);
                if (i == 9) out_10.write(out_packet);
                if (i == 10) out_11.write(out_packet);
                if (i == 11) out_12.write(out_packet);
                if (i == 12) out_13.write(out_packet);
                if (i == 13) out_14.write(out_packet);
                if (i == 14) out_15.write(out_packet);
                if (i == 15) out_16.write(out_packet);
            }
        }
    }

    // Send end-of-stream packet to all output streams
    out_packet.data = 0;
    out_packet.last = 1;
    out_1.write(out_packet);
    out_2.write(out_packet);
    out_3.write(out_packet);
    out_4.write(out_packet);
    out_5.write(out_packet);
    out_6.write(out_packet);
    out_7.write(out_packet);
    out_8.write(out_packet);
    out_9.write(out_packet);
    out_10.write(out_packet);
    out_11.write(out_packet);
    out_12.write(out_packet);
    out_13.write(out_packet);
    out_14.write(out_packet);
    out_15.write(out_packet);
    out_16.write(out_packet);
}
}
