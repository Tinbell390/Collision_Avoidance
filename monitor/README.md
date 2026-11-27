# Serial monitor / port diagnostic

## Shared serial management

All experiment monitor programs use `serial_common.SerialManager` for port enumeration,
connection, disconnect, command sending, and receive-thread management. Experiments 2
and 3 keep their CSV transfer, plotting, and analysis handlers in their own monitor
classes while inheriting connection handling.

## Firmware identity check

Before a monitor starts its receive thread, it sends `WHOAMI` and verifies the reply:

| Monitor | Expected reply |
|---|---|
| `exp0_monitor.py` | `WHOAMI test` |
| `exp1_monitor.py` | `WHOAMI espnow` |
| `exp2_monitor.py` | `WHOAMI monitor` |
| `exp3_monitor.py` | `WHOAMI monitor` |

If a board is running the wrong firmware, the monitor closes the serial port and reports
which firmware was detected and which firmware is expected. No connection is accepted
if the firmware does not respond to `WHOAMI`.

## Port recommendations

The GUI displays port name, description, VID:PID, and (when available) serial number.
Recognized USB serial IDs are sorted first:

- XIAO ESP32-C3 USB-Serial/JTAG: `303A:1001`
- CP210x: `10C4:EA60`
- CH340: `1A86:7523`

Unknown devices are marked as not recommended and require confirmation before connection.
Refresh the port list using the Refresh button or by opening the port selector.

## Diagnostic command

Run from the repository root:

```bash
python monitor/list_ports.py
```

The command only enumerates ports; it does not open them. If no ports are detected,
check that the USB cable supports data, reconnect the board, and verify serial-port
permissions (Linux users may need to log out after joining the `dialout` group).

## Connection behavior

- Connection, firmware verification, and port opening run in a worker thread.
- The GUI stays responsive while a port is opening.
- Opening/identification has a 5-second GUI watchdog.
- Firmware identification waits briefly for boot and retries up to two times.
- Receive-thread errors are queued for the GUI; the manager closes the port and resets
  the Connect button state.
- Disconnect stops receiving, joins the receive thread for up to 1 second, and closes
  the serial port.
