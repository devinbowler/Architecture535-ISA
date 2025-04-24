# Navigate to the root directory (up two levels from scripts/Linux)
cd "$(dirname "$0")"/../../

# Navigate to the gui directory
cd ./gui

# Activate the virtual environment
source .venv/bin/activate

# Run the interface script
python interface.py

# Keep the terminal open after the script finishes or is interrupted
echo "Interface process has ended. Press Enter to close this terminal."
read
