#!/bin/bash
# Script to compile and run CUDA constant recognition

# Detect GPU architecture
GPU_ARCH=${1:-sm_75}

echo "Compiling for architecture: $GPU_ARCH"

cd /workspaces/ConstantRecognition/cuda

# Compile
nvcc -O3 -arch=$GPU_ARCH constant_gpu.cu -o constant_gpu

if [ $? -eq 0 ]; then
    echo "Compilation successful!"
    echo ""
    echo "Usage: ./constant_gpu <number> <MaxK>"
    echo "Example: ./constant_gpu 3.14159265358979 7"
else
    echo "Compilation failed!"
    exit 1
fi
