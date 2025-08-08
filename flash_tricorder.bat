@echo off
echo ========================================
echo  PROP CONTROL - Flash Tricorder
echo ========================================
echo.
echo Flashing ESP32 Tricorder firmware...
echo Make sure your ESP32-2432S032C-I is connected via USB-C
echo.
pause

cd /d "C:\Tricorder Control\Tricorder-Control\firmware\tricorder"

echo.
echo Starting PlatformIO upload...
echo Hold BOOT button on ESP32 if upload fails
echo.

pio run -t upload

if %ERRORLEVEL% NEQ 0 (
    echo.
    echo ❌ Upload failed!
    echo Try holding the BOOT button and run again
    echo.
    pause
    exit /b 1
)

echo.
echo ✅ Tricorder firmware uploaded successfully!
echo You can now disconnect the device and power it on.
echo.
pause
