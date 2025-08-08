# Prop Control - Flash Tricorder (PowerShell)
Write-Host "========================================" -ForegroundColor Cyan
Write-Host "  PROP CONTROL - Flash Tricorder" -ForegroundColor Yellow
Write-Host "========================================" -ForegroundColor Cyan
Write-Host ""
Write-Host "Flashing ESP32 Tricorder firmware..." -ForegroundColor Green
Write-Host "Make sure your ESP32-2432S032C-I is connected via USB-C" -ForegroundColor White
Write-Host ""
Read-Host "Press Enter to continue"

Set-Location "C:\Tricorder Control\Tricorder-Control\firmware\tricorder"

Write-Host ""
Write-Host "Starting PlatformIO upload..." -ForegroundColor Yellow
Write-Host "Hold BOOT button on ESP32 if upload fails" -ForegroundColor Magenta
Write-Host ""

& pio run -t upload

if ($LASTEXITCODE -ne 0) {
    Write-Host ""
    Write-Host "❌ Upload failed!" -ForegroundColor Red
    Write-Host "Try holding the BOOT button and run again" -ForegroundColor Yellow
    Write-Host ""
    Read-Host "Press Enter to exit"
    exit 1
}

Write-Host ""
Write-Host "✅ Tricorder firmware uploaded successfully!" -ForegroundColor Green
Write-Host "You can now disconnect the device and power it on." -ForegroundColor White
Write-Host ""
Read-Host "Press Enter to exit"
