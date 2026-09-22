#!/usr/bin/env python3
"""KONTAKTSerial - small Windows-friendly serial terminal for ESP32 lab work."""

from __future__ import annotations

import argparse
import queue
import sys
import threading
import time
import tkinter as tk
from datetime import datetime
from tkinter import filedialog, messagebox, ttk
from tkinter.scrolledtext import ScrolledText

try:
    import serial
    from serial.tools import list_ports
except ImportError:
    print("KONTAKTSerial requires pyserial. Install it with: python -m pip install pyserial")
    raise

APP_NAME = "KONTAKTSerial"
APP_VERSION = "0.1.0"
DEFAULT_BAUD = 115200


def port_label(info) -> str:
    details = []
    if info.description and info.description != "n/a":
        details.append(info.description)
    if info.vid is not None and info.pid is not None:
        details.append(f"VID:PID={info.vid:04X}:{info.pid:04X}")
    suffix = " - " + " | ".join(details) if details else ""
    return f"{info.device}{suffix}"


class KontaktSerial(tk.Tk):
    def __init__(self, initial_port: str | None = None, initial_baud: int = DEFAULT_BAUD):
        super().__init__()
        self.title(f"{APP_NAME} {APP_VERSION}")
        self.geometry("980x680")
        self.minsize(760, 500)

        self.ser: serial.Serial | None = None
        self.stop_event = threading.Event()
        self.reader_thread: threading.Thread | None = None
        self.rx_queue: queue.Queue[bytes | tuple[str, str]] = queue.Queue()
        self.rx_bytes = 0
        self.tx_bytes = 0
        self.port_map: dict[str, str] = {}

        self.port_var = tk.StringVar(value=initial_port or "")
        self.baud_var = tk.StringVar(value=str(initial_baud))
        self.line_ending_var = tk.StringVar(value="CRLF")
        self.status_var = tk.StringVar(value="Disconnected")
        self.counter_var = tk.StringVar(value="RX 0 B   TX 0 B")
        self.timestamps_var = tk.BooleanVar(value=False)
        self.autoscroll_var = tk.BooleanVar(value=True)
        self.show_tx_var = tk.BooleanVar(value=True)
        self.hex_rx_var = tk.BooleanVar(value=False)

        self._build_ui()
        self.refresh_ports(prefer=initial_port)
        self.after(50, self._drain_rx_queue)
        self.protocol("WM_DELETE_WINDOW", self._on_close)

    def _build_ui(self) -> None:
        top = ttk.Frame(self, padding=8)
        top.pack(fill="x")

        ttk.Label(top, text="Port").grid(row=0, column=0, padx=(0, 4))
        self.port_combo = ttk.Combobox(top, textvariable=self.port_var, state="readonly", width=42)
        self.port_combo.grid(row=0, column=1, sticky="ew", padx=(0, 8))

        ttk.Button(top, text="Refresh", command=self.refresh_ports).grid(row=0, column=2, padx=(0, 12))

        ttk.Label(top, text="Baud").grid(row=0, column=3, padx=(0, 4))
        self.baud_combo = ttk.Combobox(
            top,
            textvariable=self.baud_var,
            values=("9600", "19200", "38400", "57600", "115200", "230400", "460800", "921600"),
            width=10,
        )
        self.baud_combo.grid(row=0, column=4, padx=(0, 8))

        self.connect_button = ttk.Button(top, text="Connect", command=self.toggle_connection)
        self.connect_button.grid(row=0, column=5)
        top.columnconfigure(1, weight=1)

        options = ttk.Frame(self, padding=(8, 0, 8, 6))
        options.pack(fill="x")
        ttk.Checkbutton(options, text="Timestamps", variable=self.timestamps_var).pack(side="left", padx=(0, 12))
        ttk.Checkbutton(options, text="Auto-scroll", variable=self.autoscroll_var).pack(side="left", padx=(0, 12))
        ttk.Checkbutton(options, text="Show TX", variable=self.show_tx_var).pack(side="left", padx=(0, 12))
        ttk.Checkbutton(options, text="HEX RX", variable=self.hex_rx_var).pack(side="left", padx=(0, 12))
        ttk.Button(options, text="Clear", command=self.clear_terminal).pack(side="right")
        ttk.Button(options, text="Save log...", command=self.save_log).pack(side="right", padx=(0, 8))

        self.terminal = ScrolledText(self, wrap="word", font=("Consolas", 10), state="disabled")
        self.terminal.pack(fill="both", expand=True, padx=8, pady=(0, 8))
        self.terminal.tag_configure("rx")
        self.terminal.tag_configure("tx")
        self.terminal.tag_configure("status")

        send = ttk.Frame(self, padding=(8, 0, 8, 8))
        send.pack(fill="x")
        ttk.Label(send, text="Send").grid(row=0, column=0, padx=(0, 6))

        self.send_entry = ttk.Entry(send)
        self.send_entry.grid(row=0, column=1, sticky="ew", padx=(0, 8))
        self.send_entry.bind("<Return>", lambda _event: self.send_text())

        ttk.Label(send, text="Ending").grid(row=0, column=2, padx=(0, 4))
        ttk.Combobox(
            send,
            textvariable=self.line_ending_var,
            state="readonly",
            values=("None", "LF", "CR", "CRLF"),
            width=7,
        ).grid(row=0, column=3, padx=(0, 8))

        ttk.Button(send, text="Send", command=self.send_text).grid(row=0, column=4)
        ttk.Button(send, text="HELLO123", command=lambda: self.send_literal("HELLO123")).grid(row=0, column=5, padx=(8, 0))
        send.columnconfigure(1, weight=1)

        bottom = ttk.Frame(self, padding=(8, 0, 8, 8))
        bottom.pack(fill="x")
        self.status_label = ttk.Label(bottom, textvariable=self.status_var)
        self.status_label.pack(side="left")
        ttk.Label(bottom, textvariable=self.counter_var).pack(side="right")

    def refresh_ports(self, prefer: str | None = None) -> None:
        infos = sorted(list_ports.comports(), key=lambda p: p.device)
        labels = [port_label(info) for info in infos]
        self.port_map = {label: info.device for label, info in zip(labels, infos)}
        self.port_combo["values"] = labels

        current_device = self._selected_device()
        preferred = prefer or current_device

        chosen = None
        if preferred:
            for label, device in self.port_map.items():
                if device.upper() == preferred.upper():
                    chosen = label
                    break

        if chosen is None:
            for label, info in zip(labels, infos):
                desc = f"{info.description} {info.manufacturer}".lower()
                if "ch340" in desc or "usb-serial" in desc or "wch" in desc:
                    chosen = label
                    break

        if chosen is None and labels:
            chosen = labels[0]

        if chosen:
            self.port_var.set(chosen)
        elif not self.ser:
            self.port_var.set("")

    def _selected_device(self) -> str:
        value = self.port_var.get().strip()
        if value in self.port_map:
            return self.port_map[value]
        if value.upper().startswith("COM"):
            return value.split()[0]
        return ""

    def toggle_connection(self) -> None:
        if self.ser and self.ser.is_open:
            self.disconnect()
        else:
            self.connect()

    def connect(self) -> None:
        port = self._selected_device()
        if not port:
            messagebox.showerror(APP_NAME, "Select a serial port first.")
            return
        try:
            baud = int(self.baud_var.get())
            if baud <= 0:
                raise ValueError
        except ValueError:
            messagebox.showerror(APP_NAME, "Baud rate must be a positive integer.")
            return

        try:
            self.ser = serial.Serial(
                port=port,
                baudrate=baud,
                bytesize=serial.EIGHTBITS,
                parity=serial.PARITY_NONE,
                stopbits=serial.STOPBITS_ONE,
                timeout=0.10,
                write_timeout=1.0,
                xonxoff=False,
                rtscts=False,
                dsrdtr=False,
            )
        except serial.SerialException as exc:
            self.ser = None
            messagebox.showerror(APP_NAME, f"Cannot open {port}:\n{exc}")
            return

        self.stop_event.clear()
        self.reader_thread = threading.Thread(target=self._reader_loop, name="serial-rx", daemon=True)
        self.reader_thread.start()
        self.connect_button.configure(text="Disconnect")
        self.port_combo.configure(state="disabled")
        self.baud_combo.configure(state="disabled")
        self.status_var.set(f"Connected: {port} @ {baud} 8N1, no flow control")
        self._append(f"[CONNECTED] {port} @ {baud} 8N1\n", "status")
        self.send_entry.focus_set()

    def disconnect(self) -> None:
        self.stop_event.set()
        ser = self.ser
        self.ser = None
        if ser:
            try:
                ser.close()
            except serial.SerialException:
                pass
        self.connect_button.configure(text="Connect")
        self.port_combo.configure(state="readonly")
        self.baud_combo.configure(state="normal")
        self.status_var.set("Disconnected")
        self._append("[DISCONNECTED]\n", "status")

    def _reader_loop(self) -> None:
        while not self.stop_event.is_set():
            ser = self.ser
            if ser is None or not ser.is_open:
                break
            try:
                data = ser.read(ser.in_waiting or 1)
                if data:
                    self.rx_queue.put(data)
            except serial.SerialException as exc:
                self.rx_queue.put(("error", str(exc)))
                break
            except OSError as exc:
                self.rx_queue.put(("error", str(exc)))
                break
        time.sleep(0.01)

    def _drain_rx_queue(self) -> None:
        try:
            while True:
                item = self.rx_queue.get_nowait()
                if isinstance(item, tuple):
                    _, error = item
                    self._append(f"[SERIAL ERROR] {error}\n", "status")
                    if self.ser is not None:
                        self.disconnect()
                    continue

                self.rx_bytes += len(item)
                self._update_counters()
                if self.hex_rx_var.get():
                    text = " ".join(f"{b:02X}" for b in item) + " "
                else:
                    text = item.decode("utf-8", errors="replace")
                if self.timestamps_var.get():
                    text = self._timestamp_chunks(text)
                self._append(text, "rx")
        except queue.Empty:
            pass
        finally:
            self.after(50, self._drain_rx_queue)

    def _timestamp_chunks(self, text: str) -> str:
        stamp = datetime.now().strftime("%H:%M:%S.%f")[:-3]
        lines = text.splitlines(keepends=True)
        return "".join(f"[{stamp}] {line}" for line in lines) if lines else text

    def _line_ending(self) -> bytes:
        return {
            "None": b"",
            "LF": b"\n",
            "CR": b"\r",
            "CRLF": b"\r\n",
        }[self.line_ending_var.get()]

    def send_literal(self, text: str) -> None:
        self.send_entry.delete(0, "end")
        self.send_entry.insert(0, text)
        self.send_text()

    def send_text(self) -> None:
        ser = self.ser
        if ser is None or not ser.is_open:
            messagebox.showwarning(APP_NAME, "Connect to a serial port first.")
            return

        text = self.send_entry.get()
        payload = text.encode("utf-8") + self._line_ending()
        if not payload:
            return

        try:
            written = ser.write(payload)
            ser.flush()
        except serial.SerialException as exc:
            messagebox.showerror(APP_NAME, f"Write failed:\n{exc}")
            return

        self.tx_bytes += written
        self._update_counters()
        if self.show_tx_var.get():
            suffix = {"None": "", "LF": r"\n", "CR": r"\r", "CRLF": r"\r\n"}[self.line_ending_var.get()]
            prefix = datetime.now().strftime("[%H:%M:%S.%f]")[:-3] + "] " if self.timestamps_var.get() else ""
            self._append(f"{prefix}[TX] {text}{suffix}\n", "tx")
        self.send_entry.select_range(0, "end")
        self.send_entry.focus_set()

    def _append(self, text: str, tag: str) -> None:
        self.terminal.configure(state="normal")
        self.terminal.insert("end", text, tag)
        self.terminal.configure(state="disabled")
        if self.autoscroll_var.get():
            self.terminal.see("end")

    def _update_counters(self) -> None:
        self.counter_var.set(f"RX {self.rx_bytes} B   TX {self.tx_bytes} B")

    def clear_terminal(self) -> None:
        self.terminal.configure(state="normal")
        self.terminal.delete("1.0", "end")
        self.terminal.configure(state="disabled")
        self.rx_bytes = 0
        self.tx_bytes = 0
        self._update_counters()

    def save_log(self) -> None:
        path = filedialog.asksaveasfilename(
            title="Save KONTAKTSerial log",
            defaultextension=".txt",
            filetypes=(("Text files", "*.txt"), ("All files", "*.*")),
        )
        if not path:
            return
        content = self.terminal.get("1.0", "end-1c")
        try:
            with open(path, "w", encoding="utf-8") as handle:
                handle.write(content)
        except OSError as exc:
            messagebox.showerror(APP_NAME, f"Cannot save log:\n{exc}")

    def _on_close(self) -> None:
        if self.ser is not None:
            self.disconnect()
        self.destroy()


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=f"{APP_NAME} {APP_VERSION}")
    parser.add_argument("--port", help="Serial port, for example COM4")
    parser.add_argument("--baud", type=int, default=DEFAULT_BAUD, help="Baud rate (default: 115200)")
    return parser.parse_args()


def main() -> int:
    args = parse_args()
    app = KontaktSerial(args.port, args.baud)
    app.mainloop()
    return 0


if __name__ == "__main__":
    sys.exit(main())
