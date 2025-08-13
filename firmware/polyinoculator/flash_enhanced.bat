@echo off
REM Enhanced Polyinoculator Flash Script
REM Flashes the enhanced firmware with persistent configuration support

echo.
echo ===============================================
echo    Enhanced Polyinoculator Flash Utility
echo ===============================================
echo.
echo Features in this firmware:
echo - Persistent configuration storage (NVRAM)
echo - Web-based configuration interface
echo - Individual prop addressing
echo - Device label customization
echo - SACN universe/address management
echo.
echo Hardware Configuration:
echo - LED Strip 1: D5 (GPIO5) - 7 LEDs
echo - LED Strip 2: D6 (GPIO16) - 4 LEDs  
echo - LED Strip 3: D8 (GPIO20) - 4 LEDs
echo - Total: 15 LEDs (45 DMX channels)
echo.

REM Check if device is connected
echo Checking for connected ESP32-C3 device...
pio device list | findstr /i "COM" > nul
if %errorlevel% neq 0 (
    echo.
    echo ERROR: No ESP32-C3 device detected!
    echo.
    echo Please check:
    echo 1. Device is connected via USB-C cable
    echo 2. Drivers are installed
    echo 3. Device is in bootloader mode if needed
    echo.
    pause
    exit /b 1
)

echo Device detected! Proceeding with flash...
echo.

REM Change to firmware directory
cd /d "%~dp0"

REM Clean build to ensure fresh compilation
echo Cleaning previous build...
pio run --target clean
if %errorlevel% neq 0 (
    echo.
    echo ERROR: Failed to clean build directory!
    pause
    exit /b 1
)

echo.
echo Building enhanced firmware...
pio run
if %errorlevel% neq 0 (
    echo.
    echo ERROR: Firmware compilation failed!
    echo Check the code for syntax errors.
    pause
    exit /b 1
)

echo.
echo Flashing firmware to device...
pio run --target upload
if %errorlevel% neq 0 (
    echo.
    echo ERROR: Firmware upload failed!
    echo.
    echo Troubleshooting:
    echo 1. Try holding BOOT button while connecting USB
    echo 2. Check COM port in Device Manager
    echo 3. Try different USB cable/port
    echo.
    pause
    exit /b 1
)

echo.
echo ===============================================
echo           FLASH SUCCESSFUL!
echo ===============================================
echo.
echo The enhanced polyinoculator firmware has been flashed.
echo.
echo Next Steps:
echo 1. Device will restart automatically
echo 2. Connect to serial monitor to see boot messages
echo 3. Device will connect to WiFi: "Rigging Electric"
echo 4. Access web interface at device IP address
echo 5. Configure device label and SACN addressing
echo.
echo Default Configuration:
echo - Device Label: Polyinoculator_XXXX (random)
echo - SACN Universe: 1
echo - DMX Start Address: 1
echo - Brightness: 128/255
echo.

REM Start serial monitor automatically
set /p choice="Start serial monitor to see device boot? (y/n): "
if /i "%choice%"=="y" (
    echo.
    echo Starting serial monitor... (Press Ctrl+C to exit)
    timeout /t 2 > nul
    pio device monitor
)

echo.
echo Flash process complete!
pause
