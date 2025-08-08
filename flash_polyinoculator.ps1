# Prop Control - Flash Polyinoculator (PowerShell)
Write-Host "========================================" -ForegroundColor Cyan
Write-Host "  PROP CONTROL - Flash Polyinoculator" -ForegroundColor Yellow  
Write-Host "========================================" -ForegroundColor Cyan
Write-Host ""
Write-Host "Flashing ESP32-C3 Polyinoculator firmware..." -ForegroundColor Green
Write-Host "Make sure your Seeed XIAO ESP32-C3 is connected via USB-C" -ForegroundColor White
Write-Host ""
Read-Host "Press Enter to continue"

Set-Location "C:\Tricorder Control\Tricorder-Control\firmware\polyinoculator"

Write-Host ""
Write-Host "Starting PlatformIO upload..." -ForegroundColor Yellow
Write-Host ""

& pio run -t upload

if ($LASTEXITCODE -ne 0) {
    Write-Host ""
    Write-Host "❌ Upload failed!" -ForegroundColor Red
    Write-Host "Check USB connection and try again" -ForegroundColor Yellow
    Write-Host ""
    Read-Host "Press Enter to exit"
    exit 1
}

Write-Host ""
Write-Host "✅ Polyinoculator firmware uploaded successfully!" -ForegroundColor Green
Write-Host "Device should now show LED test pattern and connect to WiFi." -ForegroundColor White
Write-Host ""
Read-Host "Press Enter to exit"
