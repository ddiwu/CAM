#!/bin/bash

# Read parameters from param.cfg
while IFS='=' read -r key value; do
    export "$key"="$value"
done < param.cfg

# Call the Python script with the parameters
python configure.py --block_num "$CUSTOMIZED_BLOCK_NUM" --block_size "$CUSTOMIZED_BLOCK_SIZE"
