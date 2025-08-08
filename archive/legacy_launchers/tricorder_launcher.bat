@echo off
echo ========================================
echo    Tricorder Control System Launcher
echo ========================================
echo.
echo Choose your option:
echo 1) Enhanced Server with Built-in Web Interface (Recommended)
echo 2) Install Node.js and setup React Web App
echo 3) Exit
echo.
set /p choice="Enter your choice (1-3): "

if "%choice%"=="1" goto enhanced_server
if "%choice%"=="2" goto setup_nodejs
if "%choice%"=="3" goto exit
goto invalid_choice

:enhanced_server
echo.
echo Starting Enhanced Server with Built-in Web Interface...
echo This includes:
echo - Device communication (UDP port 8888)
echo - Web interface (http://localhost:5000)
echo - ESP32 Simulator (http://localhost:5000/simulator)
echo.
cd /d "C:\Tricorder Control\Tricorder-Control"
.venv\Scripts\python.exe server\enhanced_server.py
goto end

:setup_nodejs
echo.
echo To run the React web app, you need to install Node.js first:
echo.
echo 1. Go to https://nodejs.org/
echo 2. Download and install the LTS version
echo 3. Restart this script after installation
echo.
echo Then you can run:
echo   npm install (in the web directory)
echo   npm run dev
echo.
pause
goto end

:invalid_choice
echo Invalid choice. Please enter 1, 2, or 3.
pause
goto end

:exit
echo Goodbye!
goto end

:end
pause
