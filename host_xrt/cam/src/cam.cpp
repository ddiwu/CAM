#include <ap_int.h>
#include <iostream>

#define key_num 2
#define val_num 80

class CAM {
public:
    ap_uint<10> dsp_vals[val_num];
    ap_uint<48> local_out[val_num];
    
    CAM() {
#pragma HLS ARRAY_PARTITION variable=dsp_vals complete dim=1
#pragma HLS ARRAY_PARTITION variable=local_out complete dim=1
    }

    // DSP48-based XOR operation
    void dsp48_xor(ap_uint<30> A, ap_uint<18> B, ap_uint<48> C, ap_uint<48> &Y) {
#pragma HLS INLINE
#pragma HLS RESOURCE variable=Y core=DSP48
        Y = (A, B) ^ C;
    }

    // DSP48-based subtraction operation
    void dsp48_sub(ap_uint<30> A, ap_uint<18> B, ap_uint<48> C, ap_uint<48> &Y) {
#pragma HLS INLINE
#pragma HLS bind_op variable=Y op=sub impl=DSP48
        Y = (A, B) - C;
    }

    // Update DSP values from input data
    void update(const ap_uint<512>* val) {
        for (int i = 0; i < val_num/8; i++) { // 100
#pragma HLS PIPELINE II=1
            ap_uint<512> chunk = val[i];
            for (int j = 0; j < 8; j++) {
#pragma HLS UNROLL
                dsp_vals[i * 8 + j] = chunk.range((j + 1) * 64 - 17, j * 64);
            }
        }
    }

    // Compare key with dsp_vals and immediately write back the result
    void compare_and_write_back(const ap_uint<48>* key, ap_uint<512>* out) {
        for (int i = 0; i < key_num; i++) { // 2
#pragma HLS PIPELINE II=1
            ap_uint<10> key_value = key[i];  // Read key from input

            for (int j = 0; j < val_num; j++) {
#pragma HLS UNROLL
                ap_uint<10> diff_result;
#pragma HLS bind_op variable=diff_result op=add impl=DSP48
                diff_result = key_value + dsp_vals[j];
                local_out[j] = diff_result;
            }

            // Writing the result back to output in chunks of 512 bits (8 values per chunk)
            for (int m = 0; m < val_num/8; m++) {
#pragma HLS PIPELINE II=1
                ap_uint<512> out_val;
                for (int n = 0; n < 8; n++) {
#pragma HLS UNROLL
                    out_val.range((n + 1) * 64 - 17, n * 64) = local_out[m * 8 + n];
                }
                out[i * val_num/8 + m] = out_val;
            }
        }
    }
};

extern "C" {
    void cam(const ap_uint<48>* key,  // Read-Only Vector 1
             const ap_uint<512>* val,  // Read-Only Vector 2 8 value
             ap_uint<512>* out        // Output Result
    ) {
#pragma HLS INTERFACE m_axi port=key offset=slave bundle=gmem1
#pragma HLS INTERFACE m_axi port=val offset=slave bundle=gmem
#pragma HLS INTERFACE m_axi port=out offset=slave bundle=gmem
#pragma HLS INTERFACE s_axilite port=return

        CAM cam_1;  // Instantiate CAM object

        cam_1.update(val);       // Update DSP values from memory
        cam_1.compare_and_write_back(key, out);  // Compare and write back the result
    }
}
