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
// void krnl_output(ap_uint<512>* out, int size, hls::stream<ap_uint<512>>& outStream) {
// // Auto-pipeline is going to apply pipeline to this loop
// mem_wr:
//     for (int i = 0; i < size; i++) {
// // #pragma HLS LOOP_TRIPCOUNT min = c_size max = c_size
//         out[i] = outStream.read();
//     }
// }

//test
void krnl_output(ap_uint<512>* out, int size, hls::stream<ap_uint<512>>& outStream1) {
    ap_uint<512> result[11];
    mem_wr:
        for (int i = 0; i < size; i++)
        {
            result[0] = outStream1.read();
            bool valid = result[0] != 511;
            if (valid) {
                out[i] = 100;
            }
        }
}
}
