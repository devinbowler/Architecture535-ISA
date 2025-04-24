@echo off
echo Starting the compilation process...

REM Get the current directory
set "SCRIPT_DIR=%~dp0"

REM Run the compilation script
call "%SCRIPT_DIR%compile.bat"

echo Starting the API in a new window...
REM Start API in a new command window
start "API Process" cmd /k call "%SCRIPT_DIR%run_api.bat"

REM Wait a moment for the API to start
timeout /t 2 /nobreak > nul

echo Starting the interface in a new window...
REM Start interface in a new command window
start "Interface Process" cmd /k call "%SCRIPT_DIR%run_interface.bat"

echo All components started
