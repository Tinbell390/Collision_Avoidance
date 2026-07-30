#!/bin/bash

set -e

echo "========================================"
echo "       Project Setup"
echo "========================================"

#--------------------------------------------------
# Python仮想環境
#--------------------------------------------------
if [ ! -d ".venv" ]; then
    echo ""
    echo "[1/4] Creating Python virtual environment..."
    python3 -m venv .venv
else
    echo ""
    echo "[1/4] Python virtual environment already exists."
fi

source .venv/bin/activate

echo ""
echo "[2/4] Installing Python packages..."

python -m pip install --upgrade pip
pip install -r requirements.txt

#--------------------------------------------------
# PlatformIO
#--------------------------------------------------
echo ""
echo "[3/4] Checking PlatformIO..."

if [ -x "$HOME/.platformio/penv/bin/pio" ]; then
    PIO="$HOME/.platformio/penv/bin/pio"
elif command -v pio >/dev/null 2>&1; then
    PIO=$(command -v pio)
else
    echo ""
    echo "ERROR: PlatformIO not found."
    echo "Please install the PlatformIO VSCode extension."
    exit 1
fi

echo "Using: $PIO"

#--------------------------------------------------
# PlatformIO Environment
#--------------------------------------------------
echo ""
echo "[4/4] Checking PlatformIO environments..."

if [ ! -d ".pio/build/vehicle" ]; then
    echo "Creating Vehicle environment..."
    "$PIO" run -e vehicle
else
    echo "Vehicle environment already exists."
fi

if [ ! -d ".pio/build/monitor" ]; then
    echo "Creating Monitor environment..."
    "$PIO" run -e monitor
else
    echo "Monitor environment already exists."
fi

echo ""
echo "========================================"
echo " Setup Complete!"
echo "========================================"
echo ""
echo "To activate the Python environment:"
echo ""
echo "source .venv/bin/activate"
echo ""