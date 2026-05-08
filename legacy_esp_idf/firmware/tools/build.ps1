$ErrorActionPreference = "Stop"
$here = Split-Path -Parent $MyInvocation.MyCommand.Path
$firmware = Resolve-Path (Join-Path $here "..")
Push-Location $firmware
try {
    if (-not $env:IDF_PATH) {
        Write-Error "IDF_PATH is not set. Run ESP-IDF export.ps1 first."
    }
    idf.py set-target esp32s3
    idf.py build
} finally {
    Pop-Location
}
