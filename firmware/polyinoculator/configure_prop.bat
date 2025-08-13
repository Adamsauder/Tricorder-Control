@echo off
REM Enhanced Prop Configuration Tool
REM Helps configure newly flashed enhanced firmware props

title Enhanced Prop Configuration Tool

echo.
echo ===============================================
echo      ENHANCED PROP CONFIGURATION TOOL
echo ===============================================
echo.
echo This tool helps you configure props running the enhanced firmware
echo with persistent configuration and advanced features.
echo.

REM Get network configuration
echo Scanning for Polyinoculator props on network...
echo.

REM Find props using nmap or ping sweep (simplified)
for /L %%i in (100,1,199) do (
    ping -n 1 -w 100 192.168.1.%%i >nul 2>&1
    if not errorlevel 1 (
        echo Found device at 192.168.1.%%i - checking if it's a prop...
        REM Could use curl to check /api/config endpoint
        curl -s --connect-timeout 2 http://192.168.1.%%i/api/config >nul 2>&1
        if not errorlevel 1 (
            echo   ^> Polyinoculator found at 192.168.1.%%i
            set /a prop_count+=1
            echo     Opening web interface: http://192.168.1.%%i
            start http://192.168.1.%%i
        )
    )
)

echo.
if defined prop_count (
    echo Found %prop_count% prop(s) on the network!
    echo Web interfaces should be opening in your browser.
) else (
    echo No props found on network 192.168.1.x
    echo.
    echo Troubleshooting:
    echo 1. Make sure props are connected to WiFi "Rigging Electric"
    echo 2. Check if you're on the same network
    echo 3. Try manually accessing: http://^<prop-ip-address^>
    echo 4. Check serial monitor for actual IP addresses
)

echo.
echo Configuration Options Available:
echo ===============================
echo.
echo Via Web Interface (http://prop-ip-address):
echo 1. Device Configuration:
echo    - Device Label (friendly name)
echo    - Prop ID (unique identifier)
echo    - Description
echo.
echo 2. SACN/DMX Settings:
echo    - SACN Universe (1-63999)
echo    - DMX Address (1-512)
echo    - Enable/Disable SACN
echo.
echo 3. LED Configuration:
echo    - Brightness (0-255)
echo    - Strip 1: %STRIP1_COUNT% LEDs (Data pin D5)
echo    - Strip 2: %STRIP2_COUNT% LEDs (Data pin D6)  
echo    - Strip 3: %STRIP3_COUNT% LEDs (Data pin D8)
echo.
echo 4. Network Settings:
echo    - WiFi SSID and password
echo    - Static IP configuration
echo    - Hostname
echo.
echo 5. Advanced Features:
echo    - Factory reset option
echo    - Configuration backup/restore
echo    - Firmware update via web
echo.
echo Enhanced Server Integration:
echo ===========================
echo.
echo For professional lighting control:
echo 1. Start enhanced server: python server/enhanced_server.py
echo 2. Access dashboard: http://localhost:8000/dashboard
echo 3. Server features:
echo    - Automatic prop discovery
echo    - Address conflict detection
echo    - Centralized configuration management
echo    - Database storage of prop settings
echo.

pause
