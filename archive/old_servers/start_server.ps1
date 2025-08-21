# Prop Control - Start Server (PowerShell)
Write-Host "========================================" -ForegroundColor Cyan
Write-Host "     PROP CONTROL - Start Server" -ForegroundColor Yellow
Write-Host "========================================" -ForegroundColor Cyan
Write-Host ""
Write-Host "Starting the Prop Control System server..." -ForegroundColor Green
Write-Host ""
Write-Host "Server will be available at:" -ForegroundColor White
Write-Host "  📊 Web Dashboard: http://localhost:5000" -ForegroundColor Cyan
Write-Host "  🌐 Device Control: http://localhost:3002" -ForegroundColor Cyan
Write-Host ""
Write-Host "Press Ctrl+C to stop the server" -ForegroundColor Yellow
Write-Host ""

Set-Location "C:\Tricorder Control\Tricorder-Control"

# Check if virtual environment exists
if (-not (Test-Path ".venv\Scripts\python.exe")) {
    Write-Host "❌ Virtual environment not found!" -ForegroundColor Red
    Write-Host "Please run: python -m venv .venv" -ForegroundColor Yellow
    Write-Host "Then: .venv\Scripts\pip install -r server\requirements.txt" -ForegroundColor Yellow
    Read-Host "Press Enter to exit"
    exit 1
}

Write-Host "✅ Starting enhanced server..." -ForegroundColor Green
Write-Host ""

& ".venv\Scripts\python.exe" "server\enhanced_server.py"

Read-Host "Press Enter to exit"
