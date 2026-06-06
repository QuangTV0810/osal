#!/bin/bash

# Script build OSAL cho SoC RTS3917N
set -e

PATH_ROOT="/home/quangtv/Workspace/SDK/Sources/Platform/osal"
BUILD_DIR="${PATH_ROOT}/build-rts3917n"
CROSS_COMPILE="/home/quangtv/Workspace/IPCam/Source/IP_Cam/toolchain/asdk-10.3.1-a7-EL-5.4-u1.0-a32nh-220218/bin/arm-linux-gcc"

echo "--- Start the OSAL build process for RTS3917N ---"

cd "$PATH_ROOT"

mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

# Chạy cấu hình CMake
cmake -DCMAKE_SYSTEM_NAME=Linux \
      -DCMAKE_SYSTEM_PROCESSOR=arm \
      -DCMAKE_C_COMPILER="$CROSS_COMPILE" \
      -DOSAL_SYSTEM_BSPTYPE=generic-linux \
      -DOSAL_SYSTEM_OSTYPE=posix \
      -DCMAKE_INSTALL_PREFIX="${BUILD_DIR}/install" \
      -DCMAKE_BUILD_TYPE=Release \
      ..

make -j2
make install

echo "--- Build successful! The product is installed at: ${BUILD_DIR}/install ---"