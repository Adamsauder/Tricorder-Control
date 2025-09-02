# Build All Prop Control Firmware
Write-Host "=====================================" -ForegroundColor Cyan
Write-Host " Building All Prop Control Firmware" -ForegroundColor Cyan
Write-Host "=====================================" -ForegroundColor Cyan
Write-Host ""

# Create firmware output directory
if (!(Test-Path "server\firmware_binaries")) {
    New-Item -ItemType Directory -Path "server\firmware_binaries" -Force | Out-Null
}

# Build Tricorder Firmware
Write-Host "[1/4] Building Tricorder Firmware..." -ForegroundColor Yellow
Set-Location "firmware\tricorder"
& pio run
if ($LASTEXITCODE -ne 0) {
    Write-Host "ERROR: Tricorder build failed!" -ForegroundColor Red
    Read-Host "Press Enter to exit"
    exit 1
}
Copy-Item ".pio\build\tricorder\firmware.bin" "..\..\server\firmware_binaries\tricorder_firmware.bin"
Write-Host "✓ Tricorder firmware built and copied" -ForegroundColor Green
Write-Host ""

# Build Polyinoculator Firmware
Write-Host "[2/4] Building Polyinoculator Firmware..." -ForegroundColor Yellow
Set-Location "..\polyinoculator"
& pio run
if ($LASTEXITCODE -ne 0) {
    Write-Host "ERROR: Polyinoculator build failed!" -ForegroundColor Red
    Read-Host "Press Enter to exit"
    exit 1
}
Copy-Item ".pio\build\polyinoculator\firmware.bin" "..\..\server\firmware_binaries\polyinoculator_firmware.bin"
Write-Host "✓ Polyinoculator firmware built and copied" -ForegroundColor Green
Write-Host ""

# Build Defragmentor Firmware
Write-Host "[3/4] Building Defragmentor Firmware..." -ForegroundColor Yellow
Set-Location "..\defragmentor"
& pio run
if ($LASTEXITCODE -ne 0) {
    Write-Host "ERROR: Defragmentor build failed!" -ForegroundColor Red
    Read-Host "Press Enter to exit"
    exit 1
}
Copy-Item ".pio\build\defragmentor\firmware.bin" "..\..\server\firmware_binaries\defragmentor_firmware.bin"
Write-Host "✓ Defragmentor firmware built and copied" -ForegroundColor Green
Write-Host ""

# Build IV Injector Firmware
Write-Host "[4/4] Building IV Injector Firmware..." -ForegroundColor Yellow
Set-Location "..\iv_injector"
& pio run
if ($LASTEXITCODE -ne 0) {
    Write-Host "ERROR: IV Injector build failed!" -ForegroundColor Red
    Read-Host "Press Enter to exit"
    exit 1
}
Copy-Item ".pio\build\iv_injector\firmware.bin" "..\..\server\firmware_binaries\iv_injector_firmware.bin"
Write-Host "✓ IV Injector firmware built and copied" -ForegroundColor Green
Write-Host ""

Write-Host "=====================================" -ForegroundColor Cyan
Write-Host " All Firmware Built Successfully!" -ForegroundColor Cyan
Write-Host "=====================================" -ForegroundColor Cyan
Write-Host ""
Write-Host "Firmware files available at: server\firmware_binaries\" -ForegroundColor White
Write-Host "- tricorder_firmware.bin" -ForegroundColor Gray
Write-Host "- polyinoculator_firmware.bin" -ForegroundColor Gray
Write-Host "- defragmentor_firmware.bin" -ForegroundColor Gray
Write-Host "- iv_injector_firmware.bin" -ForegroundColor Gray
Write-Host ""

Set-Location "..\..\"
Read-Host "Press Enter to continue"
