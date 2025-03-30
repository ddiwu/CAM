# Configurable DSP-Based CAM on FPGAs

## Overview

This project implements a **Configurable DSP-Based CAM Architecture** for FPGA platforms. The architecture is designed for **scalability**, **efficiency**, and **multi-query support**, targeting applications like **graph analytics** and **databases**.

---

## Features
- **Hierarchical Design**:
  - **CAM Cell**: Utilizes FPGA DSP slices for efficient storage and comparison operations.
  - **CAM Block**: Supports search and update operations across a number of CAM cells.
  - **CAM Unit**: Manages multiple CAM blocks with routing for larger space and concurrent search.
- **Scalability**: Parameterized to adapt as many DSPs as possible, scale across chip scale.
- **Efficiency**: Optimized for both search efficiency and update efficiency.
- **Multi-query Support**: Handles concurrent searches for high throughput and high memory resource utilization.
- **Real-world Applications**: Suitable for graph-based problems like triangle counting.

---

## Architecture

### Project Structure

Each folder in this project includes:
- **`param.cfg`**: Contains the configurable parameters for the microarchitectures.
- **`configure.sh`**: A script to generate source files based on parameters.
- **Core Sources**: Template files for modules and configuration generation.

### Detailed Design
![CAM unit architecture design](./figure/CAM_Unit.png "CAM unit architecture design")

### Implementation layout on FPGA board
![Implementation layout on FPGA board](./figure/resource_layout.png "Implementation layout on FPGA board")

## How to Use The CAM

### Step 1: Customizing Parameters
Update the `param.cfg` file to set the desired values for the CAM architecture. This file allows you to define key parameters such as the block size, the numbe of blocks, and other configuration options.

**Example `param.cfg`:**
```cfg
CUSTOMIZED_BLOCK_NUM=16      # Number of CAM blocks
CUSTOMIZED_BLOCK_SIZE=128    # Size of each CAM block
CAM_CELL_TYPE="Binary"       # Type of CAM cell (e.g., Binary, Ternary)
ROUTING_BITS=512             # Number of routing bits
```
After updating the parameters, run the configuration script:
```bash
./configure.sh
```

### Step 2: Compilation
Compile the project for the target platform (e.g., software emulation, hardware emulation, or hardware). Replace TARGET with the desired target and specify your platform. We strongly recommend using the Vitis tool (2021.2 or later) to implement the application on AMD U280 platform.
```bash
make TARGET=<sw_emu/hw_emu/hw> PLATFORM=<FPGA platform>
``` 

### Step 3: Running the Application
Run the application on the specified target to simulate or test the functionality. We strongly recommend using the [HACC cluster](https://www.amd-haccs.io/) to run the application. 
```bash
./executable -x xclbin_file
```

you will see the output results like this:
```bash
Mapping input buffer
Mapping output buffer
Copying input data to device global memory
Launching RTL kernel
Copying output data to device global memory
Comparing results
i = 0 Software result = 1 Device result = 1
i = 1 Software result = 1 Device result = 1
i = 2 Software result = 1 Device result = 1
i = 3 Software result = 0 Device result = 0
i = 4 Software result = 0 Device result = 0
i = 5 Software result = 0 Device result = 0
TEST PASSED
```

---
## An Module Test Example for Multi-Query Functionality of CAM Design

This module test demonstrates the **multi-query feature** of a CAM (Content Addressable Memory) design on an FPGA. The test evaluates the ability of the CAM to process multiple search keys in parallel across a configurable number of groups and CAM blocks, showcasing its flexibility and high throughput.

### **Multi-Query Example**

The example demonstrates the **multi-query feature**, located in the `./Example_Search_MQ` folder, where:
1. A **routing table** maps 16 CAM blocks (Block IDs) to 8 groups (Group IDs).
2. A single query (`SEARCH_MQ`) contains 8 search keys, one for each group.
3. The system performs **8 queries** simultaneously across the 16 CAM blocks to demonstrate high parallelism.

### **Details**
1. The **search key** is generated using random numbers for each query.
2. The other states in the state machine are reduced for simplicity to focus solely on the **multi-query functionality**.
3. Although the bus can transmit **16 search keys in a single cycle**, only the first 8 search keys are processed since the number of groups is set to 8. The **multi-query number = group_num** ensures the masked keys are ignored.


### **How to Run the Example**

0. **Set up the environment**:
  ```bash
  source /opt/xilinx/xrt/setup.sh
  source /YOUR_PATH/Vitis/2022.2/settings64.sh ## please change the path to your Vitis installation path
  ```
1. **Navigate to the Example Directory**:
  ```bash
   cd ./Example_Search_MQ && make all TARGET=sw_emu PLATFORM=/opt/xilinx/platforms/xilinx_u55c_gen3x16_xdma_3_202210_1/xilinx_u55c_gen3x16_xdma_3_202210_1.xpfm 
  ```

2. **Run the Example in Software Emulation Mode:**:
  ```bash
  XCL_EMULATION_MODE=sw_emu ./search_mq -x ./search_mq.xclbin
  ```

### **Observe the Output**: 
If the execution is successful, you will see results which demonstarates that the search keys are correctly routed to the corresponding CAM blocks. Like the following:
```bash
Execution completed successfully!

Routing Table (Block ID -> Group ID):
Group ID 0 -> Block ID 0
Group ID 0 -> Block ID 1
......
Group ID 7 -> Block ID e
Group ID 7 -> Block ID f

Processing SEARCH_MQ Requests:
Group ID -> Search Key:
  Group 0 -> Search Key: 78
  Group 1 -> Search Key: 3a
  Group 2 -> Search Key: 5a
  Group 3 -> Search Key: 9
  Group 4 -> Search Key: d3
  Group 5 -> Search Key: 5d
  Group 6 -> Search Key: b6
  Group 7 -> Search Key: 61
Search Keys in Each Block:
  Block ID 0 -> accepted key: [Key: 78] 
  Block ID 1 -> accepted key: [Key: 78] 
  Block ID 2 -> accepted key: [Key: 3a] 
  Block ID 3 -> accepted key: [Key: 3a] 
  Block ID 4 -> accepted key: [Key: 5a] 
  Block ID 5 -> accepted key: [Key: 5a] 
  Block ID 6 -> accepted key: [Key: 9] 
  Block ID 7 -> accepted key: [Key: 9] 
  Block ID 8 -> accepted key: [Key: d3] 
  Block ID 9 -> accepted key: [Key: d3] 
  Block ID a -> accepted key: [Key: 5d] 
  Block ID b -> accepted key: [Key: 5d] 
  Block ID c -> accepted key: [Key: b6] 
  Block ID d -> accepted key: [Key: b6] 
  Block ID e -> accepted key: [Key: 61]
```

### **Run in Hardware Emulation Mode (Optional):**
You can also run the example in hw_emu mode to observe the waveform.
Follow the same steps as above but replace sw_emu with hw_emu during the compilation and execution.


---
## Triangle Counting on FPGA
This example demonstrates triangle counting on FPGA using CAM (Content Addressable Memory). The example utilizes an edge-centric triangle counting algorithm, where the adjacency lists of the source and destination vertices are analyzed to compute the number of triangles in the graph.

### **Functionality**

- **Adjacency List Extraction**: For each edge in the graph, retrieve the adjacency lists of the source and destination vertices.

- **Intersection Computation**: Calculate the intersection of these two adjacency lists to identify common neighbors, which form the third vertex of a triangle.

- **CAM-Based Acceleration**:
  - The adjacency list of the source vertex is stored in the CAM.
  - A multi-query search is performed on the CAM using items from the adjacency list of the destination vertex as search keys.
  - The results of the multi-query search provide the intersection of the two adjacency lists.

This CAM-based implementation provides high performance by leveraging the parallel processing capabilities of CAM. The source files of this implementation can be found in the `./TriangleCount` folder.

### **How to Run the Example**

0. **Set up the environment && graph datasets**:
  ```bash
  source /opt/xilinx/xrt/setup.sh
  source /YOUR_PATH/Vitis/2021.2/settings64.sh ## please change the path to your Vitis installation path

  ## copy  your graph datasets to the ./TriangleCount/dataset/ directory
  cd ./TriangleCount/dataset/  
  ## make sure the graph datasets have three files: edge_list.txt, csr_col.txt, csr_row_2.txt.
  ## (The dataset is generated from the original edgelist file, please refer to the './dataset/partition_tc.py' for the generation of the graph datasets. Make sure you execute the partitioning if the dataset does not include the three files.)
  ```
1. **Navigate to the Example Directory**:
  ```bash
   cd ./TriangleCount && make all TARGET=hw_emu PLATFORM=/opt/xilinx/platforms/xilinx_u250_gen3x16_xdma_3_1_202020_1/xilinx_u250_gen3x16_xdma_3_1_202020_1.xpfm 
  ```

2. **Run the Example in Hardware Emulation Mode:**:
  ```bash
  XCL_EMULATION_MODE=hw_emu ./triangle_count -x ./triangle_count.xclbin
  ```

### **Observe the Output**: 
If the execution is successful, you will see results which demonstarates that the triangle counting is correctly performed. Like the following (taking the test graph dataset as an example):
```bash
col_idx length: 82
row_ptr length: 42
edge_list length: 82 edges (164 elements)
Copying data to device...
Launching kernels...
Waiting for completion...
TC kernel completed!
Mem write kernels completed!
Copying results from device...
Execution completed successfully!
Triangle count: 155
```

### **Run in Hardware Mode:**
You can also run the example in hw mode on Xilinx U250 board.
Follow the same steps as above but replace hw_emu with hw during the compilation and execution.

```bash
# for compiling the xclbin file
make TARGET=hw PLATFORM=/opt/xilinx/platforms/xilinx_u250_gen3x16_xdma_3_1_202020_1/xilinx_u250_gen3x16_xdma_3_1_202020_1.xpfm 

# for running the xclbin file
./triangle_count -x ./build_dir.hw.xilinx_u250_gen3x16_xdma_3_1_202020_1/triangle_count.xclbin -s ./dataset/as20000102
```

### **Observe the Output**: 
If the execution is successful, you will see results which demonstarates that the triangle counting is correctly performed. Like the following (taking the as20000102 graph dataset as an example):
```bash
Open the device0
Load the xclbin ./build_dir.hw.xilinx_u250_gen3x16_xdma_3_1_202020_1/triangle_count.xclbin
Kernel UUID: 1
col_idx length: 12536
row_ptr length: 12948
edge_list length: 11066 edges (22132 elements)
Copying data to device...
Launching kernels...
TriangleCount completed, execution time: 422 us
Copying results from device...
Execution completed successfully!
Triangle count: 6584
```
Here are the lastest experimental results.

| Dataset              | Triangles    | Our Solution (CAM) (ms) | Baseline (ms) | Speedup |
|----------------------|-------------|-------------------------|--------------|---------|
| facebook_combined   | 1,612,010    | 5.054                   | 18.7         | 3.70x   |
| amazon0302         | 717,719      | 23.086                  | 89.5         | 3.88x   |
| amazon0601         | 3,986,507    | 71.210                  | 230.3        | 3.23x   |
| as20000102        | 6,584        | 0.422                   | 7.4          | 17.54x  |
| cit-Patents       | 7,515,023    | 415.808                 | 800          | 1.92x   |
| ca-cit-HepPh      | 195,758,685  | 1,526.05                | 5,361.1      | 3.51x   |
| roadNet-CA        | 120,676      | 62.058                  | 108.8        | 1.75x   |
| roadNet-PA        | 67,150       | 34.559                  | 88.7         | 2.57x   |
| roadNet-TX        | 82,869       | 42.323                  | 96.8         | 2.29x   |
| soc-Slashdot0811  | 551,724      | 29.402                  | 259.7        | 8.83x   |

## Notes:
- Baselines were obtained by re-implementing the official Triangle Counting accelerator design from AMD Vitis Library on our experimental platform.
- Speedup values compare our CAM-based solution against the baseline implementation.
- Detailed resource utilization reports and results comparisons are available in:  
  `CAM/TriangleCount/results/`
---
## Contribution Guide

### How to Contribute
- Fork this repository.
- Implement new features or optimize existing ones.
- Submit a pull request following the CONTRIBUTING.md guidelines.
