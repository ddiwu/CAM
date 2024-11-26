# Configurable DSP-Based CAM Architecture Project

## Overview

This project implements a **Configurable DSP-Based CAM Architecture** for FPGA platforms. The architecture is designed for **scalability**, **efficiency**, and **multi-query support**, targeting applications like **graph analytics** and **databases**.

## Features
- **Hierarchical Design**:
  - **CAM Cell**: Utilizes FPGA DSP slices for efficient storage and comparison operations.
  - **CAM Block**: Supports search and update operations.
  - **CAM Unit**: Manages multiple CAM blocks with routing for large-scale applications.
- **Scalability**: Parameterized to adapt as many DSPs as possible.
- **Efficiency**: Optimized for both search efficiency and update efficiency.
- **Multi-query Support**: Handles concurrent searches for high throughput and high memory resource utilization.
- **Real-world Applications**: Suitable for graph-based problems like triangle counting.

---

## Architecture

### Project Structure

Each folder in this project includes:
- **`param.cfg`**: Configuration parameters for the module.
- **`configure.sh`**: A script to generate source files based on parameters.
- **Core Sources**: Template files for modules and configuration generation.

### Detailed Design
![CAM unit architecture design](./figure/CAM_Unit.png "CAM unit architecture design")

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
Compile the project for the target platform (e.g., software emulation, hardware emulation, or hardware). Replace TARGET with the desired target and specify your platform. We strongly recommend using the Vitis tool (2021.2 or later) to run the application on AMD U280 platform.
```bash
make TARGET=<sw_emu/hw_emu/hw> PLATFORM=<FPGA platform>
``` 

### Step 3: Running the Application
Run the application on the specified target to simulate or test the functionality. We strongly recommend using the [HACC cluster](https://www.amd-haccs.io/) to run the application. 
```bash
./executable -x xclbin_file
```

## Contribution Guide

### How to Contribute
- Fork this repository.
- Implement new features or optimize existing ones.
- Submit a pull request following the CONTRIBUTING.md guidelines.
