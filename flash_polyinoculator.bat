@echo off
echo ========================================
echo  PROP CONTROL - Flash Polyinoculator  
echo ========================================
echo.
echo Flashing ESP32-C3 Polyinoculator firmware...
echo Hardware Configuration:
echo   - Strip 1: 7 LEDs on GPIO10
echo   - Strip 2: 4 LEDs on GPIO6  
echo   - Strip 3: 4 LEDs on GPIO7
echo   - Total: 15 LEDs across 3 strips
echo.
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
echo Device should now show LED test pattern on all 3 strips and connect to WiFi.
echo   - Strip 1 (GPIO10): 7 LEDs
echo   - Strip 2 (GPIO0):  4 LEDs  
echo   - Strip 3 (GPIO1):  4 LEDs
echo.
pause
