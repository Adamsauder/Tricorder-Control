@echo off
echo ========================================
echo  PROP CONTROL - Flash Polyinoculator  
echo ========================================
echo.
echo Flashing ESP32-C3 Polyinoculator firmware...
echo Make sure your Seeed XIAO ESP32-C3 is connected via USB-C
echo.
pause

cd /d "C:\Tricorder Control\Tricorder-Control\firmware\polyinoculator"

echo.
echo Starting PlatformIO upload...
echo.

pio run -t upload

if %ERRORLEVEL% NEQ 0 (
    echo.
    echo ❌ Upload failed!
    echo Check USB connection and try again
    echo.
    pause
    exit /b 1
)

echo.
echo ✅ Polyinoculator firmware uploaded successfully!
echo Device should now show LED test pattern and connect to WiFi.
echo.
pause
