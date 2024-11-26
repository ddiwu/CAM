import os
import argparse


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


if __name__ == "__main__":
    # Command-line arguments
    parser = argparse.ArgumentParser(description="Replace macros in CAM block template files with custom values.")
    parser.add_argument("--bus_width", type=str, required=True, help="Customized bus width.")
    parser.add_argument("--cam_size", type=str, required=True, help="Customized CAM size.")
    parser.add_argument("--storage_type", type=str, required=True, help="Customized storage type.")
    parser.add_argument("--mask", type=str, required=True, help="Customized mask.")

    args = parser.parse_args()

    # Define replacements for each file
    source_files = {
        "./template/krnl_cam_rtl.v": {"CUSTORMIZED_BUS_WIDTH": args.bus_width},
        "./template/krnl_cam_rtl_int.sv": {"CUSTORMIZED_CAM_SIZE": args.cam_size},
        "./template/krnl_cam_rtl_cam_pre_unpipe.sv": {
            "CUSTORMIZED_STORAGE_TYPE": args.storage_type,
            "CUSTORMIZED_MASK": args.mask
        }
    }

    # Target directory for the modified files
    target_directory = "./src/hdl"

    # Process each source file and perform replacements
    for src_file, macros in source_files.items():
        print(f"Processing file: {src_file}")
        replace_macros_in_file(src_file, target_directory, macros)

    print("All replacements have been completed.")
