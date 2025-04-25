@echo off
echo Starting interface...

REM Navigate to the root directory (up two levels from scripts\Windows)
cd ..\..\

REM Check if gui directory exists
echo Checking if gui directory exists...
if not exist gui\ (
    echo ERROR: gui directory not found!
    dir
    pause
    exit /b 1
)

REM Navigate to the gui directory
cd gui

REM Check if interface.py exists
if not exist interface.py (
    echo ERROR: interface.py not found in gui directory!
    dir
    pause
    exit /b 1
)

REM Check if virtual environment exists and create it if it doesn't
if not exist .venv\ (
    echo Virtual environment not found. Creating it now...
    python -m venv .venv
    
    REM Activate the virtual environment
    call .venv\Scripts\activate.bat
    
    REM Install required packages
    pip install -r requirements.txt
    
    echo Virtual environment created and activated
) else (
    echo Virtual environment found, activating...
    call .venv\Scripts\activate.bat
    pip install -r requirements.txt
)

REM Run the interface script
echo Running interface.py...
python interface.py

REM Keep the window open after the script finishes or is interrupted
echo Interface process has ended. Press any key to close this window.
pause > nul
