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

/*******************************************************************************
Description:
    Kernel test for router module in CAM unit.
*******************************************************************************/

#include "cmdlineparser.h"
#include <iostream>
#include <cstring>
#include <ap_int.h>
#include <iomanip>

// XRT includes
#include "experimental/xrt_bo.h"
#include "experimental/xrt_device.h"
#include "experimental/xrt_kernel.h"

#define IDLE 0xffffff00
#define UPDATE_ALL 0xffffff01
#define UPDATE_ONE 0xffffff02
#define SEARCH_ONE 0xffffff03
#define SEARCH_MQ 0xffffff04 // Multi-Query

#define CUSTOMIZED_BLOCK_NUM 16 
#define CUSTOMIZED_BLOCK_SIZE 128

int main(int argc, char** argv) {
    // Command Line Parser
    sda::utils::CmdLineParser parser;
    parser.addSwitch("--xclbin_file", "-x", "input binary file string", "");
    parser.addSwitch("--device_id", "-d", "device index", "0");
    parser.parse(argc, argv);

    std::string binaryFile = parser.value("xclbin_file");
    int device_index = stoi(parser.value("device_id"));

    if (argc < 3) {
        parser.printHelp();
        return EXIT_FAILURE;
    }

    std::cout << "Open the device" << device_index << std::endl;
    auto device = xrt::device(device_index);
    std::cout << "Load the xclbin " << binaryFile << std::endl;
    auto uuid = device.load_xclbin(binaryFile);
    std::cout << "Kernel UUID: " << uuid << std::endl;

    int data_size = 1 + ((CUSTOMIZED_BLOCK_NUM - 1) / 16 + 1) + (CUSTOMIZED_BLOCK_NUM * CUSTOMIZED_BLOCK_SIZE / 16) 
                        + 32 + 32 + 32 + 1 + ((CUSTOMIZED_BLOCK_NUM - 1) / 16 + 31 * 2 + 1);
    std::cout << "data_size = " << data_size << std::endl;
    int vector_size_bytes = sizeof(ap_int<512>) * data_size;

    auto mem_read = xrt::kernel(device, uuid, "mem_read");
    xrt::kernel mem_write[CUSTOMIZED_BLOCK_NUM];
    for (int i = 0; i < CUSTOMIZED_BLOCK_NUM; i++) {
        mem_write[i] = xrt::kernel(device, uuid, ("mem_write:{mem_write_" + std::to_string(i + 1)) + "}");
        std::cout << "created mem_write[" << i << "]" << std::endl;
    }

    auto buffer_input = xrt::bo(device, vector_size_bytes, mem_read.group_id(0));
    auto source_input = buffer_input.map<ap_int<512>*>();

    std::vector<xrt::bo> buffer_output(CUSTOMIZED_BLOCK_NUM);
    for (int i = 0; i < CUSTOMIZED_BLOCK_NUM; i++) {
        buffer_output[i] = xrt::bo(device, vector_size_bytes, mem_write[i].group_id(0));
    }
    std::vector<ap_int<512>*> source_hw_results(CUSTOMIZED_BLOCK_NUM);
    for (int i = 0; i < CUSTOMIZED_BLOCK_NUM; i++) {
        source_hw_results[i] = buffer_output[i].map<ap_int<512>*>(); // Map one output buffer for validation
    }
    for (int i = 0; i < CUSTOMIZED_BLOCK_NUM; i++) {
        for (int j = 0; j < data_size; j++) {
            source_hw_results[i][j] = ~ap_int<512>(0);
        }
    }


    // initialize source_input
    for (int i = 0; i < data_size; i++) {
        source_input[i] = 0;
    }

    int idx = 0;

    // Step 1: UPDATE_ALL
    source_input[idx].range(511, 480) = UPDATE_ALL; // UPDATE_ALL
    source_input[idx].range(479, 0) = 0;
    idx++;

    // Add routing table entries directly after UPDATE_ALL
    const int aligned_num = (CUSTOMIZED_BLOCK_NUM - 1) / 16 + 1;
    ap_int<512*aligned_num> routing_table = 0;
    for (int i = 0; i < CUSTOMIZED_BLOCK_NUM; i++) {
        routing_table.range(31 + 32*i, 32*i) = i; // Table IDs: 1, 2, ..., CUSTOMIZED_BLOCK_NUM
    }
    int loop_num = (CUSTOMIZED_BLOCK_NUM - 1) / 16 + 1;
    for (int i = 0; i < loop_num; i++) {
        source_input[idx] = routing_table.range(511 + 512*i, 512*i);
        idx++;
    }

    // add data to update
    for (int i = 0; i < (CUSTOMIZED_BLOCK_NUM * CUSTOMIZED_BLOCK_SIZE / 16); i++) {
        for (int j = 0; j < 16; j++) {
            source_input[idx].range(32 * j + 31, 32 * j) = j * 16 + i; // CAM value
        }
        idx++;
    }

    // Step 2: SEARCH_ONE commands
    for (int i = 0; i < 32; i++) { // Search multiple specific entries
        source_input[idx].range(511, 480) = SEARCH_ONE;
        source_input[idx].range(31, 0) = i; // Data to search
        source_input[idx].range(95, 64) = i % CUSTOMIZED_BLOCK_NUM;     // Table ID to search in
        idx++;
    }

    // Step 3: UPDATE_ONE commands
    for (int i = 0; i < 32; i++) { // Update multiple blocks
        source_input[idx].range(511, 480) = UPDATE_ONE;
        source_input[idx].range(31, 0) = i; // Data to update
        source_input[idx].range(63, 32) = i;    // Offset
        source_input[idx].range(95, 64) = i % CUSTOMIZED_BLOCK_NUM;     // Table ID to update
        idx++;
    }

    // Step 4: SEARCH_ONE commands to verify updates
    for (int i = 0; i < 32; i++) { // Search for the updated data
        source_input[idx].range(511, 480) = SEARCH_ONE;
        source_input[idx].range(31, 0) = i; // Data to search after update
        source_input[idx].range(95, 64) = i % CUSTOMIZED_BLOCK_NUM;     // Table ID
        idx++;
    }

    // Step 5: SEARCH_MQ (search all), with 32 times

    for (int mq= 0; mq < 32; mq++) {
        source_input[idx].range(511, 480) = SEARCH_MQ;
        idx++;

        // Fill multi-query packet
        ap_int<512*aligned_num> multi_query;
        for (int i = 0; i < CUSTOMIZED_BLOCK_NUM; i++) {
            multi_query.range(32 * i + 31, 32 * i) = i + mq*2; // Example data
        }
        for (int i = 0; i < aligned_num; i++) {
            source_input[idx] = multi_query.range(511 + 512*i, 512*i);
            idx++;
        }
    }

    for (int i = 0; i < data_size; i++) {
        std::cout << "source_input[" << i << "] = ";
        for (int chunk = 0; chunk < 4; chunk++) {
            ap_uint<128> chunk_data = source_input[i].range(511 - chunk * 128, 384 - chunk * 128);
            std::cout << std::noshowbase << std::hex << std::setfill('0') << std::setw(32) <<  chunk_data << " ";
        }
        std::cout << std::endl;
    }


    // Copy input data to device
    std::cout << "Copying data to device..." << std::endl;
    buffer_input.sync(XCL_BO_SYNC_BO_TO_DEVICE);

    // Launch Kernels
    auto read_run = xrt::run(mem_read);
    read_run.set_arg(0, buffer_input);
    read_run.set_arg(1, data_size);
    read_run.start();

    std::vector<xrt::run> write_run(CUSTOMIZED_BLOCK_NUM);
    for (int i = 0; i < CUSTOMIZED_BLOCK_NUM; i++) {
        write_run[i] = xrt::run(mem_write[i]);
        write_run[i].set_arg(0, buffer_output[i]);
        write_run[i].start();
    }

    // Wait for completion
    read_run.wait();
    for (int i = 0; i < CUSTOMIZED_BLOCK_NUM; i++) {
        write_run[i].wait();
    }

    // Copy results from device
    for (auto& buf : buffer_output) {
        buf.sync(XCL_BO_SYNC_BO_FROM_DEVICE);
    }

    // Validate results
    std::cout << "Validating results..." << std::endl;
    for (int i = 0; i < CUSTOMIZED_BLOCK_NUM; i++) {
        std::cout << "source_hw_results[" << i << "] = " << std::endl;
        for (int j = 0; j < data_size; j++) {
            if (source_hw_results[i][j] == (~ap_int<512>(0))) break;
            std::cout << std::hex << std::setfill('0') << std::setw(128) << (ap_uint<512>)source_hw_results[i][j] << std::endl;
        }
        std::cout << std::endl;
    }

    return EXIT_SUCCESS;
}
