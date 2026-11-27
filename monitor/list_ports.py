#!/usr/bin/env python3
"""Print serial-port metadata without opening any port."""
from serial_common import SerialManager, EXPECTED_USB_IDS


def main():
    ports = SerialManager.diagnose_ports()
    if not ports:
        print("No serial ports detected.")
        print("Check that the USB cable supports data, reconnect the board, and check permissions.")
        return 1

    print("Serial port diagnostic (ports are not opened)")
    print("-" * 96)
    print(f"{'Recommended':<12} {'Device':<14} {'VID:PID':<12} {'Serial':<22} Description")
    print("-" * 96)
    for p in ports:
        vidpid = f"{p.vid:04X}:{p.pid:04X}" if p.vid is not None and p.pid is not None else "unknown"
        serial_no = p.serial_number or "-"
        print(f"{'YES ★' if p.recommended else 'NO':<12} {p.device:<14} {vidpid:<12} {serial_no:<22} {p.description}")
    print("\nRecommended USB-serial IDs:")
    for (vid, pid), name in sorted(EXPECTED_USB_IDS.items()):
        print(f"  {vid:04X}:{pid:04X}  {name}")
    if not any(p.recommended for p in ports):
        print("No known USB-serial ID detected. A confirmation is required in the monitor GUI.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
