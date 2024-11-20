#include <ap_int.h>
#include <hls_stream.h>

extern "C" {
    void register_slice(hls::stream<ap_uint<512>>& input, hls::stream<ap_uint<512>>& output){
        ap_uint<512> data;
        data = input.read();
        output.write(data);
    }
}