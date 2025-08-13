# Enhanced Polyinoculator Flash Script - PowerShell
# Flashes the enhanced firmware with persistent configuration support

Clear-Host
Write-Host "===============================================" -ForegroundColor Cyan
Write-Host "    Enhanced Polyinoculator Flash Utility" -ForegroundColor Yellow
Write-Host "===============================================" -ForegroundColor Cyan
Write-Host ""
Write-Host "Features in this firmware:" -ForegroundColor Green
Write-Host "- Persistent configuration storage (NVRAM)" -ForegroundColor White
Write-Host "- Web-based configuration interface" -ForegroundColor White
Write-Host "- Individual prop addressing" -ForegroundColor White
Write-Host "- Device label customization" -ForegroundColor White
Write-Host "- SACN universe/address management" -ForegroundColor White
Write-Host ""
Write-Host "Hardware Configuration:" -ForegroundColor Green
Write-Host "- LED Strip 1: D5 (GPIO5) - 7 LEDs" -ForegroundColor White
Write-Host "- LED Strip 2: D6 (GPIO16) - 4 LEDs" -ForegroundColor White
Write-Host "- LED Strip 3: D8 (GPIO20) - 4 LEDs" -ForegroundColor White
Write-Host "- Total: 15 LEDs (45 DMX channels)" -ForegroundColor Yellow
Write-Host ""

# Check if device is connected
Write-Host "Checking for connected ESP32-C3 device..." -ForegroundColor Cyan
$devices = pio device list | Select-String "COM"
if ($devices.Count -eq 0) {
    Write-Host ""
    Write-Host "ERROR: No ESP32-C3 device detected!" -ForegroundColor Red
    Write-Host ""
    Write-Host "Please check:" -ForegroundColor Yellow
    Write-Host "1. Device is connected via USB-C cable" -ForegroundColor White
    Write-Host "2. Drivers are installed" -ForegroundColor White
    Write-Host "3. Device is in bootloader mode if needed" -ForegroundColor White
    Write-Host ""
    Read-Host "Press Enter to continue"
    exit 1
}

Write-Host "Device detected! Proceeding with flash..." -ForegroundColor Green
Write-Host ""

# Change to script directory
Set-Location $PSScriptRoot

# Clean build to ensure fresh compilation
Write-Host "Cleaning previous build..." -ForegroundColor Cyan
$cleanResult = pio run --target clean
if ($LASTEXITCODE -ne 0) {
    Write-Host ""
    Write-Host "ERROR: Failed to clean build directory!" -ForegroundColor Red
    Read-Host "Press Enter to continue"
    exit 1
}

Write-Host ""
Write-Host "Building enhanced firmware..." -ForegroundColor Cyan
$buildResult = pio run
if ($LASTEXITCODE -ne 0) {
    Write-Host ""
    Write-Host "ERROR: Firmware compilation failed!" -ForegroundColor Red
    Write-Host "Check the code for syntax errors." -ForegroundColor Yellow
    Read-Host "Press Enter to continue"
    exit 1
}

Write-Host ""
Write-Host "Flashing firmware to device..." -ForegroundColor Cyan
$uploadResult = pio run --target upload
if ($LASTEXITCODE -ne 0) {
    Write-Host ""
    Write-Host "ERROR: Firmware upload failed!" -ForegroundColor Red
    Write-Host ""
    Write-Host "Troubleshooting:" -ForegroundColor Yellow
    Write-Host "1. Try holding BOOT button while connecting USB" -ForegroundColor White
    Write-Host "2. Check COM port in Device Manager" -ForegroundColor White
    Write-Host "3. Try different USB cable/port" -ForegroundColor White
    Write-Host ""
    Read-Host "Press Enter to continue"
    exit 1
}

Write-Host ""
Write-Host "===============================================" -ForegroundColor Green
Write-Host "           FLASH SUCCESSFUL!" -ForegroundColor Yellow
Write-Host "===============================================" -ForegroundColor Green
Write-Host ""
Write-Host "The enhanced polyinoculator firmware has been flashed." -ForegroundColor Green
Write-Host ""
Write-Host "Next Steps:" -ForegroundColor Cyan
Write-Host "1. Device will restart automatically" -ForegroundColor White
Write-Host "2. Connect to serial monitor to see boot messages" -ForegroundColor White
Write-Host "3. Device will connect to WiFi: 'Rigging Electric'" -ForegroundColor White
Write-Host "4. Access web interface at device IP address" -ForegroundColor White
Write-Host "5. Configure device label and SACN addressing" -ForegroundColor White
Write-Host ""
Write-Host "Default Configuration:" -ForegroundColor Cyan
Write-Host "- Device Label: Polyinoculator_XXXX (random)" -ForegroundColor White
Write-Host "- SACN Universe: 1" -ForegroundColor White
Write-Host "- DMX Start Address: 1" -ForegroundColor White
Write-Host "- Brightness: 128/255" -ForegroundColor White
Write-Host ""

# Start serial monitor automatically
$choice = Read-Host "Start serial monitor to see device boot? (y/n)"
if ($choice -match "^[Yy]$") {
    Write-Host ""
    Write-Host "Starting serial monitor... (Press Ctrl+C to exit)" -ForegroundColor Yellow
    Start-Sleep 2
    pio device monitor
}

Write-Host ""
Write-Host "Flash process complete!" -ForegroundColor Green
Read-Host "Press Enter to continue"
