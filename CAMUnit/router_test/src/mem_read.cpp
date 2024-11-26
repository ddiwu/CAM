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