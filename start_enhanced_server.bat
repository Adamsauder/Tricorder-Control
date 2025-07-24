@echo off
echo Starting Enhanced Tricorder Server with ESP32 Simulator...
echo.
echo This server includes:
echo - Device communication (UDP port 8888)
echo - Web interface (http://localhost:5000)
echo - ESP32 Simulator (http://localhost:5000/simulator)
echo.
C:/Users/adams/AppData/Local/Programs/Python/Python313/python.exe server/enhanced_server.py
pause
