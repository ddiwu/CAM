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

// AXI-Lite address 
#define CTRL_REG_ADDR 0x000
#define GIER_REG_ADDR 0x004
#define IP_IER_REG_ADDR 0x008
#define IP_ISR_REG_ADDR 0x00C
#define A_REG_ADDR 0x010
#define A_REG_ADDR_CTRL 0x018
#define B_REG_ADDR 0x01C
#define C_REG_ADDR 0x028
#define LENGTH_R_REG_ADDR 0x034
#define CTRL_1_DONE_REG_ADDR 0x03C
#define LATENCY 0x040

#define NK 1

long int val[NK][DATA_SIZE/2];

int cam_appro(long int key){
    long int xor_value[DATA_SIZE/2];
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

int cam_precise(long int key, int num){
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

    auto size = DATA_SIZE;
    // Allocate Memory in Host Memory
    long int source_sw_results[NK][2560];

    xrt::kernel krnl_input[NK];
    xrt::kernel krnl_output[NK];
    xrt::kernel rtl_kernel[NK];
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
    long int* buffer_in_map[NK];
    long int* buffer_out_map[NK];
    for (int i = 0; i < NK; i++) {
        buffer_in[i] = xrt::bo(device, 2048 * sizeof(long int), bank_assign[i]);
        buffer_out[i] = xrt::bo(device, 1024 * sizeof(long int), bank_assign[i]);
    }
    for (int i = 0; i < NK; i++) {
        buffer_in_map[i] = buffer_in[i].map<long int*>();
        buffer_out_map[i] = buffer_out[i].map<long int*>();
    }


    // Create the test data and Software Result
    buffer_in_map[0][0] = 2;
    for (int i = 8; i < 8+DATA_SIZE/2; i++) {
        buffer_in_map[0][i] = i-8;
        // buffer_in_map[1][i] = i*i;
        val[0][i-8] = i-8;
        // val[1][i] = i*i;
    }
    buffer_in_map[0][264] = 4;

    for (int i = 16+DATA_SIZE/2; i < 16+DATA_SIZE/2+256; i += 8) {
        buffer_in_map[0][i] = (i-16)/8;
        // buffer_in_map[1][i] = (i)/8;
    }

    for (int i = 0; i < 32; i++) {
        // source_sw_results[i] = cam_appro(i);
        source_sw_results[0][i] = cam_precise(i+32, 0);
        // source_sw_results[1][i] = cam_precise(i+128, 1);
    }

    for (int i = 0; i < NK; i++){
        for (int j = 0; j < 512; j++) {
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
    // xrt::run rtl_run[NK];
    xrt::run write_stage[NK];
    for (int i = 0; i < NK; i++) {
        write_stage[i] = xrt::run(krnl_output[i]);
        read_stage[i] = xrt::run(krnl_input[i]);
        // rtl_run[i] = xrt::run(rtl_kernel[i]);
    }
    for (int i = 0; i < NK; i++) {
        write_stage[i].set_arg(0, buffer_out[i]);
        write_stage[i].set_arg(1, 32);
        write_stage[i].start();

        read_stage[i].set_arg(0, buffer_in[i]);
        read_stage[i].set_arg(1, 17+32);
        read_stage[i].start();
        
        // rtl_run[i].set_arg(0, 64);
        // rtl_run[i].set_arg(1, 0);
        // rtl_run[i].start();
    }

    std::cout << "Launching RTL kernel" << std::endl;
    // uint32_t ctrl_1_done = rtl_kernel[0].read_register(CTRL_1_DONE_REG_ADDR);
    // std::cout << "kernel0: ctrl_1_done = " << ctrl_1_done << std::endl;
    // ctrl_1_done = rtl_kernel[1].read_register(CTRL_1_DONE_REG_ADDR);
    // std::cout << "kernel1: ctrl_1_done = " << ctrl_1_done << std::endl;

    // std::cout << "Reading LENGTH_R_REG_ADDR" << std::endl;
    // uint32_t si;
    // si = rtl_kernel.read_register(LENGTH_R_REG_ADDR);
    // std::cout << "si = " << si << std::endl;

    // while (!ctrl_1_done) {
    //     std::cout << "suspend 1 s" << std::endl;
    //     std::this_thread::sleep_for(std::chrono::seconds(1));
    //     ctrl_1_done = rtl_kernel[0].read_register(CTRL_1_DONE_REG_ADDR);
    // }
    // std::cout << "1 stage done" << std::endl;
    // uint32_t latency_cycle = rtl_kernel[0].read_register(A_REG_ADDR_CTRL);
    // std::cout << "kernel0: latency_cycle = " << latency_cycle << std::endl;
    // latency_cycle = rtl_kernel[1].read_register(A_REG_ADDR_CTRL);
    // std::cout << "kernel1: latency_cycle = " << latency_cycle << std::endl;
    sleep(5);
    for (int i = 0; i < NK; i++) {
        // write_stage[i].wait();
        read_stage[i].wait();
        // rtl_run[i].wait();
    }
    // }


    std::cout << "Copying output data to device global memory" << std::endl;
    for (int i = 0; i < NK; i++) {
        buffer_out[i].sync(XCL_BO_SYNC_BO_FROM_DEVICE);
    }


    std::cout << "Comparing results" << std::endl;
    int match = 0;
    for (int i = 0; i < 32; i++) {
        if (buffer_out_map[0][8*i] != source_sw_results[0][i]) {
            std::cout << "Error: Result mismatch" << std::endl;
            std::cout << "i = " << i << " Software result = " << source_sw_results[0][i]
                      << " Device result = " << buffer_out_map[0][8*i] << std::endl;
            // std::cout << "1 num = " << buffer_out_map[8*i+1] << std::endl;
            match = 1;
        }
        else 
        {
            std::cout << "i = " << i << " Software result = " << source_sw_results[0][i]
                      << " Device result = " << buffer_out_map[0][8*i] << std::endl;
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
