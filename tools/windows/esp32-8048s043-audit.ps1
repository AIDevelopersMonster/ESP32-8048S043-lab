param(
    [string]$Port = "",
    [switch]$IncludeMac
)

$stamp = Get-Date -Format "yyyyMMdd-HHmmss"
$out = "esp32-8048s043-audit-$stamp.txt"

"ESP32-8048S043 Lab passive audit" | Tee-Object -FilePath $out
"Timestamp: $(Get-Date -Format o)" | Tee-Object -FilePath $out -Append
"Computer: $env:COMPUTERNAME" | Tee-Object -FilePath $out -Append
"User: $env:USERNAME" | Tee-Object -FilePath $out -Append
"" | Tee-Object -FilePath $out -Append

"[Serial ports]" | Tee-Object -FilePath $out -Append
Get-CimInstance Win32_SerialPort | Select-Object DeviceID,Name,PNPDeviceID | Format-List | Out-String | Tee-Object -FilePath $out -Append

if ($Port) {
    "[esptool passive chip query]" | Tee-Object -FilePath $out -Append
    $args = @("-m", "esptool", "--chip", "esp32s3", "--port", $Port, "chip_id")
    python $args 2>&1 | Tee-Object -FilePath $out -Append
    python -m esptool --chip esp32s3 --port $Port flash_id 2>&1 | Tee-Object -FilePath $out -Append
    if ($IncludeMac) {
        python -m esptool --chip esp32s3 --port $Port read_mac 2>&1 | Tee-Object -FilePath $out -Append
    } else {
        "MAC read skipped. Use -IncludeMac only if you intentionally want it in the report." | Tee-Object -FilePath $out -Append
    }
}

"Audit written to $out"
