@echo off
title Prop Control System - Quick Actions

:MENU
cls
echo ================================================
echo           PROP CONTROL SYSTEM
echo         Quick Actions Menu
echo ================================================
echo.
echo  Server Actions:
echo  [1] Start Server (Enhanced Web Dashboard)
echo  [2] Start GUI Server (Desktop Application)  
echo  [3] Start Standalone Server (Command Line)
echo.
echo  Firmware Flash:
echo  [4] Flash Tricorder (ESP32 with Display)
echo  [5] Flash Polyinoculator (ESP32-C3 with LEDs)
echo.
echo  Development:
echo  [6] Open Tricorder Project in VS Code
echo  [7] Open Polyinoculator Project in VS Code
echo  [8] Open Firmware Workspace in VS Code
echo.
echo  [9] Exit
echo.
set /p choice="Select option (1-9): "

if "%choice%"=="1" goto START_SERVER
if "%choice%"=="2" goto START_GUI
if "%choice%"=="3" goto START_STANDALONE
if "%choice%"=="4" goto FLASH_TRICORDER
if "%choice%"=="5" goto FLASH_POLYINOCULATOR
if "%choice%"=="6" goto OPEN_TRICORDER
if "%choice%"=="7" goto OPEN_POLYINOCULATOR
if "%choice%"=="8" goto OPEN_WORKSPACE
if "%choice%"=="9" goto EXIT

echo Invalid choice. Please try again.
pause
goto MENU

:START_SERVER
echo Starting Enhanced Web Server...
call start_server.bat
goto MENU

:START_GUI
echo Starting GUI Server...
call start_gui_server.bat
goto MENU

:START_STANDALONE
echo Starting Standalone Server...
call start_standalone_server.bat
goto MENU

:FLASH_TRICORDER
echo Flashing Tricorder...
call flash_tricorder.bat
goto MENU

:FLASH_POLYINOCULATOR
echo Flashing Polyinoculator...
call flash_polyinoculator.bat
goto MENU

:OPEN_TRICORDER
echo Opening Tricorder project in VS Code...
cd /d "C:\Tricorder Control\Tricorder-Control\firmware\tricorder"
code .
goto MENU

:OPEN_POLYINOCULATOR
echo Opening Polyinoculator project in VS Code...
cd /d "C:\Tricorder Control\Tricorder-Control\firmware\polyinoculator"
code .
goto MENU

:OPEN_WORKSPACE
echo Opening firmware workspace in VS Code...
cd /d "C:\Tricorder Control\Tricorder-Control\firmware"
code tricorder-workspace.code-workspace
goto MENU

:EXIT
echo.
echo Thanks for using Prop Control System! 🎬
pause
exit
