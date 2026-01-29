#!/bin/bash

# Create build directory
mkdir -p build
cd build

# Run CMake configuration
cmake ..

# Build the project
make

echo "Build complete! Run with: ./build/screen-renderer"
