import os


def replace_macros_in_file(input_file_path, output_dir, replacements):
    """
    直接查找并替换文件中的宏，并将修改后的内容保存到目标目录。

    :param input_file_path: 源文件路径
    :param output_dir: 目标目录路径
    :param replacements: 字典，键为要替换的宏，值为替换后的数值或字符串
    """
    try:
        # 读取文件内容
        with open(input_file_path, 'r') as file:
            content = file.read()
    except FileNotFoundError:
        print(f"错误: 找不到文件 '{input_file_path}'。")
        return
    except Exception as e:
        print(f"错误: 读取文件 '{input_file_path}' 时发生异常: {e}")
        return

    # 替换宏
    for macro, value in replacements.items():
        if macro in content:
            print(f"在文件 '{input_file_path}' 中找到宏 '{macro}'，替换为 '{value}'。")
        content = content.replace(macro, str(value))

    # 准备输出文件路径
    file_name = os.path.basename(input_file_path)
    output_file_path = os.path.join(output_dir, file_name)

    # 确保输出目录存在
    os.makedirs(output_dir, exist_ok=True)

    # 写入替换后的内容
    try:
        with open(output_file_path, 'w') as file:
            file.write(content)
        print(f"已将修改后的文件保存到 '{output_file_path}'。\n")
    except Exception as e:
        print(f"错误: 写入文件 '{output_file_path}' 时发生异常: {e}")


if __name__ == "__main__":
    # 定义源文件路径和目标目录
    source_files = {
        "./template/krnl_cam_rtl.v": {"CUSTORMIZED_BUS_WIDTH": "512"},
        "./template/krnl_cam_rtl_int.sv": {"CUSTORMIZED_CAM_SIZE": "256"},
        "./template/krnl_cam_rtl_cam_pre_unpipe.sv": {
            "CUSTORMIZED_STORAGE_TYPE": "BINARY",
            "CUSTORMIZED_MASK": "48'h0"
        }
    }
    target_directory = "./src/hdl"

    # 遍历每个源文件并进行替换
    for src_file, macros in source_files.items():
        print(f"处理文件: {src_file}")
        replace_macros_in_file(src_file, target_directory, macros)

    print("所有替换操作已完成。")
