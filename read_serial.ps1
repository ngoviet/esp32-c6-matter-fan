$port = New-Object System.IO.Ports.SerialPort COM3, 115200, None, 8, One
$port.DtrEnable = $true
$port.RtsEnable = $true
$port.Open()

$port.DtrEnable = $false
$port.RtsEnable = $false
Start-Sleep -Milliseconds 100
$port.DtrEnable = $true
$port.RtsEnable = $true

$endTime = (Get-Date).AddSeconds(5)
$buffer = ""
while ((Get-Date) -lt $endTime) {
    if ($port.BytesToRead -gt 0) {
        $buffer += $port.ReadExisting()
    }
    Start-Sleep -Milliseconds 100
}
$port.Close()
Write-Output $buffer
