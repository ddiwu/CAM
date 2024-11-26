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

#define CUSTOMIZED_BLOCK_NUM 16 
#define CUSTOMIZED_BLOCK_SIZE 128
#define CUSTOMIZED_ROUTING_BITS 512

#define IDLE 0
#define UPDATE_ALL 1
#define UPDATE_ONE 2
#define SEARCH_ONE 3
#define SEARCH_MQ 4 // Multi-Query

#define get_state(x) (x.range(511, 480))

#define get_data_update_one(x) (x.range(31,0))
#define get_offset_update_one(x) (x.range(63,32))
#define cat_update_one(data, offset) ((ap_int<512>(offset) << 32) | ap_int<512>(data))
#define get_table_id_update(x) (x.range(95,64))
#define get_size_update_all(x) (x.range(31,0))

#define get_data_search_one(x) (x.range(31, 0))
#define get_table_id_search_one(x) (x.range(95, 64))

extern "C" {
void increment(hls::stream<ap_axiu<512, 0, 0, 0>>& input, hls::stream<ap_axiu<CUSTOMIZED_ROUTING_BITS, 0, 0, 0>>& route_table,
                  hls::stream<ap_axiu<512, 0, 0, CUSTOMIZED_BLOCK_NUM>>& output) {
#pragma HLS interface ap_ctrl_none port = return
#pragma HLS interface axis port = input
#pragma HLS interface axis port = route_table
#pragma HLS interface axis port = output

    ap_axiu<512, 0, 0, 0> in_packet;
    int state = 0; 
    static int routing_table[CUSTOMIZED_BLOCK_NUM];
    #pragma HLS ARRAY_PARTITION variable=routing_table complete dim=1

    int index = -1; // current CAM block index
    int data_update_one = -1; // data for update one
    int offset_update_one = -1; // offset for update one
    int table_id = -1; // table id for update one
    int size_update_all = -1; // size for update all
    int counter_update_all = -1; // counter for update all

    int data_search_one = -1; // data for search one
    int counter_search_mq = -1; // counter for search multi-query
    ap_axiu<512, 0, 0, CUSTOMIZED_BLOCK_NUM> out_packet;

    routing: while (1) {
    #pragma HLS pipeline II=1
        // Read input packet
        switch (state) {
            case IDLE:
                in_packet = input.read();
                state = get_state(in_packet.data);
                if (state == IDLE) {
                    continue;
                } else if (state == UPDATE_ONE) {
                    data_update_one = get_data_update_one(in_packet.data);
                    offset_update_one = get_offset_update_one(in_packet.data);
                    table_id = get_table_id_update(in_packet.data);
                    index = [&]() { 
                        int idx = -1; 
                        find_index: for (int i = 0; i < CUSTOMIZED_BLOCK_NUM; i++) { 
                        #pragma HLS unroll
                            if (routing_table[i] == table_id && idx == -1) idx = i; 
                        } 
                        return idx; 
                    }();
                    index += offset_update_one / CUSTOMIZED_BLOCK_SIZE;
                    out_packet.data = UPDATE_ONE;
                    out_packet.dest = 1 << index;
                    output << (out_packet);

                } else if (state == UPDATE_ALL) {
                    index = 0; // update all is always starts from block 0
                    counter_update_all = 0;
                    out_packet.data = UPDATE_ALL;
                    out_packet.dest = (1 << (CUSTOMIZED_BLOCK_NUM + 1)) - 1; // set all bits to 1
                    output << (out_packet);
                    ap_axiu<CUSTOMIZED_ROUTING_BITS, 0, 0, 0> route_table_in = route_table.read(); // update route table
                    for (int i = 0; i < CUSTOMIZED_BLOCK_NUM; i++) {
                    #pragma HLS unroll
                        routing_table[i] = route_table_in.data.range(32*i + 31, 32*i);
                    }

                } else if (state == SEARCH_ONE) {
                    out_packet.data = SEARCH_ONE;
                    table_id = get_table_id_search_one(in_packet.data);
                    data_search_one = get_data_search_one(in_packet.data);
                    for (int i = 0; i < CUSTOMIZED_BLOCK_NUM; i++) {
                    #pragma HLS unroll
                        if (routing_table[i] == table_id) {
                            out_packet.dest |= (1 << i);
                        }
                    }
                    output << (out_packet);

                } else if (state == SEARCH_MQ) { // no need size
                    out_packet.data = SEARCH_MQ;
                    out_packet.dest = (1 << (CUSTOMIZED_BLOCK_NUM + 1)) - 1; // set all bits to 1
                    output << (out_packet);
                    counter_search_mq = (CUSTOMIZED_BLOCK_NUM - 1) / 16 + 1;
                }
                break;
            case UPDATE_ONE:
                offset_update_one = offset_update_one % CUSTOMIZED_BLOCK_SIZE;
                out_packet.data = cat_update_one(data_update_one, offset_update_one);
                out_packet.dest = 1 << index;
                output << (out_packet);
                state = IDLE;
                break;
            case UPDATE_ALL:
                in_packet = input.read();
                counter_update_all++;
                if (counter_update_all == ((CUSTOMIZED_BLOCK_NUM * CUSTOMIZED_BLOCK_SIZE) / 16)) {
                    state = IDLE;
                }
                if (counter_update_all % 16 == 0) index++;
                out_packet.dest = 1 << index;
                out_packet.data = in_packet.data;
                output << (out_packet); // sequential update all
                break;
            case SEARCH_ONE:
                out_packet.data = data_search_one;
                for (int i = 0; i < CUSTOMIZED_BLOCK_NUM; i++) {
                #pragma HLS unroll
                    if (routing_table[i] == table_id) {
                        out_packet.dest |= (1 << i);
                    }
                }
                output << (out_packet);
                state = IDLE;
                break;
            case SEARCH_MQ:
                in_packet = input.read();
                // this is because the input packet is 512-bit and each int data is 32-bit
                for (int i = 0; i < 16; i++) {
                #pragma HLS unroll
                    int index = i + 16*(counter_search_mq - 1);
                    ap_int<32> data = in_packet.data.range(i*32 - 1, (i-1)*32);
                    out_packet.data = 0x0;
                    if ((index < CUSTOMIZED_BLOCK_NUM) && (data != 0xffffffff)) { // need to check the data is not -1
                        out_packet.data.range(i*32 - 1, (i-1)*32) = data;
                        out_packet.dest = 1 << routing_table[index];
                        output << (out_packet);
                    }
                }
                counter_search_mq--;
                if (counter_search_mq == 0) {
                    state = IDLE;
                }
                break;
        }
    }
}
}
