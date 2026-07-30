$ErrorActionPreference = "Stop"

Write-Host "========================================"
Write-Host "       Project Setup"
Write-Host "========================================"

#--------------------------------------------------
# Python Virtual Environment
#--------------------------------------------------
if (-Not (Test-Path ".venv")) {
    Write-Host ""
    Write-Host "[1/4] Creating Python virtual environment..."
    py -m venv .venv
}
else {
    Write-Host ""
    Write-Host "[1/4] Python virtual environment already exists."
}

# 仮想環境有効化
& ".\.venv\Scripts\Activate.ps1"

Write-Host ""
Write-Host "[2/4] Installing Python packages..."

python -m pip install --upgrade pip
pip install -r requirements.txt

#--------------------------------------------------
# PlatformIO
#--------------------------------------------------
Write-Host ""
Write-Host "[3/4] Checking PlatformIO..."

$PIO = $null

# PlatformIO Core
if (Test-Path "$env:USERPROFILE\.platformio\penv\Scripts\pio.exe") {
    $PIO = "$env:USERPROFILE\.platformio\penv\Scripts\pio.exe"
}
elseif (Get-Command pio -ErrorAction SilentlyContinue) {
    $PIO = (Get-Command pio).Source
}

if (-Not $PIO) {
    Write-Host ""
    Write-Host "ERROR: PlatformIO not found."
    Write-Host "Please install the PlatformIO VSCode extension."
    exit 1
}

Write-Host "Using: $PIO"

#--------------------------------------------------
# PlatformIO Environment
#--------------------------------------------------
Write-Host ""
Write-Host "[4/4] Checking PlatformIO environments..."

if (-Not (Test-Path ".pio\build\vehicle")) {
    Write-Host "Creating Vehicle environment..."
    & $PIO run -e vehicle
}
else {
    Write-Host "Vehicle environment already exists."
}

if (-Not (Test-Path ".pio\build\monitor")) {
    Write-Host "Creating Monitor environment..."
    & $PIO run -e monitor
}
else {
    Write-Host "Monitor environment already exists."
}

Write-Host ""
Write-Host "========================================"
Write-Host " Setup Complete!"
Write-Host "========================================"
Write-Host ""
Write-Host "To activate the Python environment:"
Write-Host ""
Write-Host ".\.venv\Scripts\Activate.ps1"
Write-Host ""