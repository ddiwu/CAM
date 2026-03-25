#include "tc_header.h"

#define DEBUG_POST_ROUTER_ENABLE false
#define DEBUG_POST_ROUTER(i, out_packet) \
    if (DEBUG_POST_ROUTER_ENABLE) { \
        std::cout << std::hex << "[INFO] POST_ROUTER, block " << i \
                  << " keys: " << out_packet.range(31, 0) << std::endl; \
    }

extern "C" {
void post_router(hls::stream<ap_axiu<STREAM_LENGTH, 0, 0, CUSTOMIZED_BLOCK_NUM>>& in, 
               hls::stream<ap_uint<STREAM_LENGTH>>& out_1,
               hls::stream<ap_uint<STREAM_LENGTH>>& out_2,
               hls::stream<ap_uint<STREAM_LENGTH>>& out_3,
               hls::stream<ap_uint<STREAM_LENGTH>>& out_4,
               hls::stream<ap_uint<STREAM_LENGTH>>& out_5,
               hls::stream<ap_uint<STREAM_LENGTH>>& out_6,
               hls::stream<ap_uint<STREAM_LENGTH>>& out_7,
               hls::stream<ap_uint<STREAM_LENGTH>>& out_8,
               hls::stream<ap_uint<STREAM_LENGTH>>& out_9,
               hls::stream<ap_uint<STREAM_LENGTH>>& out_10,
               hls::stream<ap_uint<STREAM_LENGTH>>& out_11,
               hls::stream<ap_uint<STREAM_LENGTH>>& out_12,
               hls::stream<ap_uint<STREAM_LENGTH>>& out_13,
               hls::stream<ap_uint<STREAM_LENGTH>>& out_14,
               hls::stream<ap_uint<STREAM_LENGTH>>& out_15,
               hls::stream<ap_uint<STREAM_LENGTH>>& out_16) {
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

    ap_uint<STREAM_LENGTH> out_packet;

    ap_axiu<STREAM_LENGTH, 0, 0, CUSTOMIZED_BLOCK_NUM> v;
    ap_uint<CUSTOMIZED_BLOCK_NUM> dest;

    post_router_loop: while (true) {
    #pragma HLS pipeline II=1
        v = in.read();
        if (v.last) break;

        if (decode_instruction(v.data) == SEARCH_MQ) {
            // Write directly to each stream without intermediate array.
            // base_packet has instruction in upper bits, zeros elsewhere.
            ap_uint<STREAM_LENGTH> base_packet = 0;
            base_packet.range(519, 512) = v.data.range(519, 512);
            ap_uint<STREAM_LENGTH> p;

            p = base_packet; p.range(31, 0) = v.data.range(31, 0);     out_1.write(p);
            p = base_packet; p.range(31, 0) = v.data.range(63, 32);    out_2.write(p);
            p = base_packet; p.range(31, 0) = v.data.range(95, 64);    out_3.write(p);
            p = base_packet; p.range(31, 0) = v.data.range(127, 96);   out_4.write(p);
            p = base_packet; p.range(31, 0) = v.data.range(159, 128);  out_5.write(p);
            p = base_packet; p.range(31, 0) = v.data.range(191, 160);  out_6.write(p);
            p = base_packet; p.range(31, 0) = v.data.range(223, 192);  out_7.write(p);
            p = base_packet; p.range(31, 0) = v.data.range(255, 224);  out_8.write(p);
            p = base_packet; p.range(31, 0) = v.data.range(287, 256);  out_9.write(p);
            p = base_packet; p.range(31, 0) = v.data.range(319, 288);  out_10.write(p);
            p = base_packet; p.range(31, 0) = v.data.range(351, 320);  out_11.write(p);
            p = base_packet; p.range(31, 0) = v.data.range(383, 352);  out_12.write(p);
            p = base_packet; p.range(31, 0) = v.data.range(415, 384);  out_13.write(p);
            p = base_packet; p.range(31, 0) = v.data.range(447, 416);  out_14.write(p);
            p = base_packet; p.range(31, 0) = v.data.range(479, 448);  out_15.write(p);
            p = base_packet; p.range(31, 0) = v.data.range(511, 480);  out_16.write(p);
        } else {
            out_packet = v.data;
            dest = v.dest;
            for (int i = 0; i < CUSTOMIZED_BLOCK_NUM; i++) {
            #pragma HLS unroll
                if (dest.range(i, i)) {
                    if (i == 0) { out_1.write(out_packet); DEBUG_POST_ROUTER(i, out_packet); }  
                    if (i == 1) { out_2.write(out_packet); DEBUG_POST_ROUTER(i, out_packet); }
                    if (i == 2) { out_3.write(out_packet); DEBUG_POST_ROUTER(i, out_packet); }
                    if (i == 3) { out_4.write(out_packet); DEBUG_POST_ROUTER(i, out_packet); }
                    if (i == 4) { out_5.write(out_packet); DEBUG_POST_ROUTER(i, out_packet); }
                    if (i == 5) { out_6.write(out_packet); DEBUG_POST_ROUTER(i, out_packet); }
                    if (i == 6) { out_7.write(out_packet); DEBUG_POST_ROUTER(i, out_packet); }
                    if (i == 7) { out_8.write(out_packet); DEBUG_POST_ROUTER(i, out_packet); }
                    if (i == 8) { out_9.write(out_packet); DEBUG_POST_ROUTER(i, out_packet); }
                    if (i == 9) { out_10.write(out_packet); DEBUG_POST_ROUTER(i, out_packet); }
                    if (i == 10) { out_11.write(out_packet); DEBUG_POST_ROUTER(i, out_packet); }
                    if (i == 11) { out_12.write(out_packet); DEBUG_POST_ROUTER(i, out_packet); }
                    if (i == 12) { out_13.write(out_packet); DEBUG_POST_ROUTER(i, out_packet); }
                    if (i == 13) { out_14.write(out_packet); DEBUG_POST_ROUTER(i, out_packet); }
                    if (i == 14) { out_15.write(out_packet); DEBUG_POST_ROUTER(i, out_packet); }
                    if (i == 15) { out_16.write(out_packet); DEBUG_POST_ROUTER(i, out_packet); }
                }
            }
        }
    }

    // Send end-of-stream packet to all output streams
    out_packet.range(519, 519) = 0;
    out_packet.range(518, 515) = END_OF_STREAM;
    out_packet.range(514, 0) = 0;
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
