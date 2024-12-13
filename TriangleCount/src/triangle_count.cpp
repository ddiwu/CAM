#include "tc_header.h"

// void PrintHex(const std::string& label, const ap_uint<512>& data) {
//     // Print label, dest, and data in one line
//     std::cout << label << " data: ";
//     for (int i = 0; i < 16; ++i) {
//         if (i > 0) std::cout << " "; // Add space between values
//         std::cout << "0x" << std::hex << std::setw(8) << std::setfill('0') << data.range(32 * i + 31, 32 * i).to_uint();
//     }
//     std::cout << std::endl; // End the line
// }


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
    for (int i = 0; i < 16; ++i) {
        #pragma HLS UNROLL
        switch (parallelism) {
            case 1:  routing_table.range(32 * i + 31, 32 * i) = 0; break;
            case 2:  routing_table.range(32 * i + 31, 32 * i) = base[i / 8]; break;
            case 4:  routing_table.range(32 * i + 31, 32 * i) = base[i / 4]; break;
            case 8:  routing_table.range(32 * i + 31, 32 * i) = base[i / 2]; break;
            case 16: routing_table.range(32 * i + 31, 32 * i) = base[i]; break;
        }
    }
    return routing_table;
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

void loadAdjList(hls::stream<bool>& loctrl,
                hls::stream<ap_uint<32>>& OffStrmA, hls::stream<ap_uint<32>>& OffStrmB, 
                hls::stream<ap_uint<32>>& LenStrmA, hls::stream<ap_uint<32>>& LenStrmB,
                ap_uint<512>* column_list_A, ap_uint<512>* column_list_B, 
                hls::stream<ap_axiu<STREAM_LENGTH, 0, 0, 0> >& tc_stream) {
    bool pp_flag = false; // pp_flag = 0, A; pp_flag = 1, B
    uint32_t IdA_last = 0xffffffff;
    ap_uint<32> offset_item;
    ap_uint<32> length_item;
    ap_axiu<STREAM_LENGTH, 0, 0, 0> tc_item;
    int parallelism; // used to determine the runtime parallelism for each src vertex based in adj list length.
    int encode_parallelism;
    int start_idx, end_idx;
    int global_index; // used to mask DUMMY data in CAM.
    while (1) {
        if (pp_flag) {
            offset_item = OffStrmB.read();
            length_item = LenStrmB.read();

            int start_idx = (offset_item) / 16;
            int end_idx = (offset_item + length_item - 1) / 16;

            Load_adj_list_B: for (int i = start_idx; i <= end_idx; i++) {
                ap_uint<512> search_key_item = column_list_B[i];
                for (int j = 0; j < 16; j++) {
                #pragma HLS UNROLL
                    global_index = i * 16 + j;
                    if (!((global_index >= offset_item) && (global_index < (offset_item + length_item)))) {
                        search_key_item.range(32 * j + 31, 32 * j) = DUMMY_DATA_MQ;
                    }
                }
                // PrintHex("search_key_item", search_key_item);
                int send_key_loop = 16 / parallelism;
                Load_adj_list_B_send_key: for (int k = 0; k < send_key_loop; k++) {
                #pragma HLS pipeline II = 1
                    tc_item.data.range(519, 519) = 0; // reserved
                    tc_item.data.range(518, 515) = SEARCH_MQ;
                    tc_item.data.range(514, 512) = encode_parallelism; // seems not used, reserved in this case.
                    for (int t = 0; t < 16; t++) {
                    #pragma HLS UNROLL
                        if (t < parallelism) { // Set the parallelism data into the lower location
                            tc_item.data.range(32 * t + 31, 32 * t) = search_key_item.range(32 * (k * parallelism + t) + 31, 32 * (k * parallelism + t));
                        } else {
                            tc_item.data.range(32 * t + 31, 32 * t) = DUMMY_DATA_MQ;
                        }
                    }
                    tc_item.last = 0;
                    tc_stream << tc_item;
                    // PrintHex("list b tc_item", tc_item.data.range(511, 0));
                }
            }

        } else {
            bool lo_flag = loctrl.read();
            if (lo_flag) break;

            offset_item = OffStrmA.read();
            length_item = LenStrmA.read();

            if (offset_item != IdA_last) {
                IdA_last = offset_item;
                // Update the routing table first.
                parallelism = ReturnParallelism(length_item);
                tc_item.data.range(519, 519) = 0; // reserved
                tc_item.data.range(514, 512) = 0; // parallelism, reserved in this case.
                tc_item.data.range(511, 0) = ReturnRoutingTable(parallelism);
                tc_item.data.range(518, 515) = SET_ROUTING_TABLE; // set the routing table.
                tc_item.last = 0;
                tc_stream << tc_item;

                // need to add a reset flag to reset the CAM content.
                tc_item.data.range(518, 515) = RESET_CAM;
                tc_item.last = 0;
                tc_stream << tc_item;

                // ******* [IMPORTANT] *******; when setting the routing table, the content in CAM should be set to 0;
                // update the CAM content. Step 1: update the parallelism.
                encode_parallelism = EncodeParallelism(parallelism);
                tc_item.data.range(518, 515) = UPDATE_DUPLICATE; // duplicate list A's adjlist.
                tc_item.data.range(514, 512) = encode_parallelism;
                // update the CAM content. Step 2: update the adjlist.
                start_idx = (offset_item) / 16; // need to mask DUMMY data in CAM.
                end_idx = (offset_item + length_item - 1) / 16; // corner case, offset = 0, length = 16. 
                Load_adj_list_A: for (int i = start_idx; i <= end_idx; i++) {
                #pragma HLS pipeline II = 1
                    ap_uint<512> column_list_item = column_list_A[i];
                    for (int j = 0; j < 16; j++) {
                    #pragma HLS UNROLL
                        global_index = i * 16 + j;
                        if ((global_index >= offset_item) && (global_index < (offset_item + length_item))) {
                            tc_item.data(32 * j + 31, 32 * j) = column_list_item.range(32 * j + 31, 32 * j);
                        } else {
                            tc_item.data(32 * j + 31, 32 * j) = DUMMY_DATA_UD;
                        }
                    }
                    tc_item.last = 0;
                    tc_stream << tc_item;
                }
            }
        }
        pp_flag = !pp_flag; // for an edge <A, B>, should process A first, then B.
    }
    tc_item.last = 1;
    tc_item.data = 0;
    tc_stream << tc_item;
}

extern "C" {
void triangle_count(ap_uint<64>* edge_list, ap_uint<64>* offset, ap_uint<512>* column_list, 
                    int edge_num, hls::stream<ap_axiu<STREAM_LENGTH, 0, 0, 0>>& tc_stream) {
#pragma HLS INTERFACE m_axi offset = slave latency = 16 num_read_outstanding = 32 max_read_burst_length = 32 bundle = gmem0 port = edge_list
#pragma HLS INTERFACE m_axi offset = slave latency = 16 num_read_outstanding = 32 max_read_burst_length = 2 bundle = gmem1 port = offset
#pragma HLS INTERFACE m_axi offset = slave latency = 16 num_read_outstanding = 16 max_read_burst_length = 2 bundle = gmem2 port = column_list

#pragma HLS INTERFACE s_axilite port = edge_list bundle = control
#pragma HLS INTERFACE s_axilite port = offset bundle = control
#pragma HLS INTERFACE s_axilite port = column_list bundle = control
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

    loadEdgeList(edge_num, edge_list, edgeStrm, lelctrl); // each edge has two vertices, 64 bits
    loadOffset (edgeStrm, lelctrl, offset, offset, OffStrmA, OffStrmB, LenStrmA, LenStrmB, loctrl);
    loadAdjList(loctrl, OffStrmA, OffStrmB, LenStrmA, LenStrmB, column_list, column_list, tc_stream);

}
}
