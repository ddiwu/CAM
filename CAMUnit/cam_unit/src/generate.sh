#!/bin/bash

# Define the source directories and the target tmp_src directory
TMP_SRC_DIR="./src"
TMP_HDL_DIR="./src/hdl"
TMP_TEMPLATE_DIR="./src/tmp_template"

if [ -d "$TMP_HDL_DIR" ]; then
    echo "Removing existing temporary source directory: $TMP_HDL_DIR"
    rm -rf "$TMP_HDL_DIR"
fi

if [ -d "$TMP_TEMPLATE_DIR" ]; then
    echo "Removing existing temporary source directory: $TMP_TEMPLATE_DIR"
    rm -rf "$TMP_TEMPLATE_DIR"
fi

# Ensure the tmp_src directory exists
echo "Creating temporary template directory: $TMP_HDL_DIR"
mkdir -p "$TMP_HDL_DIR"

echo "Creating temporary template directory: $TMP_TEMPLATE_DIR"
mkdir -p "$TMP_TEMPLATE_DIR"

# Copy source files to the tmp_src directory
echo "Copying source files to $TMP_SRC_DIR..."
cp -r "../cam_block/template/"* "$TMP_TEMPLATE_DIR/"
cp -r "../cam_block/src/krnl_input.cpp" "$TMP_SRC_DIR/"
cp -r "../cam_block/src/krnl_output.cpp" "$TMP_SRC_DIR/"
cp -r "../cam_block/src/hdl/"*  "$TMP_HDL_DIR/"
cp -r "../router_test/post_router_template.cpp" "$TMP_TEMPLATE_DIR/"
cp -r "../router_test/src/mem_read.cpp" "$TMP_SRC_DIR/"
cp -r "../router_test/src/router.cpp" "$TMP_SRC_DIR/"
cp -r "../router_test/src/mem_write.cpp" "$TMP_SRC_DIR/"
cp -r "../router_test/src/post_router.cpp" "$TMP_SRC_DIR/"

# Verify the copied files
echo "Copied the following files to $TMP_SRC_DIR:"
ls "$TMP_SRC_DIR"

echo "Copied the following files to $TMP_TEMPLATE_DIR:"
ls "$TMP_TEMPLATE_DIR"

# Add any additional preparation steps here
echo "Temporary source preparation complete."
