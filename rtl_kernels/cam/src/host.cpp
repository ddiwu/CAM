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

#define DATA_SIZE 4096

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

long int val[DATA_SIZE/2];

int cam_appro(long int key){
    long int xor_value[DATA_SIZE/2];
    int num_1[DATA_SIZE/2];
    for (int i = 0; i < DATA_SIZE/2; i++) {
        xor_value[i] = val[i] ^ key;
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

int cam_precise(long int key){
    for (int i = 0; i < DATA_SIZE/2; i++){
        if (val[i] == key){
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
    long int source_sw_results[256];

    auto rtl_kernel = xrt::kernel(device, uuid, "krnl_cam_rtl", xrt::kernel::cu_access_mode::exclusive);
    // auto cl_kernel = xrt::kernel(device, uuid, "krnl_cam");

    int bank_assign[3] = {0, 0, 1};
    // for (int i = 0; i < 3; i++) {
    //     bank_assign[i] = i;
    // }

    xrt::bo buffer_r1 = xrt::bo(device, size * sizeof(long int), bank_assign[0]);
    // xrt::bo buffer_r2 = xrt::bo(device, size * sizeof(long int), bank_assign[1]);
    xrt::bo buffer_rw_0 = xrt::bo(device, 256 * sizeof(long int), bank_assign[2]);
    // xrt::bo buffer_rw_1 = xrt::bo(device, size * sizeof(long int), cl_kernel.group_id(0));
    // xrt::bo buffer_r3 = xrt::bo(device, size * sizeof(long int), cl_kernel.group_id(1));
    // xrt::bo buffer_w = xrt::bo(device, size * sizeof(long int), cl_kernel.group_id(2));

    auto buffer_r1_map = buffer_r1.map<long int*>();
    // auto buffer_r2_map = buffer_r2.map<long int*>();
    auto buffer_rw_0_map = buffer_rw_0.map<long int*>();
    // auto buffer_rw_1_map = buffer_rw_1.map<long int*>();
    // auto buffer_r3_map = buffer_r3.map<long int*>();
    // auto buffer_w_map = buffer_w.map<long int*>();

    // Create the test data and Software Result
    for (int i = 0; i < DATA_SIZE/2; i++) {
        buffer_r1_map[i] = i;
        val[i] = i;
        // buffer_r2_map[i] = 0;
    }

    for (int i = DATA_SIZE/2; i < (DATA_SIZE/2+256); i += 8) {
        buffer_r1_map[i] = (i)/2;
    }

    // for (int i = 0; i < 32; i++) {
    //     for (int j = 0; j < 256; j++) {
    //         source_sw_results[i*256+j] = (j) ^ i;
    //     }
    // }

    for (int i = 0; i < 32; i++) {
        // source_sw_results[i] = cam_appro(i);
        source_sw_results[i] = cam_precise(4*(i+256));
    }

    for (int i = 0; i < 256; i++) {
        buffer_rw_0_map[i] = 0;
    }

    std::cout << "Copying input data to device global memory" << std::endl;
    buffer_r1.sync(XCL_BO_SYNC_BO_TO_DEVICE);
    // buffer_r2.sync(XCL_BO_SYNC_BO_TO_DEVICE);
    buffer_rw_0.sync(XCL_BO_SYNC_BO_TO_DEVICE);

    std::cout << "Launching RTL kernel" << std::endl;

    xrt::run rtl_run = rtl_kernel(buffer_r1, /*buffer_r2,*/ buffer_rw_0, 32, 0); // can not be 1 without update
    uint32_t ctrl_1_done = rtl_kernel.read_register(CTRL_1_DONE_REG_ADDR);
    std::cout << "ctrl_1_done = " << ctrl_1_done << std::endl;

    std::cout << "Reading LENGTH_R_REG_ADDR" << std::endl;
    uint32_t si;
    si = rtl_kernel.read_register(LENGTH_R_REG_ADDR);
    std::cout << "si = " << si << std::endl;

    while (!ctrl_1_done) {
        std::cout << "suspend 1 s" << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(1));
        ctrl_1_done = rtl_kernel.read_register(CTRL_1_DONE_REG_ADDR);
    }
    std::cout << "1 stage done" << std::endl;
    uint32_t latency_cycle = rtl_kernel.read_register(A_REG_ADDR_CTRL);
    std::cout << "latency_cycle = " << latency_cycle << std::endl;
    // sleep(5);
    rtl_run.wait();
    // uint32_t latency_cycle = rtl_kernel.read_register(LATENCY);
    // std::cout << "latency_cycle = " << latency_cycle << std::endl;

    std::cout << "Copying output data to device global memory" << std::endl;
    buffer_rw_0.sync(XCL_BO_SYNC_BO_FROM_DEVICE);
    // buffer_rw_0.copy(buffer_rw_1);
    // buffer_rw_1.sync(XCL_BO_SYNC_BO_TO_DEVICE);
    // buffer_r3.sync(XCL_BO_SYNC_BO_TO_DEVICE);

    // std::cout << "Launching CL kernel" << std::endl;
    // xrt::run cl_run = cl_kernel(buffer_rw_1, buffer_r3, buffer_w, size);
 
    // cl_run.wait();

    // std::cout << "Copying output data to host local memory" << std::endl;
    // buffer_w.sync(XCL_BO_SYNC_BO_FROM_DEVICE);


    std::cout << "Comparing results" << std::endl;
    int match = 0;
    for (int i = 0; i < 32; i++) {
        if (buffer_rw_0_map[8*i] != source_sw_results[i]) {
            std::cout << "Error: Result mismatch" << std::endl;
            std::cout << "i = " << i << " Software result = " << source_sw_results[i]
                      << " Device result = " << buffer_rw_0_map[8*i] << std::endl;
            std::cout << "1 num = " << buffer_rw_0_map[8*i+1] << std::endl;
            match = 1;
        }
        else 
        {std::cout << "i = " << i << " Software result = " << source_sw_results[i]
                      << " Device result = " << buffer_rw_0_map[8*i] << std::endl;
        std::cout << "1 num = " << buffer_rw_0_map[8*i+1] << std::endl;}
    }

    std::cout << "TEST " << (match ? "FAILED" : "PASSED") << std::endl;

    return (match ? EXIT_FAILURE : EXIT_SUCCESS); 
}
