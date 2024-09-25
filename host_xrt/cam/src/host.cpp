#include "cmdlineparser.h"
#include <iostream>
#include <cstring>
#include <ap_int.h>

// XRT includes
#include "experimental/xrt_bo.h"
#include "experimental/xrt_device.h"
#include "experimental/xrt_kernel.h"

#define key_num 2
#define val_num 80

short int countones(ap_uint<48> x) {
    short int count = 0;
    while (x) {
        count += x & 1;
        x >>= 1;
    }
    return count;
}

int main(int argc, char** argv) {
    // Command Line Parser
    sda::utils::CmdLineParser parser;

    // Switches
    //**************//"<Full Arg>",  "<Short Arg>", "<Description>", "<Default>"
    parser.addSwitch("--xclbin_file", "-x", "input binary file string", "");
    parser.addSwitch("--device_id", "-d", "device index", "0");
    parser.parse(argc, argv);

    // Read settings
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

        auto krnl = xrt::kernel(device, uuid, "cam");

    std::cout << "Allocate Buffer in Global Memory\n";
    xrt::bo bo_key;
    xrt::bo bo_val;
    xrt::bo bo_out;

//word parallelism
    size_t key_bytes = 8 * key_num;
    size_t val_bytes = 8 * val_num;
    size_t out_bytes = 8 * key_num * val_num;

    bo_key = xrt::bo(device, key_bytes, krnl.group_id(0));
    bo_val = xrt::bo(device, val_bytes, krnl.group_id(1));
    bo_out = xrt::bo(device, out_bytes, krnl.group_id(2));

    auto bo_key_map = bo_key.map<ap_uint<48>*>();
    auto bo_val_map = bo_val.map<ap_uint<48>*>();
    auto bo_out_map = bo_out.map<ap_uint<48>*>();

    ap_uint<48> bufReference[key_num * val_num];

    bo_key_map[0] = 0x000000000000;
    bo_key_map[1] = 0x100000000010;
    for (int i = 0; i < val_num; i++) {
        bo_val_map[i] = i;
    }
    for (int i = 0; i < key_num; i++) {
        for (int j = 0; j < val_num; j++) {
            bufReference[i*val_num + j] = bo_key_map[i] - bo_val_map[j];
            //bufReference[i*val_num + j] = countones(bo_key_map[i] ^ bo_val_map[j]);
        }
    }

    xrt::run run;
    std::cout << "synchronize input buffer data to device global memory\n";
    bo_key.sync(XCL_BO_SYNC_BO_TO_DEVICE);
    bo_val.sync(XCL_BO_SYNC_BO_TO_DEVICE);

    std::cout << "Executing Kernel\n";
    run = krnl(bo_key, bo_val, bo_out);

    run.wait();

    std::cout << "Get the output data from the device" << std::endl;
    bo_out.sync(XCL_BO_SYNC_BO_FROM_DEVICE);
    // for (int i = 0; i < key_num * val_num; i++) {
    //     std::cout << "Output[" << i << "]: " << bo_out_map[i] << std::endl;
    // }
    // for (int i = 0; i < key_num * val_num; i++) {
    //     std::cout << "Reference[" << i << "]: " << bufReference[i] << std::endl;
    // }

    for (int i = 0; i < key_num * val_num; i++) {
        if (bo_out_map[i] != bufReference[i]) {
            std::cout << "Test failed" << std::endl;
            std::cout << "i = " << i << " Expected: " << bufReference[i] << " Got: " << bo_out_map[i] << std::endl;
            return EXIT_FAILURE;
        }
    }
    std::cout << "Test passed" << std::endl;
    return 0;
}