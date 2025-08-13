@echo off
REM Enhanced Firmware Flash Script - Auto Platform Detection
REM This script automatically detects the environment and runs the appropriate flash script

title Enhanced Tricorder Firmware Flasher

echo.
echo ==========================================
echo  ENHANCED TRICORDER FIRMWARE FLASHER
echo ==========================================
echo.

REM Check if we're in Windows Subsystem for Linux
if exist /proc/version (
    echo Detected WSL environment, using Linux script...
    bash flash_enhanced.sh
    goto :end
)

REM Check if PowerShell is available and version 5.1+
powershell -Command "if ($PSVersionTable.PSVersion.Major -ge 5) { exit 0 } else { exit 1 }" >nul 2>&1
if %errorlevel% equ 0 (
    echo Detected PowerShell 5.1+, using enhanced PowerShell script...
    powershell -ExecutionPolicy Bypass -File flash_enhanced.ps1
) else (
    echo Using Windows batch script...
    call flash_enhanced.bat
)

:end
echo.
echo Flash process completed!
pause
