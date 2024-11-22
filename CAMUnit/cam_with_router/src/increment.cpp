/**
* Copyright (C) 2019-2021 Xilinx, Inc
*
* Licensed under the Apache License, Version 2.0 (the "License"). You may
* not use this file except in compliance with the License. A copy of the
* License is located at
*
*     http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
* WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
* License for the specific language governing permissions and limitations
* under the License.
*/
#include <ap_axi_sdata.h>
#include <ap_int.h>
#include <hls_stream.h>

extern "C" {
void increment(hls::stream<ap_axiu<512, 0, 0, 0>>& input,
                //   hls::stream<ap_axiu<512, 0, 0, 0>>& output,
                hls::stream<ap_uint<512>>& output1,
                hls::stream<ap_uint<512>>& output2,
                hls::stream<ap_uint<512>>& output3,
                hls::stream<ap_uint<512>>& output4,
                hls::stream<ap_uint<512>>& output5,
                hls::stream<ap_uint<512>>& output6,
                hls::stream<ap_uint<512>>& output7,
                hls::stream<ap_uint<512>>& output8,
                hls::stream<ap_uint<512>>& output9,
                hls::stream<ap_uint<512>>& output10,
                hls::stream<ap_uint<512>>& output11,
                hls::stream<ap_uint<512>>& output12,
                hls::stream<ap_uint<512>>& output13,
                hls::stream<ap_uint<512>>& output14,
                hls::stream<ap_uint<512>>& output15,
                hls::stream<ap_uint<512>>& output16,
                  hls::stream<ap_axiu<512, 0, 0, 16>>& lut_mask) {
#pragma HLS interface ap_ctrl_none port = return
#pragma HLS interface axis port = input
#pragma HLS interface axis port = lut_mask

    routing: while (1) {
    #pragma HLS pipeline II=1
        // Read LUT and MASK from the lut_mask stream
        ap_axiu<512, 0, 0, 16> lut_mask_packet = lut_mask.read();
        ap_uint<512> LUT_data = lut_mask_packet.data;
        ap_uint<16> MASK_data = lut_mask_packet.user;

        // Read input packet
        ap_axiu<512, 0, 0, 0> in_packet = input.read();
        ap_uint<512> in_data = in_packet.data;

        // Prepare output packet
        ap_axiu<512, 0, 0, 0> out_packet;
        ap_uint<512> out_data;

        // Perform routing directly
        for (int i = 0; i < 16; i++) {
#pragma HLS unroll
            ap_uint<32> idx = LUT_data.range((i + 1) * 32 - 1, i * 32); // Extract LUT index
            bool mask_bit = MASK_data[i]; // Extract MASK bit
            ap_uint<32> routed_data = (mask_bit == 0)
                                          ? in_data.range((idx + 1) * 32 - 1, idx * 32)
                                          : 0;
            out_data.range((i + 1) * 32 - 1, i * 32) = routed_data;
        }

        // Pack and write the output
        out_packet.data = out_data;
        out_packet.last = in_packet.last;
        // output.write(out_packet);
        output1.write(out_data);
        output2.write(out_data);
        output3.write(out_data);
        output4.write(out_data);
        output5.write(out_data);
        output6.write(out_data);
        output7.write(out_data);
        output8.write(out_data);
        output9.write(out_data);
        output10.write(out_data);
        output11.write(out_data);
        output12.write(out_data);
        output13.write(out_data);
        output14.write(out_data);
        output15.write(out_data);
        output16.write(out_data);
    

        // Break if the input stream indicates the last packet
        if (in_packet.last) {
            break;
        }
    }
}
}
