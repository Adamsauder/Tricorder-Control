#!/usr/bin/env pwsh

Write-Host "================================================" -ForegroundColor Cyan
Write-Host "Tricorder Control System - GUI Server" -ForegroundColor Cyan
Write-Host "Version 0.1 - 'Mission Control'" -ForegroundColor Cyan
Write-Host "================================================" -ForegroundColor Cyan
Write-Host ""
Write-Host "This GUI server provides a visual interface for device control" -ForegroundColor Green
Write-Host "Features:" -ForegroundColor Yellow
Write-Host "- Visual device management dashboard" -ForegroundColor White
Write-Host "- Real-time server monitoring" -ForegroundColor White
Write-Host "- Interactive command sending" -ForegroundColor White
Write-Host "- LED color controls with visual buttons" -ForegroundColor White
Write-Host "- Statistics and comprehensive logging" -ForegroundColor White
Write-Host "- Auto-discovery and device details" -ForegroundColor White
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

Write-Host "Starting GUI server..." -ForegroundColor Green
Write-Host ""

try {
    # Start the GUI server
    & ".venv\Scripts\python.exe" "server\gui_server.py"
}
catch {
    Write-Host "Error starting GUI server: $_" -ForegroundColor Red
}
finally {
    Write-Host ""
    Write-Host "GUI server stopped." -ForegroundColor Yellow
    Read-Host "Press Enter to exit"
}
