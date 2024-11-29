/*******************************************************************************
Description:
    Host code for simplified kernel operation: SEARCH_MQ.
*******************************************************************************/

#include "cmdlineparser.h"
#include <iostream>
#include <cstring>
#include <ap_int.h>
#include <iomanip>
#include <random>

// XRT includes
#include "experimental/xrt_bo.h"
#include "experimental/xrt_device.h"
#include "experimental/xrt_kernel.h"

#define UPDATE_ALL 0xffffff01
#define SEARCH_MQ 0xffffff04

#define CUSTOMIZED_BLOCK_NUM 16
#define CUSTOMIZED_BLOCK_SIZE 128

int main(int argc, char** argv) {

    // Random number engine (Mersenne Twister), used for generate the search key.
    std::mt19937 engine(std::random_device{}()); // Seed the engine
    // Distribution for random integers in the range [0, 255]
    std::uniform_int_distribution<int> dist(0, 255);

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

    // Calculate data size
    int data_size = (CUSTOMIZED_BLOCK_NUM - 1) / 16 + 1; // Update routing table data
    data_size += (1 + ((CUSTOMIZED_BLOCK_NUM - 1) / 16 + 1)) * 32; // (SEARCH_MQ and query data) * 32 times
    std::cout << "data_size = " << data_size << std::endl;
    int vector_size_bytes = sizeof(ap_int<512>) * data_size;

    // Create input buffer
    auto mem_read = xrt::kernel(device, uuid, "mem_read");
    std::vector<xrt::kernel> mem_write_kernels(CUSTOMIZED_BLOCK_NUM);
    for (int i = 0; i < CUSTOMIZED_BLOCK_NUM; i++) {
        mem_write_kernels[i] = xrt::kernel(device, uuid, ("mem_write:{mem_write_" + std::to_string(i + 1)) + "}");
    }
    auto buffer_input = xrt::bo(device, vector_size_bytes, mem_read.group_id(0));
    auto source_input = buffer_input.map<ap_int<512>*>();

    // Create output buffers for mem_write
    std::vector<xrt::bo> buffer_output(CUSTOMIZED_BLOCK_NUM);
    std::vector<ap_uint<512>*> source_output(CUSTOMIZED_BLOCK_NUM);
    for (int i = 0; i < CUSTOMIZED_BLOCK_NUM; i++) {
        buffer_output[i] = xrt::bo(device, vector_size_bytes, mem_write_kernels[i].group_id(0));
        source_output[i] = buffer_output[i].map<ap_uint<512>*>();
    }

    // Initialize source_input
    int idx = 0;

    // Step 1: UPDATE routing table entries directly, the block number should be less than 16
    for (int j = 0; j < 16; j++) {
        source_input[idx].range(32 * j + 31, 32 * j) = j / 2; 
        /*the routing table shuold be 0 0 1 1 2 2 3 3 ..., which means that the group number is 8*/
    }
    idx++;


    // Step 2: SEARCH_MQ Command * 32 times
    for (int k = 0; k < 32; k++) {
        source_input[idx].range(511, 480) = SEARCH_MQ; // Command
        idx++;
        for (int i = 0; i < 16; i++) {
            source_input[idx].range(32 * i + 31, 32 * i) = dist(engine); // generate the random search key, value range is 0-255
        }
        idx++;
        /*Note that the search keys is 0 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15, but actually only
        the 0-7 can be searched since the group number is 8, according to the routing table. */
    }

    // Initialize output buffers
    for (int i = 0; i < CUSTOMIZED_BLOCK_NUM; i++) {
        for (int j = 0; j < data_size; j++) {
            source_output[i][j] = 0x0;
        }
    }

    // Print source_input
    for (int i = 0; i < data_size; i++) {
        std::cout << "source_input[" << i << "] = ";
        for (int chunk = 0; chunk < 4; chunk++) {
            ap_uint<128> chunk_data = source_input[i].range(511 - chunk * 128, 384 - chunk * 128);
            std::cout << std::noshowbase << std::hex << std::setfill('0') << std::setw(32) << chunk_data << " ";
        }
        std::cout << std::endl;
    }

    // Copy input data to device
    std::cout << "Copying data to device..." << std::endl;
    buffer_input.sync(XCL_BO_SYNC_BO_TO_DEVICE);

    std::cout << "Launching mem_write kernels..." << std::endl;

    // Launch mem_write kernels
    std::vector<xrt::run> write_run(CUSTOMIZED_BLOCK_NUM);
    for (int i = 0; i < CUSTOMIZED_BLOCK_NUM; i++) {
        write_run[i] = xrt::run(mem_write_kernels[i]);
        write_run[i].set_arg(0, buffer_output[i]);
        write_run[i].start();
    }

    std::cout << "Launching mem_read kernel..." << std::endl;
    auto read_run = xrt::run(mem_read);
    read_run.set_arg(0, buffer_input);
    read_run.set_arg(1, data_size);
    read_run.start();

    // Wait for completion
    std::cout << "Waiting for completion..." << std::endl;
    read_run.wait();
    for (int i = 0; i < CUSTOMIZED_BLOCK_NUM; i++) {
        write_run[i].wait();
    }

    // Copy results from device
    std::cout << "Copying results from device..." << std::endl;
    for (int i = 0; i < CUSTOMIZED_BLOCK_NUM; i++) {
        buffer_output[i].sync(XCL_BO_SYNC_BO_FROM_DEVICE);
    }

    // Validate results
    std::cout << "Validating results..." << std::endl;
    for (int i = 0; i < CUSTOMIZED_BLOCK_NUM; i++) {
        std::cout << "source_output[" << i << "] = " << std::endl;
        for (int j = 0; j < data_size; j++) {
            std::cout << std::hex << std::setfill('0') << std::setw(128) << source_output[i][j] << std::endl;
        }
        std::cout << std::endl;
    }

    std::cout << "Execution completed successfully!" << std::endl;

    // Print Routing Table: Block ID -> Group ID
    std::cout << "\nRouting Table (Block ID -> Group ID):" << std::endl;
    for (int block_id = 0; block_id < 16; block_id++) {
        int group_id = source_input[0].range(32 * block_id + 31, 32 * block_id);
        std::cout << "Group ID " << group_id << " -> Block ID " << block_id << std::endl;
    }

    // 1. Print Search Keys and Associated Results for Each SEARCH_MQ Request
    std::cout << "\nProcessing SEARCH_MQ Requests:" << std::endl;
    int index = 1; // Start with the second data entry in the source_input, first data entry is the routing table
    int num_groups = CUSTOMIZED_BLOCK_NUM / 2; // Example logic, assuming 2 blocks per group

    for (int search_idx = 0; search_idx < 1; search_idx++) {
        // Ensure the current state indicates a SEARCH_MQ request
        ap_uint<32> state = source_input[index].range(511, 480);
        if (state != SEARCH_MQ) {
            std::cerr << "Error: Expected SEARCH_MQ state but found " << state << " at index " << index << std::endl;
            break;
        }
        // Move to the next data entry for search keys
        index++;
        
        // 1.1 Print Search Keys for Each Group (only 8 data entries for search keys)
        std::cout << "  Group ID -> Search Key:" << std::endl;
        for (int group_id = 0; group_id < num_groups; group_id++) {
            int search_key = source_input[index].range(32 * group_id + 31, 32 * group_id);
            std::cout << "    Group " << group_id << " -> Search Key: " << search_key << std::endl;
        }

        // 1.2 Print Search Keys in Each Block (only 8 data entries for search results)
        std::cout << "  Search Keys in Each Block:" << std::endl;
        for (int block_id = 0; block_id < CUSTOMIZED_BLOCK_NUM; block_id++) {
            std::cout << "    Block ID " << block_id << " -> accepted key: ";
            int value = source_output[block_id][1].range(31, 0); // Extract search key
            std::cout << "[Key: " << value << "] " << std::endl;
        }
        // Move to the next SEARCH_MQ state
        index++;
    }

    return EXIT_SUCCESS;
}
