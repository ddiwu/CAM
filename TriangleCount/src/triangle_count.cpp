#include "tc_header.h"

void print_stream_data(ap_uint<STREAM_LENGTH> input_data, const std::string& list_name) {
    // Decode the instruction
    ap_uint<4> instruction = decode_instruction(input_data);
    ap_uint<512> data = get_data(input_data);

    // Map the instruction type to a string
    std::string instruction_str;
    switch (instruction) {
        case IDLE:                instruction_str = "IDLE"; break;
        case UPDATE_ALL:          instruction_str = "UPDATE_ALL"; break;
        case UPDATE_GROUP:        instruction_str = "UPDATE_GROUP"; break;
        case UPDATE_ONE:          instruction_str = "UPDATE_ONE"; break;
        case SEARCH_ONE:          instruction_str = "SEARCH_ONE"; break;
        case SEARCH_MQ:           instruction_str = "SEARCH_MQ"; break;
        case SET_ROUTING_TABLE:   instruction_str = "SET_ROUTING_TABLE"; break;
        case RESET_CAM:           instruction_str = "RESET_CAM"; break;
        case UPDATE_DUPLICATE:    instruction_str = "UPDATE_DUPLICATE"; break;
        case END_OF_STREAM:       instruction_str = "END_OF_STREAM"; break;
        default:                  instruction_str = "UNKNOWN"; break;
    }

    // Print the output in a single line
    std::cout << list_name << " | Instruction: " << instruction_str << " | Data: ";
    for (int i = 15; i >= 0; --i) {
        std::cout << "0x" << std::hex << data.range(i * 32 + 31, i * 32).to_uint();
        if (i > 0) std::cout << " "; // Add a space between chunks
    }
    std::cout << std::endl;
}

ap_uint<32> ReturnParallelism(ap_uint<32> length) {
    #pragma HLS INLINE
    return (length <= 128) ? 16 :
           (length <= 256) ? 8 :
           (length <= 512) ? 4 :
           (length <= 1024) ? 2 :
           (length <= 2048) ? 1 : 0; // note that 0 means overflow.
}

ap_uint<3> EncodeParallelism(ap_uint<32> parallelism) {
    #pragma HLS INLINE
    return (parallelism == 1) ? 1 : 
           (parallelism == 2) ? 2 : 
           (parallelism == 4) ? 3 : 
           (parallelism == 8) ? 4 : 
           (parallelism == 16) ? 5 : 0;
}

ap_uint<512> ReturnRoutingTable(ap_uint<32> parallelism) {
    #pragma HLS INLINE
    const int base[16] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15};
    ap_uint<512> routing_table = 0;
    for (ap_uint<32> i = 0; i < 16; ++i) {
        #pragma HLS UNROLL
        switch (parallelism) {
            case 1:  routing_table.range((i << 5) + 31, (i << 5)) = 0; break;
            case 2:  routing_table.range((i << 5) + 31, (i << 5)) = base[i >> 3]; break; // i / 8
            case 4:  routing_table.range((i << 5) + 31, (i << 5)) = base[i >> 2]; break; // i / 4
            case 8:  routing_table.range((i << 5) + 31, (i << 5)) = base[i >> 1]; break; // i / 2
            case 16: routing_table.range((i << 5) + 31, (i << 5)) = base[i]; break; // i
        }
    }
    return routing_table;
}

ap_uint<32> GetMultiQueries (ap_uint<32> parallelism) {
    #pragma HLS INLINE
    return (parallelism == 1) ? 16 : 
           (parallelism == 2) ? 8 : 
           (parallelism == 4) ? 4 : 
           (parallelism == 8) ? 2 : 
           (parallelism == 16) ? 1 : 0;
}

void loadEdgeList(int edge_num, ap_uint<64>* EdgeArr, hls::stream<ap_uint<64>>& eStrmOut, hls::stream<bool>& lelctrl) {
    ap_uint<64> edge_item;
    for (int i = 0; i < edge_num; i++) {
#pragma HLS pipeline ii = 1
        edge_item = EdgeArr[i];
        eStrmOut << edge_item;
        lelctrl << false;
    }
    lelctrl << true;
}

// In this function, we use two RowA and RowB to let II=1.
void loadOffset(hls::stream<ap_uint<64>>& eStrmOut, hls::stream<bool>& lelctrl, ap_uint<64>* RowA, ap_uint<64>* RowB, 
                hls::stream<ap_uint<32>>& OffStrmA, hls::stream<ap_uint<32>>& OffStrmB,
                hls::stream<ap_uint<32>>& LenStrmA, hls::stream<ap_uint<32>>& LenStrmB,
                hls::stream<bool>& loctrl) {
    ap_uint<64> edge_item;
    uint32_t IdA_last = 0xffffffff; // default value
    uint32_t offsetA, lengthA;
    uint32_t offsetB, lengthB;
    ap_uint<32> offset_a_item;
    ap_uint<32> length_a_item;
    ap_uint<32> offset_b_item;
    ap_uint<32> length_b_item;
    load_offset: while (1) {
#pragma HLS pipeline II = 1
        bool lel_flag = lelctrl.read();
        if (lel_flag) {
            break;
        }
        edge_item = eStrmOut.read();

        uint32_t IdA = edge_item.range(31, 0); // source vertex
        uint32_t IdB = edge_item.range(63, 32); // destination vertex
        if (IdA_last != IdA) {
            IdA_last = IdA;
            ap_uint<64> rowA_item = RowA[IdA];
            offsetA = rowA_item.range(31, 0);
            lengthA = rowA_item.range(63, 32) - rowA_item.range(31, 0);
        }
        ap_uint<64> rowB_item = RowB[IdB];
        offsetB = rowB_item.range(31, 0);
        lengthB = rowB_item.range(63, 32) - rowB_item.range(31, 0);
        if ((lengthA != 0) && (lengthB != 0)) {
            offset_a_item = offsetA;
            length_a_item = lengthA;
            OffStrmA << offset_a_item;
            LenStrmA << length_a_item;
            offset_b_item = offsetB;
            length_b_item = lengthB;
            OffStrmB << offset_b_item;
            LenStrmB << length_b_item;
            loctrl << false;
            DEBUG("[INFO] IdA: ", IdA, " IdB: ", IdB, " offsetA: ", offsetA, " lengthA: ", lengthA, " offsetB: ", offsetB, " lengthB: ", lengthB);
        }
    }
    loctrl << true;
}

void duplicateStream (hls::stream<ap_uint<32>>& OffStrmA, hls::stream<ap_uint<32>>& OffStrmB, 
                      hls::stream<ap_uint<32>>& LenStrmA, hls::stream<ap_uint<32>>& LenStrmB, 
                      hls::stream<bool>& loctrl, 
                      hls::stream<ap_uint<32>>& OffA1, hls::stream<ap_uint<32>>& OffA2, hls::stream<ap_uint<32>>& OffA3, 
                      hls::stream<ap_uint<32>>& LenA1, hls::stream<ap_uint<32>>& LenA2, hls::stream<ap_uint<32>>& LenA3, 
                      hls::stream<ap_uint<32>>& OffB1, hls::stream<ap_uint<32>>& OffB2, 
                      hls::stream<ap_uint<32>>& LenB1, hls::stream<ap_uint<32>>& LenB2, 
                      hls::stream<bool>& Ctl1, hls::stream<bool>& Ctl2, hls::stream<bool>& Ctl3, hls::stream<bool>& Ctl4) {
    ap_uint<32> offset_item_a;
    ap_uint<32> length_item_a;
    ap_uint<32> offset_item_b;
    ap_uint<32> length_item_b;
    duplicate_stream: while (1) {
#pragma HLS pipeline II = 1
        bool lo_flag = loctrl.read();
        if (lo_flag) {
            Ctl1 << true;
            Ctl2 << true;
            Ctl3 << true;
            Ctl4 << true;
            break;
        }

        offset_item_a = OffStrmA.read();
        length_item_a = LenStrmA.read();
        offset_item_b = OffStrmB.read();
        length_item_b = LenStrmB.read();
        OffA1 << offset_item_a;
        OffA2 << offset_item_a;
        OffA3 << offset_item_a;
        LenA1 << length_item_a;
        LenA2 << length_item_a;
        LenA3 << length_item_a;
        OffB1 << offset_item_b;
        OffB2 << offset_item_b;
        LenB1 << length_item_b;
        LenB2 << length_item_b;
        Ctl1 << false;
        Ctl2 << false;
        Ctl3 << false;
        Ctl4 << false;
    }
}

void loadSrcAdjList(hls::stream<ap_uint<32>>& OffA1, 
                    hls::stream<ap_uint<32>>& LenA1, 
                    hls::stream<bool>& Ctl1, 
                    ap_uint<512>* column_list_1, 
                    hls::stream<ap_uint<512>>& tc_stream_a_merge,
                    hls::stream<ap_uint<8>>& tc_stream_a_merge_inst) {

    // State machine states
    enum State { READ_INPUT, CAM_RESETTING, PROCESS_ADJLIST};
    State state = READ_INPUT;

    // Local variables
    ap_uint<32> offset_item = 0, length_item = 0, IdA_last = 0xffffffff;
    ap_uint<32> parallelism = 0, encode_parallelism = 0, start_idx = 0, end_idx = 0, global_index = 0;
    // ap_uint<STREAM_LENGTH> tc_item;
    ap_uint<512> tc_item_data;
    ap_uint<8> tc_item_inst;

    loadSrcAdjList_loop: while (1) {
        #pragma HLS pipeline II=1
        switch (state) {
            case READ_INPUT: {
                bool ctl_flag = Ctl1.read();
                if (ctl_flag) {
                    return;
                } else {
                    offset_item = OffA1.read();
                    length_item = LenA1.read();
                    if (offset_item != IdA_last) {
                        IdA_last = offset_item;
                        // Update the routing table, hardware friendly.
                        ap_uint<32> length_aligned = (((offset_item + length_item - 1) >> 4) - (offset_item >> 4) + 1) << 4; 
                        parallelism = ReturnParallelism(length_aligned);
                        tc_item_inst.range(7, 7) = 0; // Reserved
                        tc_item_inst.range(2, 0) = 0; // Parallelism reserved
                        tc_item_data.range(511, 0) = ReturnRoutingTable(parallelism);
                        tc_item_inst.range(6, 3) = SET_ROUTING_TABLE; // Set routing table
                        tc_stream_a_merge << tc_item_data;
                        tc_stream_a_merge_inst << tc_item_inst;
                        state = CAM_RESETTING;
                    } else {
                        state = READ_INPUT;
                    }
                }
                break;
            }

            case CAM_RESETTING: {
                tc_item_inst.range(6, 3) = RESET_CAM;
                tc_stream_a_merge_inst << tc_item_inst;
                tc_item_data.range(511, 0) = 0;
                tc_stream_a_merge << tc_item_data;
                state = PROCESS_ADJLIST;
                // calculate the start_idx and end_idx for the next state.
                encode_parallelism = EncodeParallelism(parallelism);
                start_idx = (offset_item >> 4);
                end_idx = ((offset_item + length_item - 1) >> 4);
                break;
            }

            case PROCESS_ADJLIST: {
                tc_item_inst.range(6, 3) = UPDATE_DUPLICATE; // duplicate list A's adjlist.
                tc_item_inst.range(2, 0) = encode_parallelism;
                if (start_idx <= end_idx) {
                    ap_uint<512> column_list_item = column_list_1[start_idx];
                    for (ap_uint<32> j = 0; j < 16; j++) {
                    #pragma HLS UNROLL
                        global_index = (start_idx << 4) + j;
                        if ((global_index >= offset_item) && (global_index < (offset_item + length_item))) {
                            tc_item_data.range((j << 5) + 31, (j << 5)) = column_list_item.range((j << 5) + 31, (j << 5));
                        } else {
                            tc_item_data.range((j << 5) + 31, (j << 5)) = DUMMY_DATA_UD;
                        }
                    }
                    tc_stream_a_merge << tc_item_data;
                    tc_stream_a_merge_inst << tc_item_inst;
                    start_idx++;
                } else {
                    state = READ_INPUT;
                }
                break;
            }
        }
    }
}


// need to split these function into two parts. One is for ADJ list load, another is generating STREAM_KEYS.
void loadDstAdjList(hls::stream<ap_uint<32>>& OffB1, hls::stream<ap_uint<32>>& LenB1, 
                    hls::stream<bool>& Ctl2, 
                    ap_uint<512>* column_list_2, 
                    hls::stream<ap_uint<32>>& b_item_num,
                    hls::stream<ap_uint<512>>& tc_stream_data) {
// State machine states
    enum State { READ_INPUT, PROCESS_ADJLIST};
    State state = READ_INPUT;
    // Local variables
    ap_uint<32> offset_b = 0, length_b = 0;
    ap_uint<32> start_idx = 0, end_idx = 0, global_index = 0;
    ap_uint<512> tc_item_data;
    ap_uint<32> i = 0, j = 0;

    loadDstAdjList_loop: while (1) {
        #pragma HLS pipeline II=1
        switch (state) {
            case READ_INPUT: {
                bool ctl_flag = Ctl2.read();
                if (ctl_flag) {
                    return;
                } else {
                    offset_b = OffB1.read();
                    length_b = LenB1.read();
                    start_idx = (offset_b >> 4);
                    end_idx = ((offset_b + length_b - 1) >> 4);
                    i = start_idx;
                    b_item_num << (end_idx - start_idx + 1);
                    state = PROCESS_ADJLIST;
                }
                break;
            }

            case PROCESS_ADJLIST: {
                if (i <= end_idx) {
                    tc_item_data = column_list_2[i];
                    for (j = 0; j < 16; j++) {
                    #pragma HLS UNROLL
                        global_index = (i << 4) + j;
                        if (!((global_index >= offset_b) && (global_index < (offset_b + length_b)))) {
                            tc_item_data.range((j << 5) + 31, (j << 5)) = DUMMY_DATA_MQ;
                        }
                    }
                    tc_stream_data << tc_item_data;
                    i++;
                } else {
                    state = READ_INPUT; // Transition back to IDLE after processing
                }
                break;
            }
        }
    }
}

void generateMultiQueries(hls::stream<ap_uint<32>>& OffA2, 
                        hls::stream<ap_uint<32>>& LenA2, 
                        hls::stream<bool>& Ctl3, 
                        hls::stream<ap_uint<32>>& b_item_num,
                        hls::stream<ap_uint<512>>& tc_stream_b_data, 
                        hls::stream<ap_uint<512>>& tc_stream_b_merge,
                        hls::stream<ap_uint<8>>& tc_stream_b_merge_inst) {
    enum State { READ_INPUT, GET_B_ITEM, STREAM_KEYS};
    State state = READ_INPUT;
    // Local variables
    ap_uint<32> offset_a = 0, length_a = 0;
    ap_uint<32> parallelism = 0, encode_parallelism = 0, multi_queries = 0;
    ap_uint<32> start_idx = 0, end_idx = 0, global_index = 0;
    ap_uint<512> stream_b_item = 0;
    ap_uint<32> b_counter = 0;
    // ap_uint<STREAM_LENGTH> tc_item;
    ap_uint<512> tc_item_data;
    ap_uint<8> tc_item_inst;
    ap_uint<32> i = 0, j = 0; // Loop indices

    generateMultiQueries_loop: while (1) {
        #pragma HLS pipeline II=1
        switch (state) {
            case READ_INPUT: {
                bool ctl_flag = Ctl3.read();
                if (ctl_flag) {
                    return;
                } else {
                    offset_a = OffA2.read();
                    length_a = LenA2.read();
                    ap_uint<32> aligned_length = (((offset_a + length_a - 1) >> 4) - (offset_a >> 4) + 1) << 4;
                    parallelism = ReturnParallelism(aligned_length);
                    encode_parallelism = EncodeParallelism(parallelism);
                    multi_queries = GetMultiQueries(parallelism);
                    b_counter = b_item_num.read();
                    i = 0;
                    state = GET_B_ITEM;
                }
                break;
            }

            case GET_B_ITEM: {
                stream_b_item = tc_stream_b_data.read();
                j = 0;
                state = STREAM_KEYS;
                break;
            }

            case STREAM_KEYS: {
                if (j < multi_queries) {
                    tc_item_inst.range(7, 7) = 0; // Reserved
                    tc_item_inst.range(6, 3) = SEARCH_MQ;
                    tc_item_inst.range(2, 0) = encode_parallelism;
                    for (ap_uint<32> t = 0; t < 16; t++) {
                    #pragma HLS UNROLL
                        if (t < parallelism) { // Set the parallelism data into the lower location
                            tc_item_data.range((t << 5) + 31, (t << 5)) = 
                                stream_b_item.range((((j << (encode_parallelism - 1)) + t) << 5) + 31, ((j << (encode_parallelism - 1)) + t) << 5);
                        } else {
                            tc_item_data.range((t << 5) + 31, (t << 5)) = DUMMY_DATA_MQ;
                        }
                    }
                    tc_stream_b_merge << tc_item_data;
                    tc_stream_b_merge_inst << tc_item_inst;
                    j++; // Increment key streaming loop index
                } else {
                    i++; // Increment outer adjacency list index
                    if (i < b_counter) {
                        state = GET_B_ITEM;
                    } else {
                        state = READ_INPUT;
                    }
                }
                break;
            }
        }
    }
}

ap_uint<32> GetNumberQueries(ap_uint<32> start_idx, ap_uint<32> end_idx, ap_uint<32> length_aligned) {
    #pragma HLS inline
    ap_uint<32> temp = end_idx - start_idx + 1;
    // Simplify shift calculations using direct mapping
    ap_uint<32> shift_value = 0;
    if (length_aligned <= 128) {
        shift_value = 0;
    } else if (length_aligned <= 256) {
        shift_value = 1;
    } else if (length_aligned <= 512) {
        shift_value = 2;
    } else if (length_aligned <= 1024) {
        shift_value = 3;
    } else if (length_aligned <= 2048) {
        shift_value = 4;
    } else {
        shift_value = 32; // should not exist this case
    }

    return (temp << shift_value);
}

void mergeTcStream(hls::stream<ap_uint<32>>& OffA3, hls::stream<ap_uint<32>>& LenA3, 
                   hls::stream<ap_uint<32>>& OffB2, hls::stream<ap_uint<32>>& LenB2, 
                   hls::stream<bool>& Ctl3, 
                   hls::stream<ap_uint<512>>& tc_stream_a_merge, 
                   hls::stream<ap_uint<8>>& tc_stream_a_merge_inst, 
                   hls::stream<ap_uint<512>>& tc_stream_b_merge, 
                   hls::stream<ap_uint<8>>& tc_stream_b_merge_inst, 
                   hls::stream<ap_axiu<STREAM_LENGTH, 0, 0, 0>>& tc_stream) {
enum State { READ_INPUT, FORWARD_STREAM_A, FORWARD_STREAM_B };
    State state = READ_INPUT;

    // Local variables
    ap_uint<32> offset_a = 0, length_a = 0, IdA_last = 0xffffffff;
    ap_uint<32> offset_b = 0, length_b = 0;
    ap_uint<32> counter_a = 0, counter_b = 0;
    ap_uint<32> number_a = 0, number_b = 0;
    ap_uint<32> parallelism = 0;
    ap_uint<32> start_idx_a = 0, end_idx_a = 0;
    ap_uint<32> start_idx_b = 0, end_idx_b = 0;

    ap_axiu<STREAM_LENGTH, 0, 0, 0> item;

    mergeTcStream_loop: while (1) {
        #pragma HLS pipeline II=1
        switch (state) {
            case READ_INPUT:{
                bool ctl_flag = Ctl3.read();
                if (ctl_flag) {
                    item.data = 0;
                    item.last = 1;
                    tc_stream << item;
                    return;
                } else {
                    offset_a = OffA3.read();
                    length_a = LenA3.read();
                    offset_b = OffB2.read();
                    length_b = LenB2.read();
                     // hardware friendly.
                    ap_uint<32> length_aligned = (((offset_a + length_a - 1) >> 4) - (offset_a >> 4) + 1) << 4;
                    start_idx_a = (offset_a) >> 4;
                    end_idx_a = (offset_a + length_a - 1) >> 4; 
                    // one rest_CAM, one ROUTING_TABLE + several UPDATE_DUPLICATE.
                    number_a = 1 + 1 + end_idx_a - start_idx_a + 1;
                    start_idx_b = (offset_b) >> 4;
                    end_idx_b = (offset_b + length_b - 1) >> 4;
                    number_b = GetNumberQueries(start_idx_b, end_idx_b, length_aligned);
                    counter_a = 0;
                    counter_b = 0;
                    if (offset_a != IdA_last) {
                        IdA_last = offset_a;
                        state = FORWARD_STREAM_A;
                    } else {
                        state = FORWARD_STREAM_B;
                    }
                }
                break;
            }

            case FORWARD_STREAM_A: {
                if (counter_a < number_a) {
                    item.data.range(511, 0) = tc_stream_a_merge.read();
                    item.data.range(519, 512) = tc_stream_a_merge_inst.read();
                    item.last = 0;
                    tc_stream << item;
                    counter_a++;
                    if (counter_a == number_a) {
                        state = FORWARD_STREAM_B;
                    }
                } else {
                    state = FORWARD_STREAM_B;
                }
                break;
            }

            case FORWARD_STREAM_B: {
                if (counter_b < number_b) {
                    item.data.range(511, 0) = tc_stream_b_merge.read();
                    item.data.range(519, 512) = tc_stream_b_merge_inst.read();
                    item.last = 0;
                    tc_stream << item;
                    counter_b++;
                    if (counter_b == number_b) {
                        state = READ_INPUT;
                    }
                } else {
                    state = READ_INPUT;
                }
                break;
            }
        }
    }
}


extern "C" {
void triangle_count(ap_uint<64>* edge_list, 
                    ap_uint<64>* offset_1, ap_uint<64>* offset_2, 
                    ap_uint<512>* column_list_1, ap_uint<512>* column_list_2, 
                    int edge_num, 
                    hls::stream<ap_axiu<STREAM_LENGTH, 0, 0, 0>>& tc_stream) {
#pragma HLS INTERFACE m_axi offset = slave latency = 16 num_read_outstanding = 32 max_read_burst_length = 32 bundle = gmem0 port = edge_list
#pragma HLS INTERFACE m_axi offset = slave latency = 16 num_read_outstanding = 32 max_read_burst_length = 32 bundle = gmem1 port = offset_1
#pragma HLS INTERFACE m_axi offset = slave latency = 16 num_read_outstanding = 32 max_read_burst_length = 32 bundle = gmem2 port = offset_2
#pragma HLS INTERFACE m_axi offset = slave latency = 16 num_read_outstanding = 32 max_read_burst_length = 64 bundle = gmem3 port = column_list_1
#pragma HLS INTERFACE m_axi offset = slave latency = 16 num_read_outstanding = 32 max_read_burst_length = 64 bundle = gmem4 port = column_list_2

#pragma HLS INTERFACE s_axilite port = edge_list bundle = control
#pragma HLS INTERFACE s_axilite port = offset_1 bundle = control
#pragma HLS INTERFACE s_axilite port = offset_2 bundle = control
#pragma HLS INTERFACE s_axilite port = column_list_1 bundle = control
#pragma HLS INTERFACE s_axilite port = column_list_2 bundle = control
#pragma HLS INTERFACE s_axilite port = edge_num bundle = control
#pragma HLS INTERFACE s_axilite port = return bundle = control

#pragma HLS dataflow

    static hls::stream<ap_uint<64>> edgeStrm;
    static hls::stream<ap_uint<32>> OffStrmA;
    static hls::stream<ap_uint<32>> OffStrmB;
    static hls::stream<ap_uint<32>> LenStrmA;
    static hls::stream<ap_uint<32>> LenStrmB;
    static hls::stream<bool> lelctrl; // load edge list control signal
    static hls::stream<bool> loctrl; // load offset control signal
#pragma HLS STREAM variable = edgeStrm depth=16
#pragma HLS STREAM variable = OffStrmA depth=16
#pragma HLS STREAM variable = OffStrmB depth=16
#pragma HLS STREAM variable = LenStrmA depth=16
#pragma HLS STREAM variable = LenStrmB depth=16
#pragma HLS STREAM variable = lelctrl depth=16
#pragma HLS STREAM variable = loctrl depth=16

    static hls::stream<ap_uint<32>> OffA1;
    static hls::stream<ap_uint<32>> OffA2;
    static hls::stream<ap_uint<32>> OffA3;
    static hls::stream<ap_uint<32>> LenA1;
    static hls::stream<ap_uint<32>> LenA2;
    static hls::stream<ap_uint<32>> LenA3;
    static hls::stream<ap_uint<32>> OffB1;
    static hls::stream<ap_uint<32>> OffB2;
    static hls::stream<ap_uint<32>> LenB1;
    static hls::stream<ap_uint<32>> LenB2;
    static hls::stream<bool> Ctl1;
    static hls::stream<bool> Ctl2;
    static hls::stream<bool> Ctl3;
    static hls::stream<bool> Ctl4;
#pragma HLS STREAM variable = OffA1 depth=512
#pragma HLS STREAM variable = OffA2 depth=512 
#pragma HLS STREAM variable = OffA3 depth=512
#pragma HLS STREAM variable = LenA1 depth=512
#pragma HLS STREAM variable = LenA2 depth=512
#pragma HLS STREAM variable = LenA3 depth=512
#pragma HLS STREAM variable = OffB1 depth=512
#pragma HLS STREAM variable = OffB2 depth=512
#pragma HLS STREAM variable = LenB1 depth=512
#pragma HLS STREAM variable = LenB2 depth=512
#pragma HLS STREAM variable = Ctl1 depth=512
#pragma HLS STREAM variable = Ctl2 depth=512
#pragma HLS STREAM variable = Ctl3 depth=512
#pragma HLS STREAM variable = Ctl4 depth=512

    hls::stream<ap_uint<32>> item_b_stream;
    hls::stream<ap_uint<512>> tc_stream_a_merge;
    hls::stream<ap_uint<8>> tc_stream_a_merge_inst;
    hls::stream<ap_uint<512>> tc_stream_b_merge;
    hls::stream<ap_uint<8>> tc_stream_b_merge_inst;
    hls::stream<ap_uint<512>> tc_stream_b_data;
#pragma HLS STREAM variable = item_b_stream depth=512
#pragma HLS STREAM variable = tc_stream_a_merge depth=512
#pragma HLS STREAM variable = tc_stream_a_merge_inst depth=512
#pragma HLS STREAM variable = tc_stream_b_merge depth=512
#pragma HLS STREAM variable = tc_stream_b_merge_inst depth=512
#pragma HLS STREAM variable = tc_stream_b_data depth=512

    loadEdgeList(edge_num, edge_list, edgeStrm, lelctrl); // each edge has two vertices, 64 bits
    loadOffset(edgeStrm, lelctrl, offset_1, offset_2, OffStrmA, OffStrmB, LenStrmA, LenStrmB, loctrl);
    duplicateStream(OffStrmA, OffStrmB, LenStrmA, LenStrmB, loctrl, 
                    OffA1, OffA2, OffA3, LenA1, LenA2, LenA3, 
                    OffB1, OffB2, LenB1, LenB2, 
                    Ctl1, Ctl2, Ctl3, Ctl4);
    loadSrcAdjList(OffA1, LenA1, Ctl1, column_list_1, tc_stream_a_merge, tc_stream_a_merge_inst);
    loadDstAdjList(OffB1, LenB1, Ctl2, column_list_2, item_b_stream, tc_stream_b_data);
    generateMultiQueries(OffA2, LenA2, Ctl3, item_b_stream, tc_stream_b_data, 
                        tc_stream_b_merge, tc_stream_b_merge_inst);
    mergeTcStream(OffA3, LenA3, OffB2, LenB2, Ctl4, 
                    tc_stream_a_merge, tc_stream_a_merge_inst, 
                    tc_stream_b_merge, tc_stream_b_merge_inst, 
                    tc_stream);

}
}
