#!/bin/bash

# Ensure the generate.sh script exists
if [ -f "./src/generate.sh" ]; then
    echo "Running ./src/generate.sh..."
    
    # Make sure the script is executable
    chmod +x ./src/generate.sh

    # Run the script
    ./src/generate.sh

    # Check the status of the run
    if [ $? -eq 0 ]; then
        echo "generate.sh executed successfully."
    else
        echo "generate.sh encountered an error."
        exit 1
    fi
else
    echo "Error: ./src/generate.sh does not exist."
    exit 1
fi

# Read parameters from param.cfg
while IFS='=' read -r key value; do
    export "$key"="$value"
done < param.cfg

# Call the Python script with the parameters
python3 configure.py --bus_width "$BUS_WIDTH" \
                     --cam_size "$CAM_SIZE" \
                     --storage_type "$STORAGE_TYPE" \
                     --mask "$MASK" \
                     --block_num "$BLOCK_NUM" \
                     --block_size "$BLOCK_SIZE"
