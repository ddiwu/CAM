#include <ap_axi_sdata.h>
#include <ap_int.h>
#include <hls_stream.h>
#include <iostream>
#include <iomanip>

#define CUSTOMIZED_BLOCK_NUM 16 
#define CUSTOMIZED_BLOCK_SIZE 128
#define CUSTOMIZED_ROUTING_BITS 512

#define IDLE 0xffffff00
#define UPDATE_ALL 0xffffff01
#define UPDATE_ONE 0xffffff02
#define SEARCH_ONE 0xffffff03
#define SEARCH_MQ 0xffffff04 // Multi-Query
#define GET_ROUTING_TABLE 0xffffff05
#define UPDATE_GROUP 0xffffff06

#define get_state(x) (x.range(511, 480))

#define get_data_update_one(x) (x.range(31,0))
#define get_offset_update_one(x) (x.range(63,32))
#define cat_update_one(data, offset) ((ap_int<512>(offset) << 32) | ap_int<512>(data))
#define get_table_id_update(x) (x.range(95,64))

#define get_data_search_one(x) (x.range(31, 0))
#define get_table_id_search_one(x) (x.range(95, 64))
#define set_size_search_one(x, size) (x.range(479, 448) = size)

extern "C" {
void router(hls::stream<ap_axiu<512, 0, 0, 0>>& input,
                  hls::stream<ap_axiu<512, 0, 0, CUSTOMIZED_BLOCK_NUM>>& output) {
#pragma HLS interface ap_ctrl_none port = return
#pragma HLS interface axis port = input
#pragma HLS interface axis port = output

    bool exit_loop = false;

    ap_axiu<512, 0, 0, 0> in_packet;
    ap_uint<32> state = IDLE; 
    static int routing_table[CUSTOMIZED_BLOCK_NUM];
    const int routing_table_size = CUSTOMIZED_BLOCK_NUM / 16; // 512bit can support 16 data.
    int count_routing_table = 0;
    #pragma HLS ARRAY_PARTITION variable=routing_table complete dim=1

    int index = -1; // current CAM block index
    int data_update_one = -1; // data for update one
    int offset_update_one = -1; // offset for update one
    int table_id = -1; // table id for update one
    int counter_update_all = -1; // counter for update all
    int counter_update_group = -1; // counter for update group

    int data_search_one = -1; // data for search one
    int counter_search_mq = -1; // counter for search multi-query
    ap_axiu<512, 0, 0, CUSTOMIZED_BLOCK_NUM> out_packet;
    ap_uint<CUSTOMIZED_BLOCK_NUM> bit_dest_temp;

    routing: while (true) {
    #pragma HLS pipeline II=1
        // Read input packet
        /*
        std::cout << "State: " << std::hex << state << std::endl;
        std::cout << "Variables: index=" << index 
                    << ", data_update_one=" << data_update_one
                    << ", offset_update_one=" << offset_update_one
                    << ", table_id=" << table_id
                    << ", counter_update_all=" << counter_update_all
                    << ", data_search_one=" << data_search_one
                    << ", counter_search_mq=" << counter_search_mq << std::endl;
        std::cout << "Routing Table: ";
        for (int i = 0; i < CUSTOMIZED_BLOCK_NUM; i++) {
            std::cout << "[" << i << "]: 0x" << routing_table[i] << " ";
        }
        std::cout << std::endl;*/

        if (exit_loop) break;

        switch (state) {
            case IDLE:
                in_packet = input.read();
                if (in_packet.last) {
                    exit_loop = true;
                    break;
                }

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
                    out_packet.data = ((ap_int<512>)UPDATE_ONE) << 480;
                    bit_dest_temp = (ap_uint<CUSTOMIZED_BLOCK_NUM>)(1 << index);
                    out_packet.dest |= bit_dest_temp;
                    output << (out_packet);
                    std::cout << "[OUTPUT] status: " << state << " data: " << std::hex << out_packet.data << " dest: " << out_packet.dest << std::endl;

                } else if (state == UPDATE_GROUP) {
                    // TODO: update group
                    table_id = get_table_id_update(in_packet.data);
                    index = [&]() { 
                        int idx = -1; 
                        find_index: for (int i = 0; i < CUSTOMIZED_BLOCK_NUM; i++) { 
                        #pragma HLS unroll
                            if (routing_table[i] == table_id && idx == -1) idx = i; 
                        } 
                        return idx; 
                    }();
                    counter_update_group = [&]() {
                        int s = 0;
                        for (int i = 0; i < CUSTOMIZED_BLOCK_NUM; i++) {
                        #pragma HLS unroll
                            if (routing_table[i] == table_id) s++;
                        }
                        return s * (CUSTOMIZED_BLOCK_SIZE / 16);
                    }();

                } else if (state == UPDATE_ALL) {
                    index = 0; // update all is always starts from block 0
                    counter_update_all = 0;
                    out_packet.data = ((ap_int<512>)UPDATE_ALL) << 480; // set to top 32 bits
                    out_packet.dest = (1 << (CUSTOMIZED_BLOCK_NUM + 1)) - 1; // set all bits to 1
                    output << (out_packet);
                    state = GET_ROUTING_TABLE;
                    std::cout << "[OUTPUT] status: " << state << " data: " << std::hex << out_packet.data << " dest: " << out_packet.dest << std::endl;

                } else if (state == SEARCH_ONE) {
                    out_packet.data = ((ap_int<512>)SEARCH_ONE) << 480; // set to top 32 bits
                    set_size_search_one(out_packet.data, 1); // size = 1.
                    table_id = get_table_id_search_one(in_packet.data);
                    data_search_one = get_data_search_one(in_packet.data);
                    bit_dest_temp = 0;
                    for (int i = 0; i < CUSTOMIZED_BLOCK_NUM; i++) {
                    #pragma HLS unroll
                        if (routing_table[i] == table_id) {
                            ap_uint<CUSTOMIZED_BLOCK_NUM> bit_mask_t = (ap_uint<CUSTOMIZED_BLOCK_NUM>)(1 << i);
                            bit_dest_temp |= bit_mask_t;
                        }
                    }
                    out_packet.dest = bit_dest_temp;
                    output << (out_packet);
                    std::cout << "[OUTPUT] status: " << state << " data: " << std::hex << out_packet.data << " dest: " << out_packet.dest << std::endl;

                } else if (state == SEARCH_MQ) { // no need size
                    out_packet.data = ((ap_int<512>)SEARCH_MQ) << 480; // set to top 32 bits
                    out_packet.dest = (1 << (CUSTOMIZED_BLOCK_NUM + 1)) - 1; // set all bits to 1
                    output << (out_packet);
                    counter_search_mq = (CUSTOMIZED_BLOCK_NUM - 1) / 16 + 1;
                    std::cout << "[OUTPUT] status: " << state << " data: " << std::hex << out_packet.data << " dest: " << out_packet.dest << std::endl;
                }
                break;
            case UPDATE_ONE:
                offset_update_one = offset_update_one % CUSTOMIZED_BLOCK_SIZE;
                out_packet.data = cat_update_one(data_update_one, offset_update_one);
                bit_dest_temp = (ap_uint<CUSTOMIZED_BLOCK_NUM>)(1 << index);
                out_packet.dest |= bit_dest_temp;
                output << (out_packet);
                std::cout << "[OUTPUT] status: " << state << " data: " << std::hex << out_packet.data << " dest: " << out_packet.dest << std::endl;
                state = IDLE;
                break;
            case UPDATE_GROUP:
                in_packet = input.read();
                counter_update_group--;
                if (counter_update_group % (CUSTOMIZED_BLOCK_SIZE / 16) == 0) {index++;}
                if (counter_update_group == 0) {
                    state = IDLE;
                }
                out_packet.dest = 1 << index;
                out_packet.data = in_packet.data;
                output << (out_packet);
                std::cout << "[OUTPUT] status: " << state << " data: " << std::hex << out_packet.data << " dest: " << out_packet.dest << std::endl;
                break;
            case UPDATE_ALL:
                in_packet = input.read();
                counter_update_all++;
                if (counter_update_all == ((CUSTOMIZED_BLOCK_NUM * CUSTOMIZED_BLOCK_SIZE) / 16)) {
                    state = IDLE;
                }
                out_packet.dest = 1 << index;
                out_packet.data = in_packet.data;
                output << (out_packet); // sequential update all
                std::cout << "[OUTPUT] status: " << state << " data: " << std::hex << out_packet.data << " dest: " << out_packet.dest << std::endl;
                if (counter_update_all % (CUSTOMIZED_BLOCK_SIZE / 16) == 0) {index++;}
                break;
            case SEARCH_ONE:
                out_packet.data = data_search_one;
                bit_dest_temp = 0;
                for (int i = 0; i < CUSTOMIZED_BLOCK_NUM; i++) {
                #pragma HLS unroll
                    if (routing_table[i] == table_id) {
                        ap_uint<CUSTOMIZED_BLOCK_NUM> bit_mask_temp = (ap_uint<CUSTOMIZED_BLOCK_NUM>)(1 << i);
                        bit_dest_temp |= bit_mask_temp;
                    }
                }
                out_packet.dest = bit_dest_temp;
                output << (out_packet);
                std::cout << "[OUTPUT] status: " << state << " data: " << std::hex << out_packet.data << " dest: " << out_packet.dest << std::endl;
                state = IDLE;
                break;
            case SEARCH_MQ:
                in_packet = input.read();
                bit_dest_temp = 0;
                out_packet.data = 0x0;
                for (int i = 0; i < 16; i++) {
                #pragma HLS unroll
                    int index = i + 16*(counter_search_mq - 1);
                    ap_int<32> data = in_packet.data.range(32*(i+1) - 1, 32*i);
                    if ((index < CUSTOMIZED_BLOCK_NUM) && (data != 0xffffffff)) { // need to check the data is not -1
                        out_packet.data.range((i+1)*32 - 1, (i*32)) = data;
                        ap_uint<CUSTOMIZED_BLOCK_NUM> bit_mask = (ap_uint<CUSTOMIZED_BLOCK_NUM>)(1 << routing_table[index]);
                        bit_dest_temp |= bit_mask;
                    }
                }
                out_packet.dest = bit_dest_temp;
                output << (out_packet);
                std::cout << "[OUTPUT] status: " << state << " data: " << std::hex << out_packet.data << " dest: " << out_packet.dest << std::endl;
                counter_search_mq--;
                if (counter_search_mq == 0) {
                    state = IDLE;
                }
                break;
            case GET_ROUTING_TABLE:
                in_packet = input.read();
                count_routing_table++;
                if (count_routing_table == routing_table_size) {
                    count_routing_table = 0;
                    state = UPDATE_ALL;
                }
                for (int i = 0; i < 16; i++) {
                #pragma HLS unroll
                    if ((i + count_routing_table*16) < CUSTOMIZED_BLOCK_NUM) {
                        routing_table[i + count_routing_table*16] = in_packet.data.range(32*i + 31, 32*i);
                    }
                }
                break;
        }
    }
    out_packet.last = 1;
    out_packet.data = 0x0;
    out_packet.dest = (1 << (CUSTOMIZED_BLOCK_NUM + 1)) - 1; // set all bits to 1
    output << (out_packet);
    std::cout << "[OUTPUT] status: " << state << " data: " << std::hex << out_packet.data << " dest: " << out_packet.dest << std::endl;
}
}
