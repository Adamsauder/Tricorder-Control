@echo off
echo =====================================
echo  Building All Prop Control Firmware
echo =====================================
echo.

REM Create firmware output directory
if not exist "server\firmware_binaries" mkdir "server\firmware_binaries"

REM Build Tricorder Firmware
echo [1/4] Building Tricorder Firmware...
cd firmware\tricorder
pio run
if %ERRORLEVEL% NEQ 0 (
    echo ERROR: Tricorder build failed!
    pause
    exit /b 1
)
copy ".pio\build\tricorder\firmware.bin" "..\..\server\firmware_binaries\tricorder_firmware.bin"
echo ✓ Tricorder firmware built and copied

echo.

REM Build Polyinoculator Firmware
echo [2/4] Building Polyinoculator Firmware...
cd ..\polyinoculator
pio run
if %ERRORLEVEL% NEQ 0 (
    echo ERROR: Polyinoculator build failed!
    pause
    exit /b 1
)
copy ".pio\build\polyinoculator\firmware.bin" "..\..\server\firmware_binaries\polyinoculator_firmware.bin"
echo ✓ Polyinoculator firmware built and copied

echo.

REM Build Defragmentor Firmware
echo [3/4] Building Defragmentor Firmware...
cd ..\defragmentor
pio run
if %ERRORLEVEL% NEQ 0 (
    echo ERROR: Defragmentor build failed!
    pause
    exit /b 1
)
copy ".pio\build\defragmentor\firmware.bin" "..\..\server\firmware_binaries\defragmentor_firmware.bin"
echo ✓ Defragmentor firmware built and copied

echo.

REM Build IV Injector Firmware
echo [4/4] Building IV Injector Firmware...
cd ..\iv_injector
pio run
if %ERRORLEVEL% NEQ 0 (
    echo ERROR: IV Injector build failed!
    pause
    exit /b 1
)
copy ".pio\build\iv_injector\firmware.bin" "..\..\server\firmware_binaries\iv_injector_firmware.bin"
echo ✓ IV Injector firmware built and copied

echo.
echo =====================================
echo  All Firmware Built Successfully!
echo =====================================
echo.
echo Firmware files available at: server\firmware_binaries\
echo - tricorder_firmware.bin
echo - polyinoculator_firmware.bin  
echo - defragmentor_firmware.bin
echo - iv_injector_firmware.bin
echo.

cd ..\..
pause
