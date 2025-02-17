#!/bin/bash

# Build the cmake project
./build.sh $@

# Check for error
E_CODE=$?
if [ ${E_CODE} -ne 0 ]; then
    echo "Error during cmake build"
    exit ${E_CODE}
fi

BUILD_DIR="build"

# Parse script arguments
while [[ $# -gt 0 ]]; do
case $1 in
    -d|--debug)
        shift
        ;;
    -c|--clean)
        shift
        ;;
    -p|--parellel)
        shift
        shift
        ;;
    -*|--*)
        echo "Unknown option $1"
        exit 1
        ;;
   *)
        BUILD_DIR="$1" # save build directory argument
        shift
        ;;
  esac
done

EXE_PATH="${BUILD_DIR}/coil-daemon/coild"

# Check if executable exist
if [ -e ${EXE_PATH} ]; then

    # Export path environmental variables
    export COIL_BASE_CONFIG="./example-files/etc/coil/default.json"
    export COIL_USER_CONFIG="./example-files/.config/coil/config.json"

    ./${EXE_PATH}
else
    echo "Couldn't find executable file: ${EXE_PATH}"
    exit 1
fi

