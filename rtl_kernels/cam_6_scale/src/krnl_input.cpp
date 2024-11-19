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
// void krnl_input(ap_uint<512>* in, int size, hls::stream<ap_uint<512>>& inStream) {
// // Auto-pipeline is going to apply pipeline to this loop
// mem_rd:
//     for (int i = 0; i < size; i++) {
// // #pragma HLS LOOP_TRIPCOUNT min = c_size max = c_size
//         inStream << in[i];
//     }
// }

//test
void krnl_input(ap_uint<512>* in, int size, hls::stream<ap_uint<512>>& inStream1,
                                            hls::stream<ap_uint<512>>& inStream2,
                                            hls::stream<ap_uint<512>>& inStream3, 
                                            hls::stream<ap_uint<512>>& inStream4, 
                                            hls::stream<ap_uint<512>>& inStream5){ 
                                            // hls::stream<ap_uint<512>>& inStream6, 
                                            // hls::stream<ap_uint<512>>& inStream7){
                                            // hls::stream<ap_uint<512>>& inStream8,
                                            // hls::stream<ap_uint<512>>& inStream9,
                                            // hls::stream<ap_uint<512>>& inStream10,
                                            // hls::stream<ap_uint<512>>& inStream11,
                                            // hls::stream<ap_uint<512>>& inStream12,
                                            // hls::stream<ap_uint<512>>& inStream13,
                                            // hls::stream<ap_uint<512>>& inStream14){
    mem_rd:
        for (int i = 0; i < size; i++)
        {
            inStream1 << in[i];
            inStream2 << in[i];
            inStream3 << in[i];
            inStream4 << in[i];
            inStream5 << in[i];
            // inStream6 << in[i];
            // inStream7 << in[i];
            // inStream8 << in[i];
            // inStream9 << in[i];
            // inStream10 << in[i];
            // inStream11 << in[i];
            // inStream12 << in[i];
            // inStream13 << in[i];
            // inStream14 << in[i];
        }
}
}
