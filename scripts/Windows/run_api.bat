@echo off
echo Starting API...

REM Navigate to the root directory (up two levels from scripts\Windows)
cd ..\..\gui

REM Activate the virtual environment
call .venv\Scripts\activate.bat

REM Run the API script
python api.py

REM Keep the window open after the script finishes or is interrupted
echo API process has ended. Press any key to close this window.
pause > nul