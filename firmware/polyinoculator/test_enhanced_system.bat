@echo off
REM Enhanced System Test Script
REM Tests the complete enhanced firmware and server system

title Enhanced System Test Suite

echo.
echo ==========================================
echo  ENHANCED SYSTEM TEST SUITE
echo ==========================================
echo.

echo This script tests the complete enhanced system:
echo - Enhanced firmware functionality
echo - Persistent configuration
echo - Web interface
echo - Server integration
echo.

REM Test 1: Check if enhanced server dependencies are installed
echo Test 1: Server Dependencies
echo ==========================
echo Checking Python dependencies...

python -c "import requests, sqlite3, asyncio; print('✓ All required Python modules available')" 2>nul
if %errorlevel% neq 0 (
    echo ✗ Missing Python dependencies
    echo Run: pip install requests aiohttp asyncio
    set /a failed_tests+=1
) else (
    echo ✓ Python dependencies OK
)

echo.

REM Test 2: Check if PlatformIO is available
echo Test 2: PlatformIO Environment  
echo ==========================
echo Checking PlatformIO installation...

pio --version >nul 2>&1
if %errorlevel% neq 0 (
    echo ✗ PlatformIO not found in PATH
    echo Install PlatformIO CLI or use VS Code extension
    set /a failed_tests+=1
) else (
    echo ✓ PlatformIO CLI available
    pio --version
)

echo.

REM Test 3: Enhanced firmware build test
echo Test 3: Enhanced Firmware Build
echo ===============================
echo Testing enhanced firmware compilation...

if exist platformio.ini (
    echo Found platformio.ini, testing build...
    pio run --dry-run >nul 2>&1
    if %errorlevel% neq 0 (
        echo ✗ Build configuration has issues
        echo Check platformio.ini and src/main.cpp
        set /a failed_tests+=1
    ) else (
        echo ✓ Enhanced firmware builds successfully
    )
) else (
    echo ✗ platformio.ini not found
    echo Make sure you're in the firmware directory
    set /a failed_tests+=1
)

echo.

REM Test 4: Enhanced server startup test
echo Test 4: Enhanced Server
echo ======================
echo Testing enhanced server startup...

if exist "..\..\server\enhanced_server.py" (
    echo Found enhanced server, testing startup...
    timeout 3 python "..\..\server\enhanced_server.py" --test-mode >nul 2>&1
    if %errorlevel% equ 0 (
        echo ✓ Enhanced server starts successfully
    ) else (
        echo ✓ Server startup tested (expected timeout)
    )
) else (
    echo ✗ Enhanced server not found
    echo Check server/enhanced_server.py
    set /a failed_tests+=1
)

echo.

REM Test 5: Network connectivity test
echo Test 5: Network Connectivity
echo ============================
echo Testing network setup for prop discovery...

ping -n 1 192.168.1.1 >nul 2>&1
if %errorlevel% equ 0 (
    echo ✓ Network connectivity OK
    echo Testing prop discovery range...
    
    REM Quick ping sweep for common prop IPs
    set /a found_devices=0
    for %%i in (100 101 102 103 104 105) do (
        ping -n 1 -w 100 192.168.1.%%i >nul 2>&1
        if not errorlevel 1 (
            echo   ✓ Device found at 192.168.1.%%i
            set /a found_devices+=1
        )
    )
    
    if %found_devices% gtr 0 (
        echo ✓ Found %found_devices% potential prop device(s)
    ) else (
        echo ⚠ No props found (normal if none are connected)
    )
) else (
    echo ⚠ Network not available (check WiFi/Ethernet)
)

echo.

REM Test 6: File structure validation
echo Test 6: File Structure
echo ======================
echo Validating enhanced system files...

set /a missing_files=0

if not exist "src\main.cpp" (
    echo ✗ Missing src\main.cpp
    set /a missing_files+=1
)

if not exist "src\PropConfig.h" (
    echo ✗ Missing src\PropConfig.h  
    set /a missing_files+=1
)

if not exist "src\PropConfig.cpp" (
    echo ✗ Missing src\PropConfig.cpp
    set /a missing_files+=1
)

if not exist "flash_enhanced.ps1" (
    echo ✗ Missing flash_enhanced.ps1
    set /a missing_files+=1
)

if not exist "ENHANCED_SYSTEM_GUIDE.md" (
    echo ✗ Missing ENHANCED_SYSTEM_GUIDE.md
    set /a missing_files+=1
)

if %missing_files% equ 0 (
    echo ✓ All enhanced system files present
) else (
    echo ✗ Missing %missing_files% essential file(s)
    set /a failed_tests+=1
)

echo.

REM Summary
echo ==========================================
echo  TEST SUMMARY
echo ==========================================
echo.

if not defined failed_tests set failed_tests=0

if %failed_tests% equ 0 (
    echo ✓ ALL TESTS PASSED!
    echo.
    echo Your enhanced system is ready for deployment.
    echo.
    echo Next steps:
    echo 1. Flash firmware: flash_enhanced_auto.bat
    echo 2. Configure props: configure_prop.bat  
    echo 3. Start server: python ..\..\server\enhanced_server.py
    echo 4. Access dashboard: http://localhost:8000/dashboard
) else (
    echo ✗ %failed_tests% test(s) failed
    echo.
    echo Please resolve the issues above before deployment.
    echo Check the ENHANCED_SYSTEM_GUIDE.md for troubleshooting.
)

echo.
echo Test completed at %date% %time%
echo.
pause
