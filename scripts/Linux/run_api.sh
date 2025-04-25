#!/bin/bash
# Get the directory where the script is located
SCRIPT_DIR=$(dirname "$0")

# Navigate to the root directory (up two levels from scripts/Linux)
cd "$SCRIPT_DIR"/../../

# Print current directory for debugging
echo "Current directory: $(pwd)"
echo "Checking if gui directory exists..."
if [ -d "./gui" ]; then
    echo "gui directory found"
else
    echo "ERROR: gui directory not found!"
    ls -la
    exit 1
fi

# Navigate to the gui directory
cd ./gui || { echo "Failed to change to gui directory"; exit 1; }

# Print directory contents for debugging
echo "Contents of gui directory:"
ls -la

# Check if api.py exists
if [ -f "api.py" ]; then
    echo "api.py found"
else
    echo "ERROR: api.py not found in gui directory!"
    exit 1
fi

# Check if virtual environment exists and create it if it doesn't
if [ ! -d ".venv" ]; then
    echo "Virtual environment not found. Creating it now..."
    python -m venv .venv
    
    # Activate the virtual environment
    source .venv/bin/activate
    
    # Install required packages
    pip install -r requirements.txt
    
    echo "Virtual environment created and activated"
else
    echo "Virtual environment found, activating..."
    source .venv/bin/activate
    pip install -r requirements.txt
fi

# Run the API script
echo "Running api.py..."
python api.py

# Keep the terminal open after the script finishes or is interrupted
echo "API process has ended. Press Enter to close this terminal."
read