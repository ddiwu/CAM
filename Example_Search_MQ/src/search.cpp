// Add a constraint: the block number is less than 16.
// This project do not take the command into consideration, the working flow is updating the routing table and searching the MQ.
// The format of ap_axiu<512, 1, 0, CUSTOMIZED_BLOCK_NUM> is that 512 data-width and 1-bit user data and CUSTOMIZED_BLOCK_NUM 
// dest data for directing data packet to different output stream. The 1-bit user data is used to indicate the state of the packet,
// whether it is SEARCH_MQ or IDLE.

#include <ap_axi_sdata.h>
#include <ap_int.h>
#include <hls_stream.h>
#include <iostream>
#include <iomanip>

#define CUSTOMIZED_BLOCK_NUM 16
#define CUSTOMIZED_BLOCK_SIZE 128

#define MAX_BLOCK_NUM 16

#define IDLE 0xffffff00
#define SEARCH_MQ 0xffffff04

extern "C" {
void search(hls::stream<ap_axiu<512, 0, 0, 0>>& input,
            hls::stream<ap_axiu<512, 1, 0, CUSTOMIZED_BLOCK_NUM>>& output) {
#pragma HLS interface ap_ctrl_none port = return
#pragma HLS interface axis port = input
#pragma HLS interface axis port = output

    ap_axiu<512, 0, 0, 0> in_packet;
    ap_axiu<512, 1, 0, CUSTOMIZED_BLOCK_NUM> out_packet;
    static int routing_table[MAX_BLOCK_NUM];
    #pragma HLS ARRAY_PARTITION variable=routing_table complete dim=1

    // Load the routing table, only one cycle, since the MAX_BLOCK_NUM is 16.
    in_packet = input.read();
    for (int i = 0; i < MAX_BLOCK_NUM; i++) {
    #pragma HLS unroll
        if (i < CUSTOMIZED_BLOCK_NUM) {
            routing_table[i] = in_packet.data.range(32 * i + 31, 32 * i);
        } else {
            routing_table[i] = -1; // set useless data to -1
        }
    }

    // Perform SEARCH_MQ
    uint32_t state = IDLE;
    search_loop: while (true) {
    #pragma HLS pipeline II=1
        in_packet = input.read();
        if (in_packet.last) break;

        switch (state) {
            case IDLE:
                out_packet.data = in_packet.data;
                out_packet.dest = (1 << (CUSTOMIZED_BLOCK_NUM + 1)) - 1; // set all bits to 1
                out_packet.user = 0; // not search_mq command
                out_packet.last = 0;
                output << out_packet;
                // std::cout << "[OUTPUT] state: IDLE, data: " << std::hex << out_packet.data 
                //         << ", dest: " << out_packet.dest << std::endl;
                state = SEARCH_MQ;
                break;
            case SEARCH_MQ:
                out_packet.dest = (1 << (CUSTOMIZED_BLOCK_NUM + 1)) - 1; // set all bits to 1
                for (int i = 0; i < CUSTOMIZED_BLOCK_NUM; i++) {
                #pragma HLS unroll
                    int index = routing_table[i];
                    out_packet.data.range(32 * (i + 1) - 1, 32 * i) = in_packet.data.range(32 * (index + 1) - 1, 32 * index);
                }
                out_packet.user = 1; // search_mq keys
                out_packet.last = 0;
                output << out_packet;
                // std::cout << "[OUTPUT] state: SEARCH_MQ, data: " << std::hex << out_packet.data 
                //         << ", dest: " << out_packet.dest << std::endl;
                state = IDLE;
                break;
        }
    }

    out_packet.last = 1;
    out_packet.dest = (1 << (CUSTOMIZED_BLOCK_NUM + 1)) - 1; // set all bits to 1
    out_packet.data = 0x0;
    out_packet.user = 0; // not search_mq command
    output << out_packet;
    // std::cout << "[OUTPUT] state: DONE, data: " << std::hex << out_packet.data 
    //                     << ", dest: " << out_packet.dest << std::endl;
}
}
