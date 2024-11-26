#!/bin/bash

# Read parameters from param.cfg
while IFS='=' read -r key value; do
    export "$key"="$value"
done < param.cfg

# Call the Python script with the parameters
python3 configure.py --bus_width "$CUSTORMIZED_BUS_WIDTH" \
                     --cam_size "$CUSTORMIZED_CAM_SIZE" \
                     --storage_type "$CUSTORMIZED_STORAGE_TYPE" \
                     --mask "$CUSTORMIZED_MASK"
