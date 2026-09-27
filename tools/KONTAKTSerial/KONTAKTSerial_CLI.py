#!/usr/bin/env python3
"""KONTAKTSerial CLI - service UART0/P1 client for ESP32-8048S043."""

from __future__ import annotations

import argparse
import sys
import time

import serial
from serial.tools import list_ports

APP_VERSION = "0.3.0"
DEFAULT_BAUD = 115200


def list_serial_ports() -> int:
    ports = list(list_ports.comports())
    if not ports:
        print("No serial ports found.")
        return 1
    for info in ports:
        desc = info.description or ""
        vidpid = ""
        if info.vid is not None and info.pid is not None:
            vidpid = f" VID:PID={info.vid:04X}:{info.pid:04X}"
        print(f"{info.device:8} {desc}{vidpid}")
    return 0


def read_response(ser: serial.Serial, idle_timeout: float = 0.20, total_timeout: float = 70.0) -> str:
    deadline = time.monotonic() + total_timeout
    idle_deadline = time.monotonic() + idle_timeout
    data = bytearray()

    while time.monotonic() < deadline:
        chunk = ser.read(ser.in_waiting or 1)
        if chunk:
            data.extend(chunk)
            idle_deadline = time.monotonic() + idle_timeout
        elif data and time.monotonic() >= idle_deadline:
            break

    return data.decode("utf-8", errors="replace").replace("\r", "").strip()


def send_command(ser: serial.Serial, command: str) -> str:
    ser.reset_input_buffer()
    ser.write(command.encode("utf-8") + b"\r\n")
    ser.flush()
    # SCAN can legitimately take much longer than normal commands.
    timeout = 70.0 if command.strip().upper() == "MA01 SCAN" else 3.0
    return read_response(ser, total_timeout=timeout)


def open_port(port: str, baud: int) -> serial.Serial:
    return serial.Serial(
        port=port,
        baudrate=baud,
        bytesize=serial.EIGHTBITS,
        parity=serial.PARITY_NONE,
        stopbits=serial.STOPBITS_ONE,
        timeout=0.05,
        write_timeout=1.0,
        xonxoff=False,
        rtscts=False,
        dsrdtr=False,
    )


def interactive(ser: serial.Serial) -> int:
    print(f"KONTAKTSerial CLI {APP_VERSION}")
    print(f"Connected: {ser.port} @ {ser.baudrate} 8N1")
    print("Type HELP for firmware commands. Type EXIT to quit.")
    while True:
        try:
            command = input("UART0> ").strip()
        except (EOFError, KeyboardInterrupt):
            print()
            return 0
        if not command:
            continue
        if command.upper() in {"EXIT", "QUIT"}:
            return 0
        try:
            response = send_command(ser, command)
        except serial.SerialException as exc:
            print(f"Serial error: {exc}", file=sys.stderr)
            return 2
        print(response if response else "(no response)")


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description=f"KONTAKTSerial CLI {APP_VERSION} - ESP32 service UART0/P1 client"
    )
    parser.add_argument("--list", action="store_true", help="List serial ports and exit")
    parser.add_argument("--port", help="Service COM port, for example COM4")
    parser.add_argument("--baud", type=int, default=DEFAULT_BAUD, help="Baud rate (default 115200)")
    parser.add_argument("--command", "-c", help="Send one command and exit")
    return parser.parse_args()


def main() -> int:
    args = parse_args()
    if args.list:
        return list_serial_ports()
    if not args.port:
        print("Specify --port COMx, or use --list.", file=sys.stderr)
        return 2

    try:
        with open_port(args.port, args.baud) as ser:
            if args.command:
                response = send_command(ser, args.command)
                print(response if response else "(no response)")
                return 0
            return interactive(ser)
    except serial.SerialException as exc:
        print(f"Cannot open {args.port}: {exc}", file=sys.stderr)
        return 2


if __name__ == "__main__":
    raise SystemExit(main())
