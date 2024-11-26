#include <ap_axi_sdata.h>
#include <ap_int.h>
#include <hls_stream.h>
#include <iostream>
#include <iomanip>

#define CUSTOMIZED_BLOCK_NUM 16

extern "C" {
void mem_write(ap_int<512>* mem, hls::stream<ap_axiu<512, 0, 0, 0>>& stream) {
    int i = 0;
    while (true) {
        ap_axiu<512, 0, 0, 0> v = stream.read();
        if (v.last) break;
        mem[i] = v.data;
        i++;
    }
}
}
