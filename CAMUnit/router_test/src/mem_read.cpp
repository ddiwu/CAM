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
#include <iostream>

#define CUSTOMIZED_BLOCK_NUM 16
#define CUSTOMIZED_BLOCK_SIZE 128
#define CUSTOMIZED_ROUTING_BITS 512
#define IDLE 0xffffff00
#define UPDATE_ALL 0xffffff01


extern "C" {
void mem_read(ap_int<512>* mem, int size, hls::stream<ap_axiu<512, 0, 0, 0> >& stream) {
    ap_axiu<512, 0, 0, 0> value;
    read_loop:for (int i = 0; i < size; i++) {
    #pragma HLS pipeline II=1
        value.data = mem[i];
        stream << (value);
    }
    value.last = 1;
    stream << (value);
}
}