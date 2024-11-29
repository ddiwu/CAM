#include <ap_axi_sdata.h>
#include <ap_int.h>
#include <hls_stream.h>

#define CUSTOMIZED_BLOCK_NUM 16 
#define CUSTOMIZED_BLOCK_SIZE 128

extern "C" {
void mem_read(ap_int<512>* mem, int size, hls::stream<ap_axiu<512, 0, 0, 0> >& stream) {
    ap_axiu<512, 0, 0, 0> v;
    ap_int<512> mem_data;
    mem_read_loop: for (int i = 0; i < size; i++) {
    #pragma HLS pipeline II=1
        mem_data = mem[i];
        v.data = mem_data;
        v.last = 0;
        stream.write(v);
    }
    v.data = 0x0;
    v.last = 1;
    stream.write(v);
}
}
