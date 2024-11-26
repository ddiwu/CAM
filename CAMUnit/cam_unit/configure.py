import os
import argparse
import re


def replace_macros_in_file(input_file_path, output_dir, replacements):
    """
    Search for and replace macros in a file, then save the modified content to the target directory.

    :param input_file_path: Path to the source file
    :param output_dir: Path to the target directory
    :param replacements: Dictionary where keys are macros to be replaced, and values are the new values or strings
    """
    try:
        # Read the file content
        with open(input_file_path, 'r') as file:
            content = file.read()
    except FileNotFoundError:
        print(f"Error: File '{input_file_path}' not found.")
        return
    except Exception as e:
        print(f"Error: An exception occurred while reading the file '{input_file_path}': {e}")
        return

    # Replace macros in the content
    for macro, value in replacements.items():
        if macro in content:
            print(f"Macro '{macro}' found in file '{input_file_path}', replacing with '{value}'.")
        content = content.replace(macro, str(value))

    # Prepare the output file path
    file_name = os.path.basename(input_file_path)
    output_file_path = os.path.join(output_dir, file_name)

    # Ensure the output directory exists
    os.makedirs(output_dir, exist_ok=True)

    # Write the modified content to the output file
    try:
        with open(output_file_path, 'w') as file:
            file.write(content)
        print(f"Modified file saved to '{output_file_path}'.\n")
    except Exception as e:
        print(f"Error: An exception occurred while writing to the file '{output_file_path}': {e}")


def modify_config_file(template_file, output_file, customized_block_num):
    """
    Modify the config file template to replace placeholders like {{STREAM_CONNECT_OUTPUTS}} 
    with dynamically generated `stream_connect` lines for all outputs.
    """
    # Read the template file
    with open(template_file, "r") as f:
        template = f.read()

    # Generate dynamic stream_connect lines for post_router outputs
    stream_connect_outputs = "\n".join(
        [f"stream_connect=post_router_1.out_{i + 1}:krnl_cam_rtl_{i + 1}.p0" for i in range(customized_block_num)]
    )

    # Replace placeholders in the template
    cfg_content = template.replace("{{STREAM_CONNECT_OUTPUTS}}", stream_connect_outputs) \
                          .replace("CUSTOMIZED_BLOCK_NUM", str(customized_block_num))

    # Ensure the directory for the output file exists
    output_dir = os.path.dirname(output_file)
    if output_dir and not os.path.exists(output_dir):
        os.makedirs(output_dir)
        print(f"Directory '{output_dir}' has been created.")

    # Write the modified content to the output file
    with open(output_file, "w") as f:
        f.write(cfg_content)

    print(f"Configuration file '{output_file}' has been created and updated with {customized_block_num} output streams.")


def replace_macro_values(folder_path, customized_block_num, customized_block_size):
    """
    Update macro definitions in all .cpp and .h files in a folder.
    """
    # Calculate CUSTOMIZED_ROUTING_BITS
    customized_routing_bits = 32 * customized_block_num

    # Define the patterns to search and their replacements
    macro_patterns = {
        r"#define\s+CUSTOMIZED_BLOCK_NUM\s+\d+": f"#define CUSTOMIZED_BLOCK_NUM {customized_block_num}",
        r"#define\s+CUSTOMIZED_BLOCK_SIZE\s+\d+": f"#define CUSTOMIZED_BLOCK_SIZE {customized_block_size}",
        r"#define\s+CUSTOMIZED_ROUTING_BITS\s+[^\n]+": f"#define CUSTOMIZED_ROUTING_BITS {customized_routing_bits}"
    }

    # Scan all .cpp and .h files in the folder
    for root, _, files in os.walk(folder_path):
        for file in files:
            if file.endswith(".cpp") or file.endswith(".h"):
                file_path = os.path.join(root, file)
                with open(file_path, "r") as f:
                    content = f.read()

                # Replace the macros in the file content
                modified_content = content
                for pattern, replacement in macro_patterns.items():
                    modified_content = re.sub(pattern, replacement, modified_content)

                # Write the modified content back to the file
                with open(file_path, "w") as f:
                    f.write(modified_content)
                print(f"Updated macros in file: {file_path}")


def generate_kernel_from_template(template_file, output_file, customized_block_num):
    """
    Generate kernel code from template with specified number of output streams.
    """
    # Read the template file
    with open(template_file, "r") as f:
        template = f.read()

    # Generate output stream declarations
    output_stream_declarations = ",\n               ".join(
        [f"hls::stream<ap_axiu<512, 0, 0, 0>>& out_{i + 1}" for i in range(customized_block_num)]
    )

    # Generate pragma declarations for output streams
    pragma_declarations = "\n".join(
        [f"#pragma HLS interface axis port=out_{i + 1}" for i in range(customized_block_num)]
    )

    # Generate output write logic for each stream
    output_write_logic = "\n".join(
        [f"                if (dest.range({i}, {i})) out_{i + 1}.write(out_packet);" for i in range(customized_block_num)]
    )

    # Generate logic to send end-of-stream packet to all output streams
    end_stream_logic = "\n    ".join(
        [f"out_{i + 1}.write(out_packet);" for i in range(customized_block_num)]
    )

    # Replace placeholders in the template
    kernel_code = template.replace("{{OUTPUT_STREAM_DECLARATIONS}}", output_stream_declarations) \
                           .replace("{{PRAGMA_DECLARATIONS}}", pragma_declarations) \
                           .replace("{{OUTPUT_WRITE_LOGIC}}", output_write_logic) \
                           .replace("{{END_STREAM_LOGIC}}", end_stream_logic)

    # Write the generated kernel code to the output file
    with open(output_file, "w") as f:
        f.write(kernel_code)

    print(f"Kernel code with {customized_block_num} output streams has been generated in '{output_file}'.")


if __name__ == "__main__":
    # Command-line arguments
    parser = argparse.ArgumentParser(description="Configure CAM unit files with custom parameters.")
    parser.add_argument("--bus_width", type=str, required=True, help="Customized bus width.")
    parser.add_argument("--cam_size", type=str, required=True, help="Customized CAM size.")
    parser.add_argument("--storage_type", type=str, required=True, help="Customized storage type.")
    parser.add_argument("--mask", type=str, required=True, help="Customized mask.")
    parser.add_argument("--block_num", type=int, required=True, help="Number of output streams.")
    parser.add_argument("--block_size", type=int, required=True, help="Block size.")
    args = parser.parse_args()

    # Paths and configurations
    template_cfg_file = "./cam_unit_template.cfg"
    output_cfg_file = "./unit.cfg"
    kernel_template_file = "./src/tmp_template/post_router_template.cpp"
    output_kernel_file = "./src/post_router.cpp"
    source_folder = "./src"
    target_directory = "./src/hdl"

    # Step 1: Modify the config file
    modify_config_file(template_cfg_file, output_cfg_file, args.block_num)

    # Step 2: Generate the kernel code from the template
    generate_kernel_from_template(kernel_template_file, output_kernel_file, args.block_num)

    # Step 3: Replace macros in HDL source files
    source_files = {
        "./src/tmp_template/krnl_cam_rtl.v": {"CUSTORMIZED_BUS_WIDTH": args.bus_width},
        "./src/tmp_template/krnl_cam_rtl_int.sv": {"CUSTORMIZED_CAM_SIZE": args.cam_size},
        "./src/tmp_template/krnl_cam_rtl_cam_pre_unpipe.sv": {
            "CUSTORMIZED_STORAGE_TYPE": args.storage_type,
            "CUSTORMIZED_MASK": args.mask
        }
    }

    # Replace macros in each source file
    for src_file, macros in source_files.items():
        print(f"Processing file: {src_file}")
        replace_macros_in_file(src_file, target_directory, macros)

    # Step 4: Update macros in source files
    replace_macro_values(source_folder, args.block_num, args.block_size)

    print("All configuration steps have been completed.")
