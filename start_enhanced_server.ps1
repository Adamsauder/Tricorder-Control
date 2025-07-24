Write-Host "Starting Enhanced Tricorder Server with ESP32 Simulator..." -ForegroundColor Green
Write-Host ""
Write-Host "This server includes:" -ForegroundColor Yellow
Write-Host "- Device communication (UDP port 8888)" -ForegroundColor Cyan
Write-Host "- Web interface (http://localhost:5000)" -ForegroundColor Cyan
Write-Host "- ESP32 Simulator (http://localhost:5000/simulator)" -ForegroundColor Cyan
Write-Host ""

Set-Location "C:\Tricorder Control\Tricorder-Control"
& ".venv\Scripts\python.exe" "server\enhanced_server.py"

Write-Host ""
Write-Host "Press any key to continue..." -ForegroundColor Yellow
$null = $Host.UI.RawUI.ReadKey("NoEcho,IncludeKeyDown")
