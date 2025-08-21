#!/usr/bin/env pwsh

Write-Host "================================================" -ForegroundColor Cyan
Write-Host "Prop Control System - Standalone Server" -ForegroundColor Cyan
Write-Host "Version 0.1 - 'Command Center'" -ForegroundColor Cyan
Write-Host "================================================" -ForegroundColor Cyan
Write-Host ""
Write-Host "This standalone server runs independently of the web GUI" -ForegroundColor Green
Write-Host "Features:" -ForegroundColor Yellow
Write-Host "- Command-line interface for device control" -ForegroundColor White
Write-Host "- Automatic device discovery" -ForegroundColor White
Write-Host "- Real-time device monitoring" -ForegroundColor White
Write-Host "- Command history and statistics" -ForegroundColor White
Write-Host "- Background device management" -ForegroundColor White
Write-Host ""

# Change to project directory
Set-Location "C:\Tricorder Control\Tricorder-Control"

# Check if virtual environment exists
if (-not (Test-Path ".venv\Scripts\python.exe")) {
    Write-Host "ERROR: Virtual environment not found!" -ForegroundColor Red
    Write-Host "Please run the installation script first." -ForegroundColor Red
    Read-Host "Press Enter to exit"
    exit 1
}

Write-Host "Starting server..." -ForegroundColor Green
Write-Host ""

try {
    # Start the standalone server
    & ".venv\Scripts\python.exe" "server\standalone_server.py"
}
catch {
    Write-Host "Error starting server: $_" -ForegroundColor Red
}
finally {
    Write-Host ""
    Write-Host "Server stopped." -ForegroundColor Yellow
    Read-Host "Press Enter to exit"
}
