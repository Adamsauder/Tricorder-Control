# PowerShell script for OTA firmware upload
param(
    [Parameter(Mandatory=$true)]
    [string]$DeviceIP,
    
    [Parameter(Mandatory=$true)]
    [string]$FirmwarePath
)

# Read firmware file
$firmwareBytes = [System.IO.File]::ReadAllBytes($FirmwarePath)
$boundary = [System.Guid]::NewGuid().ToString()

# Create multipart form data
$LF = "`r`n"
$bodyLines = @(
    "--$boundary",
    "Content-Disposition: form-data; name=`"firmware`"; filename=`"firmware.bin`"",
    "Content-Type: application/octet-stream",
    "",
    [System.Text.Encoding]::GetEncoding("iso-8859-1").GetString($firmwareBytes),
    "--$boundary--"
)

$body = ($bodyLines -join $LF)
$bodyBytes = [System.Text.Encoding]::GetEncoding("iso-8859-1").GetBytes($body)

try {
    $response = Invoke-RestMethod -Uri "http://$DeviceIP/update" -Method POST -Body $bodyBytes -ContentType "multipart/form-data; boundary=$boundary" -TimeoutSec 60
    Write-Host "Upload successful: $response"
} catch {
    Write-Host "Upload result (connection reset expected): $($_.Exception.Message)"
}
