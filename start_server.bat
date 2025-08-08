@echo off
echo ========================================
echo     PROP CONTROL - Start Server
echo ========================================
echo.
echo Starting the Prop Control System server...
echo.
echo Server will be available at:
echo   📊 Web Dashboard: http://localhost:5000
echo   🌐 Device Control: http://localhost:3002
echo.
echo Press Ctrl+C to stop the server
echo.

cd /d "C:\Tricorder Control\Tricorder-Control"

REM Check if virtual environment exists
if not exist ".venv\Scripts\python.exe" (
    echo ❌ Virtual environment not found!
    echo Please run: python -m venv .venv
    echo Then: .venv\Scripts\pip install -r server\requirements.txt
    pause
    exit /b 1
)

echo ✅ Starting enhanced server...
echo.

.venv\Scripts\python.exe server\enhanced_server.py

pause
