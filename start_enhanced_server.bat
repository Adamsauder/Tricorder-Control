@echo off
echo Starting Enhanced Tricorder Server with Device Discovery...
echo.
echo This server includes:
echo - Device communication (UDP port 8888)
echo - Web interface (http://localhost:5000)
echo - ESP32 Simulator (http://localhost:5000/simulator)
echo - Device Discovery and Manual Add buttons
echo.
cd /d "C:\Tricorder Control\Tricorder-Control"
echo Starting server...
.venv\Scripts\python.exe server\enhanced_server.py
pause
