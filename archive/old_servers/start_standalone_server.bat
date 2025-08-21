@echo off
echo ================================================
echo Prop Control System - Standalone Server
echo Version 0.1 - "Command Center"
echo ================================================
echo.
echo This standalone server runs independently of the web GUI
echo Features:
echo - Command-line interface for device control
echo - Automatic device discovery
echo - Real-time device monitoring
echo - Command history and statistics
echo - Background device management
echo.
echo Starting server...
echo.

cd /d "C:\Tricorder Control\Tricorder-Control"

REM Check if virtual environment exists
if not exist ".venv\Scripts\python.exe" (
    echo ERROR: Virtual environment not found!
    echo Please run the installation script first.
    pause
    exit /b 1
)

REM Start the standalone server
.venv\Scripts\python.exe server\standalone_server.py

echo.
echo Server stopped.
pause
