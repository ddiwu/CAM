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

//------------------------------------------------------------------------------
//
// kernel:  krnl_cam
//
// Purpose: Demonstrate Vector Add in C based HLS kernel
//

// #define BUFFER_SIZE 256
// #define DATA_SIZE 256

// TRIPCOUNT indentifier
// const unsigned int c_len = DATA_SIZE / BUFFER_SIZE;
// const unsigned int c_size = BUFFER_SIZE;

extern "C" {
void krnl_cam(long int* a, long int* b, long int* c, const int n_elements) {
#pragma HLS INTERFACE m_axi port = a offset = slave bundle = gmem
#pragma HLS INTERFACE m_axi port = b offset = slave bundle = gmem
#pragma HLS INTERFACE m_axi port = c offset = slave bundle = gmem

#pragma HLS INTERFACE s_axilite port = a
#pragma HLS INTERFACE s_axilite port = b
#pragma HLS INTERFACE s_axilite port = c
#pragma HLS INTERFACE s_axilite port = n_elements
#pragma HLS INTERFACE s_axilite port = return

    for (int i = 0; i < n_elements * 8; i++) {
        c[i] = a[i] + b[i];
    }
}
}
