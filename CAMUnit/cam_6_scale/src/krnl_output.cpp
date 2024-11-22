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
void krnl_output(ap_uint<512>* out, int size, hls::stream<ap_uint<512>>& outStream1,
                                            hls::stream<ap_uint<512>>& outStream2,
                                            hls::stream<ap_uint<512>>& outStream3, 
                                            hls::stream<ap_uint<512>>& outStream4,
                                            hls::stream<ap_uint<512>>& outStream5){
                                            // hls::stream<ap_uint<512>>& outStream6, 
                                            // hls::stream<ap_uint<512>>& outStream7){
                                            // hls::stream<ap_uint<512>>& outStream8,
                                            // hls::stream<ap_uint<512>>& outStream9,
                                            // hls::stream<ap_uint<512>>& outStream10,
                                            // hls::stream<ap_uint<512>>& outStream11,
                                            // hls::stream<ap_uint<512>>& outStream12,
                                            // hls::stream<ap_uint<512>>& outStream13,
                                            // hls::stream<ap_uint<512>>& outStream14){
    ap_uint<512> result[14];
    mem_wr:
        for (int i = 0; i < size; i++)
        {
            result[0] = outStream1.read();
            result[1] = outStream2.read();
            result[2] = outStream3.read();
            result[3] = outStream4.read();
            result[4] = outStream5.read();
            // result[5] = outStream6.read();
            // result[6] = outStream7.read();
            // result[7] = outStream8.read();
            // result[8] = outStream9.read();
            // result[9] = outStream10.read();
            // result[10] = outStream11.read();
            // result[11] = outStream12.read();
            // result[12] = outStream13.read();
            // result[13] = outStream14.read();
            bool valid = (result[0] != 511) || (result[1] != 511) ||
                            (result[2] != 511) || (result[3] != 511) || (result[4] != 511);// || (result[5] != 511) || 
                            // (result[6] != 511);// || (result[7] != 511) || (result[8] != 511) || (result[9] != 511) || (result[10] != 511) ||
                            // (result[11] != 511) || (result[12] != 511) || (result[13] != 511);
            // bool valid = result[0] != 512;
            if (valid) {
                out[i] = 100;
            }
        }
}
}
