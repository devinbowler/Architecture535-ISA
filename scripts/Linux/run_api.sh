# Navigate to the root directory (up two levels from scripts/Linux)
cd "$(dirname "$0")"/../../

# Navigate to the gui directory
cd ./gui

# Activate the virtual environment
source .venv/bin/activate

# Run the API script
python api.py

# Keep the terminal open after the script finishes or is interrupted
echo "API process has ended. Press Enter to close this terminal."
read