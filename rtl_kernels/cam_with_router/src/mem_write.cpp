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
void mem_write(ap_int<512>* mem, int size,
                                    hls::stream<ap_uint<512>>& input1,
                                    hls::stream<ap_uint<512>>& input2,
                                    hls::stream<ap_uint<512>>& input3,
                                    hls::stream<ap_uint<512>>& input4,
                                    hls::stream<ap_uint<512>>& input5,
                                    hls::stream<ap_uint<512>>& input6,
                                    hls::stream<ap_uint<512>>& input7,
                                    hls::stream<ap_uint<512>>& input8,
                                    hls::stream<ap_uint<512>>& input9,
                                    hls::stream<ap_uint<512>>& input10,
                                    hls::stream<ap_uint<512>>& input11,
                                    hls::stream<ap_uint<512>>& input12,
                                    hls::stream<ap_uint<512>>& input13,
                                    hls::stream<ap_uint<512>>& input14,
                                    hls::stream<ap_uint<512>>& input15,
                                    hls::stream<ap_uint<512>>& input16) {
                                    // hls::stream<ap_axiu<512, 0, 0, 0> >& stream) {
    for (int i = 0; i < size; i++) {
        ap_uint<512> v1 = input1.read();
        ap_uint<512> v2 = input2.read();
        ap_uint<512> v3 = input3.read();
        ap_uint<512> v4 = input4.read();
        ap_uint<512> v5 = input5.read();
        ap_uint<512> v6 = input6.read();
        ap_uint<512> v7 = input7.read();
        ap_uint<512> v8 = input8.read();
        ap_uint<512> v9 = input9.read();
        ap_uint<512> v10 = input10.read();
        ap_uint<512> v11 = input11.read();
        ap_uint<512> v12 = input12.read();
        ap_uint<512> v13 = input13.read();
        ap_uint<512> v14 = input14.read();
        ap_uint<512> v15 = input15.read();
        ap_uint<512> v16 = input16.read();

        ap_uint<512> v;
        v = v1 || v2 || v3 || v4 || v5 || v6 || v7 || v8 || v9 || v10 || v11 || v12 || v13 || v14 || v15 || v16;
        mem[i] = v;
    }
}
}
