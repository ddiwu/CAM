#include <ap_axi_sdata.h>
#include <ap_int.h>
#include <hls_stream.h>
#include <iostream>
#include <iomanip>

extern "C" {
void mem_write(ap_int<512>* mem, hls::stream<ap_axiu<512, 0, 0, 0> >& stream) {
    int index = 0;
    mem_write_loop: while (true) {
    #pragma HLS pipeline II=1
        ap_axiu<512, 0, 0, 0> v = stream.read();
        if (v.last) break;
        mem[index] = v.data;
        index++;
    }
}
}
