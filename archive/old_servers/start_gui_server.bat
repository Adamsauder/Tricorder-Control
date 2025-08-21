@echo off
echo ================================================
echo Prop Control System - GUI Server
echo Version 0.1 - "Mission Control"
echo ================================================
echo.
echo This GUI server provides a visual interface for device control
echo Features:
echo - Visual device management dashboard
echo - Real-time server monitoring
echo - Interactive command sending
echo - LED color controls with visual buttons
echo - Statistics and comprehensive logging
echo - Auto-discovery and device details
echo.
echo Starting GUI server...
echo.

cd /d "C:\Tricorder Control\Tricorder-Control"

REM Check if virtual environment exists
if not exist ".venv\Scripts\python.exe" (
    echo ERROR: Virtual environment not found!
    echo Please run the installation script first.
    pause
    exit /b 1
)

REM Start the GUI server
.venv\Scripts\python.exe server\gui_server.py

echo.
echo GUI server stopped.
pause
