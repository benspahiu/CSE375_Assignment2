#!/bin/bash
# Simple build script for CuckooHashSet project

# Exit on any error
set -e

# Create build folder if it doesn't exist
mkdir -p build

# Enter build folder
cd build

# Configure project
cmake ..

# Build all targets
cmake --build .

# Done
echo "Build complete."