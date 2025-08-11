# Prop Control - Flash Polyinoculator (PowerShell)
Write-Host "========================================" -ForegroundColor Cyan
Write-Host "  PROP CONTROL - Flash Polyinoculator" -ForegroundColor Yellow  
Write-Host "========================================" -ForegroundColor Cyan
Write-Host ""
Write-Host "Flashing ESP32-C3 Polyinoculator firmware..." -ForegroundColor Green
Write-Host "Hardware Configuration:" -ForegroundColor White
Write-Host "  - Strip 1: 7 LEDs on GPIO10" -ForegroundColor Cyan
Write-Host "  - Strip 2: 4 LEDs on GPIO4" -ForegroundColor Cyan
Write-Host "  - Strip 3: 4 LEDs on GPIO5" -ForegroundColor Cyan
Write-Host "  - Total: 15 LEDs across 3 strips" -ForegroundColor Yellow
Write-Host ""
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
Write-Host "Device should now show LED test pattern on all 3 strips and connect to WiFi." -ForegroundColor White
Write-Host "  - Strip 1 (GPIO10): 7 LEDs" -ForegroundColor Cyan
Write-Host "  - Strip 2 (GPIO0):  4 LEDs" -ForegroundColor Cyan
Write-Host "  - Strip 3 (GPIO1):  4 LEDs" -ForegroundColor Cyan
Write-Host ""
Read-Host "Press Enter to exit"
