@echo off
echo Starting compilation process...

REM Navigate to the root directory (up two levels from scripts\Windows)
cd ..\..\simulator

REM Create the build directory if it doesn't exist
if not exist build mkdir build

REM Navigate to the build directory
cd build

REM Run CMake
cmake ..

REM Compile the code
cmake --build .

echo C code compilation completed
