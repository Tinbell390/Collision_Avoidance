"""Shared serial-port management for the experiment monitor GUIs.

Connection and identification are performed off the Tk main thread. The GUI polls
message_queue and event_queue; event items are ("connected", port) or
("disconnected", reason).
"""
from __future__ import annotations

import queue
import threading
import time
from dataclasses import dataclass

import serial
from serial.tools import list_ports

EXPECTED_USB_IDS = {
    (0x303A, 0x1001): "XIAO ESP32-C3 USB-Serial/JTAG",
    (0x10C4, 0xEA60): "CP210x USB-UART",
    (0x1A86, 0x7523): "CH340 USB-UART",
}


@dataclass(frozen=True)
class PortInfo:
    device: str
    description: str
    vid: int | None
    pid: int | None
    serial_number: str | None
    recommended: bool

    @property
    def label(self) -> str:
        vidpid = f"{self.vid:04X}:{self.pid:04X}" if self.vid is not None and self.pid is not None else "VID:PID unknown"
        marker = " ★" if self.recommended else " (not recommended)"
        return f"{self.device} - {self.description} [{vidpid}]{marker}"


class SerialManager:
    def __init__(self):
        self.ser = None
        self.running = False
        self.receive_thread = None
        self.connect_thread = None
        self.message_queue = queue.Queue()
        self.event_queue = queue.Queue()
        self._lock = threading.RLock()
        self._connecting = False
        self._ports_by_label = {}

    @property
    def connecting(self):
        with self._lock:
            return self._connecting

    @property
    def connected(self):
        with self._lock:
            return self.ser is not None and self.ser.is_open

    def log(self, text):
        print(text)
        self.message_queue.put(str(text))

    def get_message(self):
        try:
            return self.message_queue.get_nowait()
        except queue.Empty:
            return None

    def get_event(self):
        try:
            return self.event_queue.get_nowait()
        except queue.Empty:
            return None

    @staticmethod
    def port_details():
        result = []
        for p in list_ports.comports():
            recommended = (p.vid, p.pid) in EXPECTED_USB_IDS
            result.append(PortInfo(p.device, p.description or "Serial device", p.vid, p.pid, p.serial_number, recommended))
        result.sort(key=lambda p: (not p.recommended, p.device.casefold()))
        return result

    @classmethod
    def get_ports(cls):
        return [p.label for p in cls.port_details()]

    @classmethod
    def diagnose_ports(cls):
        return cls.port_details()

    def connect(self, port_label, baudrate=115200, expected_id=None):
        with self._lock:
            if self.ser is not None or self._connecting:
                return False
            self._connecting = True
        details = self.port_details()
        match = next((p for p in details if p.label == port_label or p.device == port_label), None)
        if match is None:
            with self._lock:
                self._connecting = False
            self.log("Connection Error: selected serial port is no longer available. Refresh the list.")
            self.event_queue.put(("connect_failed", "Port not available"))
            return False
        self.connect_thread = threading.Thread(target=self._connect_worker, args=(match, baudrate, expected_id), daemon=True)
        self.connect_thread.start()
        self.log(f"Connecting: {match.device} ...")
        return True

    def _connect_worker(self, port, baudrate, expected_id):
        ser = None
        try:
            ser = serial.Serial(port=None, baudrate=baudrate, timeout=0.1, write_timeout=1.0)
            try:
                ser.dtr = False
                ser.rts = False
            except (OSError, serial.SerialException):
                pass
            ser.port = port.device
            started = time.monotonic()
            ser.open()
            if time.monotonic() - started > 5.0:
                raise TimeoutError("Serial port opening took longer than 5 seconds")
            time.sleep(1.5)
            ser.reset_input_buffer()
            response = ""
            for attempt in range(3):  # initial attempt + at most two retries
                ser.write(b"WHOAMI\n")
                deadline = time.monotonic() + 0.75
                while time.monotonic() < deadline:
                    line = ser.readline()
                    if line:
                        candidate = line.decode(errors="replace").strip()
                        if candidate.startswith("WHOAMI "):
                            response = candidate
                            break
                if response:
                    break
            if not response.startswith("WHOAMI "):
                raise RuntimeError("No WHOAMI response. Check port, firmware, and USB data cable.")
            actual = response.split(maxsplit=1)[1].strip()
            if expected_id and actual != expected_id:
                raise RuntimeError(f"Wrong firmware: detected '{actual}'. Flash firmware for '{expected_id}'.")
            with self._lock:
                if not self._connecting:
                    ser.close()
                    return
                self.ser = ser
                self.running = True
                self._connecting = False
                self.receive_thread = threading.Thread(target=self.receive_loop, daemon=True)
                self.receive_thread.start()
            self.log(f"Connected: {port.device} ({actual})")
            self.event_queue.put(("connected", port.device))
            ser = None
        except Exception as exc:
            self.log(f"Connection Error: {exc}")
            self.event_queue.put(("connect_failed", str(exc)))
        finally:
            if ser is not None:
                try:
                    ser.close()
                except Exception:
                    pass
            with self._lock:
                self._connecting = False

    def disconnect(self):
        with self._lock:
            self.running = False
            ser, self.ser = self.ser, None
            connecting = self._connecting
            self._connecting = False
            rx = self.receive_thread
        if rx and rx.is_alive() and rx is not threading.current_thread():
            rx.join(timeout=1.0)
        if ser:
            try:
                ser.close()
            except Exception:
                pass
        if connecting:
            self.log("Connection attempt cancelled.")
        else:
            self.log("Disconnected")
        self.event_queue.put(("disconnected", "User disconnected"))

    def send(self, command):
        with self._lock:
            ser = self.ser
        if ser is None or not ser.is_open:
            self.log("Not connected")
            return
        try:
            ser.write((str(command).rstrip("\n") + "\n").encode())
            self.log("> " + str(command))
        except Exception as exc:
            self.log(f"Send Error: {exc}")
            self.event_queue.put(("disconnected", str(exc)))

    def send_hello(self):
        self.send("HELLO")

    def send_addpeer(self, mac):
        self.send(f"addpeer {mac}")

    def send_ping(self, mac):
        self.send(f"PING {mac}")

    def send_datareq(self, mac):
        self.send(f"DATAREQ {mac}")

    def receive_loop(self):
        while self.running:
            with self._lock:
                ser = self.ser
            if ser is None:
                break
            try:
                line = ser.readline()
                if not line:
                    continue
                text = line.decode(errors="replace").strip()
                if text:
                    self.log(text)
            except (serial.SerialException, OSError) as exc:
                if self.running:
                    self.log(f"Receive Error: {exc}")
                    with self._lock:
                        self.running = False
                        if self.ser is ser:
                            self.ser = None
                    try:
                        ser.close()
                    except Exception:
                        pass
                    self.event_queue.put(("disconnected", str(exc)))
                break
        self.running = False
