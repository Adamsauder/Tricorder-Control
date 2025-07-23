@echo off
echo Starting Fixed Tricorder Server...
cd /d "C:\Tricorder Control\Tricorder-Control"
.venv\Scripts\python.exe server\fixed_server.py
pause
