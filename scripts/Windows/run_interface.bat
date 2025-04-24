@echo off
echo Starting interface...

REM Navigate to the root directory (up two levels from scripts\Windows)
cd ..\..\

REM Navigate to the gui directory
cd .\gui

REM Activate the virtual environment
call .venv\Scripts\activate.bat

REM Run the interface script
python interface.py

REM Keep the window open after the script finishes or is interrupted
echo Interface process has ended. Press any key to close this window.
pause > nul