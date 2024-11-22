/**
* Copyright (C) 2019-2021 Xilinx, Inc
*
* Licensed under the Apache License, Version 2.0 (the "License"). You may
* not use this file except in compliance with the License. A copy of the
* License is located at
*
*     http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
* WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
* License for the specific language governing permissions and limitations
* under the License.
*/
#include "xcl2.hpp"
#include <vector>
#include <ap_int.h>
#include <iostream>
#include <cstring>
#include <thread>
#include <chrono>
#include "experimental/xrt-next.h"
#include "experimental/xrt_bo.h"
#include "experimental/xrt_device.h"
#include "experimental/xrt_kernel.h"
#include "cmdlineparser.h"

#define DATA_SIZE 512

#define NK 1
#define search_num 32
#define update_num 16
#define IDLE 0
#define UPDATE_ALL 1
#define SEARCH 2
#define UPDATE_ONE 3

int val[NK][DATA_SIZE/2];

int cam_appro(int key){
    int xor_value[DATA_SIZE/2];
    int num_1[DATA_SIZE/2];
    for (int i = 0; i < DATA_SIZE/2; i++) {
        xor_value[i] = val[0][i] ^ key;
        // for (int j = 0; j < 48; j++){
        //     num_1[i] += xor_value[i].range(j,j);
        // }   
        num_1[i] = __builtin_popcount(xor_value[i]);
    }
    int index = 0;
    for (int i = 0; i < DATA_SIZE/2; i++){
        if (num_1[index] > num_1[i]){
            index = i;
        }
    }
    return index;
}

int cam_precise(int key, int num){
    for (int i = 0; i < DATA_SIZE/2; i++){
        if (val[num][i] == key){
            return i;
        }
    }
    return DATA_SIZE-1;
}

int main(int argc, char** argv) {
    // Command Line Parser
    sda::utils::CmdLineParser parser;

    // Switches
    //**************//"<Full Arg>",  "<Short Arg>", "<Description>", "<Default>"
    parser.addSwitch("--xclbin_file", "-x", "input binary file string", "");
    parser.addSwitch("--device_id", "-d", "device index", "0");
    parser.parse(argc, argv);

    std::string binaryFile = parser.value("xclbin_file");
    int device_index = stoi(parser.value("device_id"));

    std::cout << "Open the device" << device_index << std::endl;
    auto device = xrt::device(device_index);
    std::cout << "Load the xclbin " << binaryFile << std::endl;
    auto uuid = device.load_xclbin(binaryFile); 

    // Allocate Memory in Host Memory
    int source_sw_results[NK][2560];

    xrt::kernel krnl_input[NK];
    xrt::kernel krnl_output[NK];
    // xrt::kernel rtl_kernel[NK];
    for (int i = 0; i < NK; i++) {
        krnl_input[i] = xrt::kernel(device, uuid, "krnl_input");
        krnl_output[i] = xrt::kernel(device, uuid, "krnl_output");
        // rtl_kernel[i] = xrt::kernel(device, uuid, "krnl_cam_rtl", xrt::kernel::cu_access_mode::exclusive);
    }
    // rtl_kernel[0] = xrt::kernel(device, uuid, "krnl_cam_rtl:{krnl_cam_rtl_1}", xrt::kernel::cu_access_mode::exclusive);
    // rtl_kernel[1] = xrt::kernel(device, uuid, "krnl_cam_rtl:{krnl_cam_rtl_2}", xrt::kernel::cu_access_mode::exclusive);
    // auto krnl_input = xrt::kernel(device, uuid, "krnl_input");
    // auto krnl_output = xrt::kernel(device, uuid, "krnl_output");
    // auto rtl_kernel = xrt::kernel(device, uuid, "krnl_cam_rtl", xrt::kernel::cu_access_mode::exclusive);

    int bank_assign[32] = {0, 1, 2, 3 , 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 
                           14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 
                           24, 25, 26, 27, 28, 29, 30, 31};

    xrt::bo buffer_in[NK];
    xrt::bo buffer_out[NK];
    int* buffer_in_map[NK];
    int* buffer_out_map[NK];
    for (int i = 0; i < NK; i++) {
        buffer_in[i] = xrt::bo(device, 4096 * sizeof(int), bank_assign[i]);
        buffer_out[i] = xrt::bo(device, 2048 * sizeof(int), bank_assign[i]);
    }
    for (int i = 0; i < NK; i++) {
        buffer_in_map[i] = buffer_in[i].map<int*>();
        buffer_out_map[i] = buffer_out[i].map<int*>();
    }

    buffer_in_map[0][0] = UPDATE_ALL; //start update all
    buffer_in_map[0][1] = update_num; //update_num
    for (int i = 16; i < 16+DATA_SIZE/2; i++) {
        buffer_in_map[0][i] = i-16;
        // buffer_in_map[1][i] = i*i;
        val[0][i-16] = i-16;
        // val[1][i] = i*i;
    }
    buffer_in_map[0][272] = SEARCH; //start search
    buffer_in_map[0][273] = search_num; //search_num

    for (int i = 32+DATA_SIZE/2; i < 32+DATA_SIZE/2+512; i += 16) {
        buffer_in_map[0][i] = (i-32)/4;
    }
    for (int i = 800; i <800+16*5; i++) {
        buffer_in_map[0][i] = 0;
    }
    // buffer_in_map[0][880] = UPDATE_ALL;
    // for (int i = 896; i < 896+256; i++) {
    //     buffer_in_map[0][i] = 0;
    // }
    // buffer_in_map[0][1136] = SEARCH;
    // buffer_in_map[0][1137] = search_num;
    // buffer_in_map[0][1152] = 0;
    // for (int i = 32+1136; i < 16+1136+512; i += 16) {
    //     buffer_in_map[0][i] = 0;
    // }

    for (int i = 0; i < 32; i++) {
        // source_sw_results[i] = cam_appro(i);
        source_sw_results[0][i+1] = cam_precise((i+16)*4, 0);
        // source_sw_results[1][i] = cam_precise(i+128, 1);
    }
    source_sw_results[0][0] = UPDATE_ALL;// update end

    for (int i = 0; i < NK; i++){
        for (int j = 0; j < 2048; j++) {
            buffer_out_map[i][j] = 0;
        }
    }


    std::cout << "Copying input data to device global memory" << std::endl;
    for (int i = 0; i < NK; i++) {
        buffer_in[i].sync(XCL_BO_SYNC_BO_TO_DEVICE);
        buffer_out[i].sync(XCL_BO_SYNC_BO_TO_DEVICE);
    }
    // for (int k = 0; k < 5; k++){
    xrt::run read_stage[NK];
    xrt::run write_stage[NK];
    for (int i = 0; i < NK; i++) {
        write_stage[i] = xrt::run(krnl_output[i]);
        read_stage[i] = xrt::run(krnl_input[i]);
    }
    for (int i = 0; i < NK; i++) {
        write_stage[i].set_arg(0, buffer_out[i]);
        write_stage[i].set_arg(1, 1+32);
        write_stage[i].start();

        read_stage[i].set_arg(0, buffer_in[i]);
        read_stage[i].set_arg(1, 17+33);
        read_stage[i].start();
    }

    std::cout << "Launching RTL kernel" << std::endl;
    // uint32_t ctrl_1_done = rtl_kernel[0].read_register(CTRL_1_DONE_REG_ADDR);
    // std::cout << "kernel0: ctrl_1_done = " << ctrl_1_done << std::endl;
    // ctrl_1_done = rtl_kernel[1].read_register(CTRL_1_DONE_REG_ADDR);
    // std::cout << "kernel1: ctrl_1_done = " << ctrl_1_done << std::endl;

    // sleep(20);
    for (int i = 0; i < NK; i++) {
        write_stage[i].wait();
        read_stage[i].wait();
    }
    // }


    std::cout << "Copying output data to device global memory" << std::endl;
    for (int i = 0; i < NK; i++) {
        buffer_out[i].sync(XCL_BO_SYNC_BO_FROM_DEVICE);
    }


    std::cout << "Comparing results" << std::endl;
    int match = 0;
    for (int i = 0; i < 66; i++) {
        if (buffer_out_map[0][16*i] != source_sw_results[0][i]) {
            std::cout << "Error: Result mismatch" << std::endl;
            std::cout << "i = " << i << " Software result = " << source_sw_results[0][i]
                      << " Device result = " << buffer_out_map[0][16*i] << std::endl;
            // std::cout << "1 num = " << buffer_out_map[8*i+1] << std::endl;
            match = 1;
        }
        else 
        {
            std::cout << "i = " << i << " Software result = " << source_sw_results[0][i]
                      << " Device result = " << buffer_out_map[0][16*i] << std::endl;
        // std::cout << "1 num = " << buffer_out_map[8*i+1] << std::endl;}
        }
    }
    // for (int i = 0; i < 64; i++) {
    //     if (buffer_out_map[1][8*i] != source_sw_results[1][i]) {
    //         std::cout << "Error: Result mismatch" << std::endl;
    //         std::cout << "i = " << i << " Software result = " << source_sw_results[1][i]
    //                   << " Device result = " << buffer_out_map[1][8*i] << std::endl;
    //         // std::cout << "1 num = " << buffer_out_map[8*i+1] << std::endl;
    //         match = 1;
    //     }
    //     else
    //     {
    //         std::cout << "i = " << i << " Software result = " << source_sw_results[1][i]
    //                   << " Device result = " << buffer_out_map[1][8*i] << std::endl;
    //     }
    // }

    std::cout << "TEST " << (match ? "FAILED" : "PASSED") << std::endl;

    return (match ? EXIT_FAILURE : EXIT_SUCCESS); 
}
