import os
import re


def modify_config_file(template_file, output_file, customized_block_num):
    """
    Modify the config file template to replace placeholders like {{STREAM_CONNECT_OUTPUTS}} 
    with dynamically generated `stream_connect` lines for all outputs.
    """
    # Read the template file
    with open(template_file, "r") as f:
        template = f.read()

    # Generate dynamic stream_connect lines for pre_write outputs
    stream_connect_outputs = "\n".join(
        [f"stream_connect=pre_write_1.out_{i + 1}:mem_write_{i + 1}.stream" for i in range(customized_block_num)]
    )

    # Replace placeholders in the template
    cfg_content = template.replace("{{STREAM_CONNECT_OUTPUTS}}", stream_connect_outputs) \
                          .replace("CUSTOMIZED_BLOCK_NUM", str(customized_block_num))

    # Write the modified content to the output file
    with open(output_file, "w") as f:
        f.write(cfg_content)

    print(f"Configuration file '{output_file}' has been updated with {customized_block_num} output streams.")


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
        [f"                if (i == {i}) out_{i + 1}.write(out_packet);" for i in range(customized_block_num)]
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
    # Configuration for the script
    input_cfg = "krnl_incr.cfg"  # Input configuration file
    output_cfg = "krnl_backup.cfg"  # Output modified configuration file
    folder_path = "./src"  # Path to the source folder containing .cpp and .h files
    template_file = "./pre_write_template.cpp"  # Template file for the kernel
    output_kernel_file = "./src/pre_write_backup.cpp"  # Generated kernel file
    customized_block_num = 16  # New value for CUSTOMIZED_BLOCK_NUM
    customized_block_size = 128  # New value for CUSTOMIZED_BLOCK_SIZE

    # Step 1: Modify the config file
    modify_config_file(input_cfg, output_cfg, customized_block_num)

    # Step 2: Generate the kernel code from the template
    generate_kernel_from_template(template_file, output_kernel_file, customized_block_num)

    # Step 3: Update macro values in source files
    replace_macro_values(folder_path, customized_block_num, customized_block_size)

