# Navigate to the root directory (up two levels from scripts/Linux)
cd ../../simulator

# Create the build directory if it doesn't exist
mkdir -p build

# Navigate to the build directory
cd build

# Run CMake
cmake ..

# Compile the code
make

echo "C code compilation completed"